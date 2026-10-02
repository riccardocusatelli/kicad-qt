/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef PCBNEW_JOBS_HANDLER_H
#define PCBNEW_JOBS_HANDLER_H

#include <jobs/job_dispatcher.h>
#include <pcb_plot_params.h>

class KIWAY_SERVICES;
class BOARD;
class DS_PROXY_VIEW_ITEM;
class FOOTPRINT;
class JOB_EXPORT_PCB_GERBER;
class JOB_EXPORT_PCB_GERBERS;
class JOB_FP_EXPORT_SVG;
class TOOL_MANAGER;

/**
 * Run the Pcbnew jobs (exports, DRC, upgrades...).
 *
 * This class has no GUI code: in the editor, PCBNEW_JOBS_HANDLER_GUI adds the job settings
 * dialogs and lets the jobs work on the board open in the editor.
 */
class PCBNEW_JOBS_HANDLER : public JOB_DISPATCHER
{
public:
    PCBNEW_JOBS_HANDLER( KIWAY_SERVICES* aKiway );
    virtual ~PCBNEW_JOBS_HANDLER();

    int JobExportStep( JOB* aJob );
    int JobExportRender( JOB* aJob );
    int JobExportSvg( JOB* aJob );
    int JobExportDxf( JOB* aJob );
    int JobExportPdf( JOB* aJob );
    int JobExportPs( JOB* aJob );
    int JobExportGerber( JOB* aJob );
    int JobExportGerbers( JOB* aJob );
    int JobExportGencad( JOB* aJob );
    int JobExportDrill( JOB* aJob );
    int JobExportPos( JOB* aJob );
    int JobExportFpUpgrade( JOB* aJob );
    int JobExportFpSvg( JOB* aJob );
    int JobExportDrc( JOB* aJob );
    int JobExportIpc2581( JOB* aJob );
    int JobExportOdb( JOB* aJob );
    int JobExportIpcD356( JOB* aJob );
    int JobExportStats( JOB* aJob );
    int JobUpgrade( JOB* aJob );
    int JobImport( JOB* aJob );

protected:
    /// The board open in the board editor, if any (GUI only).
    virtual BOARD* getEditorBoard() { return nullptr; }

    /// The tool manager of the board editor, if open (GUI only).
    virtual TOOL_MANAGER* getEditorToolManager() { return nullptr; }

    /**
     * Get the netlist of the schematic open in the schematic editor (GUI only).
     * @return false if no schematic editor is open; the caller then generates the netlist
     *         from the schematic file.
     */
    virtual bool getEditorNetlist( std::string& aNetlist ) { return false; }

private:
    BOARD* getBoard( const wxString& aPath = wxEmptyString );
    LSEQ convertLayerArg( wxString& aLayerString, BOARD* aBoard ) const;

    void populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS&  aPlotOpts,
                                           JOB_EXPORT_PCB_GERBER* aJob );
    void populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS& aPlotOpts,
                                           JOB_EXPORT_PCB_GERBERS* aJob );
    int  doFpExportSvg( JOB_FP_EXPORT_SVG* aSvgJob, const FOOTPRINT* aFootprint );
    void loadOverrideDrawingSheet( BOARD* brd, const wxString& aSheetPath );
    wxString resolveJobOutputPath( JOB* aJob, BOARD* aBoard, const wxString* aDrawingSheet = nullptr );

    DS_PROXY_VIEW_ITEM* getDrawingSheetProxyView( BOARD* aBrd );

    TOOL_MANAGER* getToolManager( BOARD* aBrd );

    BOARD* m_cliBoard;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif
