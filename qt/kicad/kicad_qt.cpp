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
 * @file kicad_qt.cpp
 * @brief kicad-qt: the project manager of KiCad on Qt.
 *
 * The program of kicad/kicad.cpp (PGM_KICAD) built on Qt: the frame of the project manager is
 * the one of KiCad (KICAD_MANAGER_FRAME, on the wxWidgets compatibility layer), with its project
 * tree, its launcher, the jobsets, the package manager and the local history; its KIWAY loads
 * the Qt KIFACEs (_pcbnew_qtgui.kiface, _eeschema_qtgui.kiface...).
 */

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QStringList>

#include <stdexcept>

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/init.h>
#include <wx/kiwx_window.h>
#include <wx/log.h>

#include <kiqt_app.h>

#include <api/api_server.h>
#include <common.h>
#include <confirm.h>
#include <env_vars.h>
#include <git/git_backend.h>
#include <git/libgit_backend.h>
#include <kiface.h>
#include <kiface_base.h>
#include <kiplatform/environment.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <paths.h>
#include <settings/common_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <startwizard/startwizard.h>
#include <systemdirsappend.h>
#include <qt_strings.h>
#include <thread_pool.h>
#include <wildcards_and_files_ext.h>

#ifdef KICAD_IPC_API
#include <api/api_handler_common.h>
#endif

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"


KIFACE_BASE& Kiface()
{
    // As in kicad/kicad.cpp: this function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker, not to be
    // actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in qt/kicad/kicad_qt.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in qt/kicad/kicad_qt.cpp" );
}


static PGM_KICAD program;

PGM_KICAD& PgmTop()
{
    return program;
}


/// The KIWAY of the project manager: it runs the editors of the Qt KIFACEs.
KIWAY Kiway( KFCTL_CPP_PROJECT_SUITE | KFCTL_QT | KFCTL_QT_GUI );


// The project manager has one open PROJECT: Prj() in this link image is the one of the KIWAY
PROJECT& Prj()
{
    return Kiway.Prj();
}


namespace
{
/// The frame of the project manager, while it exists.
KICAD_MANAGER_FRAME* s_managerFrame = nullptr;

/// The arguments of the command line: the project to open, and "-n" (a new instance: no project
/// opened again).
QStringList s_projectArgs;
bool        s_newInstance = false;
}


bool PGM_KICAD::OnPgmInit()
{
    PGM_BASE::BuildArgvUtf8();
    App().SetAppDisplayName( wxT( "KiCad" ) );

    // The git backend, before the programs
    SetGitBackend( new LIBGIT_BACKEND() );
    GetGitBackend()->Init();

    // With the Python interpreter (the scripting of pcbnew, its plugins)
    if( !InitPgm( false, false ) )
        return false;

    m_bm.InitSettings( new KICAD_SETTINGS );
    GetSettingsManager().RegisterSettings( PgmSettings() );
    GetSettingsManager().SetKiway( &Kiway );
    m_bm.Init();

    // The search paths of PGM_KICAD::SysSearch(): the project templates (as kicad/kicad.cpp)
    {
        SEARCH_STACK bases;

        SystemDirsAppend( &bases );

        for( unsigned i = 0; i < bases.GetCount(); ++i )
        {
            wxFileName fn( bases[i], wxEmptyString );
            fn.AppendDir( wxT( "template" ) );

            if( fn.DirExists() && fn.IsDirReadable() )
                m_bm.m_search.AddPaths( fn.GetPath() );
        }

        auto insertExpanded = [&]( const wxString& aValue )
        {
            wxString resolved = ExpandEnvVarSubstitutions( aValue, nullptr );

            if( resolved.Contains( wxT( "${" ) ) || resolved.Contains( wxT( "$(" ) ) )
                return;

            m_bm.m_search.Insert( resolved, 0 );
        };

        if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( GetLocalEnvVariables(),
                                                                          wxT( "TEMPLATE_DIR" ) ) )
        {
            if( !v->IsEmpty() )
                insertExpanded( *v );
        }

        m_bm.m_search.Insert( PATHS::GetUserTemplatesPath(), 0 );

        ENV_VAR_MAP_CITER it = GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

        if( it != GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
            insertExpanded( it->second.GetValue() );
    }

    s_managerFrame = new KICAD_MANAGER_FRAME( nullptr, wxT( "KiCad" ), wxDefaultPosition,
                                              wxSize( 775, -1 ) );

    // The top window of the KIWAY and of the application (the parent of the dialogs)
    App().SetTopWindow( s_managerFrame );
    Kiway.SetTop( s_managerFrame );

    // The settings of a new configuration (the library tables...), then the libraries
    STARTWIZARD startWizard;
    startWizard.CheckAndRun( s_managerFrame );

    GetLibraryManager().LoadGlobalTables();

#ifdef KICAD_IPC_API
    m_api_server = std::make_unique<KICAD_API_SERVER>();
    m_api_common_handler = std::make_unique<API_HANDLER_COMMON>();
    m_api_server->RegisterHandler( m_api_common_handler.get() );
#endif

    // The project of the command line, else the last one open
    KICAD_SETTINGS* settings = static_cast<KICAD_SETTINGS*>( PgmSettings() );
    wxString        projToLoad;

    if( !s_projectArgs.isEmpty() )
    {
        wxFileName tmp( ToWxString( s_projectArgs.front() ) );

        if( tmp.GetExt() != FILEEXT::ProjectFileExtension
            && tmp.GetExt() != FILEEXT::LegacyProjectFileExtension )
        {
            DisplayErrorMessage( nullptr, wxString::Format( _( "File '%s'\n"
                                                               "does not appear to be a KiCad "
                                                               "project file." ),
                                                            tmp.GetFullPath() ) );
        }
        else
        {
            projToLoad = tmp.GetFullPath();
        }
    }

    if( projToLoad.IsEmpty() && settings->m_OpenProjects.size() && !s_newInstance )
    {
        wxString last_pro = settings->m_OpenProjects.front();
        settings->m_OpenProjects.erase( settings->m_OpenProjects.begin() );

        if( wxFileExists( last_pro ) )
            projToLoad = last_pro;
    }

    bool loaded = false;

    if( !projToLoad.empty() )
    {
        wxFileName fn( projToLoad );

        if( fn.Exists() && ( fn.GetExt() == FILEEXT::ProjectFileExtension
                             || fn.GetExt() == FILEEXT::LegacyProjectFileExtension ) )
        {
            fn.MakeAbsolute();
            loaded = s_managerFrame->LoadProject( fn );
        }
    }

    if( !loaded )
        s_managerFrame->PreloadAllLibraries();

    s_managerFrame->Show( true );
    s_managerFrame->Raise();

#ifdef KICAD_IPC_API
    m_api_server->SetReadyToReply();
#endif

    return true;
}


int PGM_KICAD::OnPgmRun()
{
    return 0;
}


void PGM_KICAD::OnPgmExit()
{
    m_libraryPreloadAbort.store( true );

    GetKiCadThreadPool().purge();
    GetKiCadThreadPool().wait();

    Kiway.OnKiwayEnd();

#ifdef KICAD_IPC_API
    m_api_server.reset();
#endif

    if( m_settings_manager && m_settings_manager->IsOK() )
    {
        SaveCommonSettings();
        m_settings_manager->Save();
    }

    Destroy();

    if( GetGitBackend() )
    {
        GetGitBackend()->Shutdown();
        delete GetGitBackend();
        SetGitBackend( nullptr );
    }
}


void PGM_KICAD::MacOpenFile( const wxString& aFileName )
{
    // A project given by the system (macOS)
    if( s_managerFrame && !aFileName.empty() && wxFileExists( aFileName ) )
        s_managerFrame->LoadProject( wxFileName( aFileName ) );
}


void PGM_KICAD::Destroy()
{
    m_bm.End();
    PGM_BASE::Destroy();
}


int main( int argc, char** argv )
{
    // The application object of the core (command line, names; a program with a user
    // interface for Pgm().IsGUI()), then the program
    KIQT_APP      kiqtApp;
    wxInitializer wxBase( argc, argv );

    SetPgm( &program );
    KIPLATFORM::ENV::Init();

    QApplication app( argc, argv );
    QApplication::setApplicationName( QStringLiteral( "kicad-qt" ) );
    QApplication::setOrganizationName( QStringLiteral( "KiCad" ) );
    QApplication::setStyle( QStringLiteral( "Fusion" ) );

    // The command line of kicad: "-n" (a new instance), then the project
    const QStringList args = QApplication::arguments();

    for( int i = 1; i < args.size(); ++i )
    {
        if( args[i] == QLatin1String( "-n" ) || args[i] == QLatin1String( "--new" ) )
            s_newInstance = true;
        else if( !args[i].startsWith( QLatin1Char( '-' ) ) && !args[i].isEmpty() )
            s_projectArgs.append( args[i] );
    }

    if( !program.OnPgmInit() )
    {
        program.OnPgmExit();
        return 1;
    }

    int ret = QApplication::exec();

    s_managerFrame = nullptr;
    program.OnPgmExit();
    return ret;
}
