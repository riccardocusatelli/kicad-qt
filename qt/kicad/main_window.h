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

#pragma once

#include <functional>

#include <QMainWindow>
#include <QStringList>

class KIWAY_BASE;
class QAction;
class QFileSystemModel;
class QLabel;
class QListWidget;
class QMenu;
class QStackedWidget;
class QTabWidget;
class QToolBar;
class QTreeView;
class QT_DOCUMENT;


/**
 * The main window of kicad-qt: the files of the project, the open documents (boards and
 * schematics, made by the Qt KIFACEs) in tabs with their editing commands, the recent files.
 */
class MAIN_WINDOW : public QMainWindow
{
    Q_OBJECT

public:
    explicit MAIN_WINDOW( KIWAY_BASE& aKiway );

    /// Open a project (.kicad_pro) or a document (.kicad_pcb, .kicad_sch).
    void OpenPath( const QString& aPath );

protected:
    /// Ask to save the documents changed; the window stays open on Cancel.
    void closeEvent( QCloseEvent* aEvent ) override;

private:
    void createActions();

    void openProjectDialog();
    void openFileDialog();
    void openProject( const QString& aProjectFile );
    void openDocument( const QString& aFileName );
    void closeTab( int aIndex );

    /// Save a document; on failure, say why.  @return true if saved.
    bool saveDocument( QT_DOCUMENT* aDocument );

    /// If the document has changes, ask to save them.  @return false to keep it open.
    bool confirmClose( QT_DOCUMENT* aDocument );

    /// Enable the editing commands for the current document.
    void updateActions();

    /// The name of a document in its tab, marked if it has changes not saved.
    void updateTabTitle( QT_DOCUMENT* aDocument );

    /// Show the tools of the current document in the Place menu and the tool bar.
    void updateTools();
    void onCurrentTabChanged( int aIndex );
    void onProjectFileActivated( const QModelIndex& aIndex );

    /// Add a file to the recent files (settings of kicad-qt) and refresh the lists.
    void addRecentFile( const QString& aFileName );
    void updateRecentFiles();

    QT_DOCUMENT* currentDocument() const;

    KIWAY_BASE&        m_kiway;

    QStackedWidget*    m_center;      ///< The welcome page, or the documents
    QTabWidget*        m_tabs;
    QListWidget*       m_welcomeRecent;
    QTreeView*         m_projectTree;
    QFileSystemModel*  m_projectModel;
    QMenu*             m_recentMenu;
    QAction*           m_saveAction;
    QAction*           m_undoAction;
    QAction*           m_redoAction;
    QAction*           m_rotateAction;
    QAction*           m_deleteAction;
    QAction*           m_selectAllAction;
    QAction*           m_moveAction;
    QAction*           m_dragAction;
    QAction*           m_copyAction;
    QAction*           m_cutAction;
    QAction*           m_pasteAction;
    QMenu*             m_placeMenu;
    QToolBar*          m_toolsBar;
    QLabel*            m_cursorLabel;
    QString            m_projectFile;
};
