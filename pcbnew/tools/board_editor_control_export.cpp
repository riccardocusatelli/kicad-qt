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
 * @file board_editor_control_export.cpp
 * Board editor export commands (IPC-D-356, Hyperlynx, footprint associations, BOM, GenCAD):
 * the file dialogs around the exporters of pcbnew/exporters.
 */

#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/filedlgcustomize.h>

#include <board.h>
#include <board_design_settings.h>
#include <build_BOM_from_board.h>
#include <confirm.h>
#include <dialogs/dialog_gencad_export_options.h>
#include <exporters/export_d356.h>
#include <exporters/export_footprint_associations.h>
#include <exporters/export_gencad_writer.h>
#include <exporters/export_hyperlynx.h>
#include <kiplatform/ui.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <tools/board_editor_control.h>
#include <wildcards_and_files_ext.h>


// Subclass for wxFileDialogCustomizeHook to add the checkbox
class D365_CUSTOMIZE_HOOK : public wxFileDialogCustomizeHook
{
public:

public:
    D365_CUSTOMIZE_HOOK( bool aDoNotExportUnconnectedPads = false ) :
            m_doNotExportUnconnectedPads( aDoNotExportUnconnectedPads ),
            m_cb(nullptr)
    {};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

        m_cb = customizer.AddCheckBox( _( "Do not export unconnected pads" ) );
        m_cb->SetValue( m_doNotExportUnconnectedPads );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_doNotExportUnconnectedPads = m_cb->GetValue();
    }

    bool GetDoNotExportUnconnectedPads() const { return m_doNotExportUnconnectedPads; }
private:

    bool m_doNotExportUnconnectedPads;
    wxFileDialogCheckBox* m_cb;

    wxDECLARE_NO_COPY_CLASS( D365_CUSTOMIZE_HOOK );
};


int BOARD_EDITOR_CONTROL::GenD356File( const TOOL_EVENT& aEvent )
{
    wxFileName  fn = m_frame->GetBoard()->GetFileName();
    wxString    ext, wildcard;

    ext = FILEEXT::IpcD356FileExtension;
    wildcard = FILEEXT::IpcD356FileWildcard();
    fn.SetExt( ext );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( m_frame, _( "Generate IPC-D-356 netlist file" ), pro_dir, fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    D365_CUSTOMIZE_HOOK customizeHook( m_frame->GetPcbNewSettings()->m_ExportD356.doNotExportUnconnectedPads );
    dlg.SetCustomizeHook( customizeHook );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    IPC356D_WRITER writer( m_frame->GetBoard() );
    bool doNotExportUnconnectedPads = customizeHook.GetDoNotExportUnconnectedPads();
    writer.SetDoNotExportUnconnectedPads( doNotExportUnconnectedPads );
    m_frame->GetPcbNewSettings()->m_ExportD356.doNotExportUnconnectedPads = doNotExportUnconnectedPads;

    if( writer.Write( dlg.GetPath() ) )
    {
        DisplayInfoMessage( m_frame, wxString::Format( _( "IPC-D-356 netlist file created:\n'%s'." ),
                                        dlg.GetPath() ) );
    }
    else
    {
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ),
                                                 dlg.GetPath() ) );
    }

    return 0;
}


int BOARD_EDITOR_CONTROL::ExportHyperlynx( const TOOL_EVENT& aEvent )
{
    wxString    wildcard =  wxT( "*.hyp" );
    BOARD*      board = m_frame->GetBoard();
    wxFileName  fn = board->GetFileName();

    fn.SetExt( wxT("hyp") );

    wxFileDialog dlg( m_frame, _( "Export Hyperlynx Layout" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( wxT( "hyp" ) );

    ExportBoardToHyperlynx( board, fn );

    return 0;
}


int BOARD_EDITOR_CONTROL::ExportCmpFile( const TOOL_EVENT& aEvent )
{
    // Build the .cmp file name from the board name
    BOARD*     board = m_frame->GetBoard();
    wxString   projectDir = wxPathOnly( m_frame->Prj().GetProjectFullName() );
    wxFileName fn = board->GetFileName();

    fn.SetExt( FILEEXT::FootprintAssignmentFileExtension );

    wxFileDialog dlg( m_frame, _( "Save Footprint Association File" ), projectDir, fn.GetFullName(),
                      FILEEXT::FootprintAssignmentFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxString path = dlg.GetPath();

    if( !RecreateCmpFile( board, path ) )
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), path ) );

    return 0;
}


int BOARD_EDITOR_CONTROL::GenBOMFileFromBoard( const TOOL_EVENT& aEvent )
{
    BOARD*     board = m_frame->GetBoard();
    wxFileName fn;

    if( board->Footprints().empty() )
    {
        m_frame->ShowInfoBarError( _( "Cannot export BOM: there are no footprints on the PCB." ) );
        return 0;
    }

    /* Set the file extension: */
    fn = board->GetFileName();
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( m_frame, _( "Save Bill of Materials" ), pro_dir, fn.GetFullName(),
                      FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    fn = dlg.GetPath();

    if( !WriteBoardBOM( board, fn.GetFullPath() ) )
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), fn.GetFullPath() ) );

    return 0;
}


/* Driver function: processing starts here */
int BOARD_EDITOR_CONTROL::ExportGenCAD( const TOOL_EVENT& aEvent )
{
    DIALOG_GENCAD_EXPORT_OPTIONS optionsDialog( m_frame, _( "Export to GenCAD" ), nullptr );

    if( optionsDialog.ShowModal() == wxID_CANCEL )
        return 0;

    wxString path = optionsDialog.GetFileName();

    // Get options
    bool flipBottomPads = optionsDialog.GetOption( FLIP_BOTTOM_PADS );
    bool uniquePinName = optionsDialog.GetOption( UNIQUE_PIN_NAMES );
    bool individualShapes = optionsDialog.GetOption( INDIVIDUAL_SHAPES );
    bool storeOriginCoords = optionsDialog.GetOption( STORE_ORIGIN_COORDS );

    // No idea on *why* this should be needed... maybe to fix net names?
    m_frame->Compile_Ratsnest( true );

    GENCAD_EXPORTER exporter( m_frame->GetBoard() );

    // This is the export origin (the auxiliary axis)
    VECTOR2I GencadOffset;
    VECTOR2I auxOrigin = m_frame->GetBoard()->GetDesignSettings().GetAuxOrigin();
    GencadOffset.x = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? auxOrigin.x : 0;
    GencadOffset.y = optionsDialog.GetOption( USE_AUX_ORIGIN ) ? auxOrigin.y : 0;

    exporter.SetPlotOffet( GencadOffset );
    exporter.FlipBottomPads( flipBottomPads );
    exporter.UsePinNamesUnique( uniquePinName );
    exporter.UseIndividualShapes( individualShapes );
    exporter.StoreOriginCoordsInFile( storeOriginCoords );

    bool success = exporter.WriteFile( path );

    if( !success )
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ), path ) );

    return 0;
}
