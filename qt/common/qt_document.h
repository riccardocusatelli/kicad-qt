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

#include <vector>

#include <QString>
#include <QWidget>

#include <math/box2.h>
#include <math/vector2d.h>

#include "kiqt.h"

class EDA_ITEM;
class GAL_CANVAS;


/**
 * A document of the Qt shell (a board, a schematic), made by the KIFACE of its editor
 * (KIFACE_QT_DOCUMENT_FACTORY).
 *
 * The document is an editor: it has a selection and the editing commands of the main window
 * (undo, redo, delete, rotate, save).  QT_DOCUMENT handles the interaction on the canvas, the
 * same for every document:
 *  - a click selects the item under the cursor (Shift adds it to the selection or removes it);
 *  - dragging a selected item moves the selection, on the grid; Escape cancels the move;
 *  - dragging from an empty place selects the items in an area: to the right, the items
 *    inside it, to the left, the items it touches;
 *  - Escape clears the selection.
 * The document provides the items (itemAt(), itemsInArea()) and the changes (moveBy(),
 * rotateItems(), deleteItems()), made with the commits of its model.
 */
class KIQT_API QT_DOCUMENT : public QWidget
{
    Q_OBJECT

public:
    using QWidget::QWidget;

    /// Load a file; on failure, @a aError says why.
    virtual bool Open( const QString& aFileName, QString& aError ) = 0;

    /// Save to the file; on failure, @a aError says why.
    virtual bool Save( QString& aError ) = 0;

    /// The file shown.
    virtual QString FileName() const = 0;

    /// The canvas of the document.
    virtual GAL_CANVAS* Canvas() const = 0;

    /// Show everything.
    virtual void ZoomToFit() = 0;

    /// True if the document has changes not saved.
    virtual bool IsModified() const = 0;

    virtual bool CanUndo() const = 0;
    virtual bool CanRedo() const = 0;

    /// Undo or redo the last change.  The selection is cleared.
    void Undo();
    void Redo();

    /// The selected items.
    const std::vector<EDA_ITEM*>& Selection() const { return m_selection; }

    void SelectAll();
    void ClearSelection();

    /// Delete the selected items.
    void DeleteSelection();

    /// Rotate the selected items by 90 degrees, counterclockwise.
    void RotateSelection();

signals:
    /// The cursor moved (the text shows its position, in the units of the document).
    void cursorPositionChanged( const QString& aText );

    /// A message for the status bar.
    void statusMessage( const QString& aText );

    /// The document changed, was saved, or its selection changed: the commands may change.
    void stateChanged();

protected:
    /// Handle the interaction on @a aCanvas (from the constructor of the document).
    void setupEditing( GAL_CANVAS* aCanvas );

    /// Select @a aItems (instead of the current selection).
    void setSelection( const std::vector<EDA_ITEM*>& aItems );

    /// @a aItem leaves the document: drop it from the selection, if there.
    void forgetSelected( EDA_ITEM* aItem );

    /// Forget the selection without changing the items (they are being replaced).
    void dropSelection();

    // The items of the document

    /// The item to select at a point (the one drawn on top), or null.
    virtual EDA_ITEM* itemAt( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) = 0;

    /**
     * The items to select in an area.
     * @param aCrossing the items the area touches; otherwise the items inside it.
     */
    virtual std::vector<EDA_ITEM*> itemsInArea( const BOX2I& aArea, bool aCrossing ) = 0;

    /// The items Select All selects.
    virtual std::vector<EDA_ITEM*> allItems() = 0;

    /// Show @a aItem as selected or not.
    virtual void setItemSelected( EDA_ITEM* aItem, bool aSelected ) = 0;

    /// The selection has changed: show its properties.
    virtual void selectionChanged() = 0;

    /// A displacement on the grid (the nearest one).
    virtual VECTOR2I snapToGrid( const VECTOR2D& aDelta ) const = 0;

    // The changes

    /// Start moving the selection; false if it cannot be moved.
    virtual bool beginMove() = 0;

    /// Move the selection by @a aDelta (from where it is now), during a move.
    virtual void moveBy( const VECTOR2I& aDelta ) = 0;

    /// End the move: record it as a change, or put the items back.
    virtual void endMove( bool aCommit ) = 0;

    virtual void rotateItems( const std::vector<EDA_ITEM*>& aItems ) = 0;
    virtual void deleteItems( const std::vector<EDA_ITEM*>& aItems ) = 0;

    virtual void undo() = 0;
    virtual void redo() = 0;

private:
    void onLeftPressed( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers );
    void onLeftDragStarted( const VECTOR2D& aStart, Qt::KeyboardModifiers aModifiers );
    void onLeftDragged( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers );
    void onLeftReleased( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers, bool aDragged );
    void onCancel();

    /// End an interaction in progress (a move is cancelled).
    void cancelInteraction();

    bool isSelected( EDA_ITEM* aItem ) const;

    /// The area dragged from @a m_pressPoint to @a aPoint, and if it is a crossing one.
    BOX2I dragArea( const VECTOR2D& aPoint, bool& aCrossing ) const;

    enum class DRAG
    {
        NONE,
        MOVE,       ///< Moving the selection
        AREA        ///< Selecting the items of an area
    };

    GAL_CANVAS*            m_editCanvas = nullptr;
    std::vector<EDA_ITEM*> m_selection;

    DRAG                   m_drag = DRAG::NONE;
    VECTOR2D               m_pressPoint;
    EDA_ITEM*              m_pressItem = nullptr;
    bool                   m_pressSelected = false; ///< The press selected the item already
    VECTOR2I               m_moved;                 ///< Displacement of the move so far
};


/// The factory a KIFACE returns for KIFACE_QT_DOCUMENT_FACTORY.
typedef QT_DOCUMENT* ( *QT_DOCUMENT_FACTORY )( QWidget* aParent );
