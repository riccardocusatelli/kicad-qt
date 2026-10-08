/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file eda_draw_frame.cpp
 * EDA_DRAW_FRAME of the Qt build: the port of common/eda_draw_frame.cpp to the Qt frame.
 */

#include <eda_draw_frame.h>
#include <widgets/kistatusbar.h>

#include <cmath>

#include <QBoxLayout>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLabel>
#include <QStatusBar>
#include <QWidget>

#include <base_screen.h>
#include <bitmaps/bitmap_types.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <eda_search_data.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiface_base.h>
#include <kiway.h>
#include <lockfile.h>
#include <math/util.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
#include <tool/grid_helper.h>
#include <tool/grid_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/zoom_menu.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <widgets/msgpanel.h>
#include <widgets/wx_infobar.h>
#include <core/ki_assert.h>

#include <qt_strings.h>

#define FR_HISTORY_LIST_CNT     10   ///< Maximum size of the find/replace history stacks.

/// The fields of the status bar
enum STATUS_FIELD
{
    STATUS_MESSAGE = 0,
    STATUS_ZOOM,
    STATUS_CURSOR,
    STATUS_DELTA,
    STATUS_GRID,
    STATUS_UNITS,
    STATUS_TOOL,
    STATUS_CONSTRAINT,
    STATUS_FIELD_COUNT
};


EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, QWidget* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos,
                                const wxSize& aSize, long aStyle,
                                const wxString& aFrameName, const EDA_IU_SCALE& aIuScale ) :
        KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName,
                      aIuScale ),
        m_lastToolbarIconSize( 0 )
{
    m_undoRedoCountMax    = DEFAULT_MAX_UNDO_ITEMS;

    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvas              = nullptr;
    m_toolDispatcher      = nullptr;
    m_messagePanel        = nullptr;
    m_currentScreen       = nullptr;
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_gridColor           = COLOR4D( DARKGRAY );   // Default grid color
    m_drawBgColor         = COLOR4D( BLACK );   // the background color of the draw canvas:
                                                // BLACK for Pcbnew, BLACK or WHITE for Eeschema
    m_colorSettings       = nullptr;
    m_polarCoords         = false;
    m_findReplaceData     = std::make_unique<EDA_SEARCH_DATA>();
    m_propertiesPanel     = nullptr;

    SetUserUnits( EDA_UNITS::MM );

    createStatusBar();

    m_messagePanel = new EDA_MSG_PANEL( this );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    saveCanvasTypeSetting( m_canvasType );

    // The tools may refer to the canvas: they go first
    delete m_actions;
    m_actions = nullptr;

    delete m_toolManager;
    m_toolManager = nullptr;

    delete m_toolDispatcher;
    m_toolDispatcher = nullptr;

    delete m_canvas;
    m_canvas = nullptr;

    delete m_currentScreen;
    m_currentScreen = nullptr;

    ReleaseFile();
}


void EDA_DRAW_FRAME::createStatusBar()
{
    CreateStatusBar( STATUS_FIELD_COUNT );
    updateStatusBarWidths();
}


KISTATUSBAR* EDA_DRAW_FRAME::OnCreateStatusBar( int aNumber )
{
    return new KISTATUSBAR( aNumber, this, wxID_ANY, KISTATUSBAR::STYLE_FLAGS::WARNING_ICON );
}


void EDA_DRAW_FRAME::updateStatusBarWidths()
{
    QFontMetrics metrics( GetStatusBar()->font() );
    int          spacer = metrics.horizontalAdvance( QLatin1Char( 'M' ) );

    auto textWidth =
            [&]( const wxString& aText )
            {
                return metrics.horizontalAdvance( ToQString( aText ) ) + spacer;
            };

    // As the wxWidgets build: a negative width is a stretch factor
    std::vector<int> dims = {
        // remainder of status bar on far left is set to a default or whatever is left over.
        -2,

        // zoom:
        textWidth( wxT( "Z 762000" ) ),

        // cursor coords
        textWidth( wxT( "X 00000.0000  Y 00000.0000" ) ),

        // delta distances
        textWidth( wxT( "dx 00000.0000  dy 00000.0000  dist 00000.0000" ) ),

        // grid size
        textWidth( wxT( "grid 0000.0000 x 0000.0000" ) ),

        // units display, Inches is bigger than mm
        textWidth( _( "Inches" ) ),

        // Size for the "Current Tool" panel
        -1,

        // constraint mode
        textWidth( _( "Constrain to H, V, 45" ) )
    };

    SetStatusWidths( dims.size(), dims.data() );
}


void EDA_DRAW_FRAME::setupCentralArea()
{
    QWidget*     central = new QWidget( this );
    QVBoxLayout* layout = new QVBoxLayout( central );

    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    if( !m_infoBar )
        CreateInfoBar();

    m_infoBar->setParent( central );
    layout->addWidget( m_infoBar );

    if( m_canvas )
    {
        m_canvas->setParent( central );
        layout->addWidget( m_canvas, 1 );
    }

    m_messagePanel->setParent( central );
    layout->addWidget( m_messagePanel );

    setCentralWidget( central );
}


void EDA_DRAW_FRAME::keyPressEvent( QKeyEvent* aEvent )
{
    // The keys that the widgets of the frame didn't use are hotkeys of the tools
    if( m_toolDispatcher && m_toolDispatcher->DispatchQtEvent( aEvent ) )
    {
        aEvent->accept();
        return;
    }

    KIWAY_PLAYER::keyPressEvent( aEvent );
}


void EDA_DRAW_FRAME::handleActivateEvent( bool aActive )
{
    // Force a refresh of the message panel to ensure that the text is the right color
    // when the window activates
    if( !isMinimized() && m_messagePanel )
        m_messagePanel->update();
}


EDA_SEARCH_DATA& EDA_DRAW_FRAME::GetFindReplaceData()
{
    return *m_findReplaceData;
}


void EDA_DRAW_FRAME::ReleaseFile()
{
    if( m_file_checker )
        m_file_checker->UnlockFile();
}


bool EDA_DRAW_FRAME::LockFile( const wxString& aFileName )
{
    // We need to explicitly reset here to get the deletion before we create a new unique_ptr that
    // may be for the same file.
    m_file_checker.reset();

    m_file_checker = std::make_unique<LOCKFILE>( aFileName );

    // If the file is valid, return true.  This could mean that the file is locked or it could mean
    // that the file is read-only.
    return m_file_checker->Valid();
}


void EDA_DRAW_FRAME::ScriptingConsoleEnableDisable()
{
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PYTHON, false );

    QRect   rect = frameGeometry();
    QPoint  center = rect.center();

    if( !frame )
    {
        frame = Kiway().Player( FRAME_PYTHON, true, Kiway().GetTop() );

        // If we received an error in the CTOR due to Python-ness, don't crash
        if( !frame )
            return;

        if( !frame->isVisible() )
            frame->show();

        // Raise() does not bring the window on screen, when iconized
        if( frame->isMinimized() )
            frame->showNormal();

        frame->raise();
        frame->move( center - QPoint( frame->width() / 2, frame->height() / 2 ) );

        return;
    }

    frame->setVisible( !frame->isVisible() );
    frame->move( center - QPoint( frame->width() / 2, frame->height() / 2 ) );
}


bool EDA_DRAW_FRAME::IsScriptingConsoleVisible()
{
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PYTHON, false );
    return frame && frame->isVisible();
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    // Notify all tools the units have changed
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::updateUnits );

    UpdateStatusBar();
    UpdateMsgPanel();
    UpdateProperties();

    switch( GetUserUnits() )
    {
    default:
    case EDA_UNITS::MM:   SelectToolbarAction( ACTIONS::millimetersUnits ); break;
    case EDA_UNITS::INCH: SelectToolbarAction( ACTIONS::inchesUnits );      break;
    case EDA_UNITS::MILS: SelectToolbarAction( ACTIONS::milsUnits );        break;
    }
}


void EDA_DRAW_FRAME::ToggleUserUnits()
{
    if( m_toolManager->GetTool<COMMON_TOOLS>() )
    {
        TOOL_EVENT dummy;
        m_toolManager->GetTool<COMMON_TOOLS>()->ToggleUnits( dummy );
    }
    else
    {
        ChangeUserUnits( GetUserUnits() == EDA_UNITS::INCH ? EDA_UNITS::MM : EDA_UNITS::INCH );
    }
}


void EDA_DRAW_FRAME::CommonSettingsChanged( int aFlags )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aFlags );

    COMMON_SETTINGS*      settings = Pgm().GetCommonSettings();
    KIGFX::VIEW_CONTROLS* viewControls = GetCanvas()->GetViewControls();

    if( m_supportsAutoSave && m_autoSaveTimer.IsRunning() )
    {
        if( GetAutoSaveInterval() > 0 )
        {
            m_autoSaveTimer.StartOnce( GetAutoSaveInterval() * 1000 );
        }
        else
        {
            m_autoSaveTimer.Stop();
            m_autoSavePending = false;
        }
    }

    viewControls->LoadSettings();

    m_galDisplayOptions.ReadCommonConfig( *settings, this );

    GetToolManager()->RunAction( ACTIONS::gridPreset, GetWindowSettings( config() )->grid.last_size_idx );
    UpdateGridSelectBox();

    m_lastToolbarIconSize = settings->m_Appearance.toolbar_icon_size;

    resolveCanvasType();

    if( m_canvasType != GetCanvas()->GetBackend() )
    {
        // Try to switch (will automatically fallback if necessary)
        SwitchCanvas( m_canvasType );
        m_canvasType = GetCanvas()->GetBackend();
    }

    // Notify all tools the preferences have changed
    if( m_toolManager )
        m_toolManager->RunAction( ACTIONS::updatePreferences );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::UpdateGridSelectBox()
{
    UpdateStatusBar();
    DisplayUnitsMsg();
    DisplayGridMsg();
}


void EDA_DRAW_FRAME::SyncGridSelectBox()
{
    DisplayGridMsg();
}


void EDA_DRAW_FRAME::UpdateZoomSelectBox()
{
    UpdateStatusBar();
}


bool EDA_DRAW_FRAME::GetOverrideLocks() const
{
    return false;
}


bool EDA_DRAW_FRAME::IsGridVisible()
{
    KI_CHECK( config(), true );

    return GetWindowSettings( config() )->grid.show;
}


void EDA_DRAW_FRAME::SetGridVisibility( bool aVisible )
{
    KI_CHECK( config(), /* void */ );

    GetWindowSettings( config() )->grid.show = aVisible;

    // Update the display with the new grid
    if( GetCanvas() )
    {
        // Check to ensure these exist, since this function could be called before
        // the GAL and View have been created
        if( GetCanvas()->GetGAL() )
            GetCanvas()->GetGAL()->SetGridVisibility( aVisible );

        if( GetCanvas()->GetView() )
            GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

        GetCanvas()->Refresh();
    }
}


bool EDA_DRAW_FRAME::IsGridOverridden()
{
    KI_CHECK( config(), false );

    return GetWindowSettings( config() )->grid.overrides_enabled;
}


void EDA_DRAW_FRAME::SetGridOverrides( bool aOverride )
{
    KI_CHECK( config(), /* void */ );

    GetWindowSettings( config() )->grid.overrides_enabled = aOverride;
}


std::unique_ptr<GRID_HELPER> EDA_DRAW_FRAME::MakeGridHelper()
{
    return nullptr;
}


void EDA_DRAW_FRAME::AddStandardSubMenus( TOOL_MENU& aToolMenu )
{
    COMMON_TOOLS*     commonTools = m_toolManager->GetTool<COMMON_TOOLS>();
    CONDITIONAL_MENU& aMenu = aToolMenu.GetMenu();

    aMenu.AddSeparator( 1000 );

    std::shared_ptr<ZOOM_MENU> zoomMenu = std::make_shared<ZOOM_MENU>( this );
    zoomMenu->SetTool( commonTools );
    aToolMenu.RegisterSubMenu( zoomMenu );

    std::shared_ptr<GRID_MENU> gridMenu = std::make_shared<GRID_MENU>( this );
    gridMenu->SetTool( commonTools );
    aToolMenu.RegisterSubMenu( gridMenu );

    aMenu.AddMenu( zoomMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddMenu( gridMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    if( m_isClosing )
        return;

    SetStatusText( msg, STATUS_TOOL );
}


void EDA_DRAW_FRAME::DisplayConstraintsMsg( const wxString& msg )
{
    if( m_isClosing )
        return;

    SetStatusText( msg, STATUS_CONSTRAINT );
}


void EDA_DRAW_FRAME::DisplayGridMsg()
{
    if( m_isClosing || !config() )
        return;

    wxString msg;

    GRID_SETTINGS& gridSettings = GetWindowSettings( config() )->grid;
    int            currentIdx = gridSettings.last_size_idx;

    if( currentIdx < 0 || currentIdx >= (int) gridSettings.grids.size() )
        return;

    msg.Printf( _( "grid %s" ), gridSettings.grids[currentIdx].UserUnitsMessageText( this, false ) );

    SetStatusText( msg, STATUS_GRID );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    if( m_isClosing )
        return;

    wxString msg;

    switch( GetUserUnits() )
    {
    case EDA_UNITS::INCH: msg = _( "inches" ); break;
    case EDA_UNITS::MILS: msg = _( "mils" );   break;
    case EDA_UNITS::MM:   msg = _( "mm" );     break;
    default:              msg = _( "Units" );  break;
    }

    SetStatusText( msg, STATUS_UNITS );
}


void EDA_DRAW_FRAME::UpdateStatusBar()
{
    if( m_isClosing )
        return;

    SetStatusText( GetZoomLevelIndicator(), STATUS_ZOOM );

    // Absolute and relative cursor positions are handled by overloading this function and
    // handling the internal to user units conversion at the appropriate level.

    // refresh units display
    DisplayUnitsMsg();
}


const wxString EDA_DRAW_FRAME::GetZoomLevelIndicator() const
{
    // returns a human readable value which can be displayed as zoom
    // level indicator in dialogs.
    double zoom = m_canvas->GetGAL()->GetZoomFactor();
    return wxString::Format( wxT( "Z %.2f" ), zoom );
}


void EDA_DRAW_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    COMMON_SETTINGS* cmnCfg = Pgm().GetCommonSettings();
    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    // Read units used in dialogs and toolbars
    SetUserUnits( static_cast<EDA_UNITS>( aCfg->m_System.units ) );

    m_undoRedoCountMax = aCfg->m_System.max_undo_items;

    m_galDisplayOptions.ReadConfig( *cmnCfg, *window, this );

    m_findReplaceData->findString = aCfg->m_FindReplace.find_string;
    m_findReplaceData->replaceString = aCfg->m_FindReplace.replace_string;
    m_findReplaceData->matchMode = static_cast<EDA_SEARCH_MATCH_MODE>( aCfg->m_FindReplace.match_mode );
    m_findReplaceData->matchCase = aCfg->m_FindReplace.match_case;
    m_findReplaceData->searchAndReplace = aCfg->m_FindReplace.search_and_replace;

    for( const wxString& s : aCfg->m_FindReplace.find_history )
        m_findStringHistoryList.Add( s );

    for( const wxString& s : aCfg->m_FindReplace.replace_history )
        m_replaceStringHistoryList.Add( s );

    m_lastToolbarIconSize = cmnCfg->m_Appearance.toolbar_icon_size;
}


void EDA_DRAW_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    WINDOW_SETTINGS* window = GetWindowSettings( aCfg );

    aCfg->m_System.units = static_cast<int>( GetUserUnits() );
    aCfg->m_System.max_undo_items = GetMaxUndoItems();

    m_galDisplayOptions.WriteConfig( *window );

    aCfg->m_FindReplace.search_and_replace = m_findReplaceData->searchAndReplace;

    aCfg->m_FindReplace.find_string = m_findReplaceData->findString;
    aCfg->m_FindReplace.replace_string = m_findReplaceData->replaceString;

    aCfg->m_FindReplace.find_history.clear();
    aCfg->m_FindReplace.replace_history.clear();

    for( size_t i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
        aCfg->m_FindReplace.find_history.push_back( m_findStringHistoryList[ i ].ToStdString() );

    for( size_t i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
        aCfg->m_FindReplace.replace_history.push_back( m_replaceStringHistoryList[ i ].ToStdString() );

    // Save the units used in this frame
    if( m_toolManager )
    {
        if( COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>() )
        {
            aCfg->m_System.last_imperial_units = static_cast<int>( cmnTool->GetLastImperialUnits() );
            aCfg->m_System.last_metric_units = static_cast<int>( cmnTool->GetLastMetricUnits() );
        }
    }
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding )
{
    if( m_messagePanel && !m_isClosing )
        m_messagePanel->AppendMessage( aTextUpper, aTextLower, aPadding );
}


void EDA_DRAW_FRAME::ClearMsgPanel()
{
    if( m_messagePanel && !m_isClosing )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::SetMsgPanel( const std::vector<MSG_PANEL_ITEM>& aList )
{
    if( m_messagePanel && !m_isClosing )
    {
        m_messagePanel->EraseMsgBox();

        for( const MSG_PANEL_ITEM& item : aList )
            m_messagePanel->AppendMessage( item );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding )
{
    if( m_messagePanel && !m_isClosing )
    {
        m_messagePanel->EraseMsgBox();
        m_messagePanel->AppendMessage( aTextUpper, aTextLower, aPadding );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    KI_CHECK_RET( aItem, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    std::vector<MSG_PANEL_ITEM> items;
    aItem->GetMsgPanelInfo( GetMsgPanelContext(), items );
    SetMsgPanel( items );
}


FRAME_MSG_PANEL_CONTEXT::FRAME_MSG_PANEL_CONTEXT( EDA_DRAW_FRAME* aFrame ) :
        MSG_PANEL_CONTEXT( aFrame->GetIuScale(), aFrame->GetUserUnits() ),
        m_frame( aFrame )
{
}


void FRAME_MSG_PANEL_CONTEXT::GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits )
{
    m_frame->GetUnitPair( aPrimaryUnit, aSecondaryUnits );
}


ORIGIN_TRANSFORMS& FRAME_MSG_PANEL_CONTEXT::GetOriginTransforms()
{
    return m_frame->GetOriginTransforms();
}


FRAME_T FRAME_MSG_PANEL_CONTEXT::GetFrameType() const
{
    return m_frame->GetFrameType();
}


wxString FRAME_MSG_PANEL_CONTEXT::EllipsizeStatusText( const wxString& aText ) const
{
    // At most a third of the width of the frame, as the status texts of the wxWidgets build
    QString      text = ToQString( aText );
    QFontMetrics metrics( m_frame->GetStatusBar() ? m_frame->GetStatusBar()->font()
                                                  : m_frame->font() );

    text.replace( QLatin1Char( '\n' ), QLatin1Char( ' ' ) );
    text.replace( QLatin1Char( '\r' ), QLatin1Char( ' ' ) );
    text.replace( QLatin1Char( '\t' ), QLatin1Char( ' ' ) );

    return ToWxString( metrics.elidedText( text, Qt::ElideMiddle, m_frame->width() / 3 ) );
}


EDA_ITEM* FRAME_MSG_PANEL_CONTEXT::ResolveItem( const KIID& aId, bool aAllowNullptrReturn ) const
{
    return m_frame->ResolveItem( aId, aAllowNullptrReturn );
}


std::unique_ptr<MSG_PANEL_CONTEXT> EDA_DRAW_FRAME::createMsgPanelContext()
{
    return std::make_unique<FRAME_MSG_PANEL_CONTEXT>( this );
}


MSG_PANEL_CONTEXT* EDA_DRAW_FRAME::GetMsgPanelContext()
{
    if( !m_msgPanelContext )
        m_msgPanelContext = createMsgPanelContext();

    // Units can change at any time; the scale is fixed per frame.
    m_msgPanelContext->SetUserUnits( GetUserUnits() );
    return m_msgPanelContext.get();
}


void EDA_DRAW_FRAME::UpdateMsgPanel()
{
}


void EDA_DRAW_FRAME::ActivateGalCanvas()
{
    GetCanvas()->setEnabled( true );
    GetCanvas()->StartDrawing();
}


void EDA_DRAW_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    GetCanvas()->SwitchBackend( aCanvasType );
    m_canvasType = GetCanvas()->GetBackend();

    ActivateGalCanvas();
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::loadCanvasTypeSetting()
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    if( cfg )
        canvasType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        KI_ASSERT( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    // Legacy canvas no longer supported.  Switch to OpenGL, falls back to Cairo on failure
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    wxString envCanvasType;

    if( wxGetEnv( "KICAD_SOFTWARE_RENDERING", &envCanvasType ) )
    {
        if( envCanvasType.CmpNoCase( "1" ) == 0
            || envCanvasType.CmpNoCase( "true" ) == 0
            || envCanvasType.CmpNoCase( "yes" ) == 0 )
        {
            // Force software rendering if the environment variable is set
            canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
        }
    }

    return canvasType;
}


bool EDA_DRAW_FRAME::saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // The setting is the choice of the user (OpenGL or Cairo), shared with the wxWidgets build:
    // the Qt build, which draws with Cairo until its OpenGL canvas, does not change it
    return false;
}


VECTOR2I EDA_DRAW_FRAME::GetNearestGridPosition( const VECTOR2I& aPosition ) const
{
    const VECTOR2I& gridOrigin = GetGridOrigin();
    VECTOR2D        gridSize = GetCanvas()->GetGAL()->GetGridSize();

    double xOffset = fmod( gridOrigin.x, gridSize.x );
    int    x = KiROUND( (aPosition.x - xOffset) / gridSize.x );
    double yOffset = fmod( gridOrigin.y, gridSize.y );
    int    y = KiROUND( (aPosition.y - yOffset) / gridSize.y );

    return KiROUND( x * gridSize.x + xOffset, y * gridSize.y + yOffset );
}


VECTOR2I EDA_DRAW_FRAME::GetNearestHalfGridPosition( const VECTOR2I& aPosition ) const
{
    const VECTOR2I& gridOrigin = GetGridOrigin();
    VECTOR2D        gridSize = GetCanvas()->GetGAL()->GetGridSize() / 2.0;

    double xOffset = fmod( gridOrigin.x, gridSize.x );
    int    x = KiROUND( (aPosition.x - xOffset) / gridSize.x );
    double yOffset = fmod( gridOrigin.y, gridSize.y );
    int    y = KiROUND( (aPosition.y - yOffset) / gridSize.y );

    return KiROUND( x * gridSize.x + xOffset, y * gridSize.y + yOffset );
}


const BOX2I EDA_DRAW_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    return BOX2I();
}


void EDA_DRAW_FRAME::HardRedraw()
{
    // To be implemented by subclasses.
}


void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
}


std::vector<BOX2D> EDA_DRAW_FRAME::findDialogScreenRects()
{
    std::vector<BOX2D> rects;

    // The dialogs of the frame (top level windows) that may hide items
    for( QWidget* child : findChildren<QWidget*>( Qt::FindDirectChildrenOnly ) )
    {
        if( !child->isWindow() || !child->isVisible() || child->inherits( "QMenu" ) )
            continue;

        QPoint   topLeft = GetCanvas()->mapFromGlobal( child->frameGeometry().topLeft() );
        VECTOR2D origin = GetCanvas()->ToDevice( topLeft );
        VECTOR2D size = GetCanvas()->ToDevice( QPointF( child->frameGeometry().width(),
                                                        child->frameGeometry().height() ) );

        rects.emplace_back( origin, size );
    }

    return rects;
}


void EDA_DRAW_FRAME::FocusOnLocation( const VECTOR2I& aPos, bool aAllowScroll )
{
    bool               centerView = false;
    std::vector<BOX2D> dialogScreenRects;

    if( aAllowScroll )
    {
        BOX2D r = GetCanvas()->GetView()->GetViewport();

        // Center if we're off the current view, or within 10% of its edge
        r.Inflate( - r.GetWidth() / 10.0 );

        if( !r.Contains( aPos ) )
            centerView = true;

        dialogScreenRects = findDialogScreenRects();

        // Center if we're behind an obscuring dialog, or within 10% of its edge
        for( BOX2D rect : dialogScreenRects )
        {
            rect.Inflate( rect.GetWidth() / 10 );

            if( rect.Contains( GetCanvas()->GetView()->ToScreen( aPos ) ) )
                centerView = true;
        }
    }

    if( centerView )
    {
        try
        {
            GetCanvas()->GetView()->SetCenter( aPos, dialogScreenRects );
        }
        catch( const Clipper2Lib::Clipper2Exception& e )
        {
            KI_FAIL_MSG( wxString::Format( wxT( "Clipper2 exception occurred centering object: %s" ), e.what() ) );
        }
    }

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPos );
}


wxString EDA_DRAW_FRAME::GetScreenDesc() const
{
    // Virtual function. Base class implementation returns an empty string.
    return wxEmptyString;
}


wxString EDA_DRAW_FRAME::GetFullScreenDesc() const
{
    // Virtual function. Base class implementation returns an empty string.
    return wxEmptyString;
}


void EDA_DRAW_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();
}


void EDA_DRAW_FRAME::UpdateProperties()
{
    // The properties panel comes with the panels of the editors
}


COLOR_SETTINGS* EDA_DRAW_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    if( !m_colorSettings || aForceRefresh )
    {
        COLOR_SETTINGS* colorSettings = ::GetColorSettings( DEFAULT_THEME );

        const_cast<EDA_DRAW_FRAME*>( this )->m_colorSettings = colorSettings;
    }

    return m_colorSettings;
}


void EDA_DRAW_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    KI_ASSERT( mgr );

    mgr->SetConditions( ACTIONS::millimetersUnits, ACTION_CONDITIONS().Check( cond.Units( EDA_UNITS::MM ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,      ACTION_CONDITIONS().Check( cond.Units( EDA_UNITS::INCH ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,        ACTION_CONDITIONS().Check( cond.Units( EDA_UNITS::MILS ) ) );
}


void EDA_DRAW_FRAME::setupUnits( APP_SETTINGS_BASE* aCfg )
{
    COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>();

    if( cmnTool )
    {
        // Tell the tool what the units used last session
        cmnTool->SetLastUnits( static_cast<EDA_UNITS>( aCfg->m_System.last_imperial_units ) );
        cmnTool->SetLastUnits( static_cast<EDA_UNITS>( aCfg->m_System.last_metric_units ) );
    }

    // Tell the tool what units the frame is currently using
    switch( static_cast<EDA_UNITS>( aCfg->m_System.units ) )
    {
    default:
    case EDA_UNITS::MM:   m_toolManager->RunAction( ACTIONS::millimetersUnits ); break;
    case EDA_UNITS::INCH: m_toolManager->RunAction( ACTIONS::inchesUnits );      break;
    case EDA_UNITS::MILS: m_toolManager->RunAction( ACTIONS::milsUnits );        break;
    }
}


void EDA_DRAW_FRAME::GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits )
{
    COMMON_TOOLS* cmnTool = m_toolManager->GetTool<COMMON_TOOLS>();

    aPrimaryUnit    = GetUserUnits();
    aSecondaryUnits = EDA_UNITS::MILS;

    if( EDA_UNIT_UTILS::IsImperialUnit( aPrimaryUnit ) )
    {
        if( cmnTool )
            aSecondaryUnits = cmnTool->GetLastMetricUnits();
        else
            aSecondaryUnits = EDA_UNITS::MM;
    }
    else
    {
        if( cmnTool )
            aSecondaryUnits = cmnTool->GetLastImperialUnits();
        else
            aSecondaryUnits = EDA_UNITS::MILS;
    }
}


void EDA_DRAW_FRAME::resolveCanvasType()
{
    m_canvasType = loadCanvasTypeSetting();
}


bool EDA_DRAW_FRAME::SaveCanvasImageToFile( const wxString& aFileName, BITMAP_TYPE aBitmapType )
{
    QImage image;

    if( !GetCanvas()->GetScreenshot( image ) )
        return false;

    const char* format = "PNG";

    switch( aBitmapType )
    {
    case BITMAP_TYPE::PNG: format = "PNG";  break;
    case BITMAP_TYPE::BMP: format = "BMP";  break;
    case BITMAP_TYPE::JPG: format = "JPG";  break;
    }

    return image.save( ToQString( aFileName ), format );
}
