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

#include <future>
#include <project.h>
#include <settings/common_settings.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <widgets/kistatusbar.h>

#include <api/api_plugin_manager.h>
#include <api/api_server.h>
#include <api/python_manager.h>


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
    // The preload reads the design block libraries, so it has to stop before they go
    if( m_libraryPreloadReturn.valid() )
    {
        if( m_libraryPreloadInProgress.load() )
            m_libraryPreloadAbort.store( true );

        m_libraryPreloadReturn.wait();
    }

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


bool PGM_GUI::InitPgm( bool aHeadless, bool aIsUnitTest )
{
    initGuiToolkit();

    m_glContextManager = new GL_CONTEXT_MANAGER();
    m_background_jobs_monitor = std::make_unique<BACKGROUND_JOBS_MONITOR>();
    m_notifications_manager = std::make_unique<NOTIFICATIONS_MANAGER>();

    m_plugin_manager = std::make_unique<API_PLUGIN_MANAGER>();

    if( !PGM_BASE::InitPgm( aHeadless, aIsUnitTest ) )
        return false;

    COMMON_SETTINGS* commonSettings = GetCommonSettings();

    // If user doesn't have a saved Python interpreter, try (potentially again) to find one
    if( commonSettings->m_Api.python_interpreter.IsEmpty() )
        commonSettings->m_Api.python_interpreter = PYTHON_MANAGER::FindPythonInterpreter();

    // Now that translations are available, inform the user if the OS is unsupported
    WarnUserIfOperatingSystemUnsupported();

    GetNotificationsManager().Load();

    if( commonSettings->m_Api.enable_server )
        m_plugin_manager->ReloadPlugins();

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
    if( m_libraryPreloadInProgress.load() )
        return;

    m_libraryPreloadBackgroundJob =
            GetBackgroundJobMonitor().Create( _( "Loading Design Block Libraries" ) );

    auto preload =
        [this, aKiway]() -> void
        {
            // A task that starts after its cancel must not reach the project that is going away
            if( m_libraryPreloadAbort.load() )
            {
                m_libraryPreloadAbort.store( false );
                GetBackgroundJobMonitor().Remove( m_libraryPreloadBackgroundJob );
                m_libraryPreloadBackgroundJob.reset();
                m_libraryPreloadInProgress.store( false );
                return;
            }

            std::shared_ptr<BACKGROUND_JOB_REPORTER> reporter =
                    m_libraryPreloadBackgroundJob->m_reporter;

            DESIGN_BLOCK_LIBRARY_ADAPTER* adapter = aKiway->Prj().DesignBlockLibs();

            reporter->Report( _( "Loading Design Block Libraries" ) );
            adapter->RunPreload( *reporter, m_libraryPreloadAbort );

            GetBackgroundJobMonitor().Remove( m_libraryPreloadBackgroundJob );
            m_libraryPreloadBackgroundJob.reset();
            m_libraryPreloadInProgress.store( false );

            std::string payload = "";
            aKiway->ExpressMail( FRAME_SCH, MAIL_RELOAD_LIB, payload, nullptr, true );
            aKiway->ExpressMail( FRAME_PCB_EDITOR, MAIL_RELOAD_LIB, payload, nullptr, true );
        };

    // The preload waits on loads it queues on the pool, so it must not hold a pool thread itself
    m_libraryPreloadInProgress.store( true );
    m_libraryPreloadReturn = std::async( std::launch::async, preload );
}


void PGM_GUI::CancelDesignBlockPreload()
{
    if( m_libraryPreloadReturn.valid() )
        m_libraryPreloadAbort.store( true );
}


void PGM_GUI::WaitForDesignBlockPreload()
{
    if( m_libraryPreloadReturn.valid() )
        m_libraryPreloadReturn.wait();

    // The preload has ended, so a flag it never read can only stop the next one
    m_libraryPreloadAbort.store( false );
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


void PGM_GUI::AddLibraryLoadMessages( const std::vector<KI_ERROR>& aMessages )
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
    PGM_BASE* base = PgmOrNull();
    PGM_GUI*  pgm = base ? base->AsGui() : nullptr;
    KI_ASSERT( pgm ); // only GUI programs derive from PGM_GUI
    return *pgm;
}
