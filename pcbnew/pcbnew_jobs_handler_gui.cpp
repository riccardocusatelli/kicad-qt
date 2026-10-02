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

#include "pcbnew_jobs_handler_gui.h"

#include <confirm.h>
#include <core/ki_assert.h>
#include <dialogs/dialog_board_stats_job.h>
#include <dialogs/dialog_drc_job_config.h>
#include <dialogs/dialog_export_2581.h>
#include <dialogs/dialog_export_odbpp.h>
#include <dialogs/dialog_export_step.h>
#include <dialogs/dialog_gen_footprint_position.h>
#include <dialogs/dialog_gencad_export_options.h>
#include <dialogs/dialog_gendrill.h>
#include <dialogs/dialog_plot.h>
#include <dialogs/dialog_render_job.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_gencad.h>
#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_stats.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_pcb_drc.h>
#include <jobs/job_pcb_render.h>
#include <kiway.h>
#include <kiway_mail.h>
#include <mail_type.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>


PCBNEW_JOBS_HANDLER_GUI::PCBNEW_JOBS_HANDLER_GUI( KIWAY* aKiway ) :
        PCBNEW_JOBS_HANDLER( aKiway ),
        m_guiKiway( aKiway )
{
    RegisterConfig( "3d",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_3D* svgJob = dynamic_cast<JOB_EXPORT_PCB_3D*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( svgJob && editFrame, false );

                        DIALOG_EXPORT_STEP dlg( editFrame, aParent, "", svgJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "render",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_PCB_RENDER* renderJob = dynamic_cast<JOB_PCB_RENDER*>( job );

                        KI_CHECK( renderJob, false );

                        DIALOG_RENDER_JOB dlg( aParent, renderJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "upgrade",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "pcb_import",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "svg",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_SVG* svgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( svgJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, svgJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "gencad",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_GENCAD* gencadJob = dynamic_cast<JOB_EXPORT_PCB_GENCAD*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( gencadJob && editFrame, false );

                        DIALOG_GENCAD_EXPORT_OPTIONS dlg( editFrame, gencadJob->GetSettingsDialogTitle(), gencadJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "dxf",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_DXF* dxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( dxfJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, dxfJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "pdf",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_PDF* pdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( pdfJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, pdfJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "ps",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_PS* psJob = dynamic_cast<JOB_EXPORT_PCB_PS*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( psJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, psJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "stats",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_STATS* statsJob = dynamic_cast<JOB_EXPORT_PCB_STATS*>( job );

                        PCB_EDIT_FRAME* editFrame =
                                boardEditor();

                        KI_CHECK( statsJob && editFrame, false );

                        if( statsJob->m_filename.IsEmpty() && editFrame->GetBoard() )
                        {
                            wxFileName boardName = editFrame->GetBoard()->GetFileName();
                            statsJob->m_filename = boardName.GetFullPath();
                        }

                        wxWindow* parent = aParent ? aParent : static_cast<wxWindow*>( editFrame );

                        DIALOG_BOARD_STATS_JOB dlg( parent, statsJob );

                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "gerber",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_GERBER* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( gJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, gJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "gerbers",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_GERBERS* gJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( gJob && editFrame, false );

                        DIALOG_PLOT dlg( editFrame, aParent, gJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "hpgl",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( editFrame, false );

                        DisplayErrorMessage( editFrame,
                                             _( "Plotting to HPGL is no longer supported as of KiCad 10.0." ) );
                        return false;
                    } );
    RegisterConfig( "drill",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_DRILL* drillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( drillJob && editFrame, false );

                        DIALOG_GENDRILL dlg( editFrame, drillJob, aParent );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "pos",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_POS* posJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( posJob && editFrame, false );

                        DIALOG_GEN_FOOTPRINT_POSITION dlg( posJob, editFrame, aParent );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "fpupgrade",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "fpsvg",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "drc",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_PCB_DRC* drcJob = dynamic_cast<JOB_PCB_DRC*>( job );

                        KI_CHECK( drcJob, false );

                        DIALOG_DRC_JOB_CONFIG dlg( aParent, drcJob );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "ipc2581",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_IPC2581* ipcJob = dynamic_cast<JOB_EXPORT_PCB_IPC2581*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( ipcJob && editFrame, false );

                        DIALOG_EXPORT_2581 dlg( ipcJob, editFrame, aParent );
                        return dlg.ShowModal() == wxID_OK;
                    } );
    RegisterConfig( "ipcd356",
                    []( JOB* job, wxWindow* aParent ) -> bool
                    {
                        return true;
                    } );
    RegisterConfig( "odb",
                    [this]( JOB* job, wxWindow* aParent ) -> bool
                    {
                        JOB_EXPORT_PCB_ODB* odbJob = dynamic_cast<JOB_EXPORT_PCB_ODB*>( job );

                        PCB_EDIT_FRAME* editFrame = boardEditor();

                        KI_CHECK( odbJob && editFrame, false );

                        DIALOG_EXPORT_ODBPP dlg( odbJob, editFrame, aParent );
                        return dlg.ShowModal() == wxID_OK;
                    } );
}


PCB_EDIT_FRAME* PCBNEW_JOBS_HANDLER_GUI::boardEditor() const
{
    return dynamic_cast<PCB_EDIT_FRAME*>( m_guiKiway->Player( FRAME_PCB_EDITOR, false ) );
}


BOARD* PCBNEW_JOBS_HANDLER_GUI::getEditorBoard()
{
    PCB_EDIT_FRAME* editFrame = boardEditor();
    return editFrame ? editFrame->GetBoard() : nullptr;
}


TOOL_MANAGER* PCBNEW_JOBS_HANDLER_GUI::getEditorToolManager()
{
    PCB_EDIT_FRAME* editFrame = boardEditor();
    return editFrame ? editFrame->GetToolManager() : nullptr;
}


bool PCBNEW_JOBS_HANDLER_GUI::getEditorNetlist( std::string& aNetlist )
{
    if( !m_guiKiway->Player( FRAME_SCH, false ) )
        return false;

    m_guiKiway->ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, aNetlist );
    return true;
}
