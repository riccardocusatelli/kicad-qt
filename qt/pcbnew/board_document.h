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

#include <layer_ids.h>
#include <math/vector2d.h>

#include <qt_document.h>

class BOARD;
class BOARD_COMMIT;
class BOARD_ITEM;
class PICKED_ITEMS_LIST;
class PROPERTY_PANEL;
class QListWidget;
class QListWidgetItem;
class UNDO_STACK;

namespace KIGFX
{
class PCB_VIEW;
}


/**
 * A board in the Qt shell: the canvas, the layers (visibility, active layer), the properties
 * of the selection, and the editing of the items (move, rotate, delete, undo, save).
 *
 * The changes are BOARD_COMMITs, kept in step with the view and the selection by a
 * BOARD_COMMIT_LISTENER; they are undone by BOARD_UNDO_REDO, as in the board editor.
 */
class BOARD_DOCUMENT : public QT_DOCUMENT
{
    Q_OBJECT

public:
    explicit BOARD_DOCUMENT( QWidget* aParent = nullptr );
    ~BOARD_DOCUMENT() override;

    bool        Open( const QString& aFileName, QString& aError ) override;
    bool        Save( QString& aError ) override;
    QString     FileName() const override { return m_fileName; }
    GAL_CANVAS* Canvas() const override { return m_canvas; }
    void        ZoomToFit() override;

    bool IsModified() const override;
    bool CanUndo() const override;
    bool CanRedo() const override;

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
    void onLayerChanged( QListWidgetItem* aItem );
    void onCurrentLayerChanged( QListWidgetItem* aCurrent );

    /// Fill the layer list from the board.
    void fillLayers();

    /// Set the grid of the canvas from the settings of the PCB editor.
    void setupGrid();

    /// A new commit, kept in step with this document.
    std::unique_ptr<BOARD_COMMIT> makeCommit();

    /**
     * The items of @a aItems a change applies to: the unlocked ones, without the children of
     * a footprint that is changed too.
     */
    std::vector<BOARD_ITEM*> changeableItems( const std::vector<EDA_ITEM*>& aItems );

    /// Record a change pushed by a commit (the entries of @a aChange) for undo.
    void recordChange( PICKED_ITEMS_LIST& aChange );

    /// Undo or redo an entry.
    void applyUndoEntry( PICKED_ITEMS_LIST& aEntry );

    /// Show the board outline again, after a change of the board.
    void updateBoardOutline();

    KIGFX::PCB_VIEW* view() const;

    GAL_CANVAS*                   m_canvas;
    QListWidget*                  m_layers;
    PROPERTY_PANEL*               m_properties;

    std::unique_ptr<BOARD>        m_board;
    std::unique_ptr<UNDO_STACK>   m_undo;
    QString                       m_fileName;

    VECTOR2D                      m_gridSize;
    std::unique_ptr<BOARD_COMMIT> m_moveCommit;     ///< The move in progress
    std::vector<BOARD_ITEM*>      m_moving;
};


/// The factory of KIFACE_QT_DOCUMENT_FACTORY.
QT_DOCUMENT* CreateQtBoardDocument( QWidget* aParent );
