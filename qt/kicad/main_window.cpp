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

#include "main_window.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QTabBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <build_version.h>
#include <ki_exception.h>
#include <kiface.h>
#include <kiface_ids.h>
#include <frame_type.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include "home_widgets.h"

#include <gal_canvas.h>
#include <qt_document.h>
#include <qt_strings.h>


/// Number of recent files kept
static constexpr int MAX_RECENT_FILES = 12;

/// The setting of the recent files (QSettings of kicad-qt: the project manager of KiCad keeps
/// its own list, of projects only)
static const char* const RECENT_FILES_KEY = "files/recent";

/// The setting of the theme of the window (QSettings of kicad-qt)
static const char* const DARK_THEME_KEY = "appearance/dark";


/// The colors of the window: the light palette of the style, or a dark one.
static void applyTheme( bool aDark )
{
    if( !aDark )
    {
        QApplication::setPalette( QApplication::style()->standardPalette() );
        return;
    }

    QPalette palette;
    const QColor window( 0x2b, 0x2d, 0x30 );
    const QColor base( 0x1e, 0x1f, 0x22 );
    const QColor text( 0xdf, 0xe1, 0xe5 );
    const QColor accent( 0x35, 0x74, 0xf0 );

    palette.setColor( QPalette::Window, window );
    palette.setColor( QPalette::WindowText, text );
    palette.setColor( QPalette::Base, base );
    palette.setColor( QPalette::AlternateBase, window );
    palette.setColor( QPalette::ToolTipBase, base );
    palette.setColor( QPalette::ToolTipText, text );
    palette.setColor( QPalette::PlaceholderText, QColor( 0x8c, 0x8f, 0x94 ) );
    palette.setColor( QPalette::Text, text );
    palette.setColor( QPalette::Button, window );
    palette.setColor( QPalette::ButtonText, text );
    palette.setColor( QPalette::BrightText, Qt::white );
    palette.setColor( QPalette::Mid, QColor( 0x43, 0x45, 0x4a ) );
    palette.setColor( QPalette::Link, accent );
    palette.setColor( QPalette::Highlight, accent );
    palette.setColor( QPalette::HighlightedText, Qt::white );
    palette.setColor( QPalette::Disabled, QPalette::Text, QColor( 0x6f, 0x73, 0x7a ) );
    palette.setColor( QPalette::Disabled, QPalette::WindowText, QColor( 0x6f, 0x73, 0x7a ) );
    palette.setColor( QPalette::Disabled, QPalette::ButtonText, QColor( 0x6f, 0x73, 0x7a ) );

    QApplication::setPalette( palette );
}


/// The KIFACE that shows a document, from its extension.
static bool documentFace( const QString& aFileName, KIWAY_FACES::FACE_T& aFace )
{
    const QString suffix = QFileInfo( aFileName ).suffix().toLower();

    if( suffix == QLatin1String( "kicad_pcb" ) )
        aFace = KIWAY_FACES::FACE_PCB;
    else if( suffix == QLatin1String( "kicad_sch" ) )
        aFace = KIWAY_FACES::FACE_SCH;
    else
        return false;

    return true;
}


MAIN_WINDOW::MAIN_WINDOW( KIWAY& aKiway ) :
        m_kiway( aKiway )
{
    setWindowTitle( QStringLiteral( "KiCad" ) );
    resize( 1440, 900 );

    // Documents
    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode( true );
    m_tabs->setTabsClosable( true );
    m_tabs->setMovable( false );

    connect( m_tabs, &QTabWidget::tabCloseRequested, this, &MAIN_WINDOW::closeTab );
    connect( m_tabs, &QTabWidget::currentChanged, this, &MAIN_WINDOW::onCurrentTabChanged );

    // Home: the launcher of the editors is the first tab, as in the project manager of KiCad
    m_launcher = new LAUNCHER_PANEL;

    // The actions that currentChanged() updates do not exist yet
    {
        const QSignalBlocker blocker( m_tabs );
        m_tabs->addTab( m_launcher, tr( "Editors" ) );
    }

    m_tabs->tabBar()->setTabButton( 0, QTabBar::RightSide, nullptr );
    m_tabs->tabBar()->setTabButton( 0, QTabBar::LeftSide, nullptr );
    setCentralWidget( m_tabs );

    connect( m_launcher, &LAUNCHER_PANEL::schematicRequested, this,
             [this]()
             {
                 openDocument( siblingFile( QStringLiteral( "kicad_sch" ) ) );
             } );
    connect( m_launcher, &LAUNCHER_PANEL::boardRequested, this,
             [this]()
             {
                 openDocument( siblingFile( QStringLiteral( "kicad_pcb" ) ) );
             } );
    connect( m_launcher, &LAUNCHER_PANEL::symbolEditorRequested, this,
             [this]()
             {
                 openEditorFrame( FRAME_SCH_SYMBOL_EDITOR );
             } );
    connect( m_launcher, &LAUNCHER_PANEL::footprintEditorRequested, this,
             [this]()
             {
                 openEditorFrame( FRAME_FOOTPRINT_EDITOR );
             } );
    connect( m_launcher, &LAUNCHER_PANEL::toolRequested, this,
             [this]( int aFrameType )
             {
                 openEditorFrame( static_cast<FRAME_T>( aFrameType ) );
             } );

    // Files of the project
    m_projectModel = new QFileSystemModel( this );
    m_projectModel->setNameFilters( { QStringLiteral( "*.kicad_pro" ),
                                      QStringLiteral( "*.kicad_sch" ),
                                      QStringLiteral( "*.kicad_pcb" ) } );
    m_projectModel->setNameFilterDisables( false );

    m_projectTree = new QTreeView;
    m_projectTree->setModel( m_projectModel );
    m_projectTree->setHeaderHidden( true );

    for( int column = 1; column < m_projectModel->columnCount(); ++column )
        m_projectTree->hideColumn( column );

    connect( m_projectTree, &QTreeView::activated, this, &MAIN_WINDOW::onProjectFileActivated );

    m_projectDock = new QDockWidget( tr( "Project Files" ), this );
    m_projectDock->setObjectName( QStringLiteral( "project" ) );
    m_projectDock->setWidget( m_projectTree );
    m_projectDock->setFeatures( QDockWidget::NoDockWidgetFeatures );
    addDockWidget( Qt::LeftDockWidgetArea, m_projectDock );
    m_projectDock->hide();

    // The recent projects take the place of the files while no project is open
    m_recentProjects = new RECENT_PROJECTS_PANEL;

    connect( m_recentProjects, &RECENT_PROJECTS_PANEL::openRequested, this,
             &MAIN_WINDOW::openProjectDialog );
    connect( m_recentProjects, &RECENT_PROJECTS_PANEL::projectActivated, this,
             &MAIN_WINDOW::openProject );

    m_recentDock = new QDockWidget( tr( "Recent Projects" ), this );
    m_recentDock->setObjectName( QStringLiteral( "recent" ) );
    m_recentDock->setWidget( m_recentProjects );
    m_recentDock->setFeatures( QDockWidget::NoDockWidgetFeatures );
    m_recentDock->setTitleBarWidget( new QWidget );
    addDockWidget( Qt::LeftDockWidgetArea, m_recentDock );
    resizeDocks( { m_recentDock, m_projectDock }, { 460, 300 }, Qt::Horizontal );

    // Status bar
    m_cursorLabel = new QLabel;
    m_cursorLabel->setMinimumWidth( 220 );
    statusBar()->addPermanentWidget( m_cursorLabel );

    createActions();
    updateRecentFiles();
    updateTabBar();
}


void MAIN_WINDOW::createActions()
{
    QMenu* fileMenu = menuBar()->addMenu( tr( "&File" ) );

    QAction* openProject = fileMenu->addAction( tr( "Open &Project..." ), this,
                                                &MAIN_WINDOW::openProjectDialog );
    openProject->setShortcut( QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_O ) );

    QAction* openFile = fileMenu->addAction( tr( "&Open File..." ), this,
                                             &MAIN_WINDOW::openFileDialog );
    openFile->setShortcut( QKeySequence::Open );

    m_recentMenu = fileMenu->addMenu( tr( "Open &Recent" ) );

    fileMenu->addSeparator();

    m_saveAction = fileMenu->addAction( tr( "&Save" ), this,
                                        [this]()
                                        {
                                            if( QT_DOCUMENT* doc = currentDocument() )
                                                saveDocument( doc );
                                        } );
    m_saveAction->setShortcut( QKeySequence::Save );

    QAction* closeTab = fileMenu->addAction( tr( "&Close" ), this,
                                             [this]()
                                             {
                                                 if( m_tabs->currentIndex() >= 0 )
                                                     MAIN_WINDOW::closeTab( m_tabs->currentIndex() );
                                             } );
    closeTab->setShortcut( QKeySequence::Close );

    fileMenu->addSeparator();

    QAction* quit = fileMenu->addAction( tr( "&Quit" ), this, &QWidget::close );
    quit->setShortcut( QKeySequence::Quit );

    QMenu* editMenu = menuBar()->addMenu( tr( "&Edit" ) );

    // The commands of the current document.  The ones with keys that the other widgets use
    // (letters, Delete) work when the focus is in the documents
    auto documentAction =
            [&]( const QString& aText, const QList<QKeySequence>& aShortcuts, bool aInDocuments,
                 void ( QT_DOCUMENT::*aCommand )() ) -> QAction*
            {
                QAction* action = editMenu->addAction( aText, this,
                                                       [this, aCommand]()
                                                       {
                                                           if( QT_DOCUMENT* doc = currentDocument() )
                                                               ( doc->*aCommand )();
                                                       } );
                action->setShortcuts( aShortcuts );

                if( aInDocuments )
                {
                    action->setShortcutContext( Qt::WidgetWithChildrenShortcut );
                    m_tabs->addAction( action );
                }

                return action;
            };

    m_undoAction = documentAction( tr( "&Undo" ), QKeySequence::keyBindings( QKeySequence::Undo ),
                                   false, &QT_DOCUMENT::Undo );
    m_redoAction = documentAction( tr( "&Redo" ),
                                   { QKeySequence( Qt::CTRL | Qt::Key_Y ),
                                     QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_Z ) },
                                   false, &QT_DOCUMENT::Redo );
    editMenu->addSeparator();
    m_cutAction = documentAction( tr( "Cu&t" ), QKeySequence::keyBindings( QKeySequence::Cut ), true,
                                  &QT_DOCUMENT::Cut );
    m_copyAction = documentAction( tr( "&Copy" ), QKeySequence::keyBindings( QKeySequence::Copy ),
                                   true, &QT_DOCUMENT::Copy );
    m_pasteAction = documentAction( tr( "&Paste" ), QKeySequence::keyBindings( QKeySequence::Paste ),
                                    true, &QT_DOCUMENT::Paste );
    editMenu->addSeparator();

    // Move and Drag attach the selection to the cursor; Drag takes the connections along
    m_moveAction = editMenu->addAction( tr( "&Move" ), this,
                                        [this]()
                                        {
                                            if( QT_DOCUMENT* doc = currentDocument() )
                                                doc->MoveSelection( false );
                                        } );
    m_moveAction->setShortcut( QKeySequence( Qt::Key_M ) );
    m_moveAction->setShortcutContext( Qt::WidgetWithChildrenShortcut );
    m_tabs->addAction( m_moveAction );

    m_dragAction = editMenu->addAction( tr( "Dra&g" ), this,
                                        [this]()
                                        {
                                            if( QT_DOCUMENT* doc = currentDocument() )
                                                doc->MoveSelection( true );
                                        } );
    m_dragAction->setShortcut( QKeySequence( Qt::Key_G ) );
    m_dragAction->setShortcutContext( Qt::WidgetWithChildrenShortcut );
    m_tabs->addAction( m_dragAction );

    m_rotateAction = documentAction( tr( "R&otate" ), { QKeySequence( Qt::Key_R ) }, true,
                                     &QT_DOCUMENT::RotateSelection );
    m_deleteAction = documentAction( tr( "&Delete" ), { QKeySequence::Delete }, true,
                                     &QT_DOCUMENT::DeleteSelection );
    editMenu->addSeparator();
    m_selectAllAction = documentAction( tr( "Select &All" ), { QKeySequence::SelectAll }, true,
                                        &QT_DOCUMENT::SelectAll );

    // The tools of the current document (updateTools())
    m_placeMenu = menuBar()->addMenu( tr( "&Place" ) );

    QMenu* viewMenu = menuBar()->addMenu( tr( "&View" ) );

    QAction* zoomIn = viewMenu->addAction( tr( "Zoom &In" ), this,
                                           [this]()
                                           {
                                               if( QT_DOCUMENT* doc = currentDocument() )
                                                   doc->Canvas()->ZoomIn();
                                           } );
    zoomIn->setShortcut( QKeySequence::ZoomIn );

    QAction* zoomOut = viewMenu->addAction( tr( "Zoom &Out" ), this,
                                            [this]()
                                            {
                                                if( QT_DOCUMENT* doc = currentDocument() )
                                                    doc->Canvas()->ZoomOut();
                                            } );
    zoomOut->setShortcut( QKeySequence::ZoomOut );

    QAction* zoomFit = viewMenu->addAction( tr( "Zoom to &Fit" ), this,
                                            [this]()
                                            {
                                                if( QT_DOCUMENT* doc = currentDocument() )
                                                    doc->ZoomToFit();
                                            } );
    zoomFit->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_0 ) );

    viewMenu->addSeparator();
    viewMenu->addAction( m_recentDock->toggleViewAction() );

    QAction* darkTheme = viewMenu->addAction( tr( "&Dark Theme" ) );
    darkTheme->setCheckable( true );
    darkTheme->setChecked( QSettings().value( DARK_THEME_KEY, false ).toBool() );
    connect( darkTheme, &QAction::toggled, this,
             []( bool aDark )
             {
                 QSettings().setValue( DARK_THEME_KEY, aDark );
                 applyTheme( aDark );
             } );
    applyTheme( darkTheme->isChecked() );

    QToolBar* toolbar = addToolBar( tr( "Main" ) );
    toolbar->setObjectName( QStringLiteral( "main" ) );
    toolbar->setMovable( false );
    toolbar->setToolButtonStyle( Qt::ToolButtonTextOnly );
    toolbar->addAction( openProject );
    toolbar->addAction( openFile );
    toolbar->addAction( m_saveAction );
    toolbar->addSeparator();
    toolbar->addAction( m_undoAction );
    toolbar->addAction( m_redoAction );
    toolbar->addAction( m_rotateAction );
    toolbar->addAction( m_deleteAction );
    toolbar->addSeparator();
    toolbar->addAction( zoomIn );
    toolbar->addAction( zoomOut );
    toolbar->addAction( zoomFit );

    m_toolsBar = addToolBar( tr( "Tools" ) );
    m_toolsBar->setObjectName( QStringLiteral( "tools" ) );
    m_toolsBar->setMovable( false );
    m_toolsBar->setToolButtonStyle( Qt::ToolButtonTextOnly );

    QMenu* helpMenu = menuBar()->addMenu( tr( "&Help" ) );

    helpMenu->addAction( tr( "&About KiCad" ), this,
                         [this]()
                         {
                             QMessageBox::about( this, tr( "About KiCad" ),
                                                 tr( "<b>KiCad</b> %1<br>Qt shell on the core "
                                                     "of KiCad, without wxWidgets.<br>Qt %2" )
                                                         .arg( ToQString( GetBuildVersion() ) )
                                                         .arg( QString::fromLatin1( qVersion() ) ) );
                         } );

    updateActions();
    updateTools();
}


QT_DOCUMENT* MAIN_WINDOW::currentDocument() const
{
    return qobject_cast<QT_DOCUMENT*>( m_tabs->currentWidget() );
}


void MAIN_WINDOW::openProjectDialog()
{
    QString file = QFileDialog::getOpenFileName( this, tr( "Open Project" ), QString(),
                                                 tr( "KiCad projects (*.kicad_pro)" ) );

    if( !file.isEmpty() )
        openProject( file );
}


void MAIN_WINDOW::openFileDialog()
{
    QString dir = m_projectFile.isEmpty() ? QString() : QFileInfo( m_projectFile ).absolutePath();
    QString file = QFileDialog::getOpenFileName( this, tr( "Open File" ), dir,
                                                 tr( "KiCad boards and schematics "
                                                     "(*.kicad_pcb *.kicad_sch)" ) );

    if( !file.isEmpty() )
        openDocument( file );
}


void MAIN_WINDOW::OpenPath( const QString& aPath )
{
    if( QFileInfo( aPath ).suffix().toLower() == QLatin1String( "kicad_pro" ) )
        openProject( aPath );
    else
        openDocument( aPath );
}


bool MAIN_WINDOW::openProject( const QString& aProjectFile )
{
    QFileInfo info( aProjectFile );

    if( !info.exists() )
    {
        QMessageBox::warning( this, tr( "Open Project" ),
                              tr( "The project %1 does not exist." ).arg( aProjectFile ) );
        return false;
    }

    // The editors of the current project close first (they ask to save their changes), as in
    // the project manager of KiCad: their documents belong to the project that is unloaded
    if( info.absoluteFilePath() != m_projectFile && !m_kiway.PlayersClose( false ) )
        return false;

    m_projectFile = info.absoluteFilePath();
    Pgm().GetSettingsManager().LoadProject( ToWxString( m_projectFile ), true );

    QString dir = info.absolutePath();
    m_projectModel->setRootPath( dir );
    m_projectTree->setRootIndex( m_projectModel->index( dir ) );

    m_recentDock->hide();
    m_projectDock->show();

    const QFileInfo schematic( siblingFile( QStringLiteral( "kicad_sch" ) ) );
    const QFileInfo board( siblingFile( QStringLiteral( "kicad_pcb" ) ) );
    m_launcher->SetDocuments( schematic.exists(), board.exists() );

    setWindowTitle( tr( "%1 — KiCad" ).arg( info.completeBaseName() ) );
    addRecentFile( m_projectFile );
    statusBar()->showMessage( tr( "Project %1" ).arg( m_projectFile ), 5000 );
    return true;
}


void MAIN_WINDOW::openDocument( const QString& aFileName )
{
    const QString fileName = QFileInfo( aFileName ).absoluteFilePath();

    // Already open: show it
    for( int i = 0; i < m_tabs->count(); ++i )
    {
        auto* doc = qobject_cast<QT_DOCUMENT*>( m_tabs->widget( i ) );

        if( doc && doc->FileName() == fileName )
        {
            m_tabs->setCurrentIndex( i );
            return;
        }
    }

    KIWAY_FACES::FACE_T face;

    if( !documentFace( fileName, face ) )
    {
        QMessageBox::warning( this, tr( "Open File" ),
                              tr( "%1 is not a board or a schematic." ).arg( fileName ) );
        return;
    }

    // The schematics and the boards are edited by the editors of KiCad
    if( face == KIWAY_FACES::FACE_SCH || face == KIWAY_FACES::FACE_PCB )
    {
        openDocumentEditor( fileName, face == KIWAY_FACES::FACE_SCH ? FRAME_SCH
                                                                    : FRAME_PCB_EDITOR );
        return;
    }

    QT_DOCUMENT_FACTORY factory = nullptr;

    try
    {
        if( KIFACE* kiface = m_kiway.KiFACE( face ) )
        {
            factory = reinterpret_cast<QT_DOCUMENT_FACTORY>(
                    kiface->IfaceOrAddress( KIFACE_QT_DOCUMENT_FACTORY ) );
        }
    }
    catch( const IO_ERROR& error )
    {
        QMessageBox::critical( this, tr( "Open File" ), ToQString( error.What() ) );
        return;
    }

    if( !factory )
    {
        QMessageBox::critical( this, tr( "Open File" ),
                               tr( "The editor module cannot show %1." ).arg( fileName ) );
        return;
    }

    QT_DOCUMENT* doc = factory( m_tabs );

    connect( doc, &QT_DOCUMENT::cursorPositionChanged, m_cursorLabel, &QLabel::setText );
    connect( doc, &QT_DOCUMENT::statusMessage, this,
             [this]( const QString& aText )
             {
                 statusBar()->showMessage( aText, 8000 );
             } );
    connect( doc, &QT_DOCUMENT::stateChanged, this,
             [this, doc]()
             {
                 updateTabTitle( doc );
                 updateActions();
             } );

    QApplication::setOverrideCursor( Qt::WaitCursor );

    QString error;
    bool    opened = doc->Open( fileName, error );

    QApplication::restoreOverrideCursor();

    if( !opened )
    {
        delete doc;
        QMessageBox::critical( this, tr( "Open File" ),
                               tr( "%1 could not be opened.\n\n%2" ).arg( fileName, error ) );
        return;
    }

    int index = m_tabs->addTab( doc, QFileInfo( fileName ).fileName() );
    m_tabs->setTabToolTip( index, fileName );
    m_tabs->setCurrentIndex( index );
    updateTabBar();
    doc->Canvas()->setFocus();

    addRecentFile( fileName );
    updateActions();
}


void MAIN_WINDOW::openDocumentEditor( const QString& aFileName, FRAME_T aFrameType )
{
    // The document of another project: its project first, as the project manager of KiCad
    const QFileInfo document( aFileName );
    const QFileInfo project( document.absoluteDir().filePath( document.completeBaseName()
                                                              + QStringLiteral( ".kicad_pro" ) ) );

    if( project.exists() && project.absoluteFilePath() != m_projectFile
        && !openProject( project.absoluteFilePath() ) )
    {
        return;
    }

    const QString title = aFrameType == FRAME_SCH ? tr( "Schematic Editor" ) : tr( "PCB Editor" );
    KIWAY_PLAYER* frame = nullptr;

    try
    {
        frame = m_kiway.Player( aFrameType, true );
    }
    catch( const IO_ERROR& error )
    {
        QMessageBox::critical( this, title, ToQString( error.What() ) );
        return;
    }

    if( !frame )
    {
        QMessageBox::critical( this, title, tr( "The editor cannot be opened." ) );
        return;
    }

    QApplication::setOverrideCursor( Qt::WaitCursor );
    const std::vector<wxString> files( 1, ToWxString( document.absoluteFilePath() ) );
    const bool                  opened = frame->OpenProjectFiles( files );
    QApplication::restoreOverrideCursor();

    frame->show();
    frame->raise();
    frame->activateWindow();

    if( opened )
        addRecentFile( document.absoluteFilePath() );
}


void MAIN_WINDOW::openEditorFrame( FRAME_T aFrameType )
{
    KIWAY_PLAYER* frame = nullptr;

    try
    {
        frame = m_kiway.Player( aFrameType, true );
    }
    catch( const IO_ERROR& error )
    {
        QString title;

        switch( aFrameType )
        {
        case FRAME_SCH_SYMBOL_EDITOR: title = tr( "Symbol Editor" );        break;
        case FRAME_FOOTPRINT_EDITOR:  title = tr( "Footprint Editor" );     break;
        case FRAME_PL_EDITOR:         title = tr( "Drawing Sheet Editor" ); break;
        case FRAME_GERBER:            title = tr( "Gerber Viewer" );        break;
        case FRAME_CALC:              title = tr( "Calculator Tools" );     break;
        case FRAME_BM2CMP:            title = tr( "Image Converter" );      break;
        default:                      title = tr( "KiCad" );                break;
        }

        QMessageBox::critical( this, title, ToQString( error.What() ) );
        return;
    }

    if( !frame )
        return;

    frame->show();
    frame->raise();
    frame->activateWindow();
}


void MAIN_WINDOW::closeTab( int aIndex )
{
    auto* doc = qobject_cast<QT_DOCUMENT*>( m_tabs->widget( aIndex ) );

    if( !doc || !confirmClose( doc ) )
        return;

    m_tabs->removeTab( m_tabs->indexOf( doc ) );
    delete doc;

    if( m_tabs->count() == 1 )
    {
        m_tabs->setCurrentIndex( 0 );
        m_cursorLabel->clear();
    }

    updateTabBar();

    updateActions();
}


bool MAIN_WINDOW::saveDocument( QT_DOCUMENT* aDocument )
{
    QApplication::setOverrideCursor( Qt::WaitCursor );

    QString error;
    bool    saved = aDocument->Save( error );

    QApplication::restoreOverrideCursor();

    if( !saved )
    {
        QMessageBox::critical( this, tr( "Save" ),
                               error.isEmpty() ? tr( "%1 could not be saved." )
                                                         .arg( aDocument->FileName() )
                                               : error );
    }

    return saved;
}


bool MAIN_WINDOW::confirmClose( QT_DOCUMENT* aDocument )
{
    if( !aDocument->IsModified() )
        return true;

    m_tabs->setCurrentWidget( aDocument );

    const QMessageBox::StandardButton answer =
            QMessageBox::question( this, tr( "Close" ),
                                   tr( "Save the changes to %1?" )
                                           .arg( QFileInfo( aDocument->FileName() ).fileName() ),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                   QMessageBox::Save );

    if( answer == QMessageBox::Cancel )
        return false;

    if( answer == QMessageBox::Save )
        return saveDocument( aDocument );

    return true;
}


void MAIN_WINDOW::closeEvent( QCloseEvent* aEvent )
{
    for( int i = 0; i < m_tabs->count(); ++i )
    {
        auto* doc = qobject_cast<QT_DOCUMENT*>( m_tabs->widget( i ) );

        if( doc && !confirmClose( doc ) )
        {
            aEvent->ignore();
            return;
        }
    }

    // The editors of KiCad ask to save their changes, as when the project manager closes
    if( !m_kiway.PlayersClose( false ) )
    {
        aEvent->ignore();
        return;
    }

    aEvent->accept();
}


void MAIN_WINDOW::updateActions()
{
    QT_DOCUMENT* doc = currentDocument();
    const bool   selection = doc && !doc->Selection().empty();

    m_saveAction->setEnabled( doc && doc->IsModified() );
    m_undoAction->setEnabled( doc && doc->CanUndo() );
    m_redoAction->setEnabled( doc && doc->CanRedo() );
    m_rotateAction->setEnabled( selection );
    m_deleteAction->setEnabled( selection );
    m_moveAction->setEnabled( selection );
    m_dragAction->setEnabled( selection );
    m_copyAction->setEnabled( selection );
    m_cutAction->setEnabled( selection );
    m_pasteAction->setEnabled( doc != nullptr );
    m_selectAllAction->setEnabled( doc != nullptr );
}


void MAIN_WINDOW::updateTools()
{
    m_placeMenu->clear();
    m_toolsBar->clear();

    QT_DOCUMENT* doc = currentDocument();

    m_placeMenu->setEnabled( doc != nullptr );
    m_toolsBar->setVisible( doc != nullptr );

    if( !doc )
        return;

    m_placeMenu->addActions( doc->ToolActions() );
    m_toolsBar->addActions( doc->ToolActions() );
}


void MAIN_WINDOW::updateTabTitle( QT_DOCUMENT* aDocument )
{
    const int index = m_tabs->indexOf( aDocument );

    if( index < 0 )
        return;

    const QString name = QFileInfo( aDocument->FileName() ).fileName();

    m_tabs->setTabText( index, aDocument->IsModified() ? tr( "%1 •" ).arg( name ) : name );
}


void MAIN_WINDOW::onCurrentTabChanged( int aIndex )
{
    updateActions();
    updateTools();

    if( aIndex < 0 )
        return;

    m_cursorLabel->clear();
}


void MAIN_WINDOW::onProjectFileActivated( const QModelIndex& aIndex )
{
    if( m_projectModel->isDir( aIndex ) )
        return;

    OpenPath( m_projectModel->filePath( aIndex ) );
}


void MAIN_WINDOW::addRecentFile( const QString& aFileName )
{
    QSettings   settings;
    QStringList files = settings.value( RECENT_FILES_KEY ).toStringList();

    files.removeAll( aFileName );
    files.prepend( aFileName );

    while( files.size() > MAX_RECENT_FILES )
        files.removeLast();

    settings.setValue( RECENT_FILES_KEY, files );
    updateRecentFiles();
}


void MAIN_WINDOW::updateRecentFiles()
{
    m_recentMenu->clear();

    for( const QString& path : QSettings().value( RECENT_FILES_KEY ).toStringList() )
    {
        const QString name = QFileInfo( path ).fileName();

        QAction* action = m_recentMenu->addAction( name, this, [this, path]() { OpenPath( path ); } );
        action->setToolTip( path );
    }

    // Only the projects in the cards
    QStringList projects;

    for( const QString& path : QSettings().value( RECENT_FILES_KEY ).toStringList() )
    {
        if( QFileInfo( path ).suffix().toLower() == QLatin1String( "kicad_pro" ) )
            projects << path;
    }

    m_recentProjects->SetProjects( projects );

    m_recentMenu->setEnabled( !m_recentMenu->isEmpty() );
}


void MAIN_WINDOW::updateTabBar()
{
    m_tabs->tabBar()->setVisible( m_tabs->count() > 1 );
}


QString MAIN_WINDOW::siblingFile( const QString& aSuffix ) const
{
    if( m_projectFile.isEmpty() )
        return QString();

    const QFileInfo info( m_projectFile );

    return info.absolutePath() + QLatin1Char( '/' ) + info.completeBaseName() + QLatin1Char( '.' ) + aSuffix;
}
