/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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
 * @file eda_3d_viewer_frame_wx.cpp
 * The parts of EDA_3D_VIEWER_FRAME that depend on wxWidgets: the layout of its panes (AUI),
 * the file dialog and the clipboard.
 */

#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/statusbr.h>

#include "eda_3d_viewer_frame.h"
#include "dialogs/appearance_controls_3D.h"
#include <bitmaps.h>
#include <core/arraydim.h>
#include <eda_3d_viewer_settings.h>
#include <kiplatform/ui.h>
#include <raster_image_wx.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/wx_infobar.h>
#include <wildcards_and_files_ext.h>


void EDA_3D_VIEWER_FRAME::setupWindow( EDA_3D_VIEWER_SETTINGS* aCfg )
{
    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_3d ) );
    SetIcon( icon );

    // Create the status line
    static const int status_dims[5] = { -1, 170, 130, 130, 130 };

    wxStatusBar *status_bar = CreateStatusBar( arrayDim( status_dims ) );
    SetStatusWidths( arrayDim( status_dims ), status_dims );

    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( wxS( "InfoBar" ) )
                      .Top().Layer( 1 ) );
    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( "LayersManager" )
                      .Right().Layer( 3 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      .MinSize( FromDIP( 180 ), -1 ).BestSize( FromDIP( 190 ), -1 ) );
    m_auimgr.AddPane( m_canvas, EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    wxAuiPaneInfo& layersManager = m_auimgr.GetPane( "LayersManager" );

    if( aCfg && aCfg->m_AuiPanels.right_panel_width > 0 )
        SetAuiPaneSize( m_auimgr, layersManager, aCfg->m_AuiPanels.right_panel_width, -1 );

    if( aCfg )
        layersManager.Show( aCfg->m_AuiPanels.show_layer_manager );

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hiding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( wxS( "InfoBar" ) ).Hide();
    m_auimgr.Update();

    m_canvas->SetInfoBar( m_infoBar );
    m_canvas->SetStatusTextHandler(
            [status_bar]( const wxString& aText, int aField )
            {
                status_bar->SetStatusText( aText, aField );
            } );

    // Activates again the focus of the canvas so it will catch mouse and key events
    Bind( wxEVT_SET_FOCUS,
          [this]( wxFocusEvent& aEvent )
          {
              if( m_canvas )
                  m_canvas->SetFocus();

              aEvent.Skip();
          } );
}


void EDA_3D_VIEWER_FRAME::releaseWindow()
{
    m_auimgr.UnInit();
}


void EDA_3D_VIEWER_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    SetTitle( _( "3D Viewer" ) );
    RecreateToolbars();

    if( m_appearancePanel )
    {
        wxAuiPaneInfo& lm_pane_info = m_auimgr.GetPane( m_appearancePanel );
        lm_pane_info.Caption( _( "Appearance" ) );
    }

    SetStatusText( wxEmptyString, ACTIVITY );
    SetStatusText( wxEmptyString, HOVERED_ITEM );
}


void EDA_3D_VIEWER_FRAME::ToggleAppearanceManager()
{
    wxAuiPaneInfo& layersManager = m_auimgr.GetPane( "LayersManager" );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        // show auxiliary Vertical layers and visibility manager toolbar
        cfg->m_AuiPanels.show_layer_manager = !cfg->m_AuiPanels.show_layer_manager;

        layersManager.Show( cfg->m_AuiPanels.show_layer_manager );

        if( cfg->m_AuiPanels.show_layer_manager )
        {
            SetAuiPaneSize( m_auimgr, layersManager, cfg->m_AuiPanels.right_panel_width, -1 );
        }
        else
        {
            cfg->m_AuiPanels.right_panel_width = m_appearancePanel->GetSize().x;
            m_auimgr.Update();
        }
    }
}


bool EDA_3D_VIEWER_FRAME::getExportFileName( EDA_3D_VIEWER_EXPORT_FORMAT& aFormat, wxString& fullFileName )
{
    // Create combined wildcard for both formats
    const wxString wildcard = FILEEXT::JpegFileWildcard() + "|" + FILEEXT::PngFileWildcard();

    if( !m_defaultSaveScreenshotFileName.IsOk() )
        m_defaultSaveScreenshotFileName = Parent()->Prj().GetProjectFullName();

    // Set default extension based on current format
    const wxString defaultExt = ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ?
                                FILEEXT::JpegFileExtension : FILEEXT::PngFileExtension;
    m_defaultSaveScreenshotFileName.SetExt( defaultExt );

    wxFileDialog dlg( this, _( "3D Image File Name" ),
                      m_defaultSaveScreenshotFileName.GetPath(),
                      m_defaultSaveScreenshotFileName.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    // Set initial filter index based on current format
    dlg.SetFilterIndex( ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ? 0 : 1 );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    m_defaultSaveScreenshotFileName = dlg.GetPath();

    // Determine format based on file extension first
    wxString fileExt = m_defaultSaveScreenshotFileName.GetExt().Lower();
    EDA_3D_VIEWER_EXPORT_FORMAT detectedFormat;
    bool formatDetected = false;

    if( fileExt == wxT("jpg") || fileExt == wxT("jpeg") )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::JPEG;
        formatDetected = true;
    }
    else if( fileExt == wxT("png") )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::PNG;
        formatDetected = true;
    }

    // If format can't be determined from extension, use dropdown selection
    if( !formatDetected )
    {
        int filterIndex = dlg.GetFilterIndex();
        detectedFormat = ( filterIndex == 0 ) ? EDA_3D_VIEWER_EXPORT_FORMAT::JPEG :
                                               EDA_3D_VIEWER_EXPORT_FORMAT::PNG;

        // Append appropriate extension
        const wxString ext = ( detectedFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ?
                            FILEEXT::JpegFileExtension : FILEEXT::PngFileExtension;
        m_defaultSaveScreenshotFileName.SetExt( ext );
    }

    // Update the format parameter
    aFormat = detectedFormat;

    // Check directory permissions using the updated filename
    wxFileName fn = m_defaultSaveScreenshotFileName;

    if( !fn.IsDirWritable() )
    {
        wxString msg;
        msg.Printf( _( "Insufficient permissions to save file '%s'." ),
                   m_defaultSaveScreenshotFileName.GetFullPath() );
        wxMessageBox( msg, _( "Error" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    fullFileName = m_defaultSaveScreenshotFileName.GetFullPath();
    return true;
}


void EDA_3D_VIEWER_FRAME::copyImageToClipboard( const RASTER_IMAGE& aScreenshotImage )
{
    wxBitmap bitmap( ToWxImage( aScreenshotImage ) );
    wxLogNull doNotLog;

    if( wxTheClipboard->Open() )
    {
        wxBitmapDataObject* dobjBmp = new wxBitmapDataObject( bitmap );

        if( !wxTheClipboard->SetData( dobjBmp ) )
            wxMessageBox( _( "Failed to copy image to clipboard" ) );

        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}
