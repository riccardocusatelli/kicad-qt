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


bool PGM_GUI::InitPgm( bool aHeadless, bool aSkipPyInit, bool aIsUnitTest )
{
    initGuiToolkit();

    m_glContextManager = new GL_CONTEXT_MANAGER();
    m_background_jobs_monitor = std::make_unique<BACKGROUND_JOBS_MONITOR>();
    m_notifications_manager = std::make_unique<NOTIFICATIONS_MANAGER>();

#ifdef KICAD_IPC_API
    m_plugin_manager = std::make_unique<API_PLUGIN_MANAGER>();
#endif

    if( !PGM_BASE::InitPgm( aHeadless, aSkipPyInit, aIsUnitTest ) )
        return false;

    wxString pgm_name = programName();

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

    m_pgm_checker = std::make_unique<SINGLE_INSTANCE_CHECKER>();
    m_pgm_checker->Create( instanceCheckerName, instanceCheckerDir );

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

    configureGuiToolkit( aHeadless );

    // Now the application can safely start, show the splash screen
    if( !aHeadless )
        ShowSplash();

    return true;
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
