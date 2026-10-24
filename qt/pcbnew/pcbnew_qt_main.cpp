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
 * @file pcbnew_qt_main.cpp
 * The pcbnew program of the Qt build, as the one of single_top (common/single_top.cpp): the
 * original PCB editor, made by the KIFACE of pcbnew with the GUI (linked in this program).
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)
#include <memory>
#include <vector>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QFileInfo>
#include <QStringList>
#include <wx/init.h>

#include <kiqt_app.h>

#include <api/api_server.h>
#include <frame_type.h>
#include <kiface.h>
#include <kiface_base.h>
#include <kiplatform/environment.h>
#include <kiway.h>
#include <kiway_player.h>
#include <libraries/library_manager.h>
#include <pgm_gui.h>
#include <qt_strings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <thread_pool.h>


/// The KIWAY of the program: the PCB editor runs alone (as single_top).
static KIWAY s_kiway( KFCTL_STANDALONE );


/**
 * The program: a program with a GUI (PGM_GUI), with the server of the IPC API.  Its name is the
 * one of single_top, the friend of KIWAY which registers the KIFACE linked in the program.
 */
struct PGM_SINGLE_TOP : public PGM_GUI
{
    ~PGM_SINGLE_TOP() override { Destroy(); }

    bool OnPgmInit()
    {
        PGM_BASE::BuildArgvUtf8();

        if( !InitPgm( false, true ) )
            return false;

        // The KIFACE of pcbnew is linked in this program
        int     kifaceVersion = 0;
        KIFACE* kiface = KIFACE_GETTER( &kifaceVersion, KIFACE_VERSION, this );

        if( !kiface || !kiface->OnKifaceStart( this, KFCTL_STANDALONE, &s_kiway ) )
            return false;

        s_kiway.set_kiface( KIWAY::KifaceType( FRAME_PCB_EDITOR ), kiface );

        GetSettingsManager().SetKiway( &s_kiway );
        GetSettingsManager().RegisterSettings( new KICAD_SETTINGS );

#ifdef KICAD_IPC_API
        m_api_server = std::make_unique<KICAD_API_SERVER>();
#endif

        return true;
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

    void MacOpenFile( const wxString& aFileName ) override
    {
        if( KIWAY_PLAYER* frame = s_kiway.Player( FRAME_PCB_EDITOR, false ) )
            frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
    }

    /// The server of the IPC API is ready, once the editor is open.
    void ApiServerReady()
    {
#ifdef KICAD_IPC_API
        if( m_api_server )
            m_api_server->SetReadyToReply();
#endif
    }
};


static PGM_SINGLE_TOP s_program;


int main( int argc, char** argv )
{
    // The application object of the core (command line, names; a program with a user
    // interface for Pgm().IsGUI()), then the program
    KIQT_APP      kiqtApp;
    wxInitializer wxBase( argc, argv );

    SetPgm( &s_program );
    KIPLATFORM::ENV::Init();

    QApplication app( argc, argv );
    QApplication::setApplicationName( QStringLiteral( "pcbnew" ) );
    QApplication::setOrganizationName( QStringLiteral( "KiCad" ) );

    if( !s_program.OnPgmInit() )
    {
        s_program.OnPgmExit();
        return 1;
    }

    int ret = 0;

    // The PCB editor: the top window of the program
    KIWAY_PLAYER* frame = s_kiway.Player( FRAME_PCB_EDITOR, true );

    if( !frame )
    {
        s_program.OnPgmExit();
        return 1;
    }

    s_kiway.SetTop( frame );
    s_program.GetLibraryManager().LoadGlobalTables();

    frame->show();
    QCoreApplication::processEvents();

    // The files of the command line (a board, with the extension of the boards by default)
    std::vector<wxString> files;
    const QStringList     args = QApplication::arguments();

    for( int i = 1; i < args.size(); ++i )
    {
        if( args[i].isEmpty() )
            continue;

        QFileInfo file( args[i] );

        if( args.size() == 2 && file.suffix().isEmpty() )
            file.setFile( args[i] + QStringLiteral( ".kicad_pcb" ) );

        files.push_back( ToWxString( file.absoluteFilePath() ) );
    }

    if( !files.empty() )
        frame->OpenProjectFiles( files );

    if( KIFACE* kiface = s_kiway.KiFACE( KIWAY::KifaceType( FRAME_PCB_EDITOR ) ) )
        kiface->PreloadLibraries( &s_kiway );

    s_program.PreloadDesignBlockLibraries( &s_kiway );
    s_program.ApiServerReady();

    ret = QApplication::exec();

    s_program.OnPgmExit();
    return ret;
}
