/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file eeschema_cli.cpp
 * @brief The KIFACE of _eeschema_cli.kiface: Eeschema without GUI, loaded by kicad-cli.
 *
 * It runs the jobs (ERC, plots, netlists, BOM...) on schematic files; it has no frames.  The
 * KIFACE of the schematic editor is in eeschema.cpp.
 */

#include <cli_progress_reporter.h>
#include <reporter.h>
#include <eeschema_jobs_handler.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <kiface_netlist.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>


namespace SCH_CLI
{

static struct IFACE : public KIFACE_BASE
{
    IFACE( const char* aName, KIWAY_FACES::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY_SERVICES* aKiway ) override
    {
        InitSettings( new EESCHEMA_SETTINGS );

        // Same order as the schematic editor: the legacy settings migration of
        // EESCHEMA_SETTINGS moves settings into SYMBOL_EDITOR_SETTINGS.
        aProgram->GetSettingsManager().RegisterSettings( new SYMBOL_EDITOR_SETTINGS );
        aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );

        start_common( aCtlBits );

        m_jobHandler = std::make_unique<EESCHEMA_JOBS_HANDLER>( aKiway );
        m_jobHandler->SetReporter( &CLI_REPORTER::GetInstance() );
        m_jobHandler->SetProgressReporter( &CLI_PROGRESS_REPORTER::GetInstance() );

        return true;
    }

    void OnKifaceEnd() override
    {
        m_jobHandler.reset();
        end_common();
    }

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        return nullptr;
    }

    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
        case KIFACE_NETLIST_SCHEMATIC:
            return (void*) GenerateSchematicNetlist;

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
    std::unique_ptr<EESCHEMA_JOBS_HANDLER> m_jobHandler;

} kiface( "eeschema", KIWAY_FACES::FACE_SCH );

} // namespace SCH_CLI


KIFACE_BASE& Kiface()
{
    return SCH_CLI::kiface;
}


// KIFACE_GETTER's actual spelling is a substitution macro found in kiface.h.
KIFACE_API KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    return &SCH_CLI::kiface;
}
