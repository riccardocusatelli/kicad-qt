/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_layer_box_selector.h"

#include <board.h>
#include <layer_ids.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_presentation.h>
#include <settings/color_settings.h>
#include <tools/pcb_actions.h>
#include <dpi_scaling_common.h>


PCB_LAYER_BOX_SELECTOR::PCB_LAYER_BOX_SELECTOR( TOOLKIT_WINDOW_ARG parent, wxWindowID id,
                                                const wxString& value, const wxPoint& pos,
                                                const wxSize& size, int n,
                                                const wxString choices[], int style ) :
        LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices ),
        m_boardFrame( nullptr ),
        m_showNotEnabledBrdlayers( false ),
        m_layerPresentation( std::make_unique<PCB_LAYER_PRESENTATION>( nullptr ) ) // The parent isn't always the frame
{
}


void PCB_LAYER_BOX_SELECTOR::SetBoardFrame( PCB_BASE_FRAME* aFrame )
{
    m_boardFrame = aFrame;
    m_layerPresentation->SetBoardFrame( m_boardFrame );
}


wxString PCB_LAYER_BOX_SELECTOR::layerItemName( PCB_LAYER_ID aLayer, bool aActivated ) const
{
    wxString layerstatus;

    if( !aActivated )
        layerstatus = wxT( " " ) + _( "(not activated)" );

    wxString layername = m_layerPresentation->getLayerName( aLayer ) + layerstatus;

    if( m_layerhotkeys )
    {
        TOOL_ACTION* action = PCB_ACTIONS::LayerIDToAction( aLayer );

        if( action )
            layername = AddHotkeyName( layername, action->GetHotKey(), IS_COMMENT );
    }

    return layername;
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::isLayerEnabled( int aLayer ) const
{
    return getEnabledLayers().test( aLayer );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    static const LSET footprintEditorLayers = LSET::AllLayersMask();

    if( m_boardFrame )
        return m_boardFrame->GetBoard()->GetEnabledLayers();
    else
        return footprintEditorLayers;
}
