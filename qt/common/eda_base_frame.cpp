/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
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
 * @file eda_base_frame.cpp
 * EDA_BASE_FRAME of the Qt build: the port of common/eda_base_frame.cpp to a QMainWindow.
 */

#include <eda_base_frame.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QGuiApplication>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QStatusBar>
#include <QTimer>

#include <bitmaps.h>
#include <bitmap_store.h>
#include <dialogs/eda_view_switcher.h>
#include <confirm.h>
#include <kiface_base.h>
#include <kiway.h>
#include <local_history.h>
#include <pgm_base.h>
#include <paths.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <scoped_set_reset.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/ui/toolbar_configuration.h>
#include <toolkit/gui_services.h>
#include <trace_helpers.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_menubar.h>
#include <wx/filename.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <ui_hooks.h>


// Minimum window size
static QSize minSizeLookup( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T: return QSize( 406, 354 );
    default:                 return QSize( 500, 400 );
    }
}


static QSize defaultSize( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T: return QSize( 850, 540 );
    default:                 return QSize( 1280, 720 );
    }
}


namespace
{

/**
 * Gives the switch keys held down with Tab (Ctrl+Tab) in a frame to handleViewSwitcherKey(),
 * before the widgets of the frame (the Tab key of the focus navigation).
 */
class VIEW_SWITCHER_KEY_FILTER : public QObject
{
public:
    VIEW_SWITCHER_KEY_FILTER( EDA_BASE_FRAME* aFrame, std::function<bool( int )> aHandler ) :
            QObject( aFrame ),
            m_frame( aFrame ),
            m_handler( std::move( aHandler ) )
    {
        qApp->installEventFilter( this );
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        if( aEvent->type() != QEvent::KeyPress )
            return false;

        QKeyEvent* event = static_cast<QKeyEvent*>( aEvent );

        if( event->key() != Qt::Key_Tab && event->key() != Qt::Key_Backtab )
            return false;

        QWidget* widget = qobject_cast<QWidget*>( aWatched );

        if( !widget || widget->window() != m_frame )
            return false;

        if( TOOLKIT::IsKeyDown( PRESET_SWITCH_KEY ) )
            return m_handler( PRESET_SWITCH_KEY );
        else if( TOOLKIT::IsKeyDown( VIEWPORT_SWITCH_KEY ) )
            return m_handler( VIEWPORT_SWITCH_KEY );

        return false;
    }

private:
    EDA_BASE_FRAME*           m_frame;
    std::function<bool( int )> m_handler;
};

} // namespace


void EDA_BASE_FRAME::commonInit( FRAME_T aFrameType )
{
    m_ident             = aFrameType;
    m_maximizeByDefault = false;
    m_displayIndex      = -1;
    m_infoBar           = nullptr;
    m_settingsManager   = nullptr;
    m_supportsAutoSave  = false;
    m_autoSavePending   = false;
    m_undoRedoCountMax  = DEFAULT_MAX_UNDO_ITEMS;
    m_isClosing         = false;
    m_closeInProgress   = false;
    m_isNonUserClose    = false;
    m_autoSaveRequired  = false;
    m_autoSavePermissionError = false;
    m_mruPath           = PATHS::GetDefaultUserProjectsPath();
    m_toolbarSettings   = nullptr;
    m_tbTopMain         = nullptr;
    m_tbTopAux          = nullptr;
    m_tbRight           = nullptr;
    m_tbLeft            = nullptr;

    m_autoSaveTimer.SetHandler( [this]() { onAutoSaveTimer(); } );

    new VIEW_SWITCHER_KEY_FILTER( this,
                                  [this]( int aSwitchKey )
                                  {
                                      return handleViewSwitcherKey( aSwitchKey );
                                  } );

    // The entries of the language menu (AddMenuLanguageList()) tell all the KIWAY_PLAYERs
    // about the language change
    for( unsigned ii = 0; LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        int id = LanguagesList[ii].m_KI_Lang_Identifier;

        BindMenuCommand( id,
                         [this, id]()
                         {
                             Kiway().SetLanguage( id );
                         } );
    }

    // The menu entries and the buttons of the toolbars follow the UI conditions of their
    // actions after the events of the user, as with the update UI events of wxWidgets
    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                if( !m_isClosing )
                                    UpdateUIConditions();
                            } );

    m_settingsManager = &Pgm().GetSettingsManager();

    // Set a reasonable minimal size for the frame
    setMinimumSize( minSizeLookup( aFrameType ) );
    resize( defaultSize( aFrameType ) );

    // The frames are top level windows of their own (and are deleted when closed)
    setAttribute( Qt::WA_DeleteOnClose, false );
    setDockOptions( QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks
                    | QMainWindow::AllowTabbedDocks );
}


EDA_BASE_FRAME::EDA_BASE_FRAME( QWidget* aParent, FRAME_T aFrameType, const wxString& aTitle,
                                const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                const wxString& aFrameName, KIWAY* aKiway,
                                const EDA_IU_SCALE& aIuScale ) :
        QMainWindow( aParent ),
        TOOLS_HOLDER(),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::FRAME ),
        UNITS_PROVIDER( aIuScale, EDA_UNITS::MM ),
        m_name( aFrameName )
{
    setObjectName( ToQString( aFrameName ) );
    SetTitle( aTitle );

    // A frame with a parent is a window of its own, on top of its parent
    Qt::WindowFlags flags = Qt::Window;

    if( aStyle & wxSTAY_ON_TOP )
        flags |= Qt::WindowStaysOnTopHint;

    if( aStyle & wxFRAME_TOOL_WINDOW )
        flags |= Qt::Tool;

    if( !( aStyle & wxMINIMIZE_BOX ) || !( aStyle & wxMAXIMIZE_BOX ) )
    {
        flags |= Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint;

        if( aStyle & wxMINIMIZE_BOX )
            flags |= Qt::WindowMinimizeButtonHint;

        if( aStyle & wxMAXIMIZE_BOX )
            flags |= Qt::WindowMaximizeButtonHint;

        if( aStyle & wxCLOSE_BOX )
            flags |= Qt::WindowCloseButtonHint;
    }

    setWindowFlags( flags );

    if( aSize.x > 0 && aSize.y > 0 )
        resize( aSize.x, aSize.y );

    if( aPos != wxDefaultPosition )
        move( aPos.x, aPos.y );

    commonInit( aFrameType );
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    m_autoSaveTimer.Stop();

    ClearUndoRedoList();
}


wxString EDA_BASE_FRAME::GetName() const
{
    return m_name;
}


void EDA_BASE_FRAME::SetTitle( const wxString& aTitle )
{
    setWindowTitle( ToQString( aTitle ) );
}


wxString EDA_BASE_FRAME::GetTitle() const
{
    return ToWxString( windowTitle() );
}


void EDA_BASE_FRAME::SetStatusText( const wxString& aText, int aField )
{
    if( aField >= 0 && aField < (int) m_statusFields.size() )
        m_statusFields[aField]->setText( ToQString( aText ) );
    else if( aField == 0 && m_statusFields.empty() )
        statusBar()->showMessage( ToQString( aText ) );
}


bool EDA_BASE_FRAME::Close( bool aForce )
{
    if( aForce )
    {
        // Not vetoable: the frame closes whatever it answers
        m_isNonUserClose = true;
        SCOPED_SET_RESET<bool> closeGuard( m_closeInProgress, true );

        CLOSE_REQUEST request( false );
        canCloseWindow( request );
        m_isClosing = true;

        if( APP_SETTINGS_BASE* cfg = config() )
            SaveSettings( cfg );

        doCloseWindow();
        hide();

        if( !IsModal() )
            deleteLater();

        return true;
    }

    return close();
}


void EDA_BASE_FRAME::closeEvent( QCloseEvent* aEvent )
{
    // Guard against re-entrant close events.
    if( m_isClosing )
    {
        aEvent->accept();
        return;
    }

    // The unsaved-changes prompt in canCloseWindow() runs an event loop, so a second close
    // request can arrive while the first close is still deciding.
    if( m_closeInProgress )
    {
        aEvent->ignore();
        return;
    }

    // Don't allow closing when a modal dialog of the frame is open
    if( QWidget* modal = QApplication::activeModalWidget() )
    {
        if( modal != this && isAncestorOf( modal ) )
        {
            modal->raise();
            TOOLKIT::Bell();
            aEvent->ignore();
            return;
        }
    }

    SCOPED_SET_RESET<bool> closeGuard( m_closeInProgress, true );

    CLOSE_REQUEST request( true );

    if( canCloseWindow( request ) && !request.IsVetoed() )
    {
        m_isClosing = true;

        if( m_infoBar )
            m_infoBar->Dismiss();

        if( APP_SETTINGS_BASE* cfg = config() )
            SaveSettings( cfg );    // virtual, frame specific

        doCloseWindow();

        aEvent->accept();

        // Delete (safe delete frame) this frame only in non modal mode.
        // In modal mode, the caller will delete it.
        if( !IsModal() )
            deleteLater();
    }
    else
    {
        aEvent->ignore();
    }
}


void EDA_BASE_FRAME::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::PaletteChange || aEvent->type() == QEvent::ThemeChange )
        HandleSystemColorChange();
    else if( aEvent->type() == QEvent::WindowStateChange )
        handleIconizeEvent( isMinimized() );
    else if( aEvent->type() == QEvent::ActivationChange )
        handleActivateEvent( isActiveWindow() );

    QMainWindow::changeEvent( aEvent );
}


void EDA_BASE_FRAME::BindMenuCommand( int aId, std::function<void()> aHandler )
{
    m_menuCommands[aId] = std::move( aHandler );
}


bool EDA_BASE_FRAME::RunMenuCommand( int aId )
{
    auto it = m_menuCommands.find( aId );

    if( it == m_menuCommands.end() )
        return false;

    // A copy: the handler can bind the commands again (a menu bar created again)
    std::function<void()> handler = it->second;
    handler();
    return true;
}


int EDA_BASE_FRAME::GetAutoSaveInterval() const
{
    return Pgm().GetCommonSettings()->m_System.local_history_debounce;
}


void EDA_BASE_FRAME::onAutoSaveTimer()
{
    // A one-shot tick can already be queued when the frame starts closing; running the saver batch
    // then would serialize documents whose editors are mid-teardown, so bail once closing begins.
    if( m_isClosing )
        return;

    // When the save is deferred (an interactive operation is in progress) keep the timer armed so
    // a later tick retries.
    if( !doAutoSave() && isAutoSaveRequired() && GetAutoSaveInterval() > 0 )
    {
        m_autoSaveTimer.StartOnce( GetAutoSaveInterval() * 1000 );
        m_autoSavePending = true;
    }
    else
    {
        m_autoSavePending = false;
    }
}


bool EDA_BASE_FRAME::doAutoSave()
{
    // Defer the snapshot if the user is mid-interaction.  Serializing a large document on the
    // UI thread freezes the editor for seconds; deferring keeps the dirty flags set so the
    // rescheduled timer tick will pick the work up once the operation completes.  To avoid
    // starving the snapshot when the user parks in an interactive tool, the deferral is bounded
    // and the save is forced once it has been outstanding for longer than the cap.
    if( !canRunAutoSave() )
    {
        wxDateTime now = wxDateTime::Now();

        if( !m_autoSaveDeferredSince.IsValid() )
            m_autoSaveDeferredSince = now;

        wxTimeSpan maxDeferral = wxTimeSpan::Seconds( std::max( 60, GetAutoSaveInterval() * 12 ) );

        if( now - m_autoSaveDeferredSince < maxDeferral )
        {
            KI_LOG_TRACE( traceAutoSave, wxT( "Deferring auto save; an interactive operation is in progress." ) );
            return false;
        }

        KI_LOG_TRACE( traceAutoSave, wxT( "Auto save deferral exceeded; saving despite interactive operation." ) );
    }

    m_autoSaveDeferredSince = wxInvalidDateTime;
    m_autoSaveRequired = false;

    COMMON_SETTINGS* cs = Pgm().GetCommonSettings();

    // Incremental and zip-autosave both write outside the project tree when the user
    // selects USER_DIR, so a read-only project is fine in that mode.  Only when the
    // chosen location is the project directory does the project tree need to be writable.
    if( cs->m_Backup.location == BACKUP_LOCATION::PROJECT_DIR && Prj().IsReadOnly() )
        return true;

    if( cs->AutosaveUsesLocalHistory() )
        Kiway().LocalHistory().RunRegisteredSaversAndCommit( Prj().GetProjectPath(), wxS( "Autosave" ) );
    else
        Kiway().LocalHistory().RunRegisteredSaversAsAutosaveFiles( Prj().GetProjectPath() );

    return true;
}


void EDA_BASE_FRAME::RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions )
{
    m_uiConditions[aID] = aConditions;
}


void EDA_BASE_FRAME::UnregisterUIUpdateHandler( int aID )
{
    m_uiConditions.erase( aID );
}


void EDA_BASE_FRAME::UpdateUIConditions()
{
    SELECTION& selection = GetCurrentSelection();

    for( QAction* action : findChildren<QAction*>() )
    {
        bool ok = false;
        int  id = action->data().toInt( &ok );

        if( !ok )
            continue;

        auto it = m_uiConditions.find( id );

        if( it == m_uiConditions.end() )
            continue;

        const ACTION_CONDITIONS& cond = it->second;
        bool checkRes  = false;
        bool enableRes = true;
        bool showRes   = true;

        try
        {
            checkRes  = cond.checkCondition( selection );
            enableRes = cond.enableCondition( selection );
            showRes   = cond.showCondition( selection );
        }
        catch( std::exception& )
        {
            // Something broke with the conditions, just skip the control.
            continue;
        }

        if( showRes && ( id == ACTIONS::undo.GetUIId() || id == ACTIONS::redo.GetUIId() ) )
        {
            bool     undo = id == ACTIONS::undo.GetUIId();
            wxString msg = undo ? _( "Undo" ) : _( "Redo" );

            if( enableRes )
                msg += wxS( " " ) + ( undo ? GetUndoActionDescription() : GetRedoActionDescription() );

            // Keep the shortcut shown after the label
            QString shortcut = action->text().section( QLatin1Char( '\t' ), 1 );
            QString text = ToQString( msg );

            if( !shortcut.isEmpty() )
                text += QLatin1Char( '\t' ) + shortcut;

            action->setText( text );
        }

        action->setEnabled( enableRes );
        action->setVisible( showRes );

        if( action->isCheckable() )
            action->setChecked( checkRes );
    }
}


void EDA_BASE_FRAME::SelectToolbarAction( const TOOL_ACTION& aAction )
{
    for( ACTION_TOOLBAR* toolbar : { m_tbLeft, m_tbTopMain, m_tbTopAux, m_tbRight } )
    {
        if( toolbar )
            toolbar->SelectAction( aAction );
    }

    // The entries of the tool that runs are checked
    for( QAction* action : findChildren<QAction*>() )
    {
        bool ok = false;

        if( action->isCheckable() && action->data().toInt( &ok ) == aAction.GetUIId() && ok )
            action->setChecked( true );
    }
}


void EDA_BASE_FRAME::RegisterCustomToolbarControlFactory(
        const ACTION_TOOLBAR_CONTROL& aControlDesc,
        const std::function<void( ACTION_TOOLBAR* )>& aControlFactory )
{
    m_toolbarControlFactories.emplace( aControlDesc.GetName(), aControlFactory );
}


std::function<void( ACTION_TOOLBAR* )>*
EDA_BASE_FRAME::GetCustomToolbarControlFactory( const std::string& aName )
{
    auto it = m_toolbarControlFactories.find( aName );
    return it == m_toolbarControlFactories.end() ? nullptr : &it->second;
}


void EDA_BASE_FRAME::configureToolbars()
{
}


void EDA_BASE_FRAME::RecreateToolbars()
{
    KI_ASSERT( m_toolbarSettings );

    if( !m_toolbarSettings )
        return;

    bool allowCustom = config() ? config()->m_CustomToolbars : false;

    struct LOCATION
    {
        TOOLBAR_LOC      m_loc;
        ACTION_TOOLBAR*& m_toolbar;
        Qt::ToolBarArea  m_area;
        bool             m_vertical;
        const char*      m_name;
    };

    LOCATION locations[] = {
        { TOOLBAR_LOC::TOP_MAIN, m_tbTopMain, Qt::TopToolBarArea,   false, "TopMainToolbar" },
        { TOOLBAR_LOC::TOP_AUX,  m_tbTopAux,  Qt::TopToolBarArea,   false, "TopAuxToolbar" },
        { TOOLBAR_LOC::LEFT,     m_tbLeft,    Qt::LeftToolBarArea,  true,  "LeftToolbar" },
        { TOOLBAR_LOC::RIGHT,    m_tbRight,   Qt::RightToolBarArea, true,  "RightToolbar" }
    };

    for( LOCATION& location : locations )
    {
        if( location.m_toolbar )
            location.m_toolbar->ClearToolbar();
    }

    for( LOCATION& location : locations )
    {
        std::optional<TOOLBAR_CONFIGURATION> tbConfig =
                m_toolbarSettings->GetToolbarConfig( location.m_loc, allowCustom );

        if( !tbConfig.has_value() )
            continue;

        if( !location.m_toolbar )
        {
            location.m_toolbar = new ACTION_TOOLBAR( this, location.m_vertical );
            location.m_toolbar->setObjectName( QString::fromLatin1( location.m_name ) );

            // The auxiliary toolbar is under the main one
            if( location.m_loc == TOOLBAR_LOC::TOP_AUX )
                addToolBarBreak( Qt::TopToolBarArea );

            addToolBar( location.m_area, location.m_toolbar );
        }

        location.m_toolbar->ApplyConfiguration( tbConfig.value() );
    }

    UpdateUIConditions();
}


void EDA_BASE_FRAME::UpdateToolbarControlSizes()
{
    for( ACTION_TOOLBAR* toolbar : { m_tbTopMain, m_tbRight, m_tbLeft, m_tbTopAux } )
    {
        if( toolbar )
            toolbar->UpdateControlWidths();
    }
}


void EDA_BASE_FRAME::OnToolbarSizeChanged()
{
    for( ACTION_TOOLBAR* toolbar : { m_tbTopMain, m_tbRight, m_tbLeft, m_tbTopAux } )
    {
        if( toolbar )
            toolbar->RefreshBitmaps();
    }
}


void EDA_BASE_FRAME::setupUIConditions()
{
    // Setup the conditions to check a language menu item
    auto isCurrentLang =
            [] ( const SELECTION& aSel, int aLangIdentifier )
            {
                return Pgm().GetSelectedLanguageIdentifier() == aLangIdentifier;
            };

    for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        ACTION_CONDITIONS cond;
        cond.Check( std::bind( isCurrentLang, std::placeholders::_1,
                               LanguagesList[ii].m_WX_Lang_Identifier ) );
        RegisterUIUpdateHandler( LanguagesList[ii].m_KI_Lang_Identifier, cond );
    }
}


void EDA_BASE_FRAME::InstallMenuBar( WX_MENUBAR* aMenuBar )
{
    QMenuBar* oldMenuBar = menuWidget() ? qobject_cast<QMenuBar*>( menuWidget() ) : nullptr;

    // The main window lets the previous menu bar go (deleted later), deleted now
    setMenuBar( aMenuBar );
    delete oldMenuBar;
}


void EDA_BASE_FRAME::AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool )
{
    ACTION_MENU* langsMenu = new ACTION_MENU( false, aControlTool );
    langsMenu->SetTitle( _( "Set Language" ) );
    langsMenu->SetIcon( BITMAPS::language );

    for( unsigned ii = 0; LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        wxString label;

        if( LanguagesList[ii].m_DoNotTranslate )
            label = LanguagesList[ii].m_Lang_Label;
        else
            label = wxGetTranslation( LanguagesList[ii].m_Lang_Label );

        langsMenu->Add( label, wxEmptyString, LanguagesList[ii].m_KI_Lang_Identifier,
                        BITMAPS::INVALID_BITMAP, true );
    }

    aMasterMenu->Add( langsMenu );
}


void EDA_BASE_FRAME::ReCreateMenuBar()
{
    // Recreating the menu bar from within one of its menus would delete the menu that is
    // running: always recreate it from the event loop
    QTimer::singleShot( 0, this,
                        [this]()
                        {
                            if( !m_isClosing )
                                doReCreateMenuBar();
                        } );
}


void EDA_BASE_FRAME::ShowChangedLanguage()
{
    TOOLS_HOLDER::ShowChangedLanguage();

    ReCreateMenuBar();
}


void EDA_BASE_FRAME::CommonSettingsChanged( int aFlags )
{
    TOOLS_HOLDER::CommonSettingsChanged( aFlags );

    if( Pgm().GetCommonSettings()->AutosaveUsesLocalHistory() )
        Kiway().LocalHistory().Init( Prj().GetProjectPath() );

    GetBitmapStore()->ThemeChanged();
    ThemeChanged();

    // For icons in menus, icon scaling & hotkeys
    ReCreateMenuBar();
}


void EDA_BASE_FRAME::ThemeChanged()
{
    ClearScaledBitmapCache();

    for( ACTION_TOOLBAR* toolbar : { m_tbTopMain, m_tbRight, m_tbLeft, m_tbTopAux } )
    {
        if( toolbar )
            toolbar->RefreshBitmaps();
    }
}


void EDA_BASE_FRAME::HandleSystemColorChange()
{
    // Update the icon theme when the system theme changes and update the toolbars
    GetBitmapStore()->ThemeChanged();
    ThemeChanged();

    // For icons in menus, icon scaling & hotkeys
    ReCreateMenuBar();
}


void EDA_BASE_FRAME::LoadWindowState( const WINDOW_STATE& aState )
{
    bool  wasDefault = false;
    QRect frame( aState.pos_x, aState.pos_y, aState.size_x, aState.size_y );

    KI_LOG_TRACE( traceDisplayLocation, wxS( "Config position (%d, %d) with size (%d, %d)" ),
                  frame.x(), frame.y(), frame.width(), frame.height() );

    // Ensure minimum size is set if the stored config was zero-initialized
    QSize minSize = minSizeLookup( m_ident );

    if( frame.width() < minSize.width() || frame.height() < minSize.height() )
    {
        frame.setSize( defaultSize( m_ident ) );
        wasDefault = true;
    }

    const QList<QScreen*> screens = QGuiApplication::screens();
    bool                  center = false;

    if( aState.display >= (unsigned) screens.size() )
    {
        KI_LOG_TRACE( traceDisplayLocation, wxS( "Previous display not found" ) );

        // If it isn't attached, use the first display
        QRect area = QGuiApplication::primaryScreen()->availableGeometry();

        center = true;
        frame.setWidth( std::min( frame.width(), area.width() ) );
        frame.setHeight( std::min( frame.height(), area.height() ) );
    }
    else
    {
        QRect area = screens[aState.display]->availableGeometry();

        // The upper edge of the window must be on the display
        if( frame.left() > area.right() || frame.right() < area.left()
            || frame.top() < area.top() || frame.top() > area.bottom() )
        {
            center = true;
            KI_LOG_TRACE( traceDisplayLocation, wxS( "Resetting to default position" ) );
        }

        // Clamp the saved size to the current display, in case the window was sized for a
        // larger external monitor that is no longer attached.
        frame.setWidth( std::min( frame.width(), area.width() ) );
        frame.setHeight( std::min( frame.height(), area.height() ) );
    }

    resize( frame.size() );

    if( center )
    {
        QRect area = screen() ? screen()->availableGeometry()
                              : QGuiApplication::primaryScreen()->availableGeometry();
        move( area.center() - QPoint( frame.width() / 2, frame.height() / 2 ) );
    }
    else
    {
        move( frame.topLeft() );
    }

    // Maximize if we were maximized before
    if( aState.maximized || ( wasDefault && m_maximizeByDefault ) )
        setWindowState( windowState() | Qt::WindowMaximized );

    m_displayIndex = screens.indexOf( screen() );
}


void EDA_BASE_FRAME::ensureWindowIsOnScreen()
{
    QScreen* scr = screen();

    if( !scr )
        return;

    QRect area = scr->availableGeometry();
    QRect frame = frameGeometry();

    if( !area.intersects( frame ) )
        move( area.topLeft() );
}


void EDA_BASE_FRAME::LoadWindowSettings( const WINDOW_SETTINGS* aCfg )
{
    LoadWindowState( aCfg->state );

    m_mruPath = aCfg->mru_path;

    TOOLS_HOLDER::CommonSettingsChanged();
}


void EDA_BASE_FRAME::SaveWindowSettings( WINDOW_SETTINGS* aCfg )
{
    if( isMinimized() )
        return;

    // If the window is maximized, we use the saved window size from before it was maximized
    QRect frame = isMaximized() ? normalGeometry() : geometry();

    aCfg->state.pos_x     = frame.x();
    aCfg->state.pos_y     = frame.y();
    aCfg->state.size_x    = frame.width();
    aCfg->state.size_y    = frame.height();
    aCfg->state.maximized = isMaximized();
    aCfg->state.display   = std::max<int>( 0, QGuiApplication::screens().indexOf( screen() ) );

    KI_LOG_TRACE( traceDisplayLocation, wxS( "Saving config position (%d, %d) with size (%d, %d)" ),
                  frame.x(), frame.y(), frame.width(), frame.height() );

    aCfg->mru_path = m_mruPath;
}


void EDA_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    LoadWindowSettings( GetWindowSettings( aCfg ) );
}


void EDA_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    KI_CHECK( config(), /* void */ );

    SaveWindowSettings( GetWindowSettings( aCfg ) );

    bool fileOpen = m_isClosing && m_isNonUserClose;

    wxString currentlyOpenedFile = GetCurrentFileName();

    if( Pgm().GetCommonSettings()->m_Session.remember_open_files && !currentlyOpenedFile.IsEmpty() )
    {
        wxFileName rfn( currentlyOpenedFile );
        rfn.MakeRelativeTo( Prj().GetProjectPath() );
        Prj().GetLocalSettings().SaveFileState( rfn.GetFullPath(), &aCfg->m_Window, fileOpen );
    }
}


WINDOW_SETTINGS* EDA_BASE_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    return &aCfg->m_Window;
}


APP_SETTINGS_BASE* EDA_BASE_FRAME::config() const
{
    // KICAD_MANAGER_FRAME overrides this
    return Kiface().KifaceSettings();
}


const SEARCH_STACK& EDA_BASE_FRAME::sys_search()
{
    return Kiface().KifaceSearch();
}


wxString EDA_BASE_FRAME::help_name()
{
    return Kiface().GetHelpFileName();
}


void EDA_BASE_FRAME::CallAfter( std::function<void()> aFunction )
{
    QTimer::singleShot( 0, this, std::move( aFunction ) );
}


void EDA_BASE_FRAME::ShowPreferences( wxString aStartPage, wxString aStartParentPage )
{
    // The dialog of the preferences (PAGED_DIALOG with the panels of all the editors) is not
    // ported to Qt yet
    DisplayInfoMessage( this, _( "The preferences are not available yet in this version of "
                                 "KiCad." ) );
}


void EDA_BASE_FRAME::PrintMsg( const wxString& text )
{
    SetStatusText( text );
}


void EDA_BASE_FRAME::CreateInfoBar()
{
    // The frame puts the info bar above its main area (see EDA_DRAW_FRAME)
    m_infoBar = new WX_INFOBAR( this );
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                                       INFOBAR_MESSAGE_TYPE aType )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aErrorMsg, 8000, wxICON_ERROR, aType );
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                                       std::function<void(void)> aCallback )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    if( aCallback )
        m_infoBar->SetCallback( aCallback );

    GetInfoBar()->ShowMessageFor( aErrorMsg, 6000, wxICON_ERROR );
}


void EDA_BASE_FRAME::ShowInfoBarWarning( const wxString& aWarningMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aWarningMsg, 6000, wxICON_WARNING );
}


void EDA_BASE_FRAME::ShowInfoBarMsg( const wxString& aMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aMsg, 8000, wxICON_INFORMATION );
}


bool EDA_BASE_FRAME::IsWritable( const wxFileName& aFileName, bool aVerbose )
{
    wxString msg;
    wxFileName fn = aFileName;

    // Check for absence of a file path with a file name.  Unfortunately KiCad
    // uses paths relative to the current project path without the ./ part which
    // confuses wxFileName. Making the file name path absolute may be less than
    // elegant but it solves the problem.
    if( fn.GetPath().IsEmpty() && fn.HasName() )
        fn.MakeAbsolute();

    KI_CHECK_MSG( fn.IsOk(), false,
                  wxT( "File name object is invalid.  Bad programmer!" ) );
    KI_CHECK_MSG( !fn.GetPath().IsEmpty(), false,
                  wxT( "File name object path <" ) + fn.GetFullPath() +
                  wxT( "> is not set.  Bad programmer!" ) );

    if( fn.IsDir() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), fn.GetPath() );
    }
    else if( !fn.FileExists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), fn.GetFullPath() );
    }
    else if( fn.FileExists() && !fn.IsFileWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), fn.GetFullPath() );
    }

    if( !msg.IsEmpty() )
    {
        if( aVerbose )
            DisplayErrorMessage( this, msg );

        return false;
    }

    return true;
}


bool EDA_BASE_FRAME::IsContentModified() const
{
    // This function should be overridden in child classes
    return false;
}


void EDA_BASE_FRAME::ClearUndoRedoList()
{
    ClearUndoORRedoList( UNDO_LIST );
    ClearUndoORRedoList( REDO_LIST );
}


void EDA_BASE_FRAME::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_undoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetUndoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( UNDO_LIST, extraitems );
    }
}


void EDA_BASE_FRAME::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_redoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetRedoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( REDO_LIST, extraitems );
    }
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromUndoList( )
{
    return m_undoList.PopCommand();
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromRedoList( )
{
    return m_redoList.PopCommand();
}


wxString EDA_BASE_FRAME::GetUndoActionDescription() const
{
    if( GetUndoCommandCount() > 0 )
        return m_undoList.m_CommandsList.back()->GetDescription();

    return wxEmptyString;
}


wxString EDA_BASE_FRAME::GetRedoActionDescription() const
{
    if( GetRedoCommandCount() > 0 )
        return m_redoList.m_CommandsList.back()->GetDescription();

    return wxEmptyString;
}


void EDA_BASE_FRAME::OnModify()
{
    m_autoSaveRequired = true;

    // Arm the timer of the auto save (in the wxWidgets build, after the next event)
    if( !m_isClosing && m_supportsAutoSave && !m_autoSavePending && GetAutoSaveInterval() > 0 )
    {
        KI_LOG_TRACE( traceAutoSave, wxT( "Starting auto save timer." ) );
        m_autoSaveTimer.StartOnce( GetAutoSaveInterval() * 1000 );
        m_autoSavePending = true;
        m_autoSaveDeferredSince = wxInvalidDateTime;
    }
}


void EDA_BASE_FRAME::ChangeUserUnits( EDA_UNITS aUnits )
{
    SetUserUnits( aUnits );
    unitsChangeRefresh();

    // A copy: a listener may remove itself
    std::map<int, std::function<void( EDA_BASE_FRAME* )>> listeners = m_unitsChangedListeners;

    for( const auto& [id, listener] : listeners )
        listener( this );
}


int EDA_BASE_FRAME::AddUnitsChangedListener( std::function<void( EDA_BASE_FRAME* )> aListener )
{
    int id = m_nextUnitsChangedListener++;
    m_unitsChangedListeners[id] = std::move( aListener );
    return id;
}


void EDA_BASE_FRAME::RemoveUnitsChangedListener( int aId )
{
    m_unitsChangedListeners.erase( aId );
}


wxSize EDA_BASE_FRAME::GetWindowSize()
{
    return wxSize( width(), height() );
}
