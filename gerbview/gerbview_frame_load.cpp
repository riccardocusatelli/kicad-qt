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
 * @file gerbview_frame_load.cpp
 * GERBVIEW_FRAME commands loading Gerber, Excellon and job files: they run the readers
 * (readgerb.cpp, excellon_read_drill_file.cpp, job_file_reader.cpp), report their messages
 * and add the items to the view.
 */

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>

#include <core/ki_assert.h>
#include <dialogs/html_message_box.h>
#include <excellon_defaults.h>
#include <excellon_image.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerber_jobfile_reader.h>
#include <gerbview_frame.h>
#include <gerbview_settings.h>
#include <kiplatform/ui.h>
#include <reporter.h>
#include <view/view.h>
#include <wildcards_and_files_ext.h>


/* Read a gerber file, RS274D, RS274X or RS274X2 format.
 */
bool GERBVIEW_FRAME::Read_GERBER_File( const wxString& GERBER_FullFileName )
{
    wxString msg;

    int layer = GetActiveLayer();
    GERBER_FILE_IMAGE_LIST* images = GetImagesList();
    GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );

    if( gerber != nullptr )
    {
        Erase_Current_DrawLayer( false );
    }

    // use an unique ptr while we load to free on exception properly
    std::unique_ptr<GERBER_FILE_IMAGE> gerber_uptr = std::make_unique<GERBER_FILE_IMAGE>( layer );

    // Read the gerber file. The image will be added only if it can be read
    // to avoid broken data.
    bool success = gerber_uptr->LoadGerberFile( GERBER_FullFileName );

    if( !success )
    {
        gerber_uptr.reset();
        msg.Printf( _( "File '%s' not found" ), GERBER_FullFileName );
        ShowInfoBarError( msg );
        return false;
    }

    gerber = gerber_uptr.release();
    KI_ASSERT( gerber != nullptr );
    images->AddGbrImage( gerber, layer );

    // Display errors list
    if( gerber->GetMessages().size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Errors" ) );
        dlg.ListSet( gerber->GetMessages() );
        dlg.ShowModal();
    }

    /* if the gerber file has items using D codes but missing D codes definitions,
     * it can be a deprecated RS274D file (i.e. without any aperture information),
     * or has missing definitions,
     * warn the user:
     */
    if( gerber->GetItemsCount() && gerber->m_Has_MissingDCode )
    {
        if( !gerber->m_Has_DCode )
            msg = _("Warning: this file has no D-Code definition\n"
                    "Therefore the size of some items is undefined");
        else
            msg = _("Warning: this file has some missing D-Code definitions\n"
                    "Therefore the size of some items is undefined");

        wxMessageBox( msg );
    }

    if( GetCanvas() )
    {
        if( gerber->m_ImageNegative )
        {
            // TODO: find a way to handle negative images
            // (maybe convert geometry into positives?)
        }

        for( GERBER_DRAW_ITEM* item : gerber->GetItems() )
            GetCanvas()->GetView()->Add( (KIGFX::VIEW_ITEM*) item );
    }

    return true;
}


bool GERBVIEW_FRAME::Read_EXCELLON_File( const wxString& aFullFileName )
{
    wxString msg;
    int layerId = GetActiveLayer();      // current layer used in GerbView
    GERBER_FILE_IMAGE_LIST* images = GetGerberLayout()->GetImagesList();
    GERBER_FILE_IMAGE* gerber_layer = images->GetGbrImage( layerId );

    // If the active layer contains old gerber or nc drill data, remove it
    if( gerber_layer )
        Erase_Current_DrawLayer( false );

    std::unique_ptr<EXCELLON_IMAGE> drill_layer_uptr = std::make_unique<EXCELLON_IMAGE>( layerId );

    EXCELLON_DEFAULTS nc_defaults;
    GERBVIEW_SETTINGS* cfg = static_cast<GERBVIEW_SETTINGS*>( config() );
    cfg->GetExcellonDefaults( nc_defaults );

    // Read the Excellon drill file:
    bool success = drill_layer_uptr->LoadFile( aFullFileName, &nc_defaults );

    if( !success )
    {
        drill_layer_uptr.reset();
        msg.Printf( _( "File %s not found." ), aFullFileName );
        ShowInfoBarError( msg );
        return false;
    }

    EXCELLON_IMAGE* drill_layer = drill_layer_uptr.release();

    layerId = images->AddGbrImage( drill_layer, layerId );

    if( layerId < 0 )
    {
        delete drill_layer;
        ShowInfoBarError( _( "No empty layers to load file into." ) );
        return false;
    }

    // Display errors list
    if( drill_layer->GetMessages().size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error reading EXCELLON drill file" ) );
        dlg.ListSet( drill_layer->GetMessages() );
        dlg.ShowModal();
    }

    if( GetCanvas() )
    {
        for( GERBER_DRAW_ITEM* item : drill_layer->GetItems() )
            GetCanvas()->GetView()->Add( (KIGFX::VIEW_ITEM*) item );
    }

    return success;
}


bool GERBVIEW_FRAME::LoadGerberJobFile( const wxString& aFullFileName )
{
    wxFileName filename = aFullFileName;
    wxString currentPath;
    bool success = true;

    if( !filename.IsOk() )
    {
        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this, _( "Open Gerber Job File" ),
                          currentPath,
                          filename.GetFullName(),
                          FILEEXT::GerberJobFileWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        filename = dlg.GetPath();
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }
    else
    {
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    WX_STRING_REPORTER reporter;

    if( filename.IsOk() )
    {
        GERBER_JOBFILE_READER gbjReader( filename.GetFullPath(), &reporter );

        if( gbjReader.ReadGerberJobFile() )
        {
            // Update the list of recent drill files.
            UpdateFileHistory( filename.GetFullPath(), &m_jobFileHistory );

            Clear_DrawLayers( false );
            ClearMsgPanel();

            wxArrayString& gbrfiles = gbjReader.GetGerberFiles();

            // 0 = Gerber file type
            std::vector<int> fileTypesVec( gbrfiles.Count(), 0 );
            success = LoadListOfGerberAndDrillFiles( currentPath, gbrfiles, &fileTypesVec );

            Zoom_Automatique( false );
        }
    }

    SortLayersByX2Attributes();

    if( reporter.HasMessage() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( reporter.GetMessages() );
        mbox.ShowModal();
    }

    return success;
}
