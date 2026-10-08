/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file eda_3d_viewer_frame_qt.cpp
 * The parts of EDA_3D_VIEWER_FRAME that depend on the GUI toolkit, on Qt (see
 * 3d-viewer/3d_viewer/eda_3d_viewer_frame_wx.cpp): the layout of the window (the appearance
 * panel in a dock), the file dialog and the clipboard.
 */

#include <QClipboard>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>

#include "eda_3d_viewer_frame.h"
#include "dialogs/appearance_controls_3D.h"
#include <bitmaps.h>
#include <confirm.h>
#include <eda_3d_viewer_settings.h>
#include <qt_strings.h>
#include <raster_image_qt.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_infobar.h>
#include <wildcards_and_files_ext.h>


/// The name of the dock of the appearance panel (the name of its pane in the wxWidgets build).
static const QString LAYERS_MANAGER = QStringLiteral( "LayersManager" );


void EDA_3D_VIEWER_FRAME::setupWindow( EDA_3D_VIEWER_SETTINGS* aCfg )
{
    // Give it an icon
    setWindowIcon( KiBitmapBundle( BITMAPS::icon_3d ) );

    // Create the status line: the fields of EDA_3D_VIEWER_STATUSBAR, as the wxWidgets build
    // (the first one is stretched)
    static const int status_dims[5] = { -1, 170, 130, 130, 130 };

    CreateStatusBar( 5 );
    SetStatusWidths( 5, status_dims );

    // The main area: the infobar above the canvas
    QWidget*     central = new QWidget( this );
    QVBoxLayout* layout = new QVBoxLayout( central );

    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    CreateInfoBar();
    m_infoBar->setParent( central );
    layout->addWidget( m_infoBar );

    m_canvas->setParent( central );
    layout->addWidget( m_canvas, 1 );

    setCentralWidget( central );

    // The appearance panel, on the right
    QDockWidget* layersManager = new QDockWidget( ToQString( _( "Appearance" ) ), this );
    layersManager->setObjectName( LAYERS_MANAGER );
    layersManager->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
    layersManager->setWidget( m_appearancePanel );
    layersManager->setMinimumWidth( 180 );
    addDockWidget( Qt::RightDockWidgetArea, layersManager );

    if( aCfg && aCfg->m_AuiPanels.right_panel_width > 0 )
        resizeDocks( { layersManager }, { aCfg->m_AuiPanels.right_panel_width }, Qt::Horizontal );

    if( aCfg )
        layersManager->setVisible( aCfg->m_AuiPanels.show_layer_manager );

    // We don't want the infobar displayed right away
    m_infoBar->hide();

    m_canvas->SetInfoBar( m_infoBar );
    m_canvas->SetStatusTextHandler(
            [this]( const wxString& aText, int aField )
            {
                SetStatusText( aText, aField );
            } );

    // The frame gives its focus to the canvas, so it will catch mouse and key events
    setFocusProxy( m_canvas );
}


void EDA_3D_VIEWER_FRAME::releaseWindow()
{
    // The canvas does not report to the status bar of the frame being deleted
    m_canvas->SetStatusTextHandler( nullptr );
}


void EDA_3D_VIEWER_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    SetTitle( _( "3D Viewer" ) );
    RecreateToolbars();

    if( QDockWidget* layersManager = findChild<QDockWidget*>( LAYERS_MANAGER ) )
        layersManager->setWindowTitle( ToQString( _( "Appearance" ) ) );

    SetStatusText( wxEmptyString, ACTIVITY );
    SetStatusText( wxEmptyString, HOVERED_ITEM );
}


void EDA_3D_VIEWER_FRAME::ToggleAppearanceManager()
{
    QDockWidget* layersManager = findChild<QDockWidget*>( LAYERS_MANAGER );

    if( !layersManager )
        return;

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        // show auxiliary Vertical layers and visibility manager toolbar
        cfg->m_AuiPanels.show_layer_manager = !cfg->m_AuiPanels.show_layer_manager;

        if( !cfg->m_AuiPanels.show_layer_manager )
            cfg->m_AuiPanels.right_panel_width = m_appearancePanel->width();

        layersManager->setVisible( cfg->m_AuiPanels.show_layer_manager );

        if( cfg->m_AuiPanels.show_layer_manager && cfg->m_AuiPanels.right_panel_width > 0 )
        {
            resizeDocks( { layersManager }, { cfg->m_AuiPanels.right_panel_width },
                         Qt::Horizontal );
        }
    }
}


bool EDA_3D_VIEWER_FRAME::getExportFileName( EDA_3D_VIEWER_EXPORT_FORMAT& aFormat,
                                             wxString& fullFileName )
{
    // Combined filters for both formats
    const QStringList filters = ToQtFileFilters( FILEEXT::JpegFileWildcard() )
                                + ToQtFileFilters( FILEEXT::PngFileWildcard() );

    if( !m_defaultSaveScreenshotFileName.IsOk() )
        m_defaultSaveScreenshotFileName = Parent()->Prj().GetProjectFullName();

    // Set default extension based on current format
    const wxString defaultExt = ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ?
                                FILEEXT::JpegFileExtension : FILEEXT::PngFileExtension;
    m_defaultSaveScreenshotFileName.SetExt( defaultExt );

    QFileDialog dlg( this, ToQString( _( "3D Image File Name" ) ),
                     ToQString( m_defaultSaveScreenshotFileName.GetFullPath() ) );

    dlg.setAcceptMode( QFileDialog::AcceptSave );
    dlg.setOption( QFileDialog::HideNameFilterDetails );
    dlg.setNameFilters( filters );

    // Set initial filter index based on current format
    dlg.selectNameFilter( filters[( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ? 0 : 1] );

    if( dlg.exec() != QDialog::Accepted || dlg.selectedFiles().isEmpty() )
        return false;

    m_defaultSaveScreenshotFileName = ToWxString( dlg.selectedFiles().first() );

    // Determine format based on file extension first
    wxString fileExt = m_defaultSaveScreenshotFileName.GetExt().Lower();
    EDA_3D_VIEWER_EXPORT_FORMAT detectedFormat;
    bool formatDetected = false;

    if( fileExt == wxT( "jpg" ) || fileExt == wxT( "jpeg" ) )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::JPEG;
        formatDetected = true;
    }
    else if( fileExt == wxT( "png" ) )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::PNG;
        formatDetected = true;
    }

    // If format can't be determined from extension, use dropdown selection
    if( !formatDetected )
    {
        int filterIndex = filters.indexOf( dlg.selectedNameFilter() );
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
        DisplayError( this, msg );
        return false;
    }

    fullFileName = m_defaultSaveScreenshotFileName.GetFullPath();
    return true;
}


void EDA_3D_VIEWER_FRAME::copyImageToClipboard( const RASTER_IMAGE& aScreenshotImage )
{
    QImage image = ToQImage( aScreenshotImage );

    if( image.isNull() )
    {
        DisplayError( this, _( "Failed to copy image to clipboard" ) );
        return;
    }

    QGuiApplication::clipboard()->setImage( image );
}
