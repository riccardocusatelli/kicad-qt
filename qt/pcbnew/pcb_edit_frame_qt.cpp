/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file pcb_edit_frame_qt.cpp
 * The parts of PCB_EDIT_FRAME with Qt: the layout of its panes, its events and the event
 * counters of the canvas (see pcbnew/pcb_edit_frame_wx.cpp).
 *
 * The remote commands of the standalone editors are received by KIWAY_PLAYER::CreateServer()
 * and the dropped files by EDA_BASE_FRAME::dropEvent(): the frame has no events of its own for
 * them.
 */

#include <memory>

#include <QApplication>
#include <QBoxLayout>
#include <QDockWidget>
#include <QEvent>

#include <class_draw_panel_gal.h>
#include <core/profile.h>
#include <id.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <ui_hooks.h>
#include <view/view.h>
#include <view/qt_view_controls.h>
#include <widgets/appearance_controls.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_infobar.h>


/**
 * The events of the frame that Qt gives to the event filters: the first resizes of the frame,
 * until it is shown with its final size.
 */
struct PCB_EDIT_FRAME::TOOLKIT_STATE : public QObject
{
    TOOLKIT_STATE( PCB_EDIT_FRAME* aFrame ) :
            QObject( aFrame ),
            m_frame( aFrame )
    {
        aFrame->installEventFilter( this );
    }

    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        // This is used temporarily to fix a client size issue that causes zoom to fit to
        // calculate the wrong zoom size.  See PCB_EDIT_FRAME::onSize().  It is needed only
        // until the frame is done resizing and the final client size is established.
        if( !m_zoomFitDone && aWatched == m_frame
            && ( aEvent->type() == QEvent::Resize || aEvent->type() == QEvent::Show )
            && m_frame->isVisible() )
        {
            m_zoomFitDone = true;

            // After the frame lays out its children with the new size
            m_frame->CallAfter( [frame = m_frame]() { frame->onSize(); } );
        }

        return false;
    }

    PCB_EDIT_FRAME* m_frame;
    bool            m_zoomFitDone = false;
};


void PCB_EDIT_FRAME::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void PCB_EDIT_FRAME::setupLayout()
{
    CreateInfoBar();

    // The canvas, the info bar and the message panel
    setupCentralArea();

    // Secondary infobar stacked above the main one.  Load-time notices (such as the WRL -> STEP
    // migration prompt) belong here so they aren't clobbered by the main infobar's read-only
    // warnings, DRC rule errors, etc.
    m_loadNoticeInfoBar = new WX_INFOBAR( centralWidget() );

    if( QBoxLayout* layout = qobject_cast<QBoxLayout*>( centralWidget()->layout() ) )
        layout->insertWidget( 0, m_loadNoticeInfoBar );

    m_loadNoticeInfoBar->hide();

    PCBNEW_SETTINGS* settings = GetPcbNewSettings();

    // The panels docked on the right side: the appearance and the selection filter, one above
    // the other, then the design blocks
    QDockWidget* appearance = addPane( m_appearancePanel, wxS( "LayersManager" ),
                                       _( "Appearance" ), PANE_DOCK::RIGHT,
                                       m_ShowLayerManagerTools );
    QDockWidget* selectionFilter = addPane( m_selectionFilterPanel, wxS( "SelectionFilter" ),
                                            _( "Selection Filter" ), PANE_DOCK::RIGHT,
                                            m_ShowLayerManagerTools );

    splitDockWidget( appearance, selectionFilter, Qt::Vertical );

    // The appearance and the selection filter are always shown together (they are hidden
    // with the layers manager tools, not closed)
    for( QDockWidget* dock : { appearance, selectionFilter } )
        dock->setFeatures( dock->features() & ~QDockWidget::DockWidgetClosable );

    QDockWidget* designBlocks = addPane( m_designBlocksPane, DesignBlocksPaneName(),
                                         _( "Design Blocks" ), PANE_DOCK::RIGHT,
                                         settings->m_AuiPanels.design_blocks_show );

    // The design blocks can't be docked on the top and on the bottom sides
    designBlocks->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    m_designBlocksPane->setMinimumSize( 240, 60 );

    // The properties on the left side
    addPane( m_propertiesPanel, PropertiesPaneName(), _( "Properties" ), PANE_DOCK::LEFT,
             settings->m_AuiPanels.show_properties );
    m_propertiesPanel->setMinimumSize( 240, 60 );

    // The net inspector and the search on the bottom side
    QDockWidget* netInspector = addPane( m_netInspectorPanel, NetInspectorPanelName(),
                                         _( "Net Inspector" ), PANE_DOCK::BOTTOM,
                                         m_ShowNetInspector );
    QDockWidget* search = addPane( m_searchPane, SearchPaneName(), _( "Search" ),
                                   PANE_DOCK::BOTTOM, m_ShowSearch );

    m_netInspectorPanel->setMinimumSize( 240, 60 );
    m_searchPane->setMinimumSize( 180, 60 );

    tabifyDockWidget( netInspector, search );

    // The best sizes of the panels (as the wxWidgets build; the sizes of Qt are independent of
    // the DPI)
    resizeDocks( { appearance, designBlocks }, { m_appearancePanel->sizeHint().width(), 300 },
                 Qt::Horizontal );

    // The selection filter doesn't need to grow in the vertical direction when docked
    resizeDocks( { selectionFilter }, { m_selectionFilterPanel->sizeHint().height() },
                 Qt::Vertical );

    resizeDocks( { netInspector }, { 200 }, Qt::Vertical );
}


void PCB_EDIT_FRAME::setupToolkitEvents()
{
    m_toolkitState.reset( new TOOLKIT_STATE( this ) );

    // Menu Files:
    for( int id = ID_FILE1; id <= ID_FILEMAX; ++id )
        BindMenuCommand( id, [this, id]() { OnFileHistory( id ); } );

    BindMenuCommand( ID_FILE_LIST_CLEAR, [this]() { OnClearFileHistory(); } );

    for( int id : { wxID_EXIT, wxID_CLOSE } )
        BindMenuCommand( id, [this, id]() { OnQuit( id ); } );

    // Tracks and vias sizes general options
    for( int id = ID_POPUP_PCB_SELECT_WIDTH_START_RANGE;
         id <= ID_POPUP_PCB_SELECT_WIDTH_END_RANGE; ++id )
    {
        BindMenuCommand( id, [this, id]() { Tracks_and_Vias_Size_Event( id ); } );
    }

    // After the events of the user: the choice boxes of the track width and the via size
    // follow the board, and the net names of the tracks follow the viewport
    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                if( m_SelTrackWidthBox )
                                    OnUpdateSelectTrackWidth();

                                if( m_SelViaSizeBox )
                                    OnUpdateSelectViaSize();

                                if( !GetCanvas() )
                                    return;

                                BOX2D viewport = GetCanvas()->GetView()->GetViewport();

                                if( viewport != m_lastNetnamesViewport )
                                {
                                    redrawNetnames();
                                    m_lastNetnamesViewport = viewport;
                                }
                            } );
}


void PCB_EDIT_FRAME::releaseToolkitEvents()
{
    if( !m_toolkitState )
        return;

    removeEventFilter( m_toolkitState.get() );
    m_toolkitState.reset();
}


void PCB_EDIT_FRAME::showEventCounters()
{
    GetCanvas()->m_PaintEventCounter->Show();
    GetCanvas()->m_PaintEventCounter->Reset();

    KIGFX::QT_VIEW_CONTROLS* vc =
            static_cast<KIGFX::QT_VIEW_CONTROLS*>( GetCanvas()->GetViewControls() );
    vc->m_MotionEventCounter->Show();
    vc->m_MotionEventCounter->Reset();
}
