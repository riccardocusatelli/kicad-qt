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
 * @file test_tool_framework.cpp
 * qa_qt_tool_framework: the tool framework of KiCad on Qt (phase 3, milestone 3A).
 *
 * A frame of the Qt build (EDA_DRAW_FRAME) shows a board in its canvas (EDA_DRAW_PANEL_GAL);
 * the tools of KiCad (COMMON_TOOLS, ZOOM_TOOL) and a test tool get the mouse and key events
 * of Qt through the TOOL_DISPATCHER and QT_VIEW_CONTROLS.  The test sends the events as a user
 * would, and checks what the tools did.
 *
 * Usage: qa_qt_tool_framework <board.kicad_pcb> [screenshot.png]
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include <kiqt.h>     // Qt, without its METHOD macro

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPrinter>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

#include <wx/dcmemory.h>
#include <wx/init.h>
#include <wx/print.h>

#include <kiqt_app.h>

#include <board.h>
#include <class_draw_panel_gal.h>
#include <design_block.h>
#include <dialogs/dialog_design_block_properties.h>
#include <dialogs/dialog_unit_entry.h>
#include <eda_draw_frame.h>
#include <gal/gal_display_options.h>
#include <gal/gal_print.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>
#include <kiface_base.h>
#include <kiway.h>
#include <page_info.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <title_block.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/editor_conditions.h>
#include <tool/ui/toolbar_configuration.h>
#include <tool/common_tools.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/zoom_tool.h>
#include <view/view_controls.h>
#include <widgets/wx_grid.h>
#include <widgets/wx_infobar.h>
#include <ui_hooks.h>
#include <zoom_defines.h>

#include <qt_strings.h>
#include <widgets/kistatusbar.h>


namespace
{

int s_failures = 0;


void check( bool aOk, const char* aWhat )
{
    std::printf( "  %s  %s\n", aOk ? "ok  " : "FAIL", aWhat );

    if( !aOk )
        s_failures++;
}


/// The GAL of the test canvas (--opengl: the OpenGL GAL).
EDA_DRAW_PANEL_GAL::GAL_TYPE s_galType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;


/// The canvas of the test: a board in a PCB_VIEW, as the canvas of the PCB editor.
class TEST_CANVAS : public EDA_DRAW_PANEL_GAL
{
public:
    TEST_CANVAS( QWidget* aParent, KIGFX::GAL_DISPLAY_OPTIONS& aOptions, BOARD* aBoard ) :
            EDA_DRAW_PANEL_GAL( aParent, aOptions, s_galType )
    {
        m_gal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );

        KIGFX::PCB_VIEW* view = new KIGFX::PCB_VIEW();
        m_view = view;
        m_view->SetGAL( m_gal );

        m_painter = std::make_unique<KIGFX::PCB_PAINTER>( m_gal, FRAME_PCB_EDITOR );
        m_view->SetPainter( m_painter.get() );
        m_view->SetScaleLimits( ZOOM_MAX_LIMIT_PCBNEW, ZOOM_MIN_LIMIT_PCBNEW );

        view->SetDefaultLayerOrder();
        view->SetDefaultLayerDeps( false );
        view->UpdateDisplayOptions( PCB_DISPLAY_OPTIONS() );
        view->LoadColors( ::GetColorSettings( DEFAULT_THEME ) );
        view->ShowBoard( aBoard );
        view->SyncLayersVisibility( aBoard );

        createViewControls();
    }

    ~TEST_CANVAS() override
    {
        // The board outlives the view: its items leave the view first, as when the frames of
        // pcbnew close
        m_view->Clear();
    }
};


/// The actions of the test tool.
TOOL_ACTION s_testMain( TOOL_ACTION_ARGS()
        .Name( "qa.TestTool" )        // the name of the tool: InvokeTool() activates it
        .Scope( AS_GLOBAL ) );

TOOL_ACTION s_testMark( TOOL_ACTION_ARGS()
        .Name( "qa.TestTool.mark" )
        .Scope( AS_GLOBAL )
        .FriendlyName( "Mark" ) );


/// A tool that records the events it gets, with a context menu (as a selection tool).
class TEST_TOOL : public TOOL_INTERACTIVE
{
public:
    struct RECORD
    {
        TOOL_EVENT_CATEGORY category;
        TOOL_ACTIONS        action;
        int                 buttons;
        int                 modifiers;
        VECTOR2D            position;
    };

    TEST_TOOL() :
            TOOL_INTERACTIVE( "qa.TestTool" )
    {}

    void Reset( RESET_REASON aReason ) override {}

    bool Init() override
    {
        CONDITIONAL_MENU& menu = m_menu->GetMenu();

        menu.AddItem( s_testMark, SELECTION_CONDITIONS::ShowAlways );
        menu.AddSeparator();
        menu.AddItem( ACTIONS::zoomFitScreen, SELECTION_CONDITIONS::ShowAlways );

        getEditFrame<EDA_DRAW_FRAME>()->AddStandardSubMenus( *m_menu );
        return true;
    }

    int Main( const TOOL_EVENT& aEvent )
    {
        while( TOOL_EVENT* evt = Wait() )
        {
            if( evt->Category() == TC_MOUSE )
            {
                m_records.push_back( { evt->Category(), evt->Action(), evt->Buttons(),
                                       evt->Modifier(),
                                       evt->HasPosition() ? evt->Position() : VECTOR2D() } );
            }

            if( evt->IsClick( BUT_RIGHT ) )
                m_menu->ShowContextMenu( m_selection );
            else
                evt->SetPassEvent();
        }

        return 0;
    }

    int Mark( const TOOL_EVENT& aEvent )
    {
        m_marks++;
        return 0;
    }

    void setTransitions() override
    {
        Go( &TEST_TOOL::Main, s_testMain.MakeEvent() );
        Go( &TEST_TOOL::Mark, s_testMark.MakeEvent() );
    }

    /// The last record of an action, or nullptr.
    const RECORD* Last( TOOL_ACTIONS aAction ) const
    {
        for( auto it = m_records.rbegin(); it != m_records.rend(); ++it )
        {
            if( it->action == aAction )
                return &*it;
        }

        return nullptr;
    }

    int Count( TOOL_ACTIONS aAction ) const
    {
        int count = 0;

        for( const RECORD& record : m_records )
            count += record.action == aAction;

        return count;
    }

    std::vector<RECORD> m_records;
    int                 m_marks = 0;
    SELECTION           m_selection;
};


/// The toolbars of the test frame: buttons, a toggle, a group (as the toolbars of the editors).
class TEST_TOOLBAR_SETTINGS : public TOOLBAR_SETTINGS
{
public:
    TEST_TOOLBAR_SETTINGS() :
            TOOLBAR_SETTINGS( wxS( "qa_qt_toolbars" ) )
    {}

    std::optional<TOOLBAR_CONFIGURATION> DefaultToolbarConfig( TOOLBAR_LOC aToolbar ) override
    {
        TOOLBAR_CONFIGURATION config;

        switch( aToolbar )
        {
        case TOOLBAR_LOC::TOP_MAIN:
            config.AppendAction( ACTIONS::zoomInCenter )
                  .AppendAction( ACTIONS::zoomOutCenter )
                  .AppendAction( ACTIONS::zoomFitScreen );
            config.AppendSeparator();
            config.AppendAction( ACTIONS::zoomTool );
            break;

        case TOOLBAR_LOC::LEFT:
            config.AppendAction( ACTIONS::toggleGrid );
            config.AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                                .AddAction( ACTIONS::millimetersUnits )
                                .AddAction( ACTIONS::inchesUnits )
                                .AddAction( ACTIONS::milsUnits ) );
            break;

        default:
            return std::nullopt;
        }

        return config;
    }
};


/// A frame of the Qt build with the tools of KiCad, as the frames of the editors set them up.
class TEST_FRAME : public EDA_DRAW_FRAME
{
public:
    TEST_FRAME( KIWAY* aKiway, BOARD* aBoard ) :
            EDA_DRAW_FRAME( aKiway, nullptr, FRAME_PCB_EDITOR, wxT( "QA tool framework" ),
                            wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE,
                            wxT( "QaToolFrame" ), pcbIUScale ),
            m_board( aBoard )
    {
        m_page = aBoard->GetPageSettings();

        LoadSettings( config() );

        TEST_CANVAS* canvas = new TEST_CANVAS( this, GetGalDisplayOptions(), aBoard );
        SetCanvas( canvas );

        // The tools
        m_toolManager = new TOOL_MANAGER;
        m_toolManager->SetEnvironment( aBoard, canvas->GetView(), canvas->GetViewControls(),
                                       config(), this );
        m_actions = new ACTIONS();
        m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

        m_toolManager->RegisterTool( new COMMON_TOOLS );
        m_toolManager->RegisterTool( new ZOOM_TOOL );
        m_toolManager->RegisterTool( m_testTool = new TEST_TOOL );
        m_toolManager->InitTools();

        canvas->SetEventDispatcher( m_toolDispatcher );

        setupUIConditions();
        setupUnits( config() );

        // The hotkeys of the actions
        m_toolManager->GetActionManager()->UpdateHotKeys( true );

        // The toolbars
        m_toolbarSettings = &m_toolbarConfig;
        RecreateToolbars();

        CreateInfoBar();
        setupCentralArea();
        resize( 1000, 700 );

        m_toolManager->InvokeTool( "qa.TestTool" );

        ActivateGalCanvas();
    }

    ~TEST_FRAME()
    {
        // The tools go before the board
        delete m_toolManager;
        m_toolManager = nullptr;
    }

    void SetPageSettings( const PAGE_INFO& aPageSettings ) override { m_page = aPageSettings; }
    const PAGE_INFO& GetPageSettings() const override { return m_page; }

    const VECTOR2I GetPageSizeIU() const override
    {
        return VECTOR2I( m_page.GetWidthIU( pcbIUScale.IU_PER_MILS ),
                         m_page.GetHeightIU( pcbIUScale.IU_PER_MILS ) );
    }

    const VECTOR2I& GetGridOrigin() const override { return m_gridOrigin; }
    void SetGridOrigin( const VECTOR2I& aPosition ) override { m_gridOrigin = aPosition; }

    const TITLE_BLOCK& GetTitleBlock() const override { return m_titleBlock; }
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override { m_titleBlock = aTitleBlock; }

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override
    {
        return m_board->GetBoundingBox();
    }

    TEST_TOOL* TestTool() const { return m_testTool; }

    QString StatusField( int aField ) const { return ToQString( m_statusBar->GetStatusText( aField ) ); }

    ACTION_TOOLBAR* LeftToolbar() const { return m_tbLeft; }

    /// The button of a toolbar with a UI id (an action or a group).
    QAction* ToolbarItem( int aId ) const
    {
        for( ACTION_TOOLBAR* toolbar : { m_tbTopMain, m_tbLeft } )
        {
            for( QAction* action : toolbar->actions() )
            {
                if( action->data().isValid() && action->data().toInt() == aId )
                    return action;
            }
        }

        return nullptr;
    }

    void setupUIConditions() override
    {
        EDA_DRAW_FRAME::setupUIConditions();

        EDITOR_CONDITIONS cond( this );
        m_toolManager->GetActionManager()->SetConditions(
                ACTIONS::toggleGrid, ACTION_CONDITIONS().Check( cond.GridVisible() ) );
    }

    TEST_TOOLBAR_SETTINGS m_toolbarConfig;

private:
    BOARD*      m_board;
    PAGE_INFO   m_page;
    TITLE_BLOCK m_titleBlock;
    VECTOR2I    m_gridOrigin;
    TEST_TOOL*  m_testTool = nullptr;
};


/// The program: settings and project management (no GUI of wxWidgets).
class PGM_QT_TEST : public PGM_BASE
{
public:
    ~PGM_QT_TEST() override { Destroy(); }
};


/// Let the event loop run (the events posted by the canvas, the timers of the menus).
void settle( int aMilliseconds = 50 )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// The cursor at a point of the canvas (the tools read the position of the cursor).
void moveTo( EDA_DRAW_PANEL_GAL* aCanvas, QPoint aPoint )
{
    QCursor::setPos( aCanvas->mapToGlobal( aPoint ) );

    QMouseEvent move( QEvent::MouseMove, aPoint, aCanvas->mapToGlobal( aPoint ), Qt::NoButton,
                      QGuiApplication::mouseButtons(), Qt::NoModifier );
    QApplication::sendEvent( aCanvas, &move );
}


/// The buttons pressed, for the state the dispatcher reads (QGuiApplication::mouseButtons()
/// follows the events of the platform, which the test doesn't send).
Qt::MouseButtons s_buttons = Qt::NoButton;


void press( EDA_DRAW_PANEL_GAL* aCanvas, QPoint aPoint, Qt::MouseButton aButton,
            Qt::KeyboardModifiers aModifiers = Qt::NoModifier )
{
    s_buttons |= aButton;
    QMouseEvent event( QEvent::MouseButtonPress, aPoint, aCanvas->mapToGlobal( aPoint ), aButton,
                       s_buttons, aModifiers );
    QApplication::sendEvent( aCanvas, &event );
}


void release( EDA_DRAW_PANEL_GAL* aCanvas, QPoint aPoint, Qt::MouseButton aButton,
              Qt::KeyboardModifiers aModifiers = Qt::NoModifier )
{
    s_buttons &= ~aButton;
    QMouseEvent event( QEvent::MouseButtonRelease, aPoint, aCanvas->mapToGlobal( aPoint ),
                       aButton, s_buttons, aModifiers );
    QApplication::sendEvent( aCanvas, &event );
}


void drag( EDA_DRAW_PANEL_GAL* aCanvas, QPoint aPoint )
{
    QCursor::setPos( aCanvas->mapToGlobal( aPoint ) );

    QMouseEvent move( QEvent::MouseMove, aPoint, aCanvas->mapToGlobal( aPoint ), Qt::NoButton,
                      s_buttons, Qt::NoModifier );
    QApplication::sendEvent( aCanvas, &move );
}


void key( QWidget* aWidget, int aKey, Qt::KeyboardModifiers aModifiers = Qt::NoModifier,
          const QString& aText = QString() )
{
    QKeyEvent press( QEvent::KeyPress, aKey, aModifiers, aText );
    QApplication::sendEvent( aWidget, &press );

    QKeyEvent release( QEvent::KeyRelease, aKey, aModifiers, aText );
    QApplication::sendEvent( aWidget, &release );
}


void wheel( EDA_DRAW_PANEL_GAL* aCanvas, QPoint aPoint, int aDelta )
{
    QWheelEvent event( aPoint, aCanvas->mapToGlobal( aPoint ), QPoint(), QPoint( 0, aDelta ),
                       Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false );
    QApplication::sendEvent( aCanvas, &event );
}


/// Run a function on the next popup menu (or modal dialog) that opens.
void onNextPopup( std::function<void( QWidget* )> aFunction, bool aDialog = false )
{
    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [poll, aFunction, aDialog]()
                      {
                          QWidget* widget = aDialog ? QApplication::activeModalWidget()
                                                    : QApplication::activePopupWidget();

                          if( widget )
                          {
                              poll->stop();
                              poll->deleteLater();
                              aFunction( widget );
                          }
                      } );

    poll->start( 20 );
}

} // namespace


namespace
{

/// A printout of two pages: the Cairo GAL of the printouts and the DC of the page draw on them.
class TEST_PRINTOUT : public wxPrintout
{
public:
    TEST_PRINTOUT() : wxPrintout( wxS( "Test" ) ) {}

    bool HasPage( int aPage ) override { return aPage >= 1 && aPage <= 2; }

    void GetPageInfo( int* aMinPage, int* aMaxPage, int* aSelFrom, int* aSelTo ) override
    {
        *aMinPage = *aSelFrom = 1;
        *aMaxPage = *aSelTo = 2;
    }

    bool OnPrintPage( int aPage ) override
    {
        m_pages++;
        m_pageSize = GetLogicalPageRect().GetSize();

        {
            std::unique_ptr<KIGFX::GAL_PRINT> galPrint =
                    KIGFX::GAL_PRINT::Create( m_options, GetDC() );
            KIGFX::GAL* gal = galPrint->GetGAL();

            galPrint->SetSheetSize( VECTOR2D( 8.27, 11.69 ) );
            gal->SetClearColor( KIGFX::COLOR4D( 0.8, 0.9, 1.0, 1.0 ) );

            KIGFX::GAL_DRAWING_CONTEXT ctx( gal );
            gal->ClearScreen();
            gal->SetIsFill( true );
            gal->SetIsStroke( false );
            gal->SetFillColor( KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
            gal->DrawRectangle( VECTOR2D( -1e6, -1e6 ), VECTOR2D( 1e6, 1e6 ) );
        }

        // The DC of the page draws over the drawing of the GAL
        wxDC* dc = GetDC();
        dc->SetPen( wxPen( wxColour( 0, 0, 0 ), 4 ) );
        dc->DrawLine( 0, 0, m_pageSize.x, m_pageSize.y );
        return true;
    }

    int    m_pages = 0;
    wxSize m_pageSize;

private:
    KIGFX::GAL_DISPLAY_OPTIONS m_options;
};


/// The DC of the compatibility layer (its coordinates) and the printing of a printout.
void testPrinting()
{
    std::printf( "Drawing on a wxDC and printing (wxPrintout, GAL_PRINT)\n" );

    {
        // The logical coordinates: the y axis up from the bottom of the bitmap
        wxBitmap bitmap( 100, 100 );
        bitmap.fill( Qt::white );

        {
            wxMemoryDC dc( bitmap );
            dc.SetDeviceOrigin( 0, 100 );
            dc.SetAxisOrientation( true, true );
            check( dc.DeviceToLogicalY( 80 ) == 20 && dc.LogicalToDeviceY( 20 ) == 80,
                   "the logical coordinates of a DC with the y axis up" );

            dc.SetPen( wxPen( wxColour( 255, 0, 0 ), 1, wxPENSTYLE_TRANSPARENT ) );
            dc.SetBrush( wxBrush( wxColour( 255, 0, 0 ) ) );
            dc.DrawRectangle( 10, 10, 20, 20 );

            // A pie of a quarter of a circle, counterclockwise from the right
            dc.SetBrush( wxBrush( wxColour( 0, 0, 255 ) ) );
            dc.SetDeviceOrigin( 0, 0 );
            dc.SetAxisOrientation( true, false );
            dc.DrawArc( 90, 50, 50, 10, 50, 50 );
        }

        QImage image = bitmap.toImage();
        check( QColor( image.pixel( 20, 80 ) ) == QColor( 255, 0, 0 ),
               "a rectangle at its logical position" );
        check( QColor( image.pixel( 20, 20 ) ) == QColor( 255, 255, 255 ),
               "nothing at the mirrored position" );
        check( QColor( image.pixel( 70, 30 ) ) == QColor( 0, 0, 255 ),
               "an arc goes counterclockwise (its pie)" );
        check( QColor( image.pixel( 70, 70 ) ) == QColor( 255, 255, 255 ),
               "nothing on the other side of the arc" );
    }

    QTemporaryDir dir;
    QString       pdfFile = dir.filePath( QStringLiteral( "test.pdf" ) );

    QPrinter printer( QPrinter::HighResolution );
    printer.setOutputFormat( QPrinter::PdfFormat );
    printer.setOutputFileName( pdfFile );

    wxPrintData printData;
    printData.SetPaperId( wxPAPER_A4 );
    printData.SetOrientation( wxPORTRAIT );
    printData.ApplyCompat( printer );
    printer.setOutputFileName( pdfFile );

    TEST_PRINTOUT printout;
    bool          printed = wxPrinter::PrintPagesCompat( printer, &printout, false );

    check( printed && printout.m_pages == 2, "the printout prints its pages" );
    check( printout.m_pageSize.x > 0 && printout.m_pageSize.x < printout.m_pageSize.y,
           "a page of A4, in portrait" );

    QFile pdf( pdfFile );
    bool  isPdf = pdf.open( QIODevice::ReadOnly ) && pdf.read( 4 ) == "%PDF";
    check( isPdf && pdf.size() > 10000, "the PDF file has the pages (with their images)" );
}

} // namespace


int main( int argc, char** argv )
{
    // --opengl: the canvas draws with the OpenGL GAL (it needs a display with OpenGL)
    if( argc > 1 && std::string( argv[1] ) == "--opengl" )
    {
        s_galType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

        for( int ii = 1; ii < argc - 1; ++ii )
            argv[ii] = argv[ii + 1];

        argc--;
    }

    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s [--opengl] <board.kicad_pcb> [screenshot.png]\n",
                      argv[0] );
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

    {
        TEST_FRAME frame( &kiway, board.get() );
        frame.show();
        frame.activateWindow();
        settle( 200 );

        EDA_DRAW_PANEL_GAL* canvas = frame.GetCanvas();
        KIGFX::VIEW*        view = canvas->GetView();
        TEST_TOOL*          tool = frame.TestTool();
        TOOL_MANAGER*       toolMgr = frame.GetToolManager();
        QPoint              center( canvas->width() / 2, canvas->height() / 2 );

        canvas->SetFocus();
        moveTo( canvas, center );
        settle();

        if( s_galType == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
        {
            std::printf( "OpenGL GAL (HIDPI_GL_CANVAS, GL_CONTEXT_MANAGER)\n" );
            check( canvas->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL,
                   "the canvas draws with OpenGL" );
            check( canvas->GetGAL()->IsInitialized(), "the OpenGL GAL is initialized" );

            // The first drawing, once the window of the canvas is shown (exposed)
            QImage image;
            bool   readBack = false;
            bool   varied = false;

            // The board fills the canvas, whatever the size it had when the frame was shown
            toolMgr->RunAction( ACTIONS::zoomFitScreen );

            for( int attempt = 0; attempt < 20 && !varied; ++attempt )
            {
                canvas->ForceRefresh();
                settle( 100 );

                readBack = canvas->GetScreenshot( image ) && image.width() > 100;

                // The board is drawn: the image is not of one colour
                for( int y = 0; readBack && y < image.height() && !varied; y += 7 )
                {
                    for( int x = 0; x < image.width() && !varied; x += 7 )
                        varied = image.pixel( x, y ) != image.pixel( 0, 0 );
                }
            }

            check( readBack, "the drawing is read back from OpenGL" );
            check( varied, "the board is drawn" );

            // The events of the OpenGL window go to the panel, and to the tools
            QWindow* glWindow = static_cast<KIGFX::OPENGL_GAL*>( canvas->GetGAL() )->GetSurface();
            int      clicks = tool->Count( TA_MOUSE_CLICK );
            QPoint   at = center + QPoint( 20, 10 );

            QMouseEvent move( QEvent::MouseMove, at, glWindow->mapToGlobal( at ), Qt::NoButton,
                              Qt::NoButton, Qt::NoModifier );
            QCoreApplication::sendEvent( glWindow, &move );

            QMouseEvent press( QEvent::MouseButtonPress, at, glWindow->mapToGlobal( at ),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier );
            QCoreApplication::sendEvent( glWindow, &press );

            QMouseEvent release( QEvent::MouseButtonRelease, at, glWindow->mapToGlobal( at ),
                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier );
            QCoreApplication::sendEvent( glWindow, &release );
            settle();

            check( tool->Count( TA_MOUSE_CLICK ) == clicks + 1,
                   "a click on the OpenGL window reaches the tool" );
        }

        std::printf( "Hotkeys (TOOL_DISPATCHER -> ACTION_MANAGER -> COMMON_TOOLS)\n" );

        key( canvas, Qt::Key_Home );
        settle();
        double fitScale = view->GetScale();
        BOX2D  viewport = view->GetViewport();
        check( viewport.Contains( BOX2D( board->GetBoundingBox().GetPosition(),
                                         board->GetBoundingBox().GetSize() ) ),
               "Home: zoom to fit shows the whole board" );

        key( canvas, Qt::Key_F1 );
        settle();
        check( view->GetScale() > fitScale * 1.1, "F1: zoom in" );

        double inScale = view->GetScale();
        key( canvas, Qt::Key_F2 );
        settle();
        check( view->GetScale() < inScale / 1.1, "F2: zoom out" );

        VECTOR2D grid = view->GetGAL()->GetGridSize();
        key( canvas, Qt::Key_N, Qt::NoModifier, QStringLiteral( "n" ) );
        settle();
        check( view->GetGAL()->GetGridSize() != grid, "N: next grid" );

        EDA_UNITS units = frame.GetUserUnits();
        key( canvas, Qt::Key_U, Qt::ControlModifier );
        settle();
        check( frame.GetUserUnits() != units, "Ctrl+U: toggle units" );
        // mm <-> the imperial units used last (mils by default), as upstream
        QString unitsText = frame.GetUserUnits() == EDA_UNITS::INCH ? QStringLiteral( "inches" )
                            : frame.GetUserUnits() == EDA_UNITS::MILS ? QStringLiteral( "mils" )
                                                                      : QStringLiteral( "mm" );
        check( frame.StatusField( 5 ) == unitsText, "the status bar shows the units" );
        key( canvas, Qt::Key_U, Qt::ControlModifier );
        settle();

        std::printf( "Wheel (QT_VIEW_CONTROLS)\n" );

        double before = view->GetScale();
        wheel( canvas, center, 120 );
        settle();
        check( view->GetScale() != before, "the wheel zooms" );

        key( canvas, Qt::Key_Home );
        settle();

        std::printf( "Mouse (TOOL_DISPATCHER -> the test tool)\n" );

        QPoint at( canvas->width() / 3, canvas->height() / 3 );
        moveTo( canvas, at );
        settle();

        VECTOR2D cursor = canvas->GetViewControls()->GetMousePosition();
        check( tool->Count( TA_MOUSE_MOTION ) > 0, "motion events" );

        press( canvas, at, Qt::LeftButton );
        release( canvas, at, Qt::LeftButton );
        settle();

        const TEST_TOOL::RECORD* click = tool->Last( TA_MOUSE_CLICK );
        check( click && click->buttons == BUT_LEFT, "left click" );
        check( click && ( click->position - cursor ).EuclideanNorm() < 1.0,
               "the click is at the cursor (world coordinates)" );

        press( canvas, at, Qt::LeftButton, Qt::ShiftModifier );
        release( canvas, at, Qt::LeftButton, Qt::ShiftModifier );
        settle();
        click = tool->Last( TA_MOUSE_CLICK );
        check( click && ( click->modifiers & MD_SHIFT ), "shift click" );

        press( canvas, at, Qt::LeftButton );
        drag( canvas, at + QPoint( 30, 20 ) );
        drag( canvas, at + QPoint( 60, 40 ) );
        release( canvas, at + QPoint( 60, 40 ), Qt::LeftButton );
        settle();
        check( tool->Count( TA_MOUSE_DRAG ) >= 1, "drag" );
        check( tool->Count( TA_MOUSE_UP ) >= 1, "end of the drag" );

        std::printf( "Context menu (TOOL_MENU, CONDITIONAL_MENU, ACTION_MENU)\n" );

        int         entries = 0;
        bool        hasZoomMenu = false;

        onNextPopup(
                [&]( QWidget* aPopup )
                {
                    QMenu* menu = qobject_cast<QMenu*>( aPopup );

                    if( !menu )
                        return;

                    for( QAction* action : menu->actions() )
                    {
                        entries++;

                        if( action->menu() )
                            hasZoomMenu |= action->text().contains( QStringLiteral( "Zoom" ) );
                    }

                    for( QAction* action : menu->actions() )
                    {
                        if( action->data().toInt() == s_testMark.GetUIId() )
                        {
                            action->trigger();
                            break;
                        }
                    }

                    menu->close();
                } );

        moveTo( canvas, center );
        press( canvas, center, Qt::RightButton );
        release( canvas, center, Qt::RightButton );
        settle( 300 );

        check( entries >= 4, "the menu has the entries of the tool and the standard submenus" );
        check( hasZoomMenu, "the zoom submenu (ZOOM_MENU)" );
        check( tool->m_marks == 1, "the entry runs its action" );

        std::printf( "Zoom tool (ZOOM_TOOL)\n" );

        before = view->GetScale();
        toolMgr->PostAction( ACTIONS::zoomTool );
        settle();

        QPoint from( canvas->width() / 2 - 60, canvas->height() / 2 - 40 );
        QPoint to( canvas->width() / 2 + 60, canvas->height() / 2 + 40 );
        moveTo( canvas, from );
        press( canvas, from, Qt::LeftButton );
        drag( canvas, from + QPoint( 30, 20 ) );
        drag( canvas, to );
        release( canvas, to, Qt::LeftButton );
        settle();
        check( view->GetScale() > before * 2, "a drag zooms on its area" );

        key( canvas, Qt::Key_Escape );
        settle();
        key( canvas, Qt::Key_Home );
        settle();

        std::printf( "Dialog (DIALOG_SHIM, UNIT_BINDER, WX_PT_ENTRY_DIALOG)\n" );

        bool dialogShown = false;

        onNextPopup(
                [&]( QWidget* aDialog )
                {
                    dialogShown = aDialog->windowTitle() == QStringLiteral( "Grid Origin" );

                    QList<QLineEdit*> edits = aDialog->findChildren<QLineEdit*>();

                    if( edits.size() == 2 )
                    {
                        edits[0]->setText( QStringLiteral( "1.5" ) );
                        edits[1]->setText( QStringLiteral( "2" ) );
                    }

                    // The dialog (generated from its wxFormBuilder file) next to the screenshot
                    if( argc > 2 )
                    {
                        QString path = QString::fromUtf8( argv[2] );
                        path.replace( QStringLiteral( ".png" ), QStringLiteral( "-dialog.png" ) );
                        aDialog->grab().save( path );
                    }

                    for( QPushButton* button : aDialog->findChildren<QPushButton*>() )
                    {
                        if( button->text().remove( QLatin1Char( '&' ) ) == QStringLiteral( "OK" ) )
                            button->click();
                    }
                },
                true );

        toolMgr->RunAction( ACTIONS::gridOrigin );
        settle();

        check( dialogShown, "the dialog of the grid origin" );
        check( frame.GetGridOrigin() == VECTOR2I( pcbIUScale.mmToIU( 1.5 ),
                                                  pcbIUScale.mmToIU( 2 ) ),
               "the values entered (mm) set the grid origin" );

        std::printf( "Toolbars (ACTION_TOOLBAR, TOOLBAR_SETTINGS, UI conditions)\n" );

        QAction* zoomIn = frame.ToolbarItem( ACTIONS::zoomInCenter.GetUIId() );
        QAction* gridButton = frame.ToolbarItem( ACTIONS::toggleGrid.GetUIId() );
        QAction* unitsButton = nullptr;

        for( QAction* action : frame.LeftToolbar()->actions() )
        {
            if( action->menu() )
                unitsButton = action;
        }

        check( zoomIn && gridButton && unitsButton, "the toolbars have the buttons and the group" );

        if( zoomIn && gridButton && unitsButton )
        {
            before = view->GetScale();
            zoomIn->trigger();
            settle();
            check( view->GetScale() > before * 1.01, "a button runs its action (zoom in)" );

            KIQT::UpdateUINow();
            bool gridShown = frame.IsGridVisible();
            check( gridButton->isCheckable() && gridButton->isChecked() == gridShown,
                   "a toggle button shows the state of its UI condition" );

            gridButton->trigger();
            settle();
            KIQT::UpdateUINow();
            check( frame.IsGridVisible() != gridShown && gridButton->isChecked() == frame.IsGridVisible(),
                   "the toggle button changes the state (grid)" );

            EDA_UNITS unitsBefore = frame.GetUserUnits();
            unitsButton->trigger();
            settle();
            check( frame.GetUserUnits() != unitsBefore, "a group button cycles its actions (units)" );

            unitsButton->menu()->actions().at( 0 )->trigger();
            settle();
            check( frame.GetUserUnits() == EDA_UNITS::MM, "an action of the palette of a group" );
        }

        std::printf( "Dialog of KiCad with a grid (WX_GRID on the compatibility layer)\n" );

        {
            DESIGN_BLOCK block;
            block.SetLibId( LIB_ID( wxT( "lib" ), wxT( "block" ) ) );
            block.GetFields()[wxT( "Field1" )] = wxT( "Value1" );
            block.GetFields()[wxT( "Field2" )] = wxT( "Value2" );

            DIALOG_DESIGN_BLOCK_PROPERTIES dialog( &frame, &block );
            check( dialog.TransferDataToWindow(), "the data of the dialog goes to its controls" );

            WX_GRID* grid = nullptr;

            for( QTableView* view : dialog.findChildren<QTableView*>() )
            {
                if( WX_GRID* candidate = dynamic_cast<WX_GRID*>( view ) )
                    grid = candidate;
            }

            check( grid && grid->GetNumberRows() == 2 && grid->GetNumberCols() == 2,
                   "the grid has a row per field" );
            check( grid && grid->GetCellValue( 1, 0 ) == wxT( "Field2" )
                           && grid->GetCellValue( 1, 1 ) == wxT( "Value2" ),
                   "the cells show the fields" );
            check( grid && grid->GetColLabelValue( 0 ) == wxT( "Name" ),
                   "the labels of the columns" );

            if( grid )
            {
                grid->SetCellValue( 0, 1, wxT( "Changed" ) );

                check( dialog.TransferDataFromWindow()
                               && block.GetFields()[wxT( "Field1" )] == wxT( "Changed" ),
                       "the edited cell goes to the data" );
            }
        }

        std::printf( "Info bar (WX_INFOBAR)\n" );

        frame.ShowInfoBarMsg( wxT( "A message of the info bar" ), true );
        settle();
        check( frame.GetInfoBar()->isVisible(), "the info bar shows a message" );
        frame.GetInfoBar()->Dismiss();
        check( !frame.GetInfoBar()->isVisible(), "the info bar is dismissed" );

        if( argc > 2 )
        {
            // The frame with the board and a message, for a look at it
            frame.ShowInfoBarMsg( wxT( "Tool framework of KiCad on Qt" ), true );
            frame.SetMsgPanel( board.get() );
            settle( 200 );
            check( frame.grab().save( QString::fromUtf8( argv[2] ) ), "screenshot of the frame" );

            // The OpenGL window is not in the grab of the frame: its own drawing
            if( s_galType == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
            {
                QImage  image;
                QString path = QString::fromUtf8( argv[2] );
                path.replace( QStringLiteral( ".png" ), QStringLiteral( "-opengl.png" ) );
                check( canvas->GetScreenshot( image ) && image.save( path ),
                       "screenshot of the OpenGL canvas" );
            }
        }

        frame.Close( true );
        settle();
    }

    testPrinting();

    board.reset();

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
