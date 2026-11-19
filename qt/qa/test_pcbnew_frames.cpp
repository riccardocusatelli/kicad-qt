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
 * @file test_pcbnew_frames.cpp
 * The test of the frames of pcbnew on Qt: the KIFACE with the GUI (pcbnew/pcbnew.cpp and
 * pcbnew_gui_qt) makes the original frames (the PCB editor, the footprint editor, the footprint
 * viewer), which open a board and run their tools.
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QDialog>
#include <QDockWidget>
#include <QMainWindow>
#include <QMimeData>
#include <QDropEvent>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextLayout>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <wx/init.h>
#include <wx/dataview.h>
#include <wx/stc/stc.h>

#include <kiqt_app.h>

#include "qa_qt_frames.h"

#include <api/api_server.h>
#include <board.h>
#include <class_draw_panel_gal.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint_edit_frame.h>
#include <frame_type.h>
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pcb_edit_frame.h>
#include <pgm_gui.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_treebook.h>
#include <settings/common_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <startwizard/startwizard.h>
#include <libraries/library_manager.h>
#include <wx/filename.h>
#include <wx/wizard.h>
#include <tool/actions.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <zone.h>
#include <footprint_wizard.h>
#include <footprint_wizard_frame.h>
#include <dialogs/dialog_footprint_wizard_list.h>
#include <paths.h>
#include <pcb_text.h>
#include <QAction>
#include <QDir>
#include <tool/tool_manager.h>
#include "qa_qt_canvas.h"
#include "qa_qt_crash.h"   // last: it includes windows.h


/// The KIFACE linked in the test is the one of pcbnew in its KIWAY, as in the program of
/// single_top (the friend of KIWAY which registers it).
struct PGM_SINGLE_TOP
{
    static void RegisterKiface( KIWAY& aKiway )
    {
        aKiway.set_kiface( KIWAY::KifaceType( FRAME_PCB_EDITOR ), &Kiface() );
    }
};


namespace
{

using namespace QA_QT_FRAMES;

/**
 * The layout of the docks of a frame is kept with its settings: the properties of the footprint
 * editor moved to the other side of the frame are there when the editor is opened again.
 */
/**
 * A zone of the zone manager dragged on another one: their priorities are swapped (the drop of
 * the data view, the handlers of the dialog).  The drag is the event that starts it, the drop the
 * events of Qt on the view (QDrag::exec() would wait for the mouse).
 */
void testZonePriorityDrop( QDialog* aDialog )
{
    std::printf( "ZONE PRIORITY DRAG AND DROP\n" );

    wxDataViewCtrl* view = nullptr;

    for( QTreeView* tree : aDialog->findChildren<QTreeView*>() )
    {
        wxDataViewCtrl* candidate = dynamic_cast<wxDataViewCtrl*>( tree );

        if( candidate && candidate->model() && candidate->model()->rowCount() >= 2
            && candidate->dragDropMode() == QAbstractItemView::DragDrop )
        {
            view = candidate;
            break;
        }
    }

    check( view != nullptr, "the zones of the zone manager can be dragged and dropped" );

    if( !view )
    {
        for( QTreeView* tree : aDialog->findChildren<QTreeView*>() )
        {
            std::printf( "      view %s: data view %d, rows %d, drag and drop mode %d\n",
                         tree->metaObject()->className(),
                         dynamic_cast<wxDataViewCtrl*>( tree ) != nullptr,
                         tree->model() ? tree->model()->rowCount() : -1,
                         int( tree->dragDropMode() ) );
        }
    }

    wxWindow* dialog = dynamic_cast<wxWindow*>( aDialog );

    if( !view || !dialog )
        return;

    QAbstractItemModel* model = view->model();

    auto rowText = [&]( int aRow )
    {
        QStringList texts;

        for( int col = 0; col < model->columnCount(); ++col )
            texts << model->index( aRow, col ).data().toString();

        return texts.join( QLatin1Char( '|' ) );
    };

    QString first = rowText( 0 );
    QString second = rowText( 1 );

    // The drag of the first zone
    wxDataViewEvent begin( wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, view,
                           view->ItemCompat( model->index( 0, 0 ) ) );
    dialog->ProcessEvent( begin );
    std::unique_ptr<wxDataObject> data( begin.GetDataObject() );
    check( data != nullptr, "the zone manager gives the data of the zone dragged" );

    // Over the second one, then dropped
    QMimeData mime;
    mime.setText( QStringLiteral( "42" ) );

    QPoint          pos = view->visualRect( model->index( 1, 0 ) ).center();
    Qt::DropActions actions = Qt::CopyAction | Qt::MoveAction;

    QDragEnterEvent enter( pos, actions, &mime, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( view->viewport(), &enter );

    QDragMoveEvent move( pos, actions, &mime, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( view->viewport(), &move );
    check( enter.isAccepted() && move.isAccepted() && move.dropAction() == Qt::MoveAction,
           "the zone manager takes a zone dragged on another one (a move)" );

    QDropEvent drop( QPointF( pos ), actions, &mime, Qt::LeftButton, Qt::NoModifier );
    QApplication::sendEvent( view->viewport(), &drop );
    settle( 200 );

    std::printf( "      rows: '%s' '%s' -> '%s' '%s'\n", first.toStdString().c_str(),
                 second.toStdString().c_str(), rowText( 0 ).toStdString().c_str(),
                 rowText( 1 ).toStdString().c_str() );
    check( drop.isAccepted() && rowText( 0 ) == second && rowText( 1 ) == first,
           "a zone dropped on another one swaps their priorities" );
}


void testDockLayout( KIWAY& aKiway )
{
    std::printf( "DOCK LAYOUT\n" );

    const QString name = QStringLiteral( "PropertiesManager" );
    auto          propertiesOf = [&]( QMainWindow* aFrame )
                                 {
                                     return aFrame->findChild<QDockWidget*>(
                                             name, Qt::FindDirectChildrenOnly );
                                 };

    auto* frame = createFrame<FOOTPRINT_EDIT_FRAME>( aKiway, FRAME_FOOTPRINT_EDITOR );

    if( !frame )
        return;

    frame->show();
    settle( 300 );

    QDockWidget* dock = propertiesOf( frame );
    check( dock != nullptr, "the footprint editor has its properties pane" );

    if( !dock )
    {
        closeFrame( frame, "the footprint editor" );
        return;
    }

    Qt::DockWidgetArea original = frame->dockWidgetArea( dock );
    Qt::DockWidgetArea moved = original == Qt::RightDockWidgetArea ? Qt::LeftDockWidgetArea
                                                                    : Qt::RightDockWidgetArea;
    frame->addDockWidget( moved, dock );
    settle( 100 );
    check( frame->dockWidgetArea( dock ) == moved, "the properties pane is moved" );

    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the footprint editor" );

    // Opened again: the properties are where they were left, then put back
    frame = createFrame<FOOTPRINT_EDIT_FRAME>( aKiway, FRAME_FOOTPRINT_EDITOR );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    dock = propertiesOf( frame );

    check( dock && frame->dockWidgetArea( dock ) == moved,
           "the footprint editor opens with the properties pane where it was left" );

    if( dock )
        frame->addDockWidget( original, dock );

    settle( 100 );
    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the footprint editor" );
}


/**
 * The footprint editor: a footprint of the board is edited, with the dialogs of the editor (the
 * properties of the footprint and of a pad, the default pad, the checker, the cleanup of the
 * graphics, the enumeration of the pads).
 */
void testFootprintEditor( KIWAY& aKiway, FOOTPRINT* aFootprint, const QString& aScreenshot )
{
    std::printf( "FOOTPRINT_EDIT_FRAME\n" );

    FOOTPRINT_EDIT_FRAME* frame = createFrame<FOOTPRINT_EDIT_FRAME>( aKiway,
                                                                     FRAME_FOOTPRINT_EDITOR );
    check( frame != nullptr, "the KIFACE makes the footprint editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the footprint editor is shown" );
    check( frame->GetBoard() != nullptr, "the footprint editor has its board" );

    if( aFootprint && frame->LoadFootprintFromBoard( aFootprint ) )
    {
        settle( 300 );

        BOARD*        board = frame->GetBoard();
        TOOL_MANAGER* toolMgr = frame->GetToolManager();
        FOOTPRINT*    footprint = board->GetFirstFootprint();

        check( footprint && footprint->GetFPID() == aFootprint->GetFPID(),
               "the footprint editor edits the footprint of the board" );

        if( footprint )
        {
            PCB_SELECTION_TOOL* selTool = toolMgr->GetTool<PCB_SELECTION_TOOL>();

            testDialog( toolMgr, PCB_ACTIONS::footprintProperties, "footprint editor properties",
                        aScreenshot, "Cancel" );

            if( selTool && !footprint->Pads().empty() )
            {
                toolMgr->RunAction( ACTIONS::selectionClear );
                selTool->AddItemToSel( footprint->Pads().front() );
                settle( 50 );
                testDialog( toolMgr, PCB_ACTIONS::properties, "footprint editor pad properties",
                            aScreenshot, "Cancel" );
                toolMgr->RunAction( ACTIONS::selectionClear );
            }

            testDialog( toolMgr, PCB_ACTIONS::checkFootprint, "footprint checker", aScreenshot,
                        "Close" );
            testDialog( toolMgr, PCB_ACTIONS::cleanupGraphics, "cleanup graphics", aScreenshot,
                        "Cancel" );
            testDialog( toolMgr, PCB_ACTIONS::enumeratePads, "enumerate pads", aScreenshot,
                        "Cancel" );
        }

        if( !aScreenshot.isEmpty() )
        {
            QString path = aScreenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-footprint-editor.png" ) );
            frame->screen()->grabWindow( frame->winId() ).save( path );
        }
    }
    else
    {
        check( false, "the footprint editor loads the footprint of the board" );
    }

    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the footprint editor" );
}


void testFootprintWizard( KIWAY& aKiway )
{
    std::printf( "FOOTPRINT_WIZARD_FRAME\n" );

    // The list of the wizards is shown when the frame is activated, and by its tool: the first
    // wizard (the QFN one, the only one of the plugin folder of the test) is chosen
    int    listsShown = 0;
    QTimer answer;

    QObject::connect( &answer, &QTimer::timeout,
                      [&]()
                      {
                          auto* list = dynamic_cast<DIALOG_FOOTPRINT_WIZARD_LIST*>(
                                  QApplication::activeModalWidget() );

                          if( list )
                          {
                              ++listsShown;
                              list->EndModal( wxID_OK );
                          }
                      } );
    answer.start( 50 );

    auto* frame = createFrame<FOOTPRINT_WIZARD_FRAME>( aKiway, FRAME_FOOTPRINT_WIZARD );
    check( frame != nullptr, "the KIFACE makes the footprint wizard" );

    if( !frame )
        return;

    // A modal frame, as the footprint editor uses it: the built footprint is exported to the
    // caller, which deletes the frame
    bool shown = false;
    bool built = false;

    // When the list of the wizards is answered and closed (its modal loop ended: on macOS a
    // nested modal session ends late) and the footprint is built, or after 5 s
    QTimer        poll;
    QElapsedTimer waited;
    bool          selected = false;

    QObject::connect( &poll, &QTimer::timeout,
                      [&]()
                      {
                          bool listOpen = dynamic_cast<DIALOG_FOOTPRINT_WIZARD_LIST*>(
                                                  QApplication::activeModalWidget() ) != nullptr;

                          if( ( waited.elapsed() < 400 || listOpen ) && waited.elapsed() < 5000 )
                              return;

                          shown = frame->isVisible();

                          if( listsShown == 0 && !selected )
                          {
                              selected = true;
                              wxCommandEvent dummy;
                              frame->SelectCurrentWizard( dummy );
                          }

                          BOARD*     board = frame->GetBoard();
                          FOOTPRINT* fp = board ? board->GetFirstFootprint() : nullptr;
                          built = fp && fp->Pads().size() > 4;

                          if( !built && waited.elapsed() < 5000 )
                              return;

                          poll.stop();
                          wxCommandEvent dummy;
                          frame->ExportSelectedFootprint( dummy );
                      } );
    waited.start();
    poll.start( 50 );

    wxString result;
    bool     exported = frame->ShowModal( &result, nullptr );
    answer.stop();
    poll.stop();

    check( shown, "the footprint wizard is shown" );

    // The wizards are API plugins since the Python scripting went (KiCad 11): without a plugin
    // runtime on the runner there may be no wizard to build, so these are reported, not checked
    std::printf( "  info: lists of the wizards shown: %d, footprint built: %s, exported: %s\n",
                 listsShown, built ? "yes" : "no", exported ? "yes" : "no" );

    std::unique_ptr<FOOTPRINT> footprint( exported ? frame->GetBuiltFootprint() : nullptr );

    QPointer<QWidget> alive( frame );
    frame->Destroy();
    settle( 100 );
    QCoreApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );
    check( alive.isNull(), "the footprint wizard is deleted" );
}


/// The pixels of the screen where @a aWidget is: what the user sees there, the native OpenGL
/// windows included (the grab of the widgets has none).
QImage grabScreenArea( QWidget* aWidget )
{
    QScreen* screen = aWidget->screen();

    if( !screen )
        return QImage();

    QPoint origin = aWidget->mapToGlobal( QPoint( 0, 0 ) ) - screen->geometry().topLeft();

    return screen->grabWindow( 0, origin.x(), origin.y(), aWidget->width(), aWidget->height() )
            .toImage();
}


/// The part of the image that is black (a native window that draws nothing).
double blackFraction( const QImage& aImage )
{
    int samples = 0;
    int black = 0;

    for( int y = 0; y < aImage.height(); y += 4 )
    {
        for( int x = 0; x < aImage.width(); x += 4 )
        {
            QRgb pixel = aImage.pixel( x, y );
            samples++;

            if( qRed( pixel ) < 8 && qGreen( pixel ) < 8 && qBlue( pixel ) < 8 )
                black++;
        }
    }

    return samples ? double( black ) / samples : 1.0;
}


/// The part of two images of the same size that differs.
double differentFraction( const QImage& aA, const QImage& aB )
{
    if( aA.size() != aB.size() )
        return 1.0;

    int samples = 0;
    int different = 0;

    for( int y = 0; y < aA.height(); y += 4 )
    {
        for( int x = 0; x < aA.width(); x += 4 )
        {
            QRgb a = aA.pixel( x, y );
            QRgb b = aB.pixel( x, y );
            samples++;

            if( std::abs( qRed( a ) - qRed( b ) ) > 48 || std::abs( qGreen( a ) - qGreen( b ) ) > 48
                    || std::abs( qBlue( a ) - qBlue( b ) ) > 48 )
            {
                different++;
            }
        }
    }

    return samples ? double( different ) / samples : 1.0;
}


/// The image has more than one color.
bool isVaried( const QImage& aImage )
{
    for( int y = 0; y < aImage.height(); y += 7 )
    {
        for( int x = 0; x < aImage.width(); x += 7 )
        {
            if( aImage.pixel( x, y ) != aImage.pixel( 0, 0 ) )
                return true;
        }
    }

    return false;
}


/// The visible OpenGL windows of the program (the canvases), listed.  @return their number.
int listGlWindows( const char* aWhen )
{
    int count = 0;

    std::printf( "    OpenGL windows %s:\n", aWhen );

    for( QWindow* window : QGuiApplication::allWindows() )
    {
        if( !window->isVisible() || window->surfaceType() != QSurface::OpenGLSurface )
            continue;

        QPoint pos = window->mapToGlobal( QPoint( 0, 0 ) );
        std::printf( "      %p %d,%d %dx%d exposed %d parent %p\n", static_cast<void*>( window ),
                     pos.x(), pos.y(), window->width(), window->height(),
                     window->isExposed() ? 1 : 0, static_cast<void*>( window->parent() ) );
        count++;
    }

#ifdef _WIN32
    // The native windows of the program, as Windows sees them (one left by Qt is not listed above)
    std::printf( "    native windows %s:\n", aWhen );

    EnumWindows(
            []( HWND aWindow, LPARAM ) -> BOOL
            {
                DWORD pid = 0;
                GetWindowThreadProcessId( aWindow, &pid );

                if( pid != GetCurrentProcessId() || !IsWindowVisible( aWindow ) )
                    return TRUE;

                auto listOne =
                        []( HWND aChild, LPARAM aDepth ) -> BOOL
                        {
                            if( !IsWindowVisible( aChild ) )
                                return TRUE;

                            char className[128] = {};
                            RECT rect = {};
                            GetClassNameA( aChild, className, sizeof( className ) );
                            GetWindowRect( aChild, &rect );
                            std::printf( "        %p %s %ld,%ld %ldx%ld\n",
                                         static_cast<void*>( aChild ), className, rect.left,
                                         rect.top, rect.right - rect.left,
                                         rect.bottom - rect.top );
                            return TRUE;
                        };

                char className[128] = {};
                RECT rect = {};
                GetClassNameA( aWindow, className, sizeof( className ) );
                GetWindowRect( aWindow, &rect );
                std::printf( "      %p %s %ld,%ld %ldx%ld\n", static_cast<void*>( aWindow ),
                             className, rect.left, rect.top, rect.right - rect.left,
                             rect.bottom - rect.top );
                EnumChildWindows( aWindow, listOne, 0 );
                return TRUE;
            },
            0 );
#endif

    return count;
}


/// The widgets of @a aFrame (not its windows) with a native window of their own.
int nativeWidgets( QWidget* aFrame )
{
    int count = 0;

    for( QWidget* widget : aFrame->findChildren<QWidget*>() )
    {
        if( !widget->isWindow() && widget->internalWinId() )
            count++;
    }

    return count;
}


/**
 * The canvas of the PCB editor after the 3D viewer: the viewer is opened on the editor, then
 * closed, and the canvas shows the board again as before (a native window of the 3D viewer, or
 * a context of OpenGL, must not be left over it).
 */
void test3DViewerRoundTrip( PCB_EDIT_FRAME* aFrame, const QString& aScreenshot )
{
    std::printf( "3D VIEWER AND BACK\n" );

    EDA_DRAW_PANEL_GAL* canvas = aFrame->GetCanvas();
    TOOL_MANAGER*       toolMgr = aFrame->GetToolManager();

    auto saveShot =
            [&]( const QImage& aImage, const char* aSuffix )
            {
                if( aScreenshot.isEmpty() || aImage.isNull() )
                    return;

                QString path = aScreenshot;
                path.replace( QStringLiteral( ".png" ),
                              QStringLiteral( "-3d-%1.png" ).arg( QString::fromUtf8( aSuffix ) ) );
                aImage.save( path );
            };

    auto screenShot =
            [&]( const char* aSuffix )
            {
                if( !aScreenshot.isEmpty() )
                {
                    if( QScreen* screen = aFrame->screen() )
                        saveShot( screen->grabWindow( 0 ).toImage(), aSuffix );
                }
            };

    aFrame->raise();
    aFrame->activateWindow();
    canvas->ForceRefresh();
    settle( 300 );

    QImage before = grabScreenArea( canvas );
    QRect  canvasBefore( canvas->mapToGlobal( QPoint() ), canvas->size() );
    bool   screenGrabs = !before.isNull() && isVaried( before );
    int    glWindowsBefore = listGlWindows( "before the 3D viewer" );
    int    nativeBefore = nativeWidgets( aFrame );

    std::printf( "    canvas %d,%d %dx%d, black before %.3f\n", canvas->mapToGlobal( QPoint() ).x(),
                 canvas->mapToGlobal( QPoint() ).y(), canvas->width(), canvas->height(),
                 screenGrabs ? blackFraction( before ) : -1.0 );
    saveShot( before, "canvas-before" );
    screenShot( "screen-before" );

    // The viewer, as by its button, over the editor
    toolMgr->RunAction( ACTIONS::show3DViewer );
    settle( 300 );

    QPointer<KIWAY_PLAYER> viewer;

    for( QWidget* widget : QApplication::topLevelWidgets() )
    {
        auto* player = dynamic_cast<KIWAY_PLAYER*>( widget );

        if( player && player->isVisible() && player->GetFrameType() == FRAME_PCB_DISPLAY3D )
            viewer = player;
    }

    check( !viewer.isNull(), "the 3D viewer is shown" );

    if( !viewer )
        return;

    viewer->setGeometry( aFrame->geometry() );
    settle( 3000 );
    listGlWindows( "with the 3D viewer" );
    screenShot( "screen-viewer" );

    // Closed as by its close button
    viewer->close();
    settle( 300 );
    QCoreApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );
    settle( 300 );
    check( viewer.isNull(), "the 3D viewer is closed and deleted" );

    aFrame->raise();
    aFrame->activateWindow();
    settle( 300 );
    canvas->ForceRefresh();
    settle( 500 );

    int glWindowsAfter = listGlWindows( "after the 3D viewer" );
    check( glWindowsAfter == glWindowsBefore, "no OpenGL window of the 3D viewer is left" );

    // The widgets of the editor stay without a native window: native ones (made by a winId() of
    // the deleted viewer, on Windows) cover the OpenGL window of the canvas
    int nativeAfter = nativeWidgets( aFrame );
    std::printf( "    native widgets of the editor: %d before, %d after\n", nativeBefore,
                 nativeAfter );
    check( nativeAfter == nativeBefore, "the widgets of the editor do not become native windows" );

    // The OpenGL window of the canvas still covers it
    bool covered = false;

    for( QWindow* window : QGuiApplication::allWindows() )
    {
        if( window->isVisible() && window->surfaceType() == QSurface::OpenGLSurface
                && window->mapToGlobal( QPoint( 0, 0 ) ) == canvas->mapToGlobal( QPoint( 0, 0 ) )
                && window->size() == canvas->size() )
        {
            covered = true;
        }
    }

    check( covered, "the OpenGL window of the canvas covers it" );

    // The canvas draws the board (in its buffers)
    QImage galImage;
    bool   galVaried = false;

    for( int attempt = 0; attempt < 10 && !galVaried; ++attempt )
    {
        canvas->ForceRefresh();
        settle( 100 );
        galVaried = canvas->GetScreenshot( galImage ) && isVaried( galImage );
    }

    check( galVaried, "the canvas draws the board after the 3D viewer" );
    saveShot( galImage, "gal-after" );

    // And the screen shows it, as before the viewer
    QImage after = grabScreenArea( canvas );
    saveShot( after, "canvas-after" );
    screenShot( "screen-after" );

    if( screenGrabs )
    {
        double black = blackFraction( after );
        double different = differentFraction( before, after );

        std::printf( "    black after %.3f, different %.3f\n", black, different );
        check( black < blackFraction( before ) + 0.1, "the canvas is not black after the 3D viewer" );
        QRect canvasAfter( canvas->mapToGlobal( QPoint() ), canvas->size() );

        // The layout of the frame can change meanwhile (a bar above the canvas is hidden, as
        // seen on macOS): the view of the board is then moved and the grabs do not compare
        if( canvasAfter == canvasBefore )
        {
            check( different < 0.25, "the canvas shows the board as before the 3D viewer" );
        }
        else
        {
            std::printf( "    the canvas went from %d,%d %dx%d to %d,%d %dx%d: the screen is not "
                         "compared\n",
                         canvasBefore.x(), canvasBefore.y(), canvasBefore.width(),
                         canvasBefore.height(), canvasAfter.x(), canvasAfter.y(),
                         canvasAfter.width(), canvasAfter.height() );
        }
    }
    else
    {
        std::printf( "    (no grab of the screen: the screen is not compared)\n" );
    }
}


/**
 * The editor of the custom rules (board setup): a styled text control with the numbers of its
 * lines in its margin (margin 0 of Scintilla, of the type wxSTC_MARGIN_NUMBER).
 */
void testRulesEditor( QDialog* aDialog, const QString& aScreenshot )
{
    PAGED_DIALOG* dialog = dynamic_cast<PAGED_DIALOG*>( aDialog );

    if( !dialog )
        return;

    WX_TREEBOOK*      book = dialog->GetTreebook();
    wxStyledTextCtrl* editor = nullptr;

    // The page of the rules (the pages are made when shown)
    for( size_t i = 0; i < book->GetPageCount() && !editor; ++i )
    {
        book->ResolvePage( i );
        book->SetSelection( i );
        settle( 50 );

        for( QPlainTextEdit* text : aDialog->findChildren<QPlainTextEdit*>() )
        {
            wxStyledTextCtrl* stc = dynamic_cast<wxStyledTextCtrl*>( text );

            if( stc && stc->isVisible() && stc->GetMarginType( 0 ) == wxSTC_MARGIN_NUMBER
                && stc->GetMarginWidth( 0 ) > 0 )
            {
                editor = stc;
            }
        }
    }

    if( !editor )
    {
        for( QPlainTextEdit* text : aDialog->findChildren<QPlainTextEdit*>() )
        {
            wxStyledTextCtrl* stc = dynamic_cast<wxStyledTextCtrl*>( text );
            std::printf( "    text editor %s: styled %d, visible %d, margin 0 type %d width %d\n",
                         text->objectName().toUtf8().constData(), stc != nullptr,
                         text->isVisible(), stc ? stc->GetMarginType( 0 ) : -1,
                         stc ? stc->GetMarginWidth( 0 ) : -1 );
        }
    }

    check( editor != nullptr, "the custom rules editor has a margin of line numbers" );

    if( !editor )
        return;

    editor->SetText( wxS( "(version 1)\n(rule \"QA\"\n    (constraint clearance (min 1mm))\n"
                          "    (condition \"A.NetClass == 'Default'\"))\n" ) );
    settle( 100 );

    QWidget* margins = editor->GetMarginsWidget();
    QImage   image = margins ? margins->grab().toImage() : QImage();
    int      inked = 0;

    for( int y = 0; y < image.height(); ++y )
    {
        for( int x = 0; x < image.width(); ++x )
            inked += image.pixel( x, y ) != image.pixel( 0, 0 );
    }

    std::printf( "    margins %d px (margin 0: %d px), viewport at %d, %d pixels of numbers\n",
                 editor->GetMarginsWidth(), editor->GetMarginWidth( 0 ),
                 editor->viewport()->geometry().left(), inked );

    check( margins && margins->isVisible() && margins->width() == editor->GetMarginsWidth(),
           "the margin of the line numbers is shown" );
    check( editor->viewport()->geometry().left() >= editor->GetMarginsWidth(),
           "the text is on the right of the margin" );
    check( inked > 20, "the margin shows the numbers of the lines" );

    // The colours of the syntax: the keywords, the strings, the numbers, the parentheses
    std::set<QRgb> colours;

    for( QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next() )
    {
        for( const QTextLayout::FormatRange& range : block.layout()->formats() )
        {
            if( range.format.hasProperty( QTextFormat::ForegroundBrush ) )
                colours.insert( range.format.foreground().color().rgb() );
        }
    }

    std::printf( "    lexer %d, %zu colours of the syntax\n", editor->GetLexer(), colours.size() );
    check( editor->GetLexer() == wxSTC_LEX_LISP && colours.size() >= 4,
           "the rules editor colours the syntax" );

    if( !aScreenshot.isEmpty() )
    {
        QString path = aScreenshot;
        path.replace( QStringLiteral( ".png" ), QStringLiteral( "-custom-rules.png" ) );
        aDialog->grab().save( path );
    }

    book->SetSelection( 0 );
    settle( 30 );
}


void testPcbEditor( KIWAY& aKiway, const wxString& aBoardFile, const QString& aScreenshot )
{
    std::printf( "PCB_EDIT_FRAME\n" );

    PCB_EDIT_FRAME* frame = createFrame<PCB_EDIT_FRAME>( aKiway, FRAME_PCB_EDITOR );
    check( frame != nullptr, "the KIFACE makes the PCB editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the PCB editor is shown" );
    check( frame->GetCanvas() != nullptr, "the PCB editor has a canvas" );
    check( frame->GetToolManager() != nullptr, "the PCB editor has its tools" );

    check( frame->OpenProjectFiles( { aBoardFile } ), "the PCB editor opens the board" );
    settle( 300 );

    BOARD* board = frame->GetBoard();
    check( board && !board->Footprints().empty(), "the board has its footprints" );
    check( board && !board->Tracks().empty(), "the board has its tracks" );

    if( TOOL_MANAGER* toolMgr = frame->GetToolManager() )
    {
        toolMgr->RunAction( ACTIONS::zoomFitScreen );
        settle( 100 );
        toolMgr->RunAction( ACTIONS::zoomInCenter );
        settle( 100 );
        check( true, "the zoom actions run" );

        // The canvas draws the board (with the GAL of the settings: the OpenGL one draws in
        // its own window, which is not in the grab of the frame)
        QImage image;
        bool   varied = false;

        for( int attempt = 0; attempt < 20 && !varied; ++attempt )
        {
            frame->GetCanvas()->ForceRefresh();
            settle( 100 );

            if( !frame->GetCanvas()->GetScreenshot( image ) || image.width() < 100 )
                continue;

            for( int y = 0; y < image.height() && !varied; y += 7 )
            {
                for( int x = 0; x < image.width() && !varied; x += 7 )
                    varied = image.pixel( x, y ) != image.pixel( 0, 0 );
            }
        }

        check( varied, "the canvas draws the board" );
        checkCanvasGeometry( frame->GetCanvas(), "PCB editor" );

        test3DViewerRoundTrip( frame, aScreenshot );

        if( !aScreenshot.isEmpty() )
        {
            QString path = aScreenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-canvas.png" ) );
            check( image.save( path ), "screenshot of the canvas" );
        }
    }

    // The tools of the editor on the board
    if( TOOL_MANAGER* toolMgr = frame->GetToolManager(); toolMgr && board )
    {
        PCB_SELECTION_TOOL* selTool = toolMgr->GetTool<PCB_SELECTION_TOOL>();
        size_t              footprints = board->Footprints().size();
        size_t              tracks = board->Tracks().size();

        toolMgr->RunAction( ACTIONS::selectAll );
        settle( 100 );
        check( selTool && selTool->GetSelection().Size() > 0, "select all" );

        toolMgr->RunAction( ACTIONS::doDelete );
        settle( 100 );
        check( board->Footprints().empty() && board->Tracks().empty(), "delete the selection" );

        toolMgr->RunAction( ACTIONS::undo );
        settle( 100 );
        check( board->Footprints().size() == footprints && board->Tracks().size() == tracks,
               "undo the deletion" );

        toolMgr->RunAction( ACTIONS::selectionClear );
        settle( 50 );

        toolMgr->RunAction( PCB_ACTIONS::zoneFillAll );
        settle( 300 );

        bool filled = !board->Zones().empty();

        for( ZONE* zone : board->Zones() )
        {
            if( !zone->GetIsRuleArea() && !zone->IsFilled() )
                filled = false;
        }

        check( filled, "fill the zones" );

        // The dialogs of the editor: board setup (quasi-modal), the properties of a footprint
        // (quasi-modal), DRC (not modal)
        // The footprint chooser (a modal frame): shown, then closed as by its close button
        {
            bool shown = false;
            QTimer poll;

            QObject::connect( &poll, &QTimer::timeout,
                              [&]()
                              {
                                  for( QWidget* widget : QApplication::topLevelWidgets() )
                                  {
                                      auto* chooser = dynamic_cast<KIWAY_PLAYER*>( widget );

                                      if( chooser && chooser->isVisible()
                                              && chooser->GetFrameType() == FRAME_FOOTPRINT_CHOOSER )
                                      {
                                          poll.stop();
                                          shown = true;
                                          settle( 1000 );
                                          chooser->Close();
                                          return;
                                      }
                                  }
                              } );
            poll.start( 50 );
            toolMgr->RunAction( PCB_ACTIONS::placeFootprint );
            settle( 300 );
            toolMgr->RunAction( ACTIONS::cancelInteractive );
            settle( 300 );
            check( shown, "the footprint chooser is shown and closed" );
        }

        testDialog( toolMgr, PCB_ACTIONS::boardSetup, "board setup", aScreenshot, "Cancel",
                    [&]( QDialog* aDialog )
                    {
                        testRulesEditor( aDialog, aScreenshot );
                    } );

        // The properties of an item: selected, then its dialog
        auto testProperties =
                [&]( BOARD_ITEM* aItem, const char* aName )
                {
                    if( !aItem )
                        return;

                    toolMgr->RunAction( ACTIONS::selectionClear );
                    selTool->AddItemToSel( aItem );
                    settle( 50 );
                    testDialog( toolMgr, PCB_ACTIONS::properties, aName, aScreenshot, "Cancel" );
                    toolMgr->RunAction( ACTIONS::selectionClear );
                };

        auto firstTrack =
                [&]( KICAD_T aType ) -> BOARD_ITEM*
                {
                    for( PCB_TRACK* track : board->Tracks() )
                    {
                        if( track->Type() == aType )
                            return track;
                    }

                    return nullptr;
                };

        if( !board->Footprints().empty() )
        {
            FOOTPRINT* footprint = board->Footprints().front();
            testProperties( footprint, "footprint properties" );

            if( !footprint->Pads().empty() )
                testProperties( footprint->Pads().front(), "pad properties" );
        }

        testProperties( firstTrack( PCB_TRACE_T ), "track properties" );
        testProperties( firstTrack( PCB_VIA_T ), "via properties" );

        if( !board->Zones().empty() )
            testProperties( board->Zones().front(), "zone properties" );

        for( BOARD_ITEM* item : board->Drawings() )
        {
            if( item->Type() == PCB_TEXT_T )
            {
                testProperties( item, "text properties" );
                break;
            }
        }

        testDialog( toolMgr, ACTIONS::plot, "plot", aScreenshot, "Close" );

        // The other dialogs of the menus: preferences, libraries, edits, fabrication outputs
        // Preferences: every page is shown (the pages of the KIFACEs are made when shown)
        int prefPages = 0;

        testDialog( toolMgr, ACTIONS::openPreferences, "preferences", aScreenshot, "Cancel",
                    [&]( QDialog* aDialog )
                    {
                        PAGED_DIALOG* dialog = dynamic_cast<PAGED_DIALOG*>( aDialog );

                        if( !dialog )
                            return;

                        WX_TREEBOOK* book = dialog->GetTreebook();

                        for( size_t i = 0; i < book->GetPageCount(); ++i )
                        {
                            book->SetSelection( i );
                            settle( 30 );

                            if( book->GetCurrentPage() )
                                prefPages++;

                            if( !aScreenshot.isEmpty() )
                            {
                                QString path = aScreenshot;
                                path.replace( QStringLiteral( ".png" ),
                                              QStringLiteral( "-preferences-%1.png" ).arg( i ) );
                                aDialog->grab().save( path );
                            }
                        }

                        book->SetSelection( 0 );
                        settle( 30 );
                    } );

        check( prefPages >= 25, "the preferences show their pages" );
        testDialog( toolMgr, ACTIONS::configurePaths, "paths", aScreenshot, "Cancel" );
        testDialog( toolMgr, ACTIONS::showFootprintLibTable, "footprint libraries", aScreenshot,
                    "Cancel" );
        testDialog( toolMgr, PCB_ACTIONS::editTracksAndVias, "edit tracks and vias", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::editTextAndGraphics, "edit text and graphics",
                    aScreenshot, "Close" );
        testDialog( toolMgr, PCB_ACTIONS::globalDeletions, "global deletions", aScreenshot,
                    "Cancel" );
        testDialog( toolMgr, PCB_ACTIONS::cleanupTracksAndVias, "cleanup tracks and vias",
                    aScreenshot, "Cancel" );
        // A second copper zone (the board has one), whose priority is swapped by a drag
        ZONE* qaZone = nullptr;
        unsigned maxPriority = 0;

        for( ZONE* zone : frame->GetBoard()->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            maxPriority = std::max( maxPriority, zone->GetAssignedPriority() );

            if( !qaZone )
                qaZone = static_cast<ZONE*>( zone->Clone() );
        }

        if( qaZone )
        {
            qaZone->ResetUuidDirect();
            qaZone->SetAssignedPriority( maxPriority + 1 );
            qaZone->SetZoneName( wxS( "QA zone" ) );
            frame->GetBoard()->Add( qaZone );
        }

        testDialog( toolMgr, PCB_ACTIONS::zonesManager, "zone manager", aScreenshot, "Cancel",
                    []( QDialog* aDialog )
                    {
                        testZonePriorityDrop( aDialog );
                    } );

        if( qaZone )
        {
            // The zone manager selects the zone of its current row
            toolMgr->RunAction( ACTIONS::selectionClear );
            frame->GetCanvas()->GetView()->Remove( qaZone );
            frame->GetBoard()->Remove( qaZone );
            delete qaZone;
        }

        testDialog( toolMgr, PCB_ACTIONS::boardStatistics, "board statistics", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::generateDrillFiles, "drill files", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::generatePosFile, "position files", aScreenshot,
                    "Close" );
        // IPC-2581: the board is exported (next to the board, by default)
        wxFileName ipcFile( board->GetFileName() );
        ipcFile.SetExt( wxS( "xml" ) );

        testDialog( toolMgr, PCB_ACTIONS::generateIPC2581File, "IPC-2581", aScreenshot, "Close",
                    [&]( QDialog* aDialog )
                    {
                        pressButton( aDialog, QStringLiteral( "Export" ) );
                        settle( 1000 );
                    } );

        check( ipcFile.FileExists(), "the IPC-2581 file is exported" );

        // STEP: without the warning about the scaled models of the board, shown before
        Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning = true;
        testDialog( toolMgr, PCB_ACTIONS::exportSTEP, "STEP export", aScreenshot, "Close" );

        // DRC: run, then closed
        bool drcRun = false;

        testDialog( toolMgr, PCB_ACTIONS::runDRC, "DRC", aScreenshot, "Close",
                    [&]( QDialog* aDialog )
                    {
                        drcRun = pressButton( aDialog, QStringLiteral( "Run DRC" ) );

                        // The DRC runs in the dialog (its progress reporter), then shows the
                        // violations
                        settle( 1500 );
                    } );

        check( drcRun, "run DRC" );
        check( !board->Markers().empty(), "the DRC finds the violations of the board" );
    }

    if( !aScreenshot.isEmpty() )
    {
        // From the display: the grab of the widgets has no OpenGL window (a native window)
        QPixmap shot = frame->screen()->grabWindow( frame->winId() );
        check( shot.save( aScreenshot ), "screenshot of the PCB editor" );

        QString path = aScreenshot;
        path.replace( QStringLiteral( ".png" ), QStringLiteral( "-grab.png" ) );
        frame->grab().save( path );
    }

    testFootprintWizard( aKiway );

    // The footprint editor, with a footprint of the board
    testFootprintEditor( aKiway, board->Footprints().empty() ? nullptr : board->Footprints().front(),
                         aScreenshot );
    testDockLayout( aKiway );

    // The changes of the test are not saved (the editor would ask)
    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the PCB editor" );
}


} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> [screenshot.png]\n", argv[0] );
        return 2;
    }

    // The results show up even if the test crashes (unbuffered: with MSVC _IOLBF is a full
    // buffer and needs its size, 0 is an invalid parameter that ends the program)
    std::setvbuf( stdout, nullptr, _IONBF, 0 );
    QA_QT_CRASH::install();

    // The settings of the test, not the ones of the user (the test changes them)
    QTemporaryDir configHome;

    if( !qEnvironmentVariableIsSet( "KICAD_CONFIG_HOME" ) )
        qputenv( "KICAD_CONFIG_HOME", configHome.path().toUtf8() );

    // The documents of the test (not the ones of the user)
    QTemporaryDir documentsHome;

    if( !qEnvironmentVariableIsSet( "KICAD_DOCUMENTS_HOME" ) )
        qputenv( "KICAD_DOCUMENTS_HOME", documentsHome.path().toUtf8() );

    // The application object of a program with a user interface (Pgm().IsGUI())
    KIQT_APP      kiqtApp;
    wxInitializer wxBase( argc, argv );

    // The GUI of the program (PGM_GUI::InitPgm configures it)
    QApplication app( argc, argv );

    PGM_QT_TEST program;
    SetPgm( &program );

    if( !program.InitPgm( true ) )
    {
        std::fprintf( stderr, "cannot initialise the program\n" );
        return 2;
    }

    program.StartApiServer();

    KIWAY kiway( KFCTL_STANDALONE );
    program.GetSettingsManager().SetKiway( &kiway );

    // The settings of the project manager, as single_top (the start wizard uses them)
    program.GetSettingsManager().RegisterSettings( new KICAD_SETTINGS );
    Kiface().OnKifaceStart( &program, KFCTL_STANDALONE, &kiway );
    PGM_SINGLE_TOP::RegisterKiface( kiway );

    // The board is opened from a copy (the editor writes the project file next to it)
    QTemporaryDir dir;
    QString       copy = dir.filePath( QFileInfo( QString::fromUtf8( argv[1] ) ).fileName() );
    check( QFile::copy( QString::fromUtf8( argv[1] ), copy ), "copy the board" );

    testStartWizard( argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );
    program.GetLibraryManager().LoadGlobalTables();

    testPcbEditor( kiway, wxString::FromUTF8( copy.toUtf8().constData() ),
                   argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
