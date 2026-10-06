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

#include <memory>
#include <vector>

#include <math/vector2d.h>
#include <sch_view.h>
#include <schematic.h>

#include <qt_document.h>

class PICKED_ITEMS_LIST;
class PROPERTY_PANEL;
class QTreeWidget;
class QTreeWidgetItem;
class SCH_COMMIT;
class SCH_ITEM;
class SCH_SCREEN;
class UNDO_STACK;


/**
 * A schematic in the Qt shell: the canvas, the sheets of the hierarchy, the properties of the
 * selection, and the editing of the items of the sheet shown (move, rotate, delete, undo,
 * save).  It hosts the view (SCH_VIEW_HOST): the drawing sheet shows the sheet displayed.
 *
 * The changes are SCH_COMMITs, kept in step with the view and the selection by a
 * SCH_COMMIT_LISTENER; they are undone by SCH_UNDO_REDO, as in the schematic editor.
 */
class SCHEMATIC_DOCUMENT : public QT_DOCUMENT, public SCH_VIEW_HOST
{
    Q_OBJECT

public:
    explicit SCHEMATIC_DOCUMENT( QWidget* aParent = nullptr );
    ~SCHEMATIC_DOCUMENT() override;

    bool        Open( const QString& aFileName, QString& aError ) override;
    bool        Save( QString& aError ) override;
    QString     FileName() const override { return m_fileName; }
    GAL_CANVAS* Canvas() const override { return m_canvas; }
    void        ZoomToFit() override;

    bool IsModified() const override;
    bool CanUndo() const override;
    bool CanRedo() const override;

    bool GetCurrentSheetInfo( SCH_CURRENT_SHEET_INFO& aInfo ) const override;

protected:
    EDA_ITEM*              itemAt( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    std::vector<EDA_ITEM*> itemsInArea( const BOX2I& aArea, bool aCrossing ) override;
    std::vector<EDA_ITEM*> allItems() override;
    void                   setItemSelected( EDA_ITEM* aItem, bool aSelected ) override;
    void                   selectionChanged() override;
    VECTOR2I               snapToGrid( const VECTOR2D& aDelta ) const override;

    bool beginMove() override;
    void moveBy( const VECTOR2I& aDelta ) override;
    void endMove( bool aCommit ) override;
    void rotateItems( const std::vector<EDA_ITEM*>& aItems ) override;
    void deleteItems( const std::vector<EDA_ITEM*>& aItems ) override;
    void undo() override;
    void redo() override;

private:
    class COMMIT_LISTENER;
    class UNDO_LISTENER;

    void onCursorMoved( double aX, double aY );
    void onSheetActivated( QTreeWidgetItem* aItem );

    /// Fill the list of sheets, by page number.
    void fillSheets();

    /// Show the sheet of a page (index in the list of sheets by page number).
    void showSheet( int aPage );

    /// Set the grid of the canvas from the settings of the schematic editor.
    void setupGrid();

    SCH_SCREEN* currentScreen() const;

    /// A new commit, kept in step with this document.
    std::unique_ptr<SCH_COMMIT> makeCommit();

    /**
     * The items of @a aItems a change applies to: the unlocked ones, without the children of
     * a symbol or a sheet that is changed too.
     */
    std::vector<SCH_ITEM*> changeableItems( const std::vector<EDA_ITEM*>& aItems );

    /**
     * Redraw @a aItem after a change.
     * @param aIsAddOrDelete it has just been added to or removed from the view.
     * @param aUpdateRtree its position in the RTree of the screen shown may have changed.
     */
    void redrawItem( EDA_ITEM* aItem, bool aIsAddOrDelete, bool aUpdateRtree );

    /// Record a change pushed by a commit (the entries of @a aChange) for undo.
    void recordChange( PICKED_ITEMS_LIST& aChange );

    /// The screens changed by an undo entry are to be saved.
    static void markScreensModified( const PICKED_ITEMS_LIST& aEntry );

    /// Recalculate the connections after a change, and redraw what they change.
    void recalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanUp );

    /// Undo or redo an entry.
    void applyUndoEntry( PICKED_ITEMS_LIST& aEntry );

    KIGFX::SCH_VIEW* view() const;

    GAL_CANVAS*                 m_canvas;
    QTreeWidget*                m_sheets;
    PROPERTY_PANEL*             m_properties;

    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<UNDO_STACK> m_undo;
    QString                     m_fileName;

    VECTOR2D                    m_gridSize;
    std::unique_ptr<SCH_COMMIT> m_moveCommit;       ///< The move in progress
    std::vector<SCH_ITEM*>      m_moving;
    bool                        m_leaveSheet = false;   ///< The sheet shown has been removed
};


/// The factory of KIFACE_QT_DOCUMENT_FACTORY.
QT_DOCUMENT* CreateQtSchematicDocument( QWidget* aParent );
