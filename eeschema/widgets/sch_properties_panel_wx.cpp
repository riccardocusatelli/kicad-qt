/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

/**
 * @file sch_properties_panel_wx.cpp
 * The parts of SCH_PROPERTIES_PANEL with wxWidgets: the custom editors of wxPropertyGrid (the
 * editors are registered globally by wxPropertyGrid, and shared by the panels of the frames).
 */

#include "sch_properties_panel.h"

#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <sch_base_frame.h>
#include <core/ki_assert.h>


/// The editors of the grid used by the panel (shared with the other panels).
struct SCH_PROPERTIES_PANEL::TOOLKIT_STATE
{
    ~TOOLKIT_STATE()
    {
        m_unitEditorInstance->UpdateFrame( nullptr );
        m_fpEditorInstance->UpdateFrame( nullptr );
        m_urlEditorInstance->UpdateFrame( nullptr );
    }

    PG_UNIT_EDITOR*     m_unitEditorInstance = nullptr;
    PG_CHECKBOX_EDITOR* m_checkboxEditorInstance = nullptr;
    PG_COLOR_EDITOR*    m_colorEditorInstance = nullptr;
    PG_FPID_EDITOR*     m_fpEditorInstance = nullptr;
    PG_URL_EDITOR*      m_urlEditorInstance = nullptr;
};


void SCH_PROPERTIES_PANEL::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void SCH_PROPERTIES_PANEL::registerEditors()
{
    m_toolkitState.reset( new TOOLKIT_STATE() );
    TOOLKIT_STATE* state = m_toolkitState.get();

    bool found = false;

    KI_ASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );

    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        state->m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        state->m_unitEditorInstance->UpdateFrame( m_frame );
        found = true;
    }

    if( !found )
    {
        PG_UNIT_EDITOR* new_editor = new PG_UNIT_EDITOR( m_frame );
        state->m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new_editor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_CHECKBOX_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_CHECKBOX_EDITOR* cbEditor = new PG_CHECKBOX_EDITOR();
        state->m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( wxPropertyGrid::RegisterEditorClass( cbEditor ) );
    }
    else
    {
        state->m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_COLOR_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_COLOR_EDITOR* colorEditor = new PG_COLOR_EDITOR();
        state->m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( wxPropertyGrid::RegisterEditorClass( colorEditor ) );
    }
    else
    {
        state->m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( it->second );
    }

    auto netlistCallback = [this]()
    {

    Bind( wxEVT_MENU, &SCH_PROPERTIES_PANEL::onContextMenu, this, ID_CTX_ADD_FIELD );
    Bind( wxEVT_MENU, &SCH_PROPERTIES_PANEL::onContextMenu, this, ID_CTX_ADD_CUSTOM_PROPERTY );
    Bind( wxEVT_MENU, &SCH_PROPERTIES_PANEL::onContextMenu, this, ID_CTX_REMOVE_FIELD );
    Bind( wxEVT_MENU, &SCH_PROPERTIES_PANEL::onContextMenu, this, ID_CTX_REMOVE_CUSTOM_PROPERTY );
        return symbolNetlist();
    };

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_FPID_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        state->m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( it->second );
        state->m_fpEditorInstance->UpdateFrame( m_frame );
        state->m_fpEditorInstance->UpdateCallback( netlistCallback );
    }
    else
    {
        PG_FPID_EDITOR* fpEditor = new PG_FPID_EDITOR( m_frame, netlistCallback );
        state->m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( wxPropertyGrid::RegisterEditorClass( fpEditor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        state->m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( it->second );
        state->m_urlEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_URL_EDITOR* urlEditor = new PG_URL_EDITOR( m_frame );
        state->m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( wxPropertyGrid::RegisterEditorClass( urlEditor ) );
    }
}


bool SCH_PROPERTIES_PANEL::isKeyEditable( const wxPGProperty* aPGProp ) const
{
    PROPERTY_BASE* prop = static_cast<PROPERTY_BASE*>( aPGProp->GetClientData() );

    if( !prop )
        return false;

    EDA_ITEM* item = const_cast<SCH_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item )
        return false;

    if( prop->Group() == _HKI( "Custom Properties" ) )
        return true;

    if( item->Type() == SCH_SYMBOL_T )
    {
        SCH_FIELD* field = static_cast<SCH_SYMBOL*>( item )->GetField( prop->Name() );
        return field && !field->IsMandatory();
    }
    else if( item->Type() == SCH_SHEET_T )
    {
        SCH_FIELD* field = static_cast<SCH_SHEET*>( item )->GetField( prop->Name() );
        return field && !field->IsMandatory();
    }

    return false;
}


bool SCH_PROPERTIES_PANEL::buildContextMenu( wxMenu& aMenu, wxPGProperty* aPGProp )
{
    if( aPGProp->IsCategory() )
    {
        if( aPGProp->GetLabel() == wxGetTranslation( _HKI( "Fields" ) ) )
            aMenu.Append( ID_CTX_ADD_FIELD, _( "Add Field" ) );
        else if( aPGProp->GetLabel() == wxGetTranslation( _HKI( "Custom Properties" ) ) )
            aMenu.Append( ID_CTX_ADD_CUSTOM_PROPERTY, _( "Add Custom Property" ) );
    }
    else
    {
        PROPERTY_BASE* prop = static_cast<PROPERTY_BASE*>( aPGProp->GetClientData() );

        if( !prop )
            return false;

        if( prop->Group() == _HKI( "Fields" ) )
        {
            if( isKeyEditable( aPGProp ) )
                aMenu.Append( ID_CTX_REMOVE_FIELD, _( "Remove Field" ) );

            aMenu.Append( ID_CTX_ADD_FIELD, _( "Add Field" ) );
        }
        else if( prop->Group() == _HKI( "Custom Properties" ) )
        {
            aMenu.Append( ID_CTX_REMOVE_CUSTOM_PROPERTY, _( "Remove Custom Property" ) );
            aMenu.Append( ID_CTX_ADD_CUSTOM_PROPERTY, _( "Add Custom Property" ) );
        }
    }

    return aMenu.GetMenuItemCount() > 0;
}


void SCH_PROPERTIES_PANEL::onContextMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_CTX_ADD_FIELD:             addBlankField();                           break;
    case ID_CTX_ADD_CUSTOM_PROPERTY:   addBlankCustomProperty();                 break;
    case ID_CTX_REMOVE_FIELD:          removeField( m_contextMenuPropertyName ); break;
    case ID_CTX_REMOVE_CUSTOM_PROPERTY: removeCustomProperty( m_contextMenuPropertyName ); break;
    default:                                                                     break;
    }
}

