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
 * @file pcb_edit_frame_wx.cpp
 * The parts of PCB_EDIT_FRAME with wxWidgets: the layout of its panes, its events and the event
 * counters of the canvas.
 */

#include <memory>

#include <wx/app.h>
#include <wx/socket.h>

#include <advanced_config.h>
#include <class_draw_panel_gal.h>
#include <core/profile.h>
#include <dialogs/dialog_book_reporter.h>
#include <dialogs/dialog_footprint_fields_table.h>
#include <id.h>
#include <pcb_edit_frame.h>
#include <widgets/panel_constraints.h>
#include <widgets/git_commit_pane.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <tool/action_toolbar.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <widgets/appearance_controls.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_infobar.h>



/// The event handlers of the frame that are unbound before the frame is deleted.
struct PCB_EDIT_FRAME::TOOLKIT_STATE : public wxEvtHandler
{
    TOOLKIT_STATE( PCB_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    void onCloseModelessBookReporterDialogs( wxCommandEvent& aEvent )
    {
        m_frame->onCloseModelessBookReporterDialogs( aEvent.GetString(), aEvent.GetId() );
    }

    void onCloseFootprintFieldsTableDialog( wxCommandEvent& aEvent )
    {
        m_frame->onCloseFootprintFieldsTableDialog();
    }

    PCB_EDIT_FRAME* m_frame;
};


void PCB_EDIT_FRAME::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void PCB_EDIT_FRAME::setupLayout()
{
    // The panels only the wxWidgets build has so far
    m_constraintsPanel = new PANEL_CONSTRAINTS( this );
    m_gitCommitPane = new GIT_COMMIT_PANE( this );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Secondary infobar stacked above the main one.  Load-time notices (such as
    // the WRL -> STEP migration prompt) belong here so they aren't clobbered by
    // the main infobar's read-only warnings, DRC rule errors, etc.
    m_loadNoticeInfoBar = new WX_INFOBAR( GetToolCanvas(), wxID_ANY, true );

    unsigned int auiFlags = wxAUI_MGR_DEFAULT;
#if !defined( _WIN32 )
    // Windows cannot redraw the UI fast enough during a live resize and may lead to all kinds
    // of graphical glitches.
    auiFlags |= wxAUI_MGR_LIVE_RESIZE;
#endif
    m_auimgr.SetFlags( auiFlags );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_tbTopAux, EDA_PANE().HToolbar().Name( wxS( "TopAuxToolbar" ) )
                      .Top().Layer( 5 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( wxS( "LeftToolbar" ) )
                      .Left().Layer( 3 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( wxS( "RightToolbar" ) )
                      .Right().Layer( 3 ) );

    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( wxS( "LayersManager" ) )
                      .Right().Layer( 4 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( m_appearancePanel->GetMinSize().x, FromDIP( 60 ) )
#ifdef __WXMAC__
                      // Best size for this pane is calculated larger than necessary on wxMac
                      .BestSize( m_appearancePanel->GetMinSize().x, -1 )
#else
                      .BestSize( m_appearancePanel->GetBestSize().x, -1 )
#endif
                      .FloatingSize( m_appearancePanel->GetBestSize() )
                      .CloseButton( false ) );

    m_auimgr.AddPane( m_selectionFilterPanel, EDA_PANE().Name( wxS( "SelectionFilter" ) )
                      .Right().Layer( 4 ).Position( 2 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false )
                      // Fixed-size pane; -1 for MinSize height is required
                      .MinSize( m_selectionFilterPanel->GetMinSize().x, -1 )
                      .BestSize( m_selectionFilterPanel->GetBestSize().x, -1 )
                      .FloatingSize( m_selectionFilterPanel->GetBestSize() )
                      .CloseButton( false ) );

    m_auimgr.AddPane( m_designBlocksPane, EDA_PANE().Name( DesignBlocksPaneName() )
                      .Right().Layer( 5 )
                      .Caption( _( "Design Blocks" ) )
                      .CaptionVisible( true )
                      .PaneBorder( true )
                      .TopDockable( false )
                      .BottomDockable( false )
                      .CloseButton( true )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( FromDIP( wxSize( 800, 600 ) ) )
                      .FloatingPosition( FromDIP( wxPoint( 50, 200 ) ) )
                      .Show( true ) );

    m_auimgr.AddPane( m_propertiesPanel, EDA_PANE().Name( PropertiesPaneName() )
                      .Left().Layer( 5 )
                      .Caption( _( "Properties" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( wxSize( 300, 200 ) )
                      .CloseButton( true ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    m_auimgr.AddPane( m_netInspectorPanel, EDA_PANE().Name( NetInspectorPanelName() )
                      .Bottom()
                      .Caption( _( "Net Inspector" ) )
                      .PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 300, 200 ) ) )
                      .FloatingSize( wxSize( 300, 200 ) )
                      .CloseButton( true ) );

    m_auimgr.AddPane( m_searchPane, EDA_PANE().Name( SearchPaneName() )
                      .Bottom()
                      .Caption( _( "Search" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize ( 180, 60 ) ) )
                      .BestSize( FromDIP( wxSize ( 180, 100 ) ) )
                      .FloatingSize( FromDIP( wxSize( 480, 200 ) ) )
                      .DestroyOnClose( false )
                      .CloseButton( true ) );

    m_auimgr.AddPane( m_constraintsPanel, EDA_PANE().Name( ConstraintsPaneName() )
                      .Bottom().Layer( 1 )
                      .Caption( _( "Geometric Constraints" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 360, 120 ) ) )
                      .BestSize( FromDIP( wxSize( 600, 200 ) ) )
                      .FloatingSize( FromDIP( wxSize( 600, 240 ) ) )
                      .DestroyOnClose( false )
                      .CloseButton( true )
                      .Hide() );

    m_auimgr.AddPane( m_gitCommitPane,
                      EDA_PANE()
                              .Name( GitCommitPaneName() )
                              .Right()
                              .Layer( 5 )
                              .Caption( _( "Version Control" ) )
                              .PaneBorder( false )
                              .MinSize( wxSize( m_gitCommitPane->GetMinDockWidth(), FromDIP( 120 ) ) )
                              .BestSize( wxSize( std::max( m_gitCommitPane->GetMinDockWidth(), FromDIP( 300 ) ),
                                                 FromDIP( 300 ) ) )
                              .FloatingSize( FromDIP( wxSize( 300, 400 ) ) )
                              .DestroyOnClose( false )
                              .CloseButton( true )
                              .Hide() );

    RestoreAuiLayout();

    // A saved layout brings its own minimum, so the one the panel needs is set again.
    m_auimgr.GetPane( GitCommitPaneName() ).min_size.x = m_gitCommitPane->GetMinDockWidth();

    m_auimgr.GetPane( "LayersManager" ).Show( m_ShowLayerManagerTools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_ShowLayerManagerTools );
    m_auimgr.GetPane( PropertiesPaneName() ).Show( GetPcbNewSettings()->m_AuiPanels.show_properties );
    m_auimgr.GetPane( NetInspectorPanelName() ).Show( m_ShowNetInspector );
    m_auimgr.GetPane( SearchPaneName() ).Show( m_ShowSearch );
    m_auimgr.GetPane( DesignBlocksPaneName() ).Show( GetPcbNewSettings()->m_AuiPanels.design_blocks_show );
    m_auimgr.GetPane( GitCommitPaneName() ).Show( GetPcbNewSettings()->m_AuiPanels.show_git_commit_pane );

    // The selection filter doesn't need to grow in the vertical direction when docked
    m_auimgr.GetPane( "SelectionFilter" ).dock_proportion = 0;
    FinishAUIInitialization();
}


void PCB_EDIT_FRAME::setupToolkitEvents()
{
    Bind( wxEVT_SOCKET, &PCB_EDIT_FRAME::OnSockRequestServer, this, ID_EDA_SOCKET_EVENT_SERV );
    Bind( wxEVT_SOCKET, &PCB_EDIT_FRAME::OnSockRequest, this, ID_EDA_SOCKET_EVENT );

    Bind( wxEVT_CHOICE, &PCB_EDIT_FRAME::OnSelectZoom, this, ID_ON_ZOOM_SELECT );
    Bind( wxEVT_CHOICE, &PCB_EDIT_FRAME::OnSelectGrid, this, ID_ON_GRID_SELECT );

    // The canvas refits itself after a resize while the view is still at its zoom to fit
    Bind( wxEVT_SIZE,
          [this]( wxSizeEvent& aEvent )
          {
              // Skip() is called in the base class.
              OnSize( aEvent );
          } );

    // Menu Files:
    Bind( wxEVT_MENU,
          [this]( wxCommandEvent& aEvent )
          {
              OnFileHistory( aEvent.GetId() );
          },
          ID_FILE1, ID_FILEMAX );

    Bind( wxEVT_MENU,
          [this]( wxCommandEvent& aEvent )
          {
              OnClearFileHistory();
          },
          ID_FILE_LIST_CLEAR );

    for( int id : { wxID_EXIT, wxID_CLOSE } )
    {
        Bind( wxEVT_MENU,
              [this]( wxCommandEvent& aEvent )
              {
                  OnQuit( aEvent.GetId() );
              },
              id );
    }

    // Tracks and vias sizes general options
    Bind( wxEVT_MENU,
          [this]( wxCommandEvent& aEvent )
          {
              Tracks_and_Vias_Size_Event( aEvent.GetId() );
          },
          ID_POPUP_PCB_SELECT_WIDTH_START_RANGE, ID_POPUP_PCB_SELECT_WIDTH_END_RANGE );

    // User interface update event handlers.
    Bind( wxEVT_UPDATE_UI,
          [this]( wxUpdateUIEvent& aEvent )
          {
              OnUpdateSelectTrackWidth();
          },
          ID_AUX_TOOLBAR_PCB_TRACK_WIDTH );

    Bind( wxEVT_UPDATE_UI,
          [this]( wxUpdateUIEvent& aEvent )
          {
              OnUpdateSelectViaSize();
          },
          ID_AUX_TOOLBAR_PCB_VIA_SIZE );

    // Drop files event
    Bind( wxEVT_DROP_FILES, &PCB_EDIT_FRAME::OnDropFiles, this );

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              BOX2D viewport = GetCanvas()->GetView()->GetViewport();

              if( viewport != m_lastNetnamesViewport )
              {
                  redrawNetnames();
                  m_lastNetnamesViewport = viewport;
                  m_lastNetnamesScale = GetCanvas()->GetView()->GetScale();
              }

              // Do not forget to pass the Idle event to other clients:
              aEvent.Skip();
          } );

    m_toolkitState.reset( new TOOLKIT_STATE( this ) );

    Bind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &TOOLKIT_STATE::onCloseModelessBookReporterDialogs,
          m_toolkitState.get() );
    Bind( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, &TOOLKIT_STATE::onCloseFootprintFieldsTableDialog,
          m_toolkitState.get() );
}


void PCB_EDIT_FRAME::releaseToolkitEvents()
{
    if( !m_toolkitState )
        return;

    // Clean up mode-less dialogs.
    Unbind( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, &TOOLKIT_STATE::onCloseModelessBookReporterDialogs,
            m_toolkitState.get() );
    Unbind( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, &TOOLKIT_STATE::onCloseFootprintFieldsTableDialog,
            m_toolkitState.get() );

    m_toolkitState.reset();
}


void PCB_EDIT_FRAME::showEventCounters()
{
    GetCanvas()->m_PaintEventCounter->Show();
    GetCanvas()->m_PaintEventCounter->Reset();

    KIGFX::WX_VIEW_CONTROLS* vc =
            static_cast<KIGFX::WX_VIEW_CONTROLS*>( GetCanvas()->GetViewControls() );
    vc->m_MotionEventCounter->Show();
    vc->m_MotionEventCounter->Reset();
}
