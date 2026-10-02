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
 * @file pcb_edit_frame_export.cpp
 * PCB_EDIT_FRAME export and import commands (IDF, VRML, Specctra): error reporting and view
 * updates around the exporters of pcbnew/exporters and pcbnew/specctra_import_export.
 */

#include <wx/msgdlg.h>

#include <board.h>
#include <confirm.h>
#include <exporters/export_idf.h>
#include <exporters/export_vrml.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pcb_track.h>
#include <specctra_import_export/specctra.h>
#include <view/view.h>

using namespace DSN;


/**
 * Generate IDFv3 compliant board (*.emn) and library (*.emp) files representing the user's
 * PCB design.
 */
bool PCB_EDIT_FRAME::Export_IDF3( BOARD* aPcb, const wxString& aFullFileName,
                                  bool aUseThou, double aXRef, double aYRef,
                                  bool aIncludeUnspecified, bool aIncludeDNP )
{
    wxString errorMsg;

    bool ok = ExportBoardToIDF3( aPcb, &Prj(), aFullFileName, aUseThou, aXRef, aYRef,
                                 aIncludeUnspecified, aIncludeDNP, &errorMsg );

    if( !ok )
        wxMessageBox( errorMsg );

    return ok;
}


bool PCB_EDIT_FRAME::ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                                      bool aIncludeUnspecified, bool aIncludeDNP,
                                      bool aExport3DFiles, bool aUseRelativePaths,
                                      const wxString& a3D_Subdir,
                                      double aXRef, double aYRef )
{
    bool     success;
    wxString msgs;
    EXPORTER_VRML model3d( GetBoard() );

    success = model3d.ExportVRML_File( &Prj(), &msgs, aFullFileName, aMMtoWRMLunit,
                                       aIncludeUnspecified, aIncludeDNP,
                                       aExport3DFiles, aUseRelativePaths,
                                       a3D_Subdir, aXRef, aYRef );

    if( !msgs.IsEmpty() )
        wxMessageBox( msgs );

    return success;
}


bool PCB_EDIT_FRAME::ExportSpecctraFile( const wxString& aFullFilename )
{
    BASE_SCREEN* screen = GetScreen();
    bool         wasModified = screen->IsContentModified();
    wxString     errorText;
    bool         ok = true;

    try
    {
        DSN::ExportBoardToSpecctraFile( GetBoard(), aFullFilename );
    }
    catch( const IO_ERROR& ioe )
    {
        ok = false;

        // copy the error string to safe place, ioe is in this scope only.
        errorText = ioe.What();
    }

    // The two calls to FOOTPRINT::Flip() in ExportBoardToSpecctraFile both set the modified flag,
    // yet their actions cancel each other out, so it should be ok to clear the flag.
    if( !wasModified )
        screen->SetContentModified( false );

    if( ok )
        SetStatusText( wxString( _( "BOARD exported OK." ) ) );
    else
        DisplayErrorMessage( this, _( "Unable to export, please fix and try again" ), errorText );

    return ok;
}


bool PCB_EDIT_FRAME::ImportSpecctraSession( const wxString& fullFileName )
{
    // To avoid issues with undo/redo lists (dangling pointers) clear the lists
    // todo: use undo/redo feature
    ClearUndoRedoList();

    if( GetCanvas() )    // clear view:
    {
        for( PCB_TRACK* track : GetBoard()->Tracks() )
            GetCanvas()->GetView()->Remove( track );
    }

    try
    {
        DSN::ImportSpecctraSession( GetBoard(), fullFileName );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = _( "Board may be corrupted, do not save it.\n Fix problem and try again" );

        wxString extra = ioe.What();

        DisplayErrorMessage( this, msg, extra );
        return false;
    }

    OnModify();

    if( GetCanvas() )    // Update view:
    {
        // Update footprint positions

        // add imported tracks (previous tracks are removed, therefore all are new)
        for( PCB_TRACK* track : GetBoard()->Tracks() )
            GetCanvas()->GetView()->Add( track );
    }

    SetStatusText( wxString( _( "Session file imported and merged OK." ) ) );

    Refresh();

    return true;
}
