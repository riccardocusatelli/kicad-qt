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
 * @file pcb_design_block_preview_widget_qt.cpp
 * The widgets of PCB_DESIGN_BLOCK_PREVIEW_WIDGET with Qt (pcbnew/widgets/
 * pcb_design_block_preview_widget_wx.cpp with wxWidgets): a PCB_DRAW_PANEL_GAL set up as the
 * footprint previews, and a status label in its place.  What the widget does is in
 * pcbnew/widgets/pcb_design_block_preview_widget.cpp.
 */

#include <widgets/pcb_design_block_preview_widget.h>

#include <QLabel>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QVBoxLayout>

#include <base_units.h>
#include <board.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <qt_ui.h>


PCB_DESIGN_BLOCK_PREVIEW_WIDGET::PCB_DESIGN_BLOCK_PREVIEW_WIDGET( QWidget* aParent, PCB_EDIT_FRAME* aFrame ) :
        DESIGN_BLOCK_PREVIEW_WIDGET( aParent ),
        m_preview( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_stack( nullptr ),
        m_previewItem( nullptr )
{
    COMMON_SETTINGS* common_settings = Pgm().GetCommonSettings();
    PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    m_galDisplayOptions.ReadConfig( *common_settings, cfg->m_Window, this );
    m_galDisplayOptions.m_forceDisplayCursor = false;

    // The canvas of the previews (FOOTPRINT_PREVIEW_PANEL::New() of the wxWidgets build)
    auto galType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( common_settings->m_Graphics.canvas_type );

    PCB_DRAW_PANEL_GAL* preview = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxSize( 200, 200 ),
                                                          m_galDisplayOptions, galType );
    m_preview = preview;

    m_preview->SetStealsFocus( false );
    m_preview->GetGAL()->SetAxesEnabled( false );
    m_preview->GetGAL()->SetGridVisibility( true );

    preview->UpdateColors();

    KIGFX::VIEW* view = m_preview->GetView();
    auto         settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    if( COLOR_SETTINGS* cs = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME ) )
        settings->LoadColors( cs );

    settings->SetHighlight( false );
    settings->SetNetColorMode( NET_COLOR_MODE::OFF );

    const COLOR4D& backgroundColor = settings->GetBackgroundColor();
    const COLOR4D& foregroundColor = settings->GetCursorColor();

    m_preview->GetGAL()->SetClearColor( backgroundColor );

    // The grid of the board editor
    const GRID_SETTINGS& gridCfg = cfg->m_Window.grid;

    if( !gridCfg.grids.empty() )
    {
        int    gridIdx = std::clamp( gridCfg.last_size_idx, 0, (int) gridCfg.grids.size() - 1 );
        double gridSizeX = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                                      gridCfg.grids[gridIdx].x );
        double gridSizeY = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                                      gridCfg.grids[gridIdx].y );
        m_preview->GetGAL()->SetGridSize( VECTOR2D( gridSizeX, gridSizeY ) );
    }

    // The status, with the color scheme of the canvas so it isn't jarring when switched to
    m_statusPanel = new QWidget( this );
    m_statusPanel->setAutoFillBackground( true );

    QPalette palette = m_statusPanel->palette();
    palette.setColor( QPalette::Window, ToQColor( backgroundColor ) );
    palette.setColor( QPalette::WindowText, ToQColor( foregroundColor ) );
    m_statusPanel->setPalette( palette );

    QVBoxLayout* statusSizer = new QVBoxLayout( m_statusPanel );
    m_status = new QLabel( m_statusPanel );
    m_status->setAlignment( Qt::AlignCenter );

    QPalette statusPalette = m_status->palette();
    statusPalette.setColor( QPalette::WindowText, ToQColor( settings->GetLayerColor( LAYER_REFERENCEPART ) ) );
    m_status->setPalette( statusPalette );

    statusSizer->addStretch( 1 );
    statusSizer->addWidget( m_status );
    statusSizer->addStretch( 1 );

    QVBoxLayout* outerSizer = new QVBoxLayout( this );
    outerSizer->setContentsMargins( 0, 5, 0, 5 );

    m_stack = new QStackedLayout();
    m_stack->addWidget( m_preview );
    m_stack->addWidget( m_statusPanel );
    m_stack->setCurrentWidget( m_preview );
    outerSizer->addLayout( m_stack, 1 );

    m_preview->StartDrawing();
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    KI_CHECK( m_statusPanel, /* void */ );

    m_status->setText( ToQString( aText ) );
    m_stack->setCurrentWidget( m_statusPanel );
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::resizeEvent( QResizeEvent* aEvent )
{
    DESIGN_BLOCK_PREVIEW_WIDGET::resizeEvent( aEvent );

    if( m_previewItem )
    {
        fitOnDrawArea();
        m_preview->ForceRefresh();
    }
}


BOARD* PCB_DESIGN_BLOCK_PREVIEW_WIDGET::loadBoard( const wxString& aFileName )
{
    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

    return pi->LoadBoard( aFileName, nullptr );
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::showCanvas()
{
    m_stack->setCurrentWidget( m_preview );
}
