/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "inspectable_rows.h"

#include <algorithm>
#include <map>
#include <optional>

#include <QObject>

#include <wx/any.h>
#include <wx/intl.h>

#include <eda_item.h>
#include <geometry/eda_angle.h>
#include <libeval/numeric_evaluator.h>
#include <math/util.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <properties/property_validator.h>
#include <units_provider.h>

#include "qt_strings.h"


/// True if the property is a measure (distance or area) in internal units.
static bool isMeasure( PROPERTY_BASE* aProperty )
{
    switch( aProperty->Display() )
    {
    case PT_SIZE:
    case PT_COORD:
    case PT_AREA:
        return true;

    default:
        return false;
    }
}


/// True if the property is an angle.
static bool isAngle( PROPERTY_BASE* aProperty )
{
    return aProperty->Display() == PT_DEGREE || aProperty->Display() == PT_DECIDEGREE;
}


/// The choices of a property for an item (its enum, or the ones the item gives), or none.
static PROPERTY_CHOICES choicesOf( PROPERTY_BASE* aProperty, EDA_ITEM* aItem )
{
    if( aProperty->HasChoices() )
        return aProperty->Choices();

    return aProperty->GetChoices( aItem );
}


/// The label of a choice, as shown.
static QString choiceLabel( const PROPERTY_CHOICES& aChoices, unsigned aIndex )
{
    return ToQString( wxGetTranslation( aChoices.GetLabel( aIndex ) ) );
}


/// The value of a property of an item, as shown.
static QString valueText( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, const EDA_IU_SCALE& aIuScale,
                          EDA_UNITS aUnits )
{
    wxAny value = aItem->Get( aProperty );

    // A value that may be unset (a local override): empty when unset
    if( value.CheckType<std::optional<int>>() )
    {
        std::optional<int> optional = value.As<std::optional<int>>();

        if( !optional )
            return QString();

        value = *optional;
    }
    else if( value.CheckType<std::optional<double>>() )
    {
        std::optional<double> optional = value.As<std::optional<double>>();

        if( !optional )
            return QString();

        value = *optional;
    }

    if( isMeasure( aProperty ) )
    {
        long long iu = 0;
        double    dbl = 0.0;

        if( value.GetAs( &iu ) )
            dbl = static_cast<double>( iu );
        else if( !value.GetAs( &dbl ) )
            return QString();

        EDA_DATA_TYPE type = aProperty->Display() == PT_AREA ? EDA_DATA_TYPE::AREA
                                                             : EDA_DATA_TYPE::DISTANCE;

        return ToQString( EDA_UNIT_UTILS::UI::StringFromValue( aIuScale, aUnits, dbl, true, type ) );
    }

    if( isAngle( aProperty ) )
    {
        EDA_ANGLE angle;
        double    dbl = 0.0;

        if( value.GetAs( &angle ) )
            dbl = angle.AsDegrees();
        else if( value.GetAs( &dbl ) )
            dbl = aProperty->Display() == PT_DECIDEGREE ? dbl / 10.0 : dbl;
        else
            return QString();

        return QString::number( dbl, 'g', 6 ) + QString::fromUtf8( "°" );
    }

    PROPERTY_CHOICES choices = choicesOf( aProperty, aItem );
    int              choice = 0;

    if( choices.GetCount() > 0 && value.GetAs( &choice ) )
    {
        int index = choices.Index( choice );
        return index >= 0 ? choiceLabel( choices, index ) : QString();
    }

    bool flag = false;

    if( value.CheckType<bool>() && value.GetAs( &flag ) )
        return flag ? QStringLiteral( "1" ) : QStringLiteral( "0" );

    // A plain number (a ratio, a factor): without the trailing zeros of its text
    double number = 0.0;

    if( value.CheckType<double>() && value.GetAs( &number ) )
        return QString::number( number, 'g', 10 );

    wxString text;

    if( value.GetAs( &text ) )
        return ToQString( text );

    return QString();
}


/// How a property of an item is edited.
static PROPERTY_ROW::EDITOR editorOf( EDA_ITEM* aItem, PROPERTY_BASE* aProperty )
{
    wxAny value = aItem->Get( aProperty );

    if( choicesOf( aProperty, aItem ).GetCount() > 0 )
        return PROPERTY_ROW::EDITOR::CHOICE;

    if( value.CheckType<bool>() )
        return PROPERTY_ROW::EDITOR::CHECK;

    if( isMeasure( aProperty ) || isAngle( aProperty ) || value.CheckType<wxString>()
            || value.CheckType<int>() || value.CheckType<long>() || value.CheckType<long long>()
            || value.CheckType<unsigned>() || value.CheckType<double>()
            || value.CheckType<std::optional<int>>() || value.CheckType<std::optional<double>>() )
    {
        return PROPERTY_ROW::EDITOR::TEXT;
    }

    return PROPERTY_ROW::EDITOR::NONE;
}


std::vector<PROPERTY_ROW> PropertyRows( const std::vector<EDA_ITEM*>& aItems,
                                        const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits )
{
    std::vector<PROPERTY_ROW> rows;

    if( aItems.empty() )
        return rows;

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    // The order of the properties is the one of the first item
    EDA_ITEM*                            first = aItems.front();
    const TYPE_ID                        type = TYPE_HASH( *first );
    const std::map<PROPERTY_BASE*, int>& displayOrder = propMgr.GetDisplayOrder( type );
    const std::vector<wxString>&         groups = propMgr.GetGroupDisplayOrder( type );

    auto shownFor =
            [&]( EDA_ITEM* aItem, PROPERTY_BASE* aProperty ) -> bool
            {
                const TYPE_ID itemType = TYPE_HASH( *aItem );
                const auto&   properties = propMgr.GetProperties( itemType );

                return std::find( properties.begin(), properties.end(), aProperty ) != properties.end()
                       && propMgr.IsAvailableFor( itemType, aProperty, aItem );
            };

    struct ENTRY
    {
        size_t         m_group;
        int            m_order;
        PROPERTY_BASE* m_property;
    };

    std::vector<ENTRY> entries;

    for( PROPERTY_BASE* property : propMgr.GetProperties( type ) )
    {
        if( property->IsHiddenFromPropertiesManager() || property->IsHiddenFromDesignEditors() )
            continue;

        bool shared = std::all_of( aItems.begin(), aItems.end(),
                                   [&]( EDA_ITEM* aItem )
                                   {
                                       return shownFor( aItem, property );
                                   } );

        if( !shared )
            continue;

        auto   groupIt = std::find( groups.begin(), groups.end(), property->Group() );
        auto   orderIt = displayOrder.find( property );
        size_t group = static_cast<size_t>( groupIt - groups.begin() );
        int    order = orderIt != displayOrder.end() ? orderIt->second : 0;

        entries.push_back( { group, order, property } );
    }

    std::stable_sort( entries.begin(), entries.end(),
                      []( const ENTRY& a, const ENTRY& b )
                      {
                          return a.m_group != b.m_group ? a.m_group < b.m_group
                                                        : a.m_order < b.m_order;
                      } );

    for( const ENTRY& entry : entries )
    {
        PROPERTY_BASE* property = entry.m_property;

        PROPERTY_ROW row;
        row.m_Group = ToQString( wxGetTranslation( property->Group() ) );
        row.m_Name = ToQString( wxGetTranslation( property->Name() ) );
        row.m_Value = valueText( first, property, aIuScale, aUnits );
        row.m_Key = property;
        row.m_Editor = editorOf( first, property );

        for( EDA_ITEM* item : aItems )
        {
            if( !propMgr.IsWriteableFor( TYPE_HASH( *item ), property, item ) )
                row.m_Editor = PROPERTY_ROW::EDITOR::NONE;

            if( !row.m_Mixed && item != first
                    && valueText( item, property, aIuScale, aUnits ) != row.m_Value )
            {
                row.m_Mixed = true;
            }
        }

        if( row.m_Mixed )
            row.m_Value.clear();

        if( row.m_Editor == PROPERTY_ROW::EDITOR::CHOICE )
        {
            PROPERTY_CHOICES choices = choicesOf( property, first );

            for( unsigned ii = 0; ii < choices.GetCount(); ++ii )
                row.m_Choices << choiceLabel( choices, ii );
        }

        rows.push_back( row );
    }

    return rows;
}


/// Evaluate a number, with an expression and a unit if given; false if it is not one.
static bool evaluate( const QString& aText, EDA_UNITS aUnits, wxString& aResult )
{
    NUMERIC_EVALUATOR evaluator( aUnits );

    if( !evaluator.Process( ToWxString( aText.trimmed() ) ) || !evaluator.IsValid() )
        return false;

    aResult = evaluator.Result();
    return !aResult.empty();
}


bool SetPropertyValue( EDA_ITEM* aItem, const void* aKey, const QString& aValue,
                       const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits, QString& aError )
{
    PROPERTY_BASE* property = static_cast<PROPERTY_BASE*>( const_cast<void*>( aKey ) );
    const wxAny    current = aItem->Get( property );
    wxAny          value;

    const bool optional = current.CheckType<std::optional<int>>()
                          || current.CheckType<std::optional<double>>();

    auto invalid =
            [&]() -> bool
            {
                aError = QObject::tr( "'%1' is not a valid value for %2." )
                                 .arg( aValue, ToQString( wxGetTranslation( property->Name() ) ) );
                return false;
            };

    // Store a number as the type of the property
    auto setNumber =
            [&]( double aNumber )
            {
                if( current.CheckType<int>() || current.CheckType<std::optional<int>>() )
                {
                    if( optional )
                        value = std::optional<int>( KiROUND( aNumber ) );
                    else
                        value = static_cast<int>( KiROUND( aNumber ) );
                }
                else if( current.CheckType<long>() )
                    value = static_cast<long>( KiROUND( aNumber ) );
                else if( current.CheckType<long long>() )
                    value = static_cast<long long>( KiROUND( aNumber ) );
                else if( current.CheckType<unsigned>() )
                    value = static_cast<unsigned>( std::max( 0, KiROUND( aNumber ) ) );
                else if( current.CheckType<std::optional<double>>() )
                    value = std::optional<double>( aNumber );
                else
                    value = aNumber;
            };

    PROPERTY_CHOICES choices = choicesOf( property, aItem );

    if( optional && aValue.trimmed().isEmpty() )
    {
        // An override left empty: unset
        if( current.CheckType<std::optional<int>>() )
            value = std::optional<int>();
        else
            value = std::optional<double>();
    }
    else if( choices.GetCount() > 0 )
    {
        int index = -1;

        for( unsigned ii = 0; ii < choices.GetCount() && index < 0; ++ii )
        {
            if( choiceLabel( choices, ii ) == aValue )
                index = static_cast<int>( ii );
        }

        if( index < 0 )
            return invalid();

        // The value of the choice, or its label for a text property
        if( current.CheckType<wxString>() )
            value = choices.GetLabel( index );
        else
            value = choices.GetValue( index );
    }
    else if( current.CheckType<bool>() )
    {
        value = aValue == QLatin1String( "1" );
    }
    else if( isMeasure( property ) )
    {
        wxString result;

        if( !evaluate( aValue, aUnits, result ) )
            return invalid();

        EDA_DATA_TYPE type = property->Display() == PT_AREA ? EDA_DATA_TYPE::AREA
                                                            : EDA_DATA_TYPE::DISTANCE;

        setNumber( EDA_UNIT_UTILS::UI::DoubleValueFromString( aIuScale, aUnits, result, type ) );
    }
    else if( isAngle( property ) )
    {
        wxString result;

        if( !evaluate( aValue, EDA_UNITS::DEGREES, result ) )
            return invalid();

        double degrees = EDA_UNIT_UTILS::UI::DoubleValueFromString( result );

        if( current.CheckType<EDA_ANGLE>() )
            value = EDA_ANGLE( degrees, DEGREES_T );
        else
            setNumber( property->Display() == PT_DECIDEGREE ? degrees * 10.0 : degrees );
    }
    else if( current.CheckType<wxString>() )
    {
        value = ToWxString( aValue );
    }
    else
    {
        wxString result;

        if( !evaluate( aValue, EDA_UNITS::UNSCALED, result ) )
            return invalid();

        setNumber( EDA_UNIT_UTILS::UI::DoubleValueFromString( result ) );
    }

    if( VALIDATOR_RESULT error = property->Validate( wxAny( value ), aItem ) )
    {
        UNITS_PROVIDER units( aIuScale, aUnits );
        aError = ToQString( ( *error )->Format( &units ) );
        return false;
    }

    try
    {
        aItem->Set( property, value );
    }
    catch( const std::exception& )
    {
        return invalid();
    }

    return true;
}
