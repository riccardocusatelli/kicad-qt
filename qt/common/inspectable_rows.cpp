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

#include <QObject>

#include <wx/any.h>
#include <wx/intl.h>

#include <geometry/eda_angle.h>
#include <eda_item.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

#include "qt_strings.h"


/// The value of a property of an item, as shown.
static QString valueText( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, const EDA_IU_SCALE& aIuScale,
                          EDA_UNITS aUnits )
{
    wxAny value = aItem->Get( aProperty );

    switch( aProperty->Display() )
    {
    case PT_SIZE:
    case PT_COORD:
    case PT_AREA:
    {
        long long iu = 0;
        double    dbl = 0.0;

        if( value.GetAs( &iu ) )
            dbl = static_cast<double>( iu );
        else if( !value.GetAs( &dbl ) )
            break;

        EDA_DATA_TYPE type = aProperty->Display() == PT_AREA ? EDA_DATA_TYPE::AREA
                                                             : EDA_DATA_TYPE::DISTANCE;

        return ToQString( EDA_UNIT_UTILS::UI::StringFromValue( aIuScale, aUnits, dbl, true,
                                                               type ) );
    }

    case PT_DEGREE:
    case PT_DECIDEGREE:
    {
        EDA_ANGLE angle;
        double    dbl = 0.0;

        if( value.GetAs( &angle ) )
            dbl = angle.AsDegrees();
        else if( value.GetAs( &dbl ) )
            dbl = aProperty->Display() == PT_DECIDEGREE ? dbl / 10.0 : dbl;
        else
            break;

        return QString::number( dbl, 'g', 6 ) + QString::fromUtf8( "°" );
    }

    default:
        break;
    }

    // A plain number (a ratio, a factor): without the trailing zeros of its text
    double number = 0.0;

    if( value.CheckType<double>() && value.GetAs( &number ) )
        return QString::number( number, 'g', 10 );

    bool flag = false;

    if( value.CheckType<bool>() && value.GetAs( &flag ) )
        return flag ? QObject::tr( "Yes" ) : QObject::tr( "No" );

    wxString text;

    if( value.GetAs( &text ) )
        return ToQString( text );

    return QString();
}


std::vector<PROPERTY_ROW> PropertyRows( EDA_ITEM* aItem, const EDA_IU_SCALE& aIuScale,
                                        EDA_UNITS aUnits )
{
    std::vector<PROPERTY_ROW> rows;

    if( !aItem )
        return rows;

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    const TYPE_ID                        type = TYPE_HASH( *aItem );
    const std::map<PROPERTY_BASE*, int>& displayOrder = propMgr.GetDisplayOrder( type );
    const std::vector<wxString>&         groups = propMgr.GetGroupDisplayOrder( type );

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

        if( !propMgr.IsAvailableFor( type, property, aItem ) )
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
        PROPERTY_ROW row;
        row.m_Group = ToQString( wxGetTranslation( entry.m_property->Group() ) );
        row.m_Name = ToQString( wxGetTranslation( entry.m_property->Name() ) );
        row.m_Value = valueText( aItem, entry.m_property, aIuScale, aUnits );
        rows.push_back( row );
    }

    return rows;
}
