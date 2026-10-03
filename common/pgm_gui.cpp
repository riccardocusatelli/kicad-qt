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

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/snglinst.h>
#include <wx/splash.h>
#include <wx/tooltip.h>
#include <wx/utils.h>

#include <advanced_config.h>
#include <background_jobs_monitor.h>
#include <build_version.h>
#include <confirm.h>
#include <core/ki_assert.h>
#include <core/ki_log.h>
#include <design_block_library_adapter.h>
#include <kicad_gl/gl_context_mgr.h>
#include <kiway.h>
#include <notifications_manager.h>
#include <paths.h>
#include <pgm_gui.h>
#include <project.h>
#include <python_scripting.h>
#include <settings/common_settings.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <ui_theme.h>
#include <widgets/kistatusbar.h>

#ifdef KICAD_IPC_API
#include <api/api_plugin_manager.h>
#include <api/api_server.h>
#include <python_manager.h>
#endif


PGM_GUI::PGM_GUI() :
        m_Printing( false ),
        m_PropertyGridInitialized( false ),
        m_glContextManager( nullptr ),
        m_splash( nullptr )
{
    m_libraryPreloadInProgress.store( false );
    m_libraryPreloadAbort.store( false );
}


PGM_GUI::~PGM_GUI()
{
    HideSplash();
    Destroy();
}


void PGM_GUI::Destroy()
{
    m_pgm_checker.reset();

    PGM_BASE::Destroy();

    if( m_glContextManager )
    {
        m_glContextManager->DeleteAll();
        delete m_glContextManager;
        m_glContextManager = nullptr;
    }
}


void PGM_GUI::reportInitError( const wxString& aMessage )
{
    DisplayErrorMessage( nullptr, aMessage );
}


bool PGM_GUI::ConfirmAction( const wxString& aTitle, const wxString& aMessage )
{
    return wxMessageBox( aMessage, aTitle, wxYES_NO | wxICON_QUESTION ) == wxYES;
}


bool PGM_GUI::InitPgm( bool aHeadless, bool aSkipPyInit, bool aIsUnitTest )
{
    wxInitAllImageHandlers();

    // Register the bundled UI fonts before any window (and any wxFont) is created.
    UI_THEME::LoadUiFonts();

#if !wxCHECK_VERSION( 3, 3, 0 )
    // Without this the wxPropertyGridManager segfaults on Windows.
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();
#endif

    m_glContextManager = new GL_CONTEXT_MANAGER();
    m_background_jobs_monitor = std::make_unique<BACKGROUND_JOBS_MONITOR>();
    m_notifications_manager = std::make_unique<NOTIFICATIONS_MANAGER>();

#ifdef KICAD_IPC_API
    m_plugin_manager = std::make_unique<API_PLUGIN_MANAGER>( &App() );
#endif

    if( !PGM_BASE::InitPgm( aHeadless, aSkipPyInit, aIsUnitTest ) )
        return false;

    wxString pgm_name = programName();

#ifdef __WXGTK__
    switch( wxGetDisplayInfo().type )
    {
    case wxDisplayX11:     SetDisplayServerInfo( wxS( "X11" ) );     break;
    case wxDisplayWayland: SetDisplayServerInfo( wxS( "Wayland" ) ); break;
    case wxDisplayNone:    SetDisplayServerInfo( wxS( "None" ) );    break;
    default:               SetDisplayServerInfo( wxS( "Unknown" ) );
    }
#endif

    // Ensure the instance checker directory exists
    // It should be globally writable because it is shared between all users on Linux, and so on a
    // multi-user machine, other need to be able to access it to check for the lock files or make
    // their own lock files.
    wxString instanceCheckerDir = PATHS::GetInstanceCheckerPath();
    PATHS::EnsurePathExists( instanceCheckerDir );
    wxChmod( instanceCheckerDir,
             wxPOSIX_USER_READ | wxPOSIX_USER_WRITE | wxPOSIX_USER_EXECUTE |
             wxPOSIX_GROUP_READ | wxPOSIX_GROUP_WRITE | wxPOSIX_GROUP_EXECUTE |
             wxPOSIX_OTHERS_READ | wxPOSIX_OTHERS_WRITE | wxPOSIX_OTHERS_EXECUTE );

    wxString instanceCheckerName = wxString::Format( wxS( "%s-%s" ), pgm_name,
                                                     GetMajorMinorVersion() );

    m_pgm_checker = std::make_unique<wxSingleInstanceChecker>();
    m_pgm_checker->Create( instanceCheckerName, instanceCheckerDir );

#if wxCHECK_VERSION( 3, 3, 1 ) && defined( __WXGTK__ ) && defined( KICAD_DESKTOP_APP_NAME )
    // wxGTK feeds the class name to gdk_wayland_window_set_application_id(), so setting it
    // to the installed desktop file basename lets Wayland compositors resolve the correct
    // window icon and launch feedback. The prefix varies by build (regular, Flatpak, Nightly)
    // and is supplied by CMake. The main manager keeps the bare app name; the other GUI apps
    // are prefixed. Executables without a desktop file (pl_editor, kicad-cli) are left alone.
    wxString desktopId;

    if( pgm_name == wxT( "kicad" ) )
    {
        desktopId = wxT( KICAD_DESKTOP_APP_NAME );
    }
    else if( pgm_name == wxT( "eeschema" ) || pgm_name == wxT( "pcbnew" )
             || pgm_name == wxT( "gerbview" ) || pgm_name == wxT( "bitmap2component" )
             || pgm_name == wxT( "pcb_calculator" ) )
    {
        // pcb_calculator installs as pcbcalculator to satisfy freedesktop naming rules.
        wxString appName = pgm_name == wxT( "pcb_calculator" ) ? wxT( "pcbcalculator" ) : pgm_name;
        desktopId = wxString( wxT( KICAD_DESKTOP_APP_PREFIX ) ) + wxT( "." ) + appName;
    }

    if( !desktopId.IsEmpty() )
        App().SetClassName( desktopId );
#endif

    COMMON_SETTINGS* commonSettings = GetCommonSettings();

#ifdef KICAD_IPC_API
    // If user doesn't have a saved Python interpreter, try (potentially again) to find one
    if( commonSettings->m_Api.python_interpreter.IsEmpty() )
        commonSettings->m_Api.python_interpreter = PYTHON_MANAGER::FindPythonInterpreter();
#endif

    // Now that translations are available, inform the user if the OS is unsupported
    WarnUserIfOperatingSystemUnsupported();

    GetNotificationsManager().Load();

    // Create the python scripting stuff
    // Skip it for applications that do not use it
    if( !aSkipPyInit )
        m_python_scripting = std::make_unique<SCRIPTING>();

#ifdef KICAD_IPC_API
    if( commonSettings->m_Api.enable_server )
        m_plugin_manager->ReloadPlugins();
#endif

    // This sets the maximum tooltip display duration to 10s (up from 5) but only affects
    // Windows as other platforms display tooltips while the mouse is not moving
    if( !aHeadless )
    {
        wxToolTip::Enable( true );
        wxToolTip::SetAutoPop( 10000 );
    }

    if( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval != 0 )
        wxUpdateUIEvent::SetUpdateInterval( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval );

    // Now the application can safely start, show the splash screen
    if( !aHeadless )
        ShowSplash();

    return true;
}


wxApp& PGM_GUI::App()
{
    KI_ASSERT( wxTheApp );
    return *wxTheApp;
}


const wxString& PGM_GUI::GetTextEditor( bool aCanShowFileChooser )
{
    wxString editorname = m_text_editor;

    if( !editorname )
    {
        if( !wxGetEnv(  wxT( "EDITOR" ), &editorname ) )
        {
            // If there is no EDITOR variable set, try the desktop default
#ifdef __WXMAC__
            editorname = wxT( "/usr/bin/open -e" );
#elif __WXX11__
            editorname =  wxT( "/usr/bin/xdg-open" );
#endif
        }
    }

    // If we still don't have an editor name show a dialog asking the user to select one
    if( !editorname && aCanShowFileChooser )
    {
        DisplayInfoMessage( nullptr, _( "No default editor found, you must choose one." ) );

        editorname = AskUserForPreferredEditor();
    }

    // If we finally have a new editor name request it to be copied to m_text_editor and
    // saved to the preferences file.
    if( !editorname.IsEmpty() )
        SetTextEditor( editorname );

    // m_text_editor already has the same value that editorname, or empty if no editor was
    // found/chosen.
    return m_text_editor;
}


const wxString PGM_GUI::AskUserForPreferredEditor( const wxString& aDefaultEditor )
{
    // Create a mask representing the executable files in the current platform
#ifdef __WINDOWS__
    wxString mask( _( "Executable file" ) + wxT( " (*.exe)|*.exe" ) );
#else
    wxString mask( _( "Executable file" ) + wxT( " (*)|*" ) );
#endif

    // Extract the path, name and extension from the default editor (even if the editor's
    // name was empty, this method will succeed and return empty strings).
    wxString path, name, ext;
    wxFileName::SplitPath( aDefaultEditor, &path, &name, &ext );

    // Show the modal editor and return the file chosen (may be empty if the user cancels
    // the dialog).
    return wxFileSelector( _( "Select Preferred Editor" ), path, name, wxT( "." ) + ext,
                           mask, wxFD_OPEN | wxFD_FILE_MUST_EXIST, nullptr );
}


void PGM_GUI::ShowSplash()
{
    // Disabling until we change to load each DSO at startup rather than lazy-load when needed.
    // Note that once the splash screen is re-enabled, there are some remaining bugs to fix:
    // Any wxWidgets error dialogs that appear during startup are hidden by the splash screen,
    // so we either need to prevent these from happening (probably not feasible) or else change
    // the error-handling path to make sure errors go on top of the splash.
#if 0
    if( m_splash )
        return;

    m_splash = new WX_SPLASH( KiBitmap( BITMAPS::splash ), wxSPLASH_CENTRE_ON_SCREEN, 0,
                              NULL, -1, wxDefaultPosition, wxDefaultSize,
                              wxBORDER_NONE | wxSTAY_ON_TOP );
    wxYield();
#endif
}


void PGM_GUI::HideSplash()
{
    if( !m_splash )
        return;

    m_splash->Close( true );
    m_splash->Destroy();
    m_splash = nullptr;
}


void PGM_GUI::PreloadDesignBlockLibraries( KIWAY* aKiway )
{
    // TODO(JE) much of this code can be shared across the 3 preloads
    constexpr static int interval = 150;
    constexpr static int timeLimit = 120000;

    if( m_libraryPreloadInProgress.load() )
        return;

    m_libraryPreloadBackgroundJob =
            GetBackgroundJobMonitor().Create( _( "Loading Design Block Libraries" ) );

    auto preload =
        [this, aKiway]() -> void
        {
            std::shared_ptr<BACKGROUND_JOB_REPORTER> reporter =
                    m_libraryPreloadBackgroundJob->m_reporter;

            DESIGN_BLOCK_LIBRARY_ADAPTER* adapter = aKiway->Prj().DesignBlockLibs();

            int elapsed = 0;
            bool aborted = false;

            reporter->Report( _( "Loading Design Block Libraries" ) );
            adapter->AsyncLoad();

            while( true )
            {
                if( m_libraryPreloadAbort.load() )
                {
                    m_libraryPreloadAbort.store( false );
                    aborted = true;
                    break;
                }

                std::this_thread::sleep_for( std::chrono::milliseconds( interval ) );

                if( std::optional<float> loadStatus = adapter->AsyncLoadProgress() )
                {
                    float progress = *loadStatus;
                    reporter->SetCurrentProgress( progress );

                    if( progress >= 1 )
                        break;
                }
                else
                {
                    reporter->SetCurrentProgress( 1 );
                    break;
                }

                elapsed += interval;

                if( elapsed > timeLimit )
                    break;
            }

            // AbortAsyncLoad() sets the adapter's worker abort flag and then blocks,
            // so workers exit at their next checkpoint. BlockUntilLoaded() alone just
            // waits for each future to complete naturally, which can hang indefinitely
            // if a worker is stuck on a stalled network or filesystem operation.
            if( aborted )
                adapter->AbortAsyncLoad();
            else
                adapter->BlockUntilLoaded();

            GetBackgroundJobMonitor().Remove( m_libraryPreloadBackgroundJob );
            m_libraryPreloadBackgroundJob.reset();
            m_libraryPreloadInProgress.store( false );

            std::string payload = "";
            aKiway->ExpressMail( FRAME_SCH, MAIL_RELOAD_LIB, payload, nullptr, true );
            aKiway->ExpressMail( FRAME_PCB_EDITOR, MAIL_RELOAD_LIB, payload, nullptr, true );
        };

    thread_pool& tp = GetKiCadThreadPool();
    m_libraryPreloadInProgress.store( true );
    m_libraryPreloadReturn = tp.submit_task( preload );
}


void PGM_GUI::RegisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar )
{
    std::lock_guard<std::mutex> lock( m_libraryLoadStatusBarsMutex );

    KI_LOG_TRACE( traceLibraries, "RegisterLibraryLoadStatusBar: statusBar=%p", aStatusBar );

    if( std::find( m_libraryLoadStatusBars.begin(), m_libraryLoadStatusBars.end(), aStatusBar )
        == m_libraryLoadStatusBars.end() )
    {
        m_libraryLoadStatusBars.push_back( aStatusBar );
        KI_LOG_TRACE( traceLibraries, "  -> registered, total count=%zu",
                    m_libraryLoadStatusBars.size() );
    }
    else
    {
        KI_LOG_TRACE( traceLibraries, "  -> already registered" );
    }
}


void PGM_GUI::UnregisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar )
{
    std::lock_guard<std::mutex> lock( m_libraryLoadStatusBarsMutex );

    KI_LOG_TRACE( traceLibraries, "UnregisterLibraryLoadStatusBar: statusBar=%p", aStatusBar );

    m_libraryLoadStatusBars.erase(
            std::remove( m_libraryLoadStatusBars.begin(), m_libraryLoadStatusBars.end(),
                         aStatusBar ),
            m_libraryLoadStatusBars.end() );

    KI_LOG_TRACE( traceLibraries, "  -> remaining count=%zu", m_libraryLoadStatusBars.size() );
}


void PGM_GUI::AddLibraryLoadMessages( const std::vector<LOAD_MESSAGE>& aMessages )
{
    KI_LOG_TRACE( traceLibraries, "AddLibraryLoadMessages: message_count=%zu", aMessages.size() );

    if( aMessages.empty() )
        return;

    std::lock_guard<std::mutex> lock( m_libraryLoadStatusBarsMutex );

    KI_LOG_TRACE( traceLibraries, "  -> registered status bars=%zu",
                m_libraryLoadStatusBars.size() );

    for( KISTATUSBAR* statusBar : m_libraryLoadStatusBars )
    {
        if( statusBar )
        {
            KI_LOG_TRACE( traceLibraries, "  -> forwarding to statusBar=%p", statusBar );
            statusBar->AddWarningMessages( "load", aMessages );
        }
    }
}


void PGM_GUI::ClearLibraryLoadMessages()
{
    std::lock_guard<std::mutex> lock( m_libraryLoadStatusBarsMutex );

    KI_LOG_TRACE( traceLibraries, "ClearLibraryLoadMessages: status bars=%zu",
                m_libraryLoadStatusBars.size() );

    for( KISTATUSBAR* statusBar : m_libraryLoadStatusBars )
    {
        if( statusBar )
            statusBar->ClearWarningMessages( "load" );
    }
}


PGM_GUI& PgmGui()
{
    PGM_GUI* pgm = dynamic_cast<PGM_GUI*>( PgmOrNull() );
    KI_ASSERT( pgm ); // only GUI programs derive from PGM_GUI
    return *pgm;
}
