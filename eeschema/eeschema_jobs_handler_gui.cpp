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

#include "eeschema_jobs_handler_gui.h"

#include <confirm.h>
#include <core/ki_assert.h>
#include <dialogs/dialog_erc_job_config.h>
#include <dialogs/dialog_export_netlist.h>
#include <dialogs/dialog_plot_schematic.h>
#include <dialogs/dialog_symbol_fields_table.h>
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sch_erc.h>
#include <kiway.h>
#include <sch_edit_frame.h>


EESCHEMA_JOBS_HANDLER_GUI::EESCHEMA_JOBS_HANDLER_GUI( KIWAY* aKiway ) :
        EESCHEMA_JOBS_HANDLER( aKiway ),
        m_guiKiway( aKiway )
{
    RegisterConfig( "bom",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_SCH_BOM* bomJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( job );

                        SCH_EDIT_FRAME* editFrame =
                                schematicEditor();

                        KI_CHECK( bomJob && editFrame, false );

                        DIALOG_SYMBOL_FIELDS_TABLE dlg( editFrame, bomJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "pythonbom",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "netlist",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_SCH_NETLIST* netJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( job );

                        SCH_EDIT_FRAME* editFrame =
                                schematicEditor();

                        KI_CHECK( netJob && editFrame, false );

                        DIALOG_EXPORT_NETLIST dlg( editFrame, aParent, netJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "plot",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_SCH_PLOT* plotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( job );

                        SCH_EDIT_FRAME* editFrame =
                                schematicEditor();

                        KI_CHECK( plotJob && editFrame, false );

                        if( plotJob->m_plotFormat == SCH_PLOT_FORMAT::HPGL )
                        {
                            DisplayErrorMessage( editFrame,
                                                 _( "Plotting to HPGL is no longer supported as of KiCad 10.0." ) );
                            return false;
                        }

                        DIALOG_PLOT_SCHEMATIC dlg( editFrame, aParent, plotJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "symupgrade",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "symsvg",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "erc",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_SCH_ERC* ercJob = dynamic_cast<JOB_SCH_ERC*>( job );

                        KI_CHECK( ercJob, false );

                        DIALOG_ERC_JOB_CONFIG dlg( aParent, ercJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "upgrade",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
}


SCH_EDIT_FRAME* EESCHEMA_JOBS_HANDLER_GUI::schematicEditor() const
{
    return static_cast<SCH_EDIT_FRAME*>( m_guiKiway->Player( FRAME_SCH, false ) );
}


SCHEMATIC* EESCHEMA_JOBS_HANDLER_GUI::getEditorSchematic()
{
    SCH_EDIT_FRAME* editFrame = schematicEditor();
    return editFrame ? &editFrame->Schematic() : nullptr;
}
