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
 * @brief kicad-qt: the Qt shell of KiCad, on the core without wxWidgets.
 *
 * The program has the settings, the libraries and the project of KiCad, with the user interface
 * of the programs of KiCad (PGM_GUI, as kicad); its KIWAY loads the Qt KIFACEs: the PCB editor
 * of KiCad (_pcbnew_qtgui.kiface) edits the boards, the schematics are shown in the tabs of the
 * shell (_eeschema_qt.kiface).
 */

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QStringList>

#include <stdexcept>

#include <wx/app.h>
#include <wx/init.h>
#include <wx/kiwx_window.h>
#include <wx/log.h>

#include <kiqt_app.h>

#include <api/api_server.h>
#include <bin_mod.h>
#include <kiface.h>
#include <kiface_base.h>
#include <kiplatform/environment.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <pgm_gui.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <startwizard/startwizard.h>
#include <qt_strings.h>
#include <thread_pool.h>

#include "main_window.h"


static KIWAY s_kiway( KFCTL_CPP_PROJECT_SUITE | KFCTL_QT | KFCTL_QT_GUI );


KIFACE_BASE& Kiface()
{
    // As in kicad/kicad.cpp: this function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker, not to be
    // actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in qt/kicad/kicad_qt.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in qt/kicad/kicad_qt.cpp" );
}


/**
 * The kicad-qt program: settings, libraries, project, and the user interface of the programs of
 * KiCad (PGM_GUI, built on Qt): the editors of the KIFACEs run in it, as in the project manager.
 */
class PGM_KICAD_QT : public PGM_GUI
{
public:
    PGM_KICAD_QT() :
            m_bm( "kicad" )     // the settings of the project manager
    {}

    ~PGM_KICAD_QT() override { Destroy(); }

    bool OnPgmInit()
    {
        PGM_BASE::BuildArgvUtf8();
        wxAppConsole::GetInstance()->SetAppDisplayName( wxT( "kicad-qt" ) );

        if( !InitPgm( true, true ) )
            return false;

        m_bm.InitSettings( new KICAD_SETTINGS );
        GetSettingsManager().RegisterSettings( PgmSettings() );
        GetSettingsManager().SetKiway( &s_kiway );
        m_bm.Init();

#ifdef KICAD_IPC_API
        // The server of the IPC API of the editors
        m_api_server = std::make_unique<KICAD_API_SERVER>();
#endif

        return true;
    }

    /// A file to open, given by the system (macOS): opened by the shell.
    void MacOpenFile( const wxString& aFileName ) override
    {
        if( m_window )
            m_window->OpenPath( ToQString( aFileName ) );
    }

    void SetWindow( MAIN_WINDOW* aWindow ) { m_window = aWindow; }

    /// The server of the IPC API is ready, once the shell is open.
    void ApiServerReady()
    {
#ifdef KICAD_IPC_API
        if( m_api_server )
            m_api_server->SetReadyToReply();
#endif
    }

    void OnPgmExit()
    {
        GetKiCadThreadPool().purge();
        GetKiCadThreadPool().wait();

#ifdef KICAD_IPC_API
        m_api_server.reset();
#endif

        s_kiway.OnKiwayEnd();

        if( m_settings_manager && m_settings_manager->IsOK() )
        {
            SaveCommonSettings();
            m_settings_manager->Save();
        }

        Destroy();
    }

    APP_SETTINGS_BASE* PgmSettings() { return m_bm.m_config; }

    void Destroy() override
    {
        m_bm.End();
        PGM_GUI::Destroy();
    }

private:
    BIN_MOD      m_bm;
    MAIN_WINDOW* m_window = nullptr;
};


static PGM_KICAD_QT program;


// The project manager has one open PROJECT: Prj() in this link image is the one of the KIWAY
PROJECT& Prj()
{
    return s_kiway.Prj();
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

    if( !program.OnPgmInit() )
    {
        program.OnPgmExit();
        return 1;
    }

    int ret = 0;

    {
        MAIN_WINDOW window( s_kiway );
        program.SetWindow( &window );

        // The top window of the KIWAY and of the application (the parent of the dialogs of the
        // program), as the project manager of KiCad
        s_kiway.SetTop( &window );
        kiqtApp.SetTopWindow( wxWindowFromQt( &window ) );
        window.show();

        // The settings of a new configuration (the library tables...), then the libraries
        STARTWIZARD startWizard;
        startWizard.CheckAndRun( wxWindowFromQt( &window ) );

        program.GetLibraryManager().LoadGlobalTables();
        program.ApiServerReady();

        // Open the files of the command line
        const QStringList args = QApplication::arguments();

        for( int i = 1; i < args.size(); ++i )
        {
            if( !args[i].isEmpty() )
                window.OpenPath( args[i] );
        }

        ret = QApplication::exec();
        program.SetWindow( nullptr );
    }

    program.OnPgmExit();
    return ret;
}
