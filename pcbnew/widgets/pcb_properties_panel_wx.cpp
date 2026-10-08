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

/**
 * @file pcb_properties_panel_wx.cpp
 * The parts of PCB_PROPERTIES_PANEL with wxWidgets: the custom editors of wxPropertyGrid (the
 * editors are registered globally by wxPropertyGrid, and shared by the panels of the frames).
 */

#include "pcb_properties_panel.h"

#include <board.h>
#include <pcb_base_edit_frame.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/net_selector.h>
#include <widgets/ui_common.h>
#include <core/ki_assert.h>


class PG_NET_SELECTOR_EDITOR : public wxPGEditor
{
public:
    static const wxString EDITOR_NAME;

    PG_NET_SELECTOR_EDITOR() = default;

    wxString GetName() const override { return EDITOR_NAME; }

    wxPGWindowList CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty, const wxPoint& aPos,
                                   const wxSize& aSize ) const override
    {
        NET_SELECTOR* editor = new NET_SELECTOR( aGrid->GetPanel(), wxID_ANY, aPos, aSize, 0 );

        // wxPropertyGrid registers editors globally and the same PG_NET_SELECTOR_EDITOR
        // instance is reused by every PCB_PROPERTIES_PANEL (board editor, footprint editor).
        // Resolve the owning panel -- and therefore the live frame and board -- from the
        // grid at use time instead of caching frame state on the editor.  This avoids
        // cross-frame state corruption and nullptr derefs when one panel is destroyed while
        // another is still live.
        if( PCB_PROPERTIES_PANEL* panel = dynamic_cast<PCB_PROPERTIES_PANEL*>( aGrid->GetParent() ) )
        {
            if( PCB_BASE_EDIT_FRAME* frame = panel->GetFrame() )
            {
                if( BOARD* board = frame->GetBoard() )
                    editor->SetNetInfo( &board->GetNetInfo() );
            }
        }

        editor->SetIndeterminateString( INDETERMINATE_STATE );
        UpdateControl( aProperty, editor );

        editor->Bind( FILTERED_ITEM_SELECTED,
                      [=]( wxCommandEvent& aEvt )
                      {
                          auto& choices = const_cast<wxPGChoices&>( aProperty->GetChoices() );
                          wxString netname = editor->GetSelectedNetname();

                          if( choices.Index( netname ) == wxNOT_FOUND )
                              choices.Add( netname, editor->GetSelectedNetcode() );

                          wxVariant val( editor->GetSelectedNetcode() );
                          aGrid->ChangePropertyValue( aProperty, val );
                      } );

        return editor;
    }

    void UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const override
    {
        if( NET_SELECTOR* editor = dynamic_cast<NET_SELECTOR*>( aCtrl ) )
        {
            if( aProperty->IsValueUnspecified() )
                editor->SetIndeterminate();
            else
                editor->SetSelectedNetcode( (int) aProperty->GetValue().GetLong() );
        }
    }

    bool GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty, wxWindow* aCtrl ) const override
    {
        NET_SELECTOR* editor = dynamic_cast<NET_SELECTOR*>( aCtrl );

        if( !editor )
            return false;

        aVariant = static_cast<long>( editor->GetSelectedNetcode() );
        return true;
    }

    bool OnEvent( wxPropertyGrid* aGrid, wxPGProperty* aProperty, wxWindow* aWindow, wxEvent& aEvent ) const override
    {
        return false;
    }
};


const wxString PG_NET_SELECTOR_EDITOR::EDITOR_NAME = wxS( "PG_NET_SELECTOR_EDITOR" );


/// The editors of the grid used by the panel (shared with the other panels).
struct PCB_PROPERTIES_PANEL::TOOLKIT_STATE
{
    ~TOOLKIT_STATE()
    {
        m_unitEditorInstance->UpdateFrame( nullptr );
        m_fpEditorInstance->UpdateFrame( nullptr );
        m_urlEditorInstance->UpdateFrame( nullptr );

        // Note: the shared PG_NET_SELECTOR_EDITOR does not cache frame state; it resolves the
        // owning panel from the property grid on each CreateControls call, so no teardown is
        // needed here.
    }

    PG_UNIT_EDITOR*         m_unitEditorInstance = nullptr;
    PG_CHECKBOX_EDITOR*     m_checkboxEditorInstance = nullptr;
    PG_RATIO_EDITOR*        m_ratioEditorInstance = nullptr;
    PG_NET_SELECTOR_EDITOR* m_netSelectorEditorInstance = nullptr;
    PG_FPID_EDITOR*         m_fpEditorInstance = nullptr;
    PG_URL_EDITOR*          m_urlEditorInstance = nullptr;
};


void PCB_PROPERTIES_PANEL::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void PCB_PROPERTIES_PANEL::registerEditors()
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

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_RATIO_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_RATIO_EDITOR* ratioEditor = new PG_RATIO_EDITOR();
        state->m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( wxPropertyGrid::RegisterEditorClass( ratioEditor ) );
    }
    else
    {
        state->m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_NET_SELECTOR_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_NET_SELECTOR_EDITOR* netEditor = new PG_NET_SELECTOR_EDITOR();
        state->m_netSelectorEditorInstance = static_cast<PG_NET_SELECTOR_EDITOR*>( wxPropertyGrid::RegisterEditorClass( netEditor ) );
    }
    else
    {
        state->m_netSelectorEditorInstance = static_cast<PG_NET_SELECTOR_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_FPID_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        state->m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( it->second );
        state->m_fpEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_FPID_EDITOR* fpEditor = new PG_FPID_EDITOR( m_frame,
                                                       []()
                                                       {
                                                           return "";
                                                       });
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
