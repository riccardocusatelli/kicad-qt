/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "properties_panel.h"
#include <tool/selection.h>
#include <eda_base_frame.h>
#include <eda_item.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

#include <algorithm>
#include <iterator>
#include <set>

#include <core/ki_assert.h>

#include <wx/intl.h>


class SUPPRESS_GRID_CHANGED_EVENTS
{
public:
    SUPPRESS_GRID_CHANGED_EVENTS( PROPERTIES_PANEL* aPanel ) :
            m_panel( aPanel )
    {
        m_panel->m_SuppressGridChangeEvents++;
    }

    ~SUPPRESS_GRID_CHANGED_EVENTS()
    {
        m_panel->m_SuppressGridChangeEvents--;
    }

private:
    PROPERTIES_PANEL* m_panel;
};


void PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    SUPPRESS_GRID_CHANGED_EVENTS raii( this );

    auto reset =
            [&]()
            {
                gridReset();
            };

    if( aSelection.Empty() )
    {
        setCaption( _( "No objects selected" ) );
        reset();
        return;
    }
    else if( aSelection.Size() == 1 )
    {
        setCaption( aSelection.Front()->GetFriendlyName() );
    }
    else
    {
        setCaption( wxString::Format( _( "%d objects selected" ), aSelection.Size() ) );
    }

    // Get all the selected types
    std::set<TYPE_ID> types;

    for( EDA_ITEM* item : aSelection )
        types.insert( TYPE_HASH( *item ) );

    KI_CHECK( !types.empty(), /* void */ );  // already guarded above, but Coverity doesn't know that

    PROPERTY_MANAGER&                  propMgr = PROPERTY_MANAGER::Instance();
    std::map<wxString, PROPERTY_BASE*> commonProps;
    const std::vector<PROPERTY_BASE*>& allProperties = propMgr.GetProperties( *types.begin() );

    for( PROPERTY_BASE* property : allProperties )
        commonProps.emplace( property->Name(), property );

    std::map<wxString, int> displayOrder;
    for( const auto& entry : propMgr.GetDisplayOrder( *types.begin() ) )
        displayOrder.emplace( entry.first->Name(), entry.second );

    std::vector<wxString> groupDisplayOrder = propMgr.GetGroupDisplayOrder( *types.begin() );
    std::set<wxString>    groups( groupDisplayOrder.begin(), groupDisplayOrder.end() );

    // Get all possible properties
    for( auto itType = std::next( types.begin() ); itType != types.end(); ++itType )
    {
        TYPE_ID type = *itType;

        for( const wxString& group : propMgr.GetGroupDisplayOrder( type ) )
        {
            if( !groups.count( group ) )
            {
                groupDisplayOrder.emplace_back( group );
                groups.insert( group );
            }
        }

        for( auto it = commonProps.begin(); it != commonProps.end(); )
        {
            if( !propMgr.GetProperty( type, it->first ) )
                it = commonProps.erase( it );
            else
                ++it;
        }
    }

    bool isLibraryEditor = m_frame->IsType( FRAME_FOOTPRINT_EDITOR )
                        || m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

    bool isDesignEditor = m_frame->IsType( FRAME_PCB_EDITOR )
                       || m_frame->IsType( FRAME_SCH );

    std::set<wxString> availableProps;

    // Find a set of properties that is common to all selected items
    for( auto& [name, property] : commonProps )
    {
        if( property->IsHiddenFromPropertiesManager() )
            continue;

        if( isLibraryEditor && property->IsHiddenFromLibraryEditors() )
            continue;

        if( isDesignEditor && property->IsHiddenFromDesignEditors() )
            continue;

        wxVariant   dummy;
        PROPERTY_CHOICES choices;
        bool             writable;

        if( extractValueAndWritability( aSelection, name, dummy, writable, choices ) )
            availableProps.insert( name );
    }

    bool               writeable = true;
    std::set<wxString> existingProps;

    for( TOOLKIT_PG_PROPERTY* pgProp : gridProperties() )
    {
        wxString name = PGGetName( pgProp );

        // Store the existing name before checking available properties so we can
        // remove the properties when they are no longer available
        existingProps.insert( name );

        if( !availableProps.count( name ) )
            continue;

        wxVariant        commonVal;
        PROPERTY_CHOICES choices;

        extractValueAndWritability( aSelection, name, commonVal, writeable, choices );

        if( choices.GetCount() > 0 )
            PGSetChoices( pgProp, choices );

        PGSetValue( pgProp, commonVal );
        PGSetReadOnly( pgProp, !writeable );
    }

    if( !existingProps.empty() && existingProps == availableProps )
        return;

    // Some difference exists:  start from scratch
    reset();

    std::map<TOOLKIT_PG_PROPERTY*, int>                   pgPropOrders;
    std::map<wxString, std::vector<TOOLKIT_PG_PROPERTY*>> pgPropGroups;

    for( const wxString& name : availableProps )
    {
        PROPERTY_BASE*   property = commonProps[name];
        wxVariant        commonVal;
        PROPERTY_CHOICES choices;

        if( !extractValueAndWritability( aSelection, name, commonVal, writeable, choices ) )
            continue;

        if( TOOLKIT_PG_PROPERTY* pgProp = createPGProperty( property ) )
        {
            if( choices.GetCount() )
                PGSetChoices( pgProp, choices );

            PGSetValue( pgProp, commonVal );
            PGSetReadOnly( pgProp, !writeable );
            m_displayed.push_back( property );

            KI_ASSERT( displayOrder.count( name ) );
            pgPropOrders[pgProp] = displayOrder[name];
            pgPropGroups[property->Group()].emplace_back( pgProp );
        }
    }

    const wxString unspecifiedGroupCaption = _( "Basic Properties" );

    for( const wxString& groupName : groupDisplayOrder )
    {
        if( !pgPropGroups.count( groupName ) )
            continue;

        std::vector<TOOLKIT_PG_PROPERTY*>& properties = pgPropGroups[groupName];
        wxString                           groupCaption = wxGetTranslation( groupName );

        gridAppendCategory( groupName.IsEmpty() ? unspecifiedGroupCaption : groupCaption );

        std::sort( properties.begin(), properties.end(),
                   [&]( TOOLKIT_PG_PROPERTY*& aFirst, TOOLKIT_PG_PROPERTY*& aSecond )
                   {
                       return pgPropOrders[aFirst] < pgPropOrders[aSecond];
                   } );

        for( TOOLKIT_PG_PROPERTY* property : properties )
            gridAppend( property );
    }

    RecalculateSplitterPos();
}


bool PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    const wxAny& any = aItem->Get( aProperty );
    bool converted = false;

    if( aProperty->HasChoices() )
    {
        // handle enums as ints, since there are no default conversion functions for wxAny
        int tmp;
        converted = any.GetAs<int>( &tmp );

        if( converted )
            aValue = wxVariant( tmp );
    }

    if( !converted )                // all other types
        converted = any.GetAs( &aValue );

    if( !converted )
    {
        wxString propName = aProperty->Name();
        propName.Replace( ' ', '_' );
        KI_FAIL_MSG( wxString::Format( wxS( "Could not convert wxAny to wxVariant for %s::%s" ),
                                      aItem->GetClass(),
                                      propName ) );
    }

    return converted;
}


bool PROPERTIES_PANEL::extractValueAndWritability( const SELECTION& aSelection, const wxString& aPropName,
                                                   wxVariant& aValue, bool& aWritable, PROPERTY_CHOICES& aChoices )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    bool              different = false;
    bool              first = true;

    aWritable = true;

    for( EDA_ITEM* item : aSelection )
    {
        PROPERTY_BASE* property = propMgr.GetProperty( TYPE_HASH( *item ), aPropName );

        if( !property )
            return false;

        if( !propMgr.IsAvailableFor( TYPE_HASH( *item ), property, item ) )
            return false;

        if( property->IsHiddenFromPropertiesManager() )
            return false;

        PROPERTY_CHOICES choices = property->GetChoices( item );

        if( first )
        {
            aChoices = choices;
            first = false;
        }
        else
        {
            wxArrayString labels = choices.GetLabels();
            wxArrayInt    values = choices.GetValuesForStrings( labels );

            if( labels != aChoices.GetLabels() || values != aChoices.GetValuesForStrings( labels ) )
                return false;
        }

        // If read-only for any of the selection, read-only for the whole selection.
        if( !propMgr.IsWriteableFor( TYPE_HASH( *item ), property, item ) )
            aWritable = false;

        wxVariant value;

        if( getItemValue( item, property, value ) )
        {
            // Null value indicates different property values between items
            if( !different && !aValue.IsNull() && value != aValue )
            {
                different = true;
                aValue.MakeNull();
            }
            else if( !different )
            {
                aValue = value;
            }
        }
        else
        {
            // getItemValue returned false -- not available for this item
            return false;
        }
    }

    return true;
}
