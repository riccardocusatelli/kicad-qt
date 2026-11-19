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
 * @file test_eeschema_frames.cpp
 * The test of the frames of eeschema on Qt: the KIFACE with the GUI (eeschema/eeschema.cpp and
 * eeschema_gui_qt) makes the original frames (the schematic editor, the symbol editor, the symbol
 * viewer), which open a schematic and run their tools and dialogs.
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)
#include <cstdio>
#include <string>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QTemporaryDir>
#include <QTimer>
#include <QComboBox>
#include <QRadioButton>
#include <QMouseEvent>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QHeaderView>
#include <QTableView>
#include <QElapsedTimer>
#include <wx/init.h>
#include <wx/grid.h>

#include <kiqt_app.h>

#include "qa_qt_frames.h"

#include <class_draw_panel_gal.h>
#include <kiface_base.h>
#include <lib_symbol.h>
#include <bitmap_base.h>
#include <richio.h>
#include <sch_bitmap.h>
#include <sch_edit_frame.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <sim/simulator_frame.h>
#include <sim/sim_plot_tab.h>
#include <widgets/mathplot.h>
#include <settings/kicad_settings.h>
#include <libraries/library_manager.h>
#include <symbol_editor/symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <tool/actions.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include "qa_qt_canvas.h"
#include <widgets/lib_tree.h>
#include <widgets/webview_panel.h>
#include <wx/webview.h>
#include <wx/popupwin.h>
#include <wx/stc/stc.h>
#include <QTextBlock>
#include <QTextLayout>
#include <set>
#include "qa_qt_crash.h"   // last: it includes windows.h


/// The KIFACE linked in the test is the one of eeschema in its KIWAY, as in the program of
/// single_top (the friend of KIWAY which registers it).
struct PGM_SINGLE_TOP
{
    static void RegisterKiface( KIWAY& aKiway )
    {
        aKiway.set_kiface( KIWAY::KifaceType( FRAME_SCH ), &Kiface() );
    }
};


namespace
{

using namespace QA_QT_FRAMES;


/// The screenshot of a test, with a suffix (empty: no screenshot).
QString screenshotPath( const QString& aScreenshot, const char* aSuffix )
{
    if( aScreenshot.isEmpty() )
        return QString();

    QString path = aScreenshot;
    return path.replace( QStringLiteral( ".png" ), QStringLiteral( "-%1.png" ).arg( aSuffix ) );
}


/// The canvas of a frame draws something (not a uniform image).
bool canvasDraws( EDA_DRAW_PANEL_GAL* aCanvas, const QString& aScreenshot )
{
    QImage image;
    bool   varied = false;

    for( int attempt = 0; attempt < 20 && !varied; ++attempt )
    {
        aCanvas->ForceRefresh();
        settle( 100 );

        if( !aCanvas->GetScreenshot( image ) || image.width() < 100 )
            continue;

        for( int y = 0; y < image.height() && !varied; y += 5 )
        {
            for( int x = 0; x < image.width() && !varied; x += 5 )
                varied = image.pixel( x, y ) != image.pixel( 0, 0 );
        }
    }

    if( varied && !aScreenshot.isEmpty() )
        image.save( aScreenshot );

    return varied;
}


/// The editor of the simulation model (DIALOG_SIM_MODEL, with its property grid), opened by a
/// button of the symbol properties dialog: shown with the parameters of the model, then closed.
void testSimModelDialog( QDialog* aParent, const QString& aButton, const QString& aScreenshot )
{
    bool shown = false;
    int  params = 0;

    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [&, poll]()
                      {
                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              QDialog* dialog = qobject_cast<QDialog*>( widget );

                              if( !dialog || dialog == aParent || !dialog->isVisible() )
                                  continue;

                              poll->stop();
                              poll->deleteLater();
                              shown = true;
                              settle( 200 );

                              // The built-in model, then a device of its models: the resistor.
                              // Until they hold (at most 3 s): the dialog sets its controls once
                              // shown, later on the slower macOS in CI
                              bool resistor = false;

                              for( int i = 0; i < 30 && !resistor; ++i )
                              {
                                  for( QRadioButton* radio : dialog->findChildren<QRadioButton*>() )
                                  {
                                      if( radio->text() == QStringLiteral( "Built-in SPICE model" )
                                              && !radio->isChecked() )
                                      {
                                          radio->click();
                                      }
                                  }

                                  settle( 100 );

                                  for( QComboBox* combo : dialog->findChildren<QComboBox*>() )
                                  {
                                      int index = combo->findText( QStringLiteral( "Resistor" ) );

                                      if( index >= 0 && combo->isEnabled() )
                                      {
                                          if( combo->currentIndex() != index )
                                          {
                                              combo->setCurrentIndex( index );
                                              Q_EMIT combo->activated( index );
                                          }

                                          resistor = true;
                                          break;
                                      }
                                  }
                              }

                              // The parameters, once the grid is filled (at most 3 s: macOS in CI
                              // is slower)
                              for( int i = 0; i < 30 && params == 0; ++i )
                              {
                                  settle( 100 );

                                  for( QTreeWidget* tree : dialog->findChildren<QTreeWidget*>() )
                                  {
                                      for( QTreeWidgetItemIterator it( tree ); *it; ++it )
                                      {
                                          if( !( *it )->isHidden() && ( *it )->childCount() == 0 )
                                              params++;
                                      }
                                  }
                              }

                              if( params == 0 )
                              {
                                  // What the dialog shows, for the platforms where it fails
                                  for( QRadioButton* radio : dialog->findChildren<QRadioButton*>() )
                                  {
                                      std::printf( "    radio \"%s\" checked %d enabled %d\n",
                                                   qPrintable( radio->text() ), radio->isChecked(),
                                                   radio->isEnabled() );
                                  }

                                  for( QComboBox* combo : dialog->findChildren<QComboBox*>() )
                                  {
                                      std::printf( "    combo \"%s\" of %d, enabled %d visible %d\n",
                                                   qPrintable( combo->currentText() ),
                                                   combo->count(), combo->isEnabled(),
                                                   combo->isVisible() );
                                  }

                                  for( QTreeWidget* tree : dialog->findChildren<QTreeWidget*>() )
                                  {
                                      int items = 0;

                                      for( QTreeWidgetItemIterator it( tree ); *it; ++it )
                                          items++;

                                      std::printf( "    tree of %d items, visible %d\n", items,
                                                   tree->isVisible() );
                                  }

                                  std::fflush( stdout );
                              }

                              if( !aScreenshot.isEmpty() )
                                  dialog->grab().save( aScreenshot );

                              if( !pressButton( dialog, QStringLiteral( "Cancel" ) ) )
                                  dialog->reject();

                              return;
                          }
                      } );

    poll->start( 20 );
    QPointer<QTimer> pollPtr( poll );

    check( pressButton( aParent, aButton ), "the simulation model button" );
    settle( 300 );

    if( pollPtr && !shown )
        delete pollPtr.data();

    check( shown, "the simulation model dialog is shown" );
    check( params > 0, "the simulation model dialog shows the parameters of a resistor" );
}


/// The preview of a symbol of the library tree of the symbol editor: shown in a popup next to
/// the tree when the mouse stays on the symbol, hidden when the mouse leaves the tree.
void testLibTreePreview( SYMBOL_EDIT_FRAME* aFrame, const QString& aScreenshot )
{
    LIB_TREE*  libTree = aFrame->GetLibTree();
    QTreeView* view = libTree ? libTree->GetTreeView() : nullptr;

    if( !view || !view->model() )
    {
        std::printf( "  no library tree: preview not tested\n" );
        return;
    }

    // The first symbol of the first library that has some
    QAbstractItemModel* model = view->model();
    QModelIndex         symbol;

    for( int row = 0; row < model->rowCount() && !symbol.isValid(); ++row )
    {
        QModelIndex library = model->index( row, 0 );

        if( model->rowCount( library ) > 0 )
        {
            view->expand( library );
            symbol = model->index( 0, 0, library );
        }
    }

    if( !symbol.isValid() )
    {
        std::printf( "  no symbol in the library tree: preview not tested\n" );
        return;
    }

    // The tree is hidden while a symbol of the schematic is edited
    aFrame->ShowPane( wxS( "LibraryTree" ), true );
    aFrame->UpdatePanes();
    aFrame->activateWindow();
    aFrame->raise();
    settle( 200 );
    view->scrollTo( symbol );
    view->clearSelection();
    settle( 300 );

    if( !aFrame->isActiveWindow() )
    {
        std::printf( "  the symbol editor is not the active window: preview not tested\n" );
        return;
    }

    QPoint pos = view->visualRect( symbol ).center();
    QMouseEvent move( QEvent::MouseMove, QPointF( pos ), QPointF( view->viewport()->mapToGlobal( pos ) ),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier );
    QApplication::sendEvent( view->viewport(), &move );

    // The hover delay (400 ms), then the preview draws
    QWidget* popup = nullptr;

    for( int attempt = 0; attempt < 20 && !popup; ++attempt )
    {
        settle( 100 );

        // wxPopupWindow has no Q_OBJECT: no findChildren<wxPopupWindow*>()
        for( QWidget* child : aFrame->findChildren<QWidget*>() )
        {
            wxPopupWindow* candidate = dynamic_cast<wxPopupWindow*>( child );

            if( candidate && candidate->isVisible()
                && candidate->findChild<QWidget*>( QStringLiteral( "symHoverPreview" ) ) )
            {
                popup = candidate;
            }
        }
    }

    check( popup != nullptr, "the library tree shows the preview of the hovered symbol" );

    if( !popup )
    {
        // What the preview needs: the tree shown, in the active window
        QWidget* active = QApplication::activeWindow();
        std::printf( "  tree visible %d, tree window %s, active window %s, symbol %s\n",
                     view->isVisible(), libTree->window()->metaObject()->className(),
                     active ? active->metaObject()->className() : "none",
                     model->data( symbol ).toString().toUtf8().constData() );

        for( QWidget* child : aFrame->findChildren<QWidget*>() )
        {
            if( wxPopupWindow* candidate = dynamic_cast<wxPopupWindow*>( child ) )
            {
                std::printf( "  popup visible %d, active %d, %d widgets\n", candidate->isVisible(),
                             candidate == active,
                             int( candidate->findChildren<QWidget*>().size() ) );

                for( QWidget* widget : candidate->findChildren<QWidget*>() )
                {
                    if( !widget->objectName().isEmpty() )
                        std::printf( "    %s\n", widget->objectName().toUtf8().constData() );
                }
            }
        }

        return;
    }

    QRect treeRect( view->mapToGlobal( QPoint( 0, 0 ) ), view->size() );
    check( !treeRect.intersects( popup->geometry() ), "the preview is next to the library tree" );

    if( !aScreenshot.isEmpty() )
        popup->grab().save( screenshotPath( aScreenshot, "symbol-preview" ) );

    QEvent leave( QEvent::Leave );
    QApplication::sendEvent( view->viewport(), &leave );
    settle( 100 );
    check( !popup->isVisible(), "the preview is hidden when the mouse leaves the tree" );
}


/// The symbol editor, with a symbol of the schematic: its dialogs, then closed.
void testSymbolEditor( KIWAY& aKiway, SCH_SYMBOL* aSymbol, const QString& aScreenshot )
{
    std::printf( "SYMBOL_EDIT_FRAME\n" );

    SYMBOL_EDIT_FRAME* frame = createFrame<SYMBOL_EDIT_FRAME>( aKiway, FRAME_SCH_SYMBOL_EDITOR );
    check( frame != nullptr, "the KIFACE makes the symbol editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the symbol editor is shown" );

    if( aSymbol )
    {
        frame->LoadSymbolFromSchematic( aSymbol );
        settle( 300 );
        check( frame->GetCurSymbol() != nullptr, "the symbol editor loads the symbol of the schematic" );
        check( canvasDraws( frame->GetCanvas(), screenshotPath( aScreenshot, "symbol-editor" ) ),
               "the canvas draws the symbol" );

        TOOL_MANAGER* toolMgr = frame->GetToolManager();
        testDialog( toolMgr, SCH_ACTIONS::symbolProperties, "library symbol properties",
                    aScreenshot, QStringLiteral( "Cancel" ) );
        testDialog( toolMgr, SCH_ACTIONS::pinTable, "pin table", aScreenshot,
                    QStringLiteral( "Cancel" ) );

        if( frame->GetScreen() )
            frame->GetScreen()->SetContentModified( false );
    }

    testLibTreePreview( frame, aScreenshot );

    closeFrame( frame, "the symbol editor" );
}


/// The simulator of the schematic (SIMULATOR_FRAME, with its plots): shown, then closed.
void testSimulator( KIWAY& aKiway, TOOL_MANAGER* aToolMgr, const QString& aScreenshot )
{
    std::printf( "SIMULATOR_FRAME\n" );

    aToolMgr->RunAction( SCH_ACTIONS::showSimulator );
    settle( 500 );

    auto* frame = static_cast<SIMULATOR_FRAME*>( aKiway.Player( FRAME_SIMULATOR, false ) );
    check( frame != nullptr, "the schematic editor opens the simulator" );

    if( !frame )
        return;

    check( frame->isVisible(), "the simulator is shown" );

    if( !aScreenshot.isEmpty() )
        frame->grab().save( screenshotPath( aScreenshot, "simulator" ) );

    closeFrame( frame, "the simulator" );
}


/// A simulation of a schematic: its transient analysis run by ngspice, a voltage plotted.
void testSimulation( KIWAY& aKiway, SCH_EDIT_FRAME* aFrame, const wxString& aSchematic,
                     const QString& aScreenshot )
{
    std::printf( "SIMULATION\n" );

    check( aFrame->OpenProjectFiles( { aSchematic } ), "the schematic editor opens the circuit" );
    settle( 300 );

    // The errors of the netlist of the circuit (none expected): shown, then closed
    bool    netlistError = false;
    QTimer* poll = onNextDialog( [&]( QDialog* aDialog )
                                 {
                                     netlistError = true;

                                     for( QLabel* label : aDialog->findChildren<QLabel*>() )
                                         std::printf( "  dialog: %s\n", label->text().toUtf8().constData() );

                                     aDialog->reject();
                                 } );
    QPointer<QTimer> pollPtr( poll );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::showSimulator );
    settle( 500 );

    if( pollPtr && !netlistError )
        delete pollPtr.data();

    check( !netlistError, "the netlist of the circuit has no errors" );

    auto* frame = static_cast<SIMULATOR_FRAME*>( aKiway.Player( FRAME_SIMULATOR, false ) );
    check( frame != nullptr, "the simulator of the circuit" );

    if( !frame )
        return;

    SIM_TAB* tab = frame->GetCurrentSimTab();

    if( !tab )
        tab = frame->NewSimTab( wxS( ".tran 1u 10m" ) );

    check( tab != nullptr, "the transient analysis" );

    if( !tab )
        return;

    frame->StartSimulation();

    for( int i = 0; i < 300 && !frame->SimFinished(); ++i )
        settle( 50 );

    check( frame->SimFinished(), "ngspice runs the simulation" );

    frame->AddVoltageTrace( wxS( "/out" ) );
    settle( 300 );

    auto* plotTab = dynamic_cast<SIM_PLOT_TAB*>( frame->GetCurrentSimTab() );
    check( plotTab && !plotTab->GetTraces().empty(), "the voltage is plotted" );

    if( plotTab && plotTab->GetPlotWin() )
    {
        QWidget* plot = plotTab->GetPlotWin()->GetHandle();
        QImage   image = plot->grab().toImage();
        bool     varied = false;

        for( int y = 0; y < image.height() && !varied; y += 3 )
        {
            for( int x = 0; x < image.width() && !varied; x += 3 )
                varied = image.pixel( x, y ) != image.pixel( 0, 0 );
        }

        check( image.width() > 100 && varied, "mathplot draws the plot" );
    }

    if( !aScreenshot.isEmpty() )
        frame->grab().save( screenshotPath( aScreenshot, "simulation" ) );

    // The workbook of the simulation is modified: its changes are discarded
    bool             asked = false;
    QPointer<QTimer> discard = onNextDialog( [&]( QDialog* aDialog )
                  {
                      asked = true;
                      std::printf( "  dialog %s: %s\n", aDialog->metaObject()->className(),
                                   aDialog->windowTitle().toUtf8().constData() );

                      if( !pressButton( aDialog, "Discard Changes" ) )
                          aDialog->reject();
                  } );

    closeFrame( frame, "the simulator of the circuit" );

    if( discard )
        delete discard.data();

    check( asked, "the simulator asks to save its workbook" );
}


/// The symbol viewer: shown, then closed.
void testSymbolViewer( KIWAY& aKiway )
{
    std::printf( "SYMBOL_VIEWER_FRAME\n" );

    SYMBOL_VIEWER_FRAME* frame = createFrame<SYMBOL_VIEWER_FRAME>( aKiway, FRAME_SCH_VIEWER );
    check( frame != nullptr, "the KIFACE makes the symbol viewer" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the symbol viewer is shown" );
    closeFrame( frame, "the symbol viewer" );
}


/// The images of a schematic in the formats KiCad does not read itself: decoded by the image
/// formats of Qt, as by wxImage in the wxWidgets build (without them the schematic does not
/// open: "Failed to read image data").
/// The colours of the syntax of a SPICE netlist (the netlist of the simulator: wxSTC_LEX_SPICE).
void testSpiceSyntax( QWidget* aParent )
{
    wxStyledTextCtrl editor( aParent );
    editor.SetLexer( wxSTC_LEX_SPICE );
    editor.SetText( wxS( "* A netlist\n.title QA\nR1 in out 10k ; the load\n"
                         "C1 out 0 100n\nV1 in 0 DC 5\n.tran 1u 10m\n.end\n" ) );

    std::set<QRgb> colours;

    for( QTextBlock block = editor.document()->begin(); block.isValid(); block = block.next() )
    {
        for( const QTextLayout::FormatRange& range : block.layout()->formats() )
        {
            if( range.format.hasProperty( QTextFormat::ForegroundBrush ) )
                colours.insert( range.format.foreground().color().rgb() );
        }
    }

    std::printf( "  SPICE netlist: %zu colours of the syntax\n", colours.size() );
    check( colours.size() >= 4, "the SPICE netlist is coloured (comments, commands, elements, "
                                "numbers)" );
}


void testEmbeddedImages( SCH_EDIT_FRAME* aFrame )
{
    struct EMBEDDED
    {
        const char* format;
        const char* base64;   ///< an image of 6 x 4 pixels
        int         ppi;      ///< 0: not checked
    };

    static const EMBEDDED images[] = {
        // 3780 pixels per meter (96 dpi): 37 dots per cm, as wxImage reads them
        { "BMP",
          "Qk2GAAAAAAAAADYAAAAoAAAABgAAAAQAAAABABgAAAAAAFAAAADEDgAAxA4AAAAAAAAAAAAAPKAUPKAUPKAUKB7I"
          "KB7IKB7IAAA8oBQ8oBQ8oBQoHsgoHsgoHsgAADygFDygFDygFCgeyCgeyCgeyAAAPKAUPKAUPKAUKB7IKB7IKB7I"
          "AAA=",
          94 },
        // No resolution: the 300 dpi of KiCad
        { "GIF",
          "R0lGODdhBgAEAIIAAMwAAMwAM8wzMwCZMzOZMzOZZgAAAAAAACwAAAAABgAEAAAIFwAHEBggIICAAQIFFBxI0OCA"
          "AgQBCAgIADs=",
          300 },
        // A JPEG of a printer (CMYK, Adobe): not read by the JPEG decoder of KiCad
        { "CMYK JPEG",
          "/9j/7gAOQWRvYmUAZAAAAAAA/9sAQwACAQEBAQECAQEBAgICAgIEAwICAgIFBAQDBAYFBgYGBQYGBgcJCAYHCQcG"
          "BggLCAkKCgoKCgYICwwLCgwJCgoK/8AAFAgABAAGBEMRAE0RAFkRAEsRAP/EAB8AAAEFAQEBAQEBAAAAAAAAAAAB"
          "AgMEBQYHCAkKC//EALUQAAIBAwMCBAMFBQQEAAABfQECAwAEEQUSITFBBhNRYQcicRQygZGhCCNCscEVUtHwJDNi"
          "coIJChYXGBkaJSYnKCkqNDU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6g4SFhoeIiYqSk5SV"
          "lpeYmZqio6Slpqeoqaqys7S1tre4ubrCw8TFxsfIycrS09TV1tfY2drh4uPk5ebn6Onq8fLz9PX29/j5+v/aAA4E"
          "QwBNAFkASwAAPwDS/wCDaT/kxTxZ/wBlav8A/wBNel18df8ABbP/AJOq8P8A/ZPrT/0uvq/L34h/8hqL/r1X/wBC"
          "av38r//Z",
          0 },
        // The formats of the qtimageformats module of Qt (its plugins qtiff and qwebp)
        { "TIFF",
          "SUkqAAgAAAANAAABBAABAAAABgAAAAEBBAABAAAABAAAAAIBAwADAAAAqgAAAAMBAwABAAAAAQAAAAYBAwABAAAA"
          "AgAAABEBBAABAAAAwAAAABUBAwABAAAAAwAAABYBBAABAAAABAAAABcBBAABAAAASAAAABoBBQABAAAAsAAAABsB"
          "BQABAAAAuAAAABwBAwABAAAAAQAAACgBAwABAAAAAgAAAAAAAAAIAAgACACWAAAAAQAAAJYAAAABAAAAFKA8FKA8"
          "FKA8yB4oyB4oyB4oFKA8FKA8FKA8yB4oyB4oyB4oFKA8FKA8FKA8yB4oyB4oyB4oFKA8FKA8FKA8yB4oyB4oyB4o",
          0 },
        { "WebP",
          "UklGRiIAAABXRUJQVlA4TBYAAAAvBcAAAA9wD8EzIZ8CZf7DAz2I6H8A",
          0 },
    };

    for( const EMBEDDED& embedded : images )
    {
        // As in a file: the image item with its data in base64
        const std::string content = std::string( "(image (at 50.8 50.8) (data \"" )
                                    + embedded.base64 + "\"))";

        STRING_LINE_READER reader( content, wxS( "embedded image" ) );
        SCH_IO_KICAD_SEXPR plugin;
        SCH_SHEET          sheet;
        SCH_BITMAP*        bitmap = nullptr;
        std::string        error;

        sheet.SetScreen( new SCH_SCREEN( &aFrame->Schematic() ) );

        try
        {
            plugin.LoadContent( reader, &sheet );
        }
        catch( const IO_ERROR& e )
        {
            error = e.What().ToStdString();
        }

        for( SCH_ITEM* item : sheet.GetScreen()->Items() )
        {
            if( item->Type() == SCH_BITMAP_T )
                bitmap = static_cast<SCH_BITMAP*>( item );
        }

        const BITMAP_BASE* image = bitmap ? &bitmap->GetReferenceImage().GetImage() : nullptr;
        const VECTOR2I     size = image ? image->GetSizePixels() : VECTOR2I();

        std::printf( "  %s: %s, %d x %d pixels, %d ppi\n", embedded.format,
                     error.empty() ? "read" : error.c_str(), size.x, size.y,
                     image ? image->GetPPI() : 0 );

        const std::string what = std::string( "the schematic reads an image " ) + embedded.format;
        check( error.empty() && size == VECTOR2I( 6, 4 ), what.c_str() );

        if( embedded.ppi && image )
        {
            const std::string ppi = std::string( "the resolution of the image " ) + embedded.format;
            check( image->GetPPI() == embedded.ppi, ppi.c_str() );
        }
    }
}


/**
 * The columns of the fields table dragged to another place by their labels: the field of the
 * column dragged is there (KiCad moves the field in its model and the columns of the grid are back
 * in their order).
 */
void testFieldsTableColumnMove( TOOL_MANAGER* aToolMgr )
{
    std::printf( "FIELDS TABLE COLUMNS\n" );

    aToolMgr->RunAction( SCH_ACTIONS::editSymbolFields );
    settle( 500 );

    QDialog* dialog = nullptr;
    wxGrid*  grid = nullptr;

    for( QWidget* widget : QApplication::topLevelWidgets() )
    {
        QDialog* candidate = qobject_cast<QDialog*>( widget );

        if( !candidate || !candidate->isVisible() )
            continue;

        for( QTableView* view : candidate->findChildren<QTableView*>() )
        {
            wxGrid* candidateGrid = dynamic_cast<wxGrid*>( view );

            if( candidateGrid && candidateGrid->CanDragColMove() )
            {
                dialog = candidate;
                grid = candidateGrid;
                break;
            }
        }

        if( grid )
            break;
    }

    check( grid != nullptr, "the fields table has a grid whose columns can be dragged" );

    if( !grid || grid->GetNumberCols() < 2 )
    {
        if( dialog )
            dialog->close();

        return;
    }

    wxString first = grid->GetColLabelValue( 0 );
    wxString second = grid->GetColLabelValue( 1 );

    // The drop of the user: the header moves the section, then the grid tells KiCad
    grid->horizontalHeader()->moveSection( 0, 1 );
    settle( 300 );

    bool ordered = true;

    for( int col = 0; col < grid->GetNumberCols(); ++col )
        ordered &= grid->GetColPos( col ) == col;

    std::printf( "      columns: '%s' '%s' -> '%s' '%s'\n", first.ToStdString().c_str(),
                 second.ToStdString().c_str(), grid->GetColLabelValue( 0 ).ToStdString().c_str(),
                 grid->GetColLabelValue( 1 ).ToStdString().c_str() );
    check( grid->GetColLabelValue( 1 ) == first && grid->GetColLabelValue( 0 ) == second,
           "a column of the fields table dragged to another place moves its field" );
    check( ordered, "the columns of the grid are in the order of the fields" );

    // The move by the code (wxGrid::SetColPos) is not a move of the user
    grid->SetColPos( 0, 1 );
    settle( 300 );
    check( grid->GetColLabelValue( 1 ) == first && grid->GetColPos( 0 ) == 1,
           "a column moved by the code is not taken as a move of the user" );
    grid->ResetColPos();

    // The field back in its place
    grid->horizontalHeader()->moveSection( 0, 1 );
    settle( 300 );

    dialog->close();
    settle( 200 );
}


/**
 * The web view of the Remote Symbols panel (WEBVIEW_PANEL): a page that posts a message to the
 * "kicad" handler, and a script run by RunScriptAsync() whose result comes back.  Without a web
 * engine (a build without one) the view has no scripts: nothing to test.
 */
void testWebView( SCH_EDIT_FRAME* aFrame )
{
    std::printf( "WEB VIEW\n" );

    if( !wxWebView::HasWebEngineCompat() )
    {
        std::printf( "  no web engine in this build: skipped\n" );
        return;
    }

    auto waitFor = []( const std::function<bool()>& aDone, int aMilliseconds )
    {
        QElapsedTimer timer;
        timer.start();

        while( !aDone() && timer.elapsed() < aMilliseconds )
            settle( 20 );

        return aDone();
    };

    WEBVIEW_PANEL* panel = new WEBVIEW_PANEL( aFrame );
    panel->resize( 400, 300 );
    panel->show();

    wxString message;
    bool     added = panel->AddMessageHandler( wxS( "kicad" ),
                                               [&]( const wxString& aMsg ) { message = aMsg; } );
    check( added, "the web view takes a message handler" );

    bool loaded = false;
    panel->GetWebView()->Bind( wxEVT_WEBVIEW_LOADED, [&]( wxWebViewEvent& aEvent )
                                                     {
                                                         loaded = true;
                                                         aEvent.Skip();
                                                     } );

    panel->SetPage( wxS( "<html><body><p id='p'>QA</p><script>"
                         "window.kicad.postMessage('hello from ' + document.getElementById('p').textContent);"
                         "</script></body></html>" ) );

    check( waitFor( [&]() { return loaded; }, 30000 ), "the web view loads the page" );
    check( waitFor( [&]() { return !message.IsEmpty(); }, 15000 ),
           "the page posts a message to the handler" );
    std::printf( "  message '%s'\n", message.ToStdString().c_str() );
    check( message == wxS( "hello from QA" ), "the handler receives the message of the page" );

    wxString result;
    bool     resultError = true;
    bool     haveResult = false;
    int      clientData = 0;
    void*    resultClientData = nullptr;

    panel->Bind( wxEVT_WEBVIEW_SCRIPT_RESULT, [&]( wxWebViewEvent& aEvent )
                                              {
                                                  haveResult = true;
                                                  result = aEvent.GetString();
                                                  resultError = aEvent.IsError();
                                                  resultClientData = aEvent.GetClientData();
                                              } );

    panel->RunScriptAsync( wxS( "document.getElementById('p').textContent + (1 + 2)" ),
                           &clientData );

    check( waitFor( [&]() { return haveResult; }, 15000 ), "RunScriptAsync() sends its result" );
    std::printf( "  script result '%s' error %d\n", result.ToStdString().c_str(), resultError );
    check( !resultError && result == wxS( "QA3" ), "the result of the script is right" );
    check( resultClientData == &clientData, "the result carries the client data" );

    wxString output;
    check( panel->GetWebView()->RunScript( wxS( "6 * 7" ), &output ) && output == wxS( "42" ),
           "RunScript() gives the result of the script" );

    panel->Destroy();
    settle( 200 );
}


void testSchEditor( KIWAY& aKiway, const wxString& aSchematicFile, const wxString& aSimSchematic,
                    const QString& aScreenshot )
{
    std::printf( "SCH_EDIT_FRAME\n" );

    SCH_EDIT_FRAME* frame = createFrame<SCH_EDIT_FRAME>( aKiway, FRAME_SCH );
    check( frame != nullptr, "the KIFACE makes the schematic editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the schematic editor is shown" );
    check( frame->GetCanvas() != nullptr, "the schematic editor has a canvas" );
    check( frame->GetToolManager() != nullptr, "the schematic editor has its tools" );

    check( frame->OpenProjectFiles( { aSchematicFile } ), "the schematic editor opens the schematic" );
    settle( 300 );

    SCH_SCREEN* screen = frame->GetScreen();
    SCH_SYMBOL* symbol = nullptr;
    size_t      items = 0;

    if( screen )
    {
        for( SCH_ITEM* item : screen->Items() )
        {
            items++;

            if( !symbol && item->Type() == SCH_SYMBOL_T )
                symbol = static_cast<SCH_SYMBOL*>( item );
        }
    }

    check( items > 0, "the schematic has its items" );
    check( symbol != nullptr, "the schematic has its symbols" );

    testEmbeddedImages( frame );
    testSpiceSyntax( frame );

    TOOL_MANAGER* toolMgr = frame->GetToolManager();

    toolMgr->RunAction( ACTIONS::zoomFitScreen );
    settle( 100 );
    check( canvasDraws( frame->GetCanvas(), screenshotPath( aScreenshot, "canvas" ) ),
           "the canvas draws the schematic" );
    checkCanvasGeometry( frame->GetCanvas(), "schematic editor" );

    // The tools of the editor on the schematic
    toolMgr->RunAction( ACTIONS::selectAll );
    settle( 100 );

    SCH_SELECTION_TOOL* selTool = toolMgr->GetTool<SCH_SELECTION_TOOL>();
    check( selTool && selTool->GetSelection().Size() > 0, "select all" );

    toolMgr->RunAction( ACTIONS::doDelete );
    settle( 100 );
    check( screen->Items().empty(), "delete the selection" );

    toolMgr->RunAction( ACTIONS::undo );
    settle( 100 );

    size_t restored = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        restored++;
        (void) item;
    }

    check( restored == items, "undo the delete" );

    // The dialogs of the editor
    testDialog( toolMgr, SCH_ACTIONS::annotate, "annotate", aScreenshot,
                QStringLiteral( "Close" ) );
    testDialog( toolMgr, SCH_ACTIONS::schematicSetup, "schematic setup", aScreenshot,
                QStringLiteral( "Cancel" ) );

    testDialog( toolMgr, SCH_ACTIONS::placeGlobalLabel, "global label", aScreenshot,
                QStringLiteral( "Cancel" ) );
    toolMgr->RunAction( ACTIONS::cancelInteractive );
    settle( 100 );

    // The properties of a new directive label (its own controls: the shape, the orientation)
    testDialog( toolMgr, SCH_ACTIONS::placeClassLabel, "directive label", aScreenshot,
                QStringLiteral( "Cancel" ) );
    toolMgr->RunAction( ACTIONS::cancelInteractive );
    settle( 100 );

    // The symbol of the schematic, after the undo (the items are the restored ones)
    symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        symbol = static_cast<SCH_SYMBOL*>( item );
        break;
    }

    if( symbol && selTool )
    {
        selTool->ClearSelection();
        selTool->AddItemToSel( symbol );
        testDialog( toolMgr, SCH_ACTIONS::properties, "symbol properties", aScreenshot,
                    QStringLiteral( "Cancel" ),
                    [&]( QDialog* aDialog )
                    {
                        testSimModelDialog( aDialog, QStringLiteral( "Simulation Model..." ),
                                            screenshotPath( aScreenshot, "sim-model" ) );
                    } );
        selTool->ClearSelection();
    }

    if( !aScreenshot.isEmpty() )
        check( frame->grab().save( aScreenshot ), "screenshot of the schematic editor" );

    testFieldsTableColumnMove( toolMgr );
    testWebView( frame );

    testSimulator( aKiway, toolMgr, aScreenshot );
    testSymbolEditor( aKiway, symbol, aScreenshot );
    testSymbolViewer( aKiway );

    // The changes of the test are not saved (the editor would ask)
    screen->SetContentModified( false );

    if( !aSimSchematic.IsEmpty() )
    {
        testSimulation( aKiway, frame, aSimSchematic, aScreenshot );

        if( frame->GetScreen() )
            frame->GetScreen()->SetContentModified( false );
    }

    closeFrame( frame, "the schematic editor" );
}


} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <schematic.kicad_sch> [screenshot.png]\n", argv[0] );
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

    // The schematic is opened from a copy, with its project (the editor writes next to it)
    QTemporaryDir dir;
    QFileInfo     source( QString::fromUtf8( argv[1] ) );
    QString       copy = dir.filePath( source.fileName() );
    check( QFile::copy( source.filePath(), copy ), "copy the schematic" );

    QString project = source.path() + QLatin1Char( '/' ) + source.completeBaseName()
                      + QStringLiteral( ".kicad_pro" );

    if( QFile::exists( project ) )
        QFile::copy( project, dir.filePath( QFileInfo( project ).fileName() ) );

    // The circuit of the simulation: the rectifier of the QA data of eeschema (next to the
    // schematics of the test), copied in its own directory
    QFileInfo simSource( source.dir().filePath(
            QStringLiteral( "spice_netlists/rectifier/rectifier.kicad_sch" ) ) );
    QString   simCopy;

    if( simSource.exists() && dir.path().size() && QDir( dir.path() ).mkdir( QStringLiteral( "sim" ) ) )
    {
        // The files of the circuit: its schematic, its project and its model libraries
        QDir simDir( dir.filePath( QStringLiteral( "sim" ) ) );
        simCopy = simDir.filePath( simSource.fileName() );

        for( const QFileInfo& file : simSource.dir().entryInfoList( QDir::Files ) )
            QFile::copy( file.filePath(), simDir.filePath( file.fileName() ) );
    }

    testStartWizard( argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );

    // A library of symbols of the QA data of eeschema in the global table (the libraries of
    // KiCad are not installed for the test): the library tree of the symbol editor has symbols
    QFileInfo qaLibrary( source.dir().filePath( QStringLiteral( "libs/4xxx.kicad_sym" ) ) );

    if( qaLibrary.exists() )
    {
        QFile table( QDir( QString::fromUtf8( SETTINGS_MANAGER::GetUserSettingsPath().utf8_str().data() ) )
                             .filePath( QStringLiteral( "sym-lib-table" ) ) );

        if( table.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
        {
            table.write( QStringLiteral( "(sym_lib_table\n  (version 7)\n"
                                         "  (lib (name \"QA_4xxx\")(type \"KiCad\")(uri \"%1\")"
                                         "(options \"\")(descr \"\"))\n)\n" )
                                 .arg( qaLibrary.absoluteFilePath() ).toUtf8() );
            table.close();
        }
    }

    program.GetLibraryManager().LoadGlobalTables();

    testSchEditor( kiway, wxString::FromUTF8( copy.toUtf8().constData() ),
                   wxString::FromUTF8( simCopy.toUtf8().constData() ),
                   argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
