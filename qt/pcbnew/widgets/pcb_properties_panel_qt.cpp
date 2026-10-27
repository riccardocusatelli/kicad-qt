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


/**
 * @file pcb_properties_panel_qt.cpp
 * The parts of PCB_PROPERTIES_PANEL with Qt (pcbnew/widgets/pcb_properties_panel_wx.cpp with
 * wxWidgets): the editors of its grid.  Unlike wxPropertyGrid, whose editors are global, each
 * grid has its own editors, so they keep the frame of their panel.
 */

#include <widgets/pcb_properties_panel.h>

#include <board.h>
#include <pcb_base_edit_frame.h>
#include <widgets/net_selector.h>
#include <widgets/property_grid.h>
#include <widgets/ui_common.h>


namespace
{

/// The nets of the board, chosen with a NET_SELECTOR.
class PG_NET_SELECTOR_EDITOR : public PG_CUSTOM_EDITOR
{
public:
    PG_NET_SELECTOR_EDITOR( PCB_BASE_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    QWidget* CreateEditor( QWidget* aParent, PROPERTY_GRID* aGrid, PG_PROPERTY* aProperty ) override
    {
        NET_SELECTOR* editor = new NET_SELECTOR( aParent, wxID_ANY );

        if( BOARD* board = m_frame->GetBoard() )
            editor->SetNetInfo( &board->GetNetInfo() );

        editor->SetIndeterminateString( INDETERMINATE_STATE );

        if( aProperty->IsValueUnspecified() )
            editor->SetIndeterminate();
        else
            editor->SetSelectedNetcode( static_cast<int>( aProperty->GetValue().GetLong() ) );

        editor->Bind( FILTERED_ITEM_SELECTED,
                [editor, aGrid, aProperty]( wxCommandEvent& )
                {
                    // a net not yet in the choices of the property (a new net)
                    wxString netname = editor->GetSelectedNetname();

                    if( aProperty->GetChoices().Index( netname ) < 0 )
                        aProperty->Choices().Add( netname, editor->GetSelectedNetcode() );

                    aGrid->ChangePropertyValue( aProperty,
                                                wxVariant( static_cast<long>( editor->GetSelectedNetcode() ) ) );
                } );

        return editor;
    }

    bool GetValue( QWidget* aEditor, PG_PROPERTY* aProperty, wxVariant& aValue ) override
    {
        NET_SELECTOR* editor = dynamic_cast<NET_SELECTOR*>( aEditor );

        if( !editor || editor->IsIndeterminate() )
            return false;

        long netcode = editor->GetSelectedNetcode();

        if( !aProperty->IsValueUnspecified() && aProperty->GetValue().GetLong() == netcode )
            return false;

        aValue = netcode;
        return true;
    }

private:
    PCB_BASE_EDIT_FRAME* m_frame;
};

} // namespace


/// The state of the editors: the grid owns them.
struct PCB_PROPERTIES_PANEL::TOOLKIT_STATE
{
};


void PCB_PROPERTIES_PANEL::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void PCB_PROPERTIES_PANEL::registerEditors()
{
    m_toolkitState.reset( new TOOLKIT_STATE() );

    // the unit editors follow the units and the origin of the frame
    m_grid->SetEditorFrame( m_frame );
    m_grid->SetCustomEditor( PG_PROPERTY::EDITOR::NET,
                             std::make_unique<PG_NET_SELECTOR_EDITOR>( m_frame ) );
}
