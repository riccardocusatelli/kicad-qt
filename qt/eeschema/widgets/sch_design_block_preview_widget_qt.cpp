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
 * @file sch_design_block_preview_widget_qt.cpp
 * The widgets of SCH_DESIGN_BLOCK_PREVIEW_WIDGET with Qt (eeschema/widgets/
 * sch_design_block_preview_widget_wx.cpp with wxWidgets): a SCH_PREVIEW_PANEL, and a status
 * label in its place.  What the widget does is in
 * eeschema/widgets/sch_design_block_preview_widget.cpp.
 */

#include <widgets/sch_design_block_preview_widget.h>

#include <QLabel>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QVBoxLayout>

#include <eeschema_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <sch_preview_panel.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <qt_ui.h>


SCH_DESIGN_BLOCK_PREVIEW_WIDGET::SCH_DESIGN_BLOCK_PREVIEW_WIDGET( QWidget* aParent,
                                                                 EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType,
                                                                 bool aIncludeStatus ) :
        DESIGN_BLOCK_PREVIEW_WIDGET( aParent ),
        m_preview( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_stack( nullptr ),
        m_previewItem( nullptr )
{
    COMMON_SETTINGS*   common_settings = Pgm().GetCommonSettings();
    EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    m_galDisplayOptions.ReadConfig( *common_settings, cfg->m_Window, this );
    m_galDisplayOptions.m_forceDisplayCursor = false;

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = aCanvasType;

    // Allows only a CAIRO or OPENGL canvas:
    if( canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL && canvasType != EDA_DRAW_PANEL_GAL::GAL_FALLBACK )
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    m_preview = new SCH_PREVIEW_PANEL( this, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
                                       m_galDisplayOptions, canvasType );
    m_preview->SetStealsFocus( false );
    m_preview->GetGAL()->SetAxesEnabled( false );
    m_preview->GetGAL()->SetGridVisibility( true );

    // Early initialization of the canvas background color, before any paint event
    KIGFX::VIEW* view = m_preview->GetView();
    auto         settings = static_cast<SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    if( COLOR_SETTINGS* cs = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME ) )
        settings->LoadColors( cs );

    const COLOR4D& backgroundColor = settings->GetBackgroundColor();
    const COLOR4D& foregroundColor = settings->GetCursorColor();

    m_preview->GetGAL()->SetClearColor( backgroundColor );

    settings->m_ShowPinsElectricalType = false;
    settings->m_ShowPinNumbers = false;
    settings->m_ShowHiddenPins = false;
    settings->m_ShowHiddenFields = false;
    settings->m_ShowPinAltIcons = false;

    QVBoxLayout* outerSizer = new QVBoxLayout( this );
    m_stack = new QStackedLayout();
    m_stack->addWidget( m_preview );

    if( aIncludeStatus )
    {
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
        statusPalette.setColor( QPalette::WindowText,
                                ToQColor( settings->GetLayerColor( LAYER_REFERENCEPART ) ) );
        m_status->setPalette( statusPalette );

        statusSizer->addStretch( 1 );
        statusSizer->addWidget( m_status );
        statusSizer->addStretch( 1 );

        m_stack->addWidget( m_statusPanel );

        // The canvas and the status are aligned with small top and bottom borders
        outerSizer->setContentsMargins( 0, 5, 0, 5 );
    }
    else
    {
        outerSizer->setContentsMargins( 0, 0, 0, 0 );
    }

    m_stack->setCurrentWidget( m_preview );
    outerSizer->addLayout( m_stack, 1 );
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    KI_CHECK( m_statusPanel, /* void */ );

    m_status->setText( ToQString( aText ) );
    m_stack->setCurrentWidget( m_statusPanel );
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::resizeEvent( QResizeEvent* aEvent )
{
    DESIGN_BLOCK_PREVIEW_WIDGET::resizeEvent( aEvent );

    if( m_previewItem )
    {
        fitOnDrawArea();
        m_preview->ForceRefresh();
    }
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::showCanvas()
{
    m_stack->setCurrentWidget( m_preview );
}
