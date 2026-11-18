/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <sch_draw_panel.h>

#include <wx/debug.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/window.h>
#include <wx/windowid.h>
#include <memory>
#include <stdexcept>

#include <confirm.h>
#include <eda_draw_frame.h>
#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <layer_ids.h>
#include <math/vector2d.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <view/view_controls.h>

#include <sch_base_frame.h>
#include <sch_painter.h>
#include <zoom_defines.h>
#include <core/ki_assert.h>


SCH_DRAW_PANEL::SCH_DRAW_PANEL( TOOLKIT_WINDOW_ARG aParentWindow, wxWindowID aWindowId,
                                const wxPoint& aPosition, const wxSize& aSize,
                                KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType )
        : EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::SCH_VIEW( dynamic_cast<SCH_BASE_FRAME*>( GetParentEDAFrame() ) );
    m_view->SetGAL( m_gal );

    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    m_painter.reset( new KIGFX::SCH_PAINTER( m_gal ) );

    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );

    if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( GetParentEDAFrame() ) )
        cs = frame->GetColorSettings();

    KI_ASSERT( cs );
    m_painter->GetSettings()->LoadColors( cs );

    m_view->SetPainter( m_painter.get() );

    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );
    m_view->SetMirror( false, false );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    auto settings = m_painter->GetSettings();
    m_gal->SetClearColor( settings->GetBackgroundColor() );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    GetView()->UpdateAllLayersOrder();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    createViewControls();

    SetEvtHandlerEnabled( true );
    SetFocus();
    Show( true );
    Raise();
    StartDrawing();
}


SCH_DRAW_PANEL::~SCH_DRAW_PANEL()
{
}


void SCH_DRAW_PANEL::DisplaySymbol( LIB_SYMBOL* aSymbol )
{
    GetView()->DisplaySymbol( aSymbol );
}


void SCH_DRAW_PANEL::DisplaySheet( SCH_SCREEN *aScreen )
{
    GetView()->Clear();

    if( aScreen )
        GetView()->DisplaySheet( aScreen );
    else
        GetView()->Cleanup();
}


void SCH_DRAW_PANEL::setDefaultLayerOrder()
{
    GetView()->SetDefaultLayerOrder();
}


bool SCH_DRAW_PANEL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    // Items carried over from the previous GAL have stale caches. The rebuild waits for startup
    // events to settle, since a partial redraw would clobber the GPU buffers.
    if( rv && aGalType == GAL_TYPE_MODERN )
    {
        CallAfter( [this]()
        {
            if( EDA_DRAW_FRAME* frame = GetParentEDAFrame() )
                frame->HardRedraw();
        } );
    }

    Refresh();

    return rv;
}


void SCH_DRAW_PANEL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    GetView()->SetDefaultLayerDeps( IsAccelerated( m_backend ), m_gal && m_gal->HasRetainedScene() );
}


KIGFX::SCH_VIEW* SCH_DRAW_PANEL::GetView() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_view );
}


void SCH_DRAW_PANEL::onPaint( wxPaintEvent& aEvent )
{
    // The first wxPaintEvent can be fired at startup before the GAL engine is fully initialized
    // (depending on platforms). Do nothing in this case
    if( !m_gal->IsInitialized() || !m_gal->IsVisible() )
        return;

    EDA_DRAW_PANEL_GAL::onPaint( aEvent );
}
