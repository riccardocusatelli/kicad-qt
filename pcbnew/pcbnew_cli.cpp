/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file pcbnew_cli.cpp
 * @brief The KIFACE of _pcbnew_cli.kiface: Pcbnew without GUI, loaded by kicad-cli.
 *
 * It runs the jobs (plots, exports, DRC...) on board files; it has no frames.  The KIFACE of
 * the board editor is in pcbnew.cpp.
 *
 * Built with KICAD_QT_MODULE, it is also the KIFACE of the Qt shell (_pcbnew_qt.kiface), which
 * adds the board documents (qt/pcbnew).
 */

#include <3d_viewer/eda_3d_viewer_settings.h>
#include <cli_progress_reporter.h>
#include <drc/drc_test_provider.h>
#include <reporter.h>
#include <footprint_editor_settings.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <pcbnew_jobs_handler.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <settings/cvpcb_settings.h>
#include <settings/settings_manager.h>

#ifdef KICAD_QT_MODULE
#include <board_document.h>
#endif


namespace PCB_CLI
{

static struct IFACE : public KIFACE_BASE
{
    IFACE( const char* aName, KIWAY_FACES::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY_SERVICES* aKiway ) override
    {
        // The kiface is linked from static archives: keep the self-registering DRC tests.
        LinkDrcTestProviders();

        InitSettings( new PCBNEW_SETTINGS );

        SETTINGS_MANAGER& mgr = aProgram->GetSettingsManager();

        // Same order as the board editor: the legacy settings migration of PCBNEW_SETTINGS
        // moves settings into these.
        mgr.RegisterSettings( new FOOTPRINT_EDITOR_SETTINGS );
        mgr.RegisterSettings( new EDA_3D_VIEWER_SETTINGS );
        mgr.RegisterSettings( KifaceSettings() );
        mgr.RegisterSettings( new CVPCB_SETTINGS );

        start_common( aCtlBits );

        m_jobHandler = std::make_unique<PCBNEW_JOBS_HANDLER>( aKiway );
        m_jobHandler->SetReporter( &CLI_REPORTER::GetInstance() );
        m_jobHandler->SetProgressReporter( &CLI_PROGRESS_REPORTER::GetInstance() );

        return true;
    }

    void OnKifaceEnd() override
    {
        m_jobHandler.reset();
        end_common();
    }

    TOOLKIT_WINDOW* CreateKiWindow( TOOLKIT_WINDOW* aParent, int aClassId, KIWAY* aKiway,
                                    int aCtlBits = 0 ) override
    {
        return nullptr;
    }

    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
        case KIFACE_FOOTPRINT_LIBRARY_ADAPTER:
            return PROJECT_PCB::FootprintLibAdapter( &Pgm().GetSettingsManager().Prj() );

        case KIFACE_TEST_FOOTPRINT_LINK:
            return reinterpret_cast<void*>( &PROJECT_PCB::TestFootprintLink );

#ifdef KICAD_QT_MODULE
        case KIFACE_QT_DOCUMENT_FACTORY:
            return reinterpret_cast<void*>( &CreateQtBoardDocument );
#endif

        default:
            return nullptr;
        }
    }

    void GetActions( std::vector<TOOL_ACTION*>& aActions ) const override
    {
    }

    int HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter ) override
    {
        return m_jobHandler->RunJob( aJob, aReporter, aProgressReporter );
    }

private:
    std::unique_ptr<PCBNEW_JOBS_HANDLER> m_jobHandler;

} kiface( "pcbnew", KIWAY_FACES::FACE_PCB );

} // namespace PCB_CLI


KIFACE_BASE& Kiface()
{
    return PCB_CLI::kiface;
}


// KIFACE_GETTER's actual spelling is a substitution macro found in kiface.h.
KIFACE_API KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &PCB_CLI::kiface;
}
