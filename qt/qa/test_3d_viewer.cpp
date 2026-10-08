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
 * @file test_3d_viewer.cpp
 * qa_qt_3d_viewer: the 3D canvas of KiCad on Qt (phase 3, milestone 3B.3).
 *
 * The 3D canvas of the board (EDA_3D_CANVAS, with the OpenGL renderer and the ray tracer of
 * KiCad) in a window of Qt: the test draws a board, moves the camera with the mouse events of
 * Qt, renders an image off screen (the export of the 3D viewer) and with the ray tracer.  It
 * needs a display with OpenGL (xvfb-run).
 *
 * Usage: qa_qt_3d_viewer <board.kicad_pcb> [screenshot.png]
 */

#include <kicad_gl/kiglad.h> // Must be included first

#include <cstdio>
#include <memory>
#include <set>
#include <string>

#include <kiqt.h>     // Qt, without its METHOD macro

#include <QApplication>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTimer>
#include <QWidget>

#include <wx/init.h>

#include <kiqt_app.h>

#include <3d_canvas/board_adapter.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <3d_rendering/track_ball.h>
#include <3d_viewer/eda_3d_viewer_settings.h>
#include <bitmaps.h>
#include <board.h>
#include <dialog_shim.h>
#include <dialogs/dialog_color_picker.h>
#include <dialogs/dialog_export_3d_image.h>
#include <eda_list_dialog.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <common_ogl/ogl_attr_list.h>
#include <kiface_base.h>
#include <kiway.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <raster_image.h>
#include <raster_image_qt.h>
#include <settings/settings_manager.h>
#include <toolkit/gui_services.h>


namespace
{

int s_failures = 0;


void check( bool aOk, const char* aWhat )
{
    std::printf( "  %s  %s\n", aOk ? "ok  " : "FAIL", aWhat );

    if( !aOk )
        s_failures++;
}


/// The program of the test.
class PGM_QT_TEST : public PGM_BASE
{
public:
    ~PGM_QT_TEST() override { Destroy(); }
};


/// Let the event loop run (the refresh requests and the timers of the canvas).
void settle( int aMilliseconds = 50 )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// The number of different colors of an image (a drawn board has many).
size_t colorCount( const RASTER_IMAGE& aImage )
{
    std::set<unsigned int> colors;

    if( !aImage.IsOk() )
        return 0;

    const unsigned char* rgb = aImage.GetData();

    for( int ii = 0; ii < aImage.GetWidth() * aImage.GetHeight() && colors.size() < 1000; ++ii )
    {
        colors.insert( ( rgb[0] << 16 ) | ( rgb[1] << 8 ) | rgb[2] );
        rgb += 3;
    }

    return colors.size();
}


/// The image of the canvas, drawn now.
RASTER_IMAGE screenshot( EDA_3D_CANVAS* aCanvas )
{
    aCanvas->DoRePaint();
    aCanvas->DoRePaint();
    return aCanvas->GetScreenshot();
}


/// The mouse events of Qt, as the OpenGL window of the canvas gives them.
Qt::MouseButtons s_buttons = Qt::NoButton;


void mouse( QWidget* aCanvas, QEvent::Type aType, QPoint aPoint, Qt::MouseButton aButton )
{
    if( aType == QEvent::MouseButtonPress )
        s_buttons |= aButton;
    else if( aType == QEvent::MouseButtonRelease )
        s_buttons &= ~aButton;

    QMouseEvent event( aType, aPoint, aCanvas->mapToGlobal( aPoint ), aButton, s_buttons,
                       Qt::NoModifier );
    QApplication::sendEvent( aCanvas, &event );
}


void wheel( QWidget* aCanvas, QPoint aPoint, int aDelta )
{
    QWheelEvent event( aPoint, aCanvas->mapToGlobal( aPoint ), QPoint(), QPoint( 0, aDelta ),
                       Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false );
    QApplication::sendEvent( aCanvas, &event );
}

} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> [screenshot.png]\n", argv[0] );
        return 2;
    }

    // The results show up even if the test crashes
    std::setvbuf( stdout, nullptr, _IOLBF, 0 );

    // The application object of a program with a user interface (Pgm().IsGUI())
    KIQT_APP      kiqtApp;
    wxInitializer wxBase( argc, argv );

    PGM_QT_TEST program;
    SetPgm( &program );

    if( !program.InitPgm( true, true ) )
    {
        std::fprintf( stderr, "cannot initialise the program\n" );
        return 2;
    }

    QApplication app( argc, argv );

    KIWAY kiway( KFCTL_STANDALONE );
    program.GetSettingsManager().SetKiway( &kiway );
    Kiface().OnKifaceStart( &program, KFCTL_STANDALONE, &kiway );

    std::unique_ptr<BOARD> board( PCB_IO_MGR::Load( PCB_IO_MGR::KICAD_SEXP,
                                                    wxString::FromUTF8( argv[1] ) ) );
    check( board != nullptr, "load the board" );

    if( !board )
        return 1;

    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
    check( cfg != nullptr, "the settings of the 3D viewer" );

    if( !cfg )
        return 1;

    cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
    cfg->m_Camera.animation_enabled = false;

    {
        S3D_CACHE*    cache = PROJECT_PCB::Get3DCacheManager( &program.GetSettingsManager().Prj() );
        BOARD_ADAPTER adapter;
        TRACK_BALL    camera( 2 * RANGE_SCALE_3D );

        adapter.SetBoard( board.get() );
        adapter.m_Cfg = cfg;
        adapter.InitSettings( nullptr, nullptr );

        // The canvas in a panel of a window, as in the 3D viewer (the canvas draws while the
        // window of its parent is shown)
        QWidget      window;
        QVBoxLayout* windowLayout = new QVBoxLayout( &window );
        QWidget*     panel = new QWidget( &window );
        QVBoxLayout* layout = new QVBoxLayout( panel );
        windowLayout->setContentsMargins( 0, 0, 0, 0 );
        windowLayout->addWidget( panel );
        layout->setContentsMargins( 0, 0, 0, 0 );

        EDA_3D_CANVAS* canvas = new EDA_3D_CANVAS( panel,
                                                   OGL_ATT_LIST::GetAttributesList(
                                                           ANTIALIASING_MODE::AA_NONE, true ),
                                                   adapter, camera, cache );
        canvas->SetAnimationEnabled( false );

        std::vector<wxString> statusTexts( 5 );

        canvas->SetStatusTextHandler(
                [&]( const wxString& aText, int aField )
                {
                    statusTexts[aField] = aText;
                } );

        layout->addWidget( canvas );
        window.resize( 800, 600 );
        window.show();
        settle( 300 );

        std::printf( "3D canvas (EDA_3D_CANVAS, HIDPI_GL_3D_CANVAS of Qt)\n" );

        check( canvas->IsShownOnScreen(), "the canvas is shown" );
        check( canvas->GetCurrentRender() != nullptr, "the OpenGL renderer" );

        // The board drawn by the OpenGL renderer (the first drawing builds the scene)
        RASTER_IMAGE image;

        for( int attempt = 0; attempt < 20 && colorCount( image ) < 20; ++attempt )
        {
            image = screenshot( canvas );
            settle( 100 );
        }

        check( image.IsOk() && image.GetWidth() > 0, "screenshot of the canvas" );
        check( colorCount( image ) >= 20, "the board is drawn in 3D" );

        if( argc > 2 )
            check( ToQImage( image ).save( QString::fromUtf8( argv[2] ) ), "save the screenshot" );

        std::printf( "Camera and mouse (MOUSE_STATE from the events of Qt)\n" );

        QPoint center( canvas->width() / 2, canvas->height() / 2 );
        float  zoom = camera.GetZoom();

        wheel( canvas, center, 120 );
        settle();
        check( camera.GetZoom() != zoom, "the wheel zooms" );
        check( statusTexts[ZOOM_LEVEL].StartsWith( wxT( "zoom" ) ), "the zoom in the status bar" );

        glm::mat4 rotation = camera.GetRotationMatrix();

        mouse( canvas, QEvent::MouseMove, center, Qt::NoButton );
        mouse( canvas, QEvent::MouseButtonPress, center, Qt::LeftButton );

        for( int step = 1; step <= 5; ++step )
            mouse( canvas, QEvent::MouseMove, center + QPoint( step * 20, step * 10 ), Qt::NoButton );

        mouse( canvas, QEvent::MouseButtonRelease, center + QPoint( 100, 50 ), Qt::LeftButton );
        settle();
        check( camera.GetRotationMatrix() != rotation, "a drag with the left button rotates" );

        glm::mat4 view = camera.GetViewMatrix();
        check( canvas->SetView3D( VIEW3D_TYPE::VIEW3D_TOP ), "a view command" );
        settle( 100 );
        check( camera.GetViewMatrix() != view, "the view from the top" );

        RASTER_IMAGE top = screenshot( canvas );
        check( colorCount( top ) >= 20, "the board drawn from the top" );

        if( argc > 2 )
        {
            QString path = QString::fromUtf8( argv[2] );
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-top.png" ) );
            check( ToQImage( top ).save( path ), "save the view from the top" );
        }

        std::printf( "Off-screen rendering (the export of the 3D viewer)\n" );

        {
            std::unique_ptr<TOOLKIT_WINDOW> hidden = TOOLKIT::CreateHiddenWindow( &window,
                                                                                  wxSize( 400, 300 ) );
            TRACK_BALL     exportCamera( camera );
            exportCamera.SetCurWindowSize( wxSize( 400, 300 ) );

            auto offscreen = std::make_unique<EDA_3D_CANVAS>( hidden.get(),
                                                              OGL_ATT_LIST::GetAttributesList(
                                                                      ANTIALIASING_MODE::AA_NONE, true ),
                                                              adapter, exportCamera, cache );
            offscreen->ResizeCanvas( 400, 300 );

            std::vector<unsigned char> buffer( 400 * 300 * 4 );
            offscreen->RenderToFrameBuffer( buffer.data(), 400, 300 );

            std::set<unsigned int> colors;

            for( size_t ii = 0; ii + 3 < buffer.size() && colors.size() < 100; ii += 4 )
                colors.insert( ( buffer[ii] << 16 ) | ( buffer[ii + 1] << 8 ) | buffer[ii + 2] );

            check( colors.size() >= 20, "the board rendered off screen" );
        }

        std::printf( "Ray tracing (RENDER_3D_RAYTRACE_GL)\n" );

        cfg->m_Render.engine = RENDER_ENGINE::RAYTRACING;
        canvas->RenderEngineChanged();

        RASTER_IMAGE traced;
        QElapsedTimer timer;
        timer.start();

        // The ray tracer draws the image by blocks, from the event loop
        while( timer.elapsed() < 60000 && canvas->GetCurrentRender()->IsReloadRequestPending() )
        {
            canvas->DoRePaint();
            settle( 50 );
        }

        for( int pass = 0; pass < 200; ++pass )
        {
            canvas->DoRePaint();
            settle( 20 );
        }

        traced = canvas->GetScreenshot();
        check( colorCount( traced ) >= 20, "the board drawn by the ray tracer" );

        if( argc > 2 )
        {
            QString path = QString::fromUtf8( argv[2] );
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-raytracing.png" ) );
            check( ToQImage( traced ).save( path ), "save the ray traced image" );
        }

        cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
        canvas->ReleaseOpenGL();
    }

    std::printf( "Widgets and dialogs of the 3D viewer\n" );

    {
        QWidget window;
        window.resize( 300, 200 );
        window.show();
        settle();

        // The visibility toggle of the appearance panel
        int  toggled = -1;
        auto toggle = new BITMAP_TOGGLE( &window, 7, KiBitmapBundle( BITMAPS::visibility ),
                                         KiBitmapBundle( BITMAPS::visibility_off ), true );
        toggle->SetToggleCallback( [&]( bool aChecked ) { toggled = aChecked; } );
        toggle->show();
        settle();

        QMouseEvent click( QEvent::MouseButtonRelease, QPointF( 2, 2 ), QPointF( 2, 2 ),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier );
        QApplication::sendEvent( toggle, &click );
        settle();
        check( toggled == 0 && !toggle->GetValue(), "a click on the visibility toggle" );

        // The color swatch and the color picker of KiCad
        bool changed = false;
        auto swatch = new COLOR_SWATCH( &window, KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ), 3,
                                        KIGFX::COLOR4D::WHITE, KIGFX::COLOR4D::BLACK, SWATCH_SMALL );
        swatch->SetChangedCallback( [&]() { changed = true; } );
        swatch->show();
        settle();

        QTimer::singleShot( 300,
                [&, argc, argv]()
                {
                    QWidget*   dialog = QApplication::activeModalWidget();
                    QLineEdit* hex = dialog ? dialog->findChild<QLineEdit*>() : nullptr;

                    check( dynamic_cast<DIALOG_COLOR_PICKER*>( dialog ) != nullptr,
                           "the color picker of KiCad" );

                    if( hex )
                        hex->setText( QStringLiteral( "#00FF00" ) );

                    if( dialog && argc > 2 )
                    {
                        QString path = QString::fromUtf8( argv[2] );
                        path.replace( QStringLiteral( ".png" ), QStringLiteral( "-color-picker.png" ) );
                        dialog->grab().save( path );
                    }

                    if( DIALOG_SHIM* shim = dynamic_cast<DIALOG_SHIM*>( dialog ) )
                        shim->EndModal( wxID_OK );
                } );

        swatch->GetNewSwatchColor();
        settle();
        check( changed && swatch->GetSwatchColor() == KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 ),
               "a color chosen in the picker" );

        // The list of the presets to delete
        wxArrayString              headers;
        std::vector<wxArrayString> items;
        headers.Add( wxT( "Presets" ) );

        for( const char* name : { "Zeta", "Alpha", "Mid" } )
        {
            wxArrayString item;
            item.Add( wxString::FromUTF8( name ) );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG list( &window, wxT( "Delete Preset" ), headers, items, wxT( "Mid" ) );
        check( list.GetTextSelection() == wxT( "Mid" ), "the list dialog preselects an item" );
        check( list.GetSelection() == 1, "the items of the list dialog are sorted" );

        // The size of an exported image
        EDA_3D_VIEWER_SETTINGS::EXPORT_IMAGE_SETTINGS exportCfg;
        exportCfg.width = 0;
        exportCfg.height = 0;
        exportCfg.x_resolution = 300.0;
        exportCfg.y_resolution = 300.0;
        exportCfg.lock_aspect_ratio = true;
        exportCfg.size_units = 0;
        exportCfg.resolution_units = 0;

        DIALOG_EXPORT_3D_IMAGE exportDlg( &window, wxSize( 400, 200 ), &exportCfg );
        QList<QDoubleSpinBox*> spins = exportDlg.findChildren<QDoubleSpinBox*>();

        if( spins.size() >= 2 )
            spins[0]->setValue( 800 );

        QTimer::singleShot( 200, [&]() { exportDlg.EndModal( wxID_OK ); } );
        exportDlg.ShowModal();
        check( exportDlg.GetSize() == wxSize( 800, 400 ), "the aspect ratio of the exported image" );
    }

    board.reset();

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
