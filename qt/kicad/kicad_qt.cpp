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
 * The program has the settings, the libraries and the project of KiCad (PGM_BASE, as
 * kicad-cli); the boards and the schematics are shown by the Qt KIFACEs (_pcbnew_qt.kiface,
 * _eeschema_qt.kiface), loaded through KIWAY_BASE.
 */

#include <QApplication>
#include <QStringList>

#include <wx/app.h>
#include <wx/init.h>

#include <bin_mod.h>
#include <kiface.h>
#include <kiplatform/environment.h>
#include <kiway_base.h>
#include <libraries/library_manager.h>
#include <pgm_base.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <thread_pool.h>

#include "main_window.h"


static KIWAY_BASE s_kiway( KFCTL_CPP_PROJECT_SUITE | KFCTL_QT );


/**
 * The kicad-qt program: settings, libraries, project.  The user interface is Qt, so it derives
 * from PGM_BASE (not PGM_GUI, which is the wxWidgets one).
 */
class PGM_KICAD_QT : public PGM_BASE
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

        GetLibraryManager().LoadGlobalTables();
        return true;
    }

    void OnPgmExit()
    {
        GetKiCadThreadPool().purge();
        GetKiCadThreadPool().wait();

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
        PGM_BASE::Destroy();
    }

private:
    BIN_MOD m_bm;
};


static PGM_KICAD_QT program;


// The project manager has one open PROJECT: Prj() in this link image is the one of KIWAY_BASE
PROJECT& Prj()
{
    return s_kiway.Prj();
}


int main( int argc, char** argv )
{
    // The application object of the core (command line, names), then the program
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
        window.show();

        // Open the files of the command line
        const QStringList args = QApplication::arguments();

        for( int i = 1; i < args.size(); ++i )
        {
            if( !args[i].isEmpty() )
                window.OpenPath( args[i] );
        }

        ret = QApplication::exec();
    }

    program.OnPgmExit();
    return ret;
}
