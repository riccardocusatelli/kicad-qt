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
class SCH_LINE;
class PROPERTY_PANEL;
class QTreeWidget;
class QTreeWidgetItem;
class SCH_COMMIT;
class SCH_ITEM;
class SCH_SCREEN;
class UNDO_STACK;


/**
 * A schematic in the Qt shell: the canvas, the sheets of the hierarchy, the properties of the
 * selection, and the editing of the items of the sheet shown: move and drag, rotate, delete,
 * edit the properties, copy and paste, draw wires and labels (schematic_tools.h), undo, save.
 * It hosts the view (SCH_VIEW_HOST): the drawing sheet shows the sheet displayed.
 *
 * The changes are SCH_COMMITs, kept in step with the view and the selection by a
 * SCH_COMMIT_LISTENER; they are undone by SCH_UNDO_REDO, as in the schematic editor.  The
 * clipboard has the format of the schematic editor (SCH_IO_KICAD_SEXPR).
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

    // For the tools of the document

    SCHEMATIC*       Schematic() const { return m_schematic.get(); }
    KIGFX::SCH_VIEW* View() const { return view(); }
    SCH_SCREEN*      CurrentScreen() const { return currentScreen(); }

    /// A new commit, kept in step with this document.
    std::unique_ptr<SCH_COMMIT> MakeCommit();

protected:
    EDA_ITEM*              itemAt( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    std::vector<EDA_ITEM*> itemsInArea( const BOX2I& aArea, bool aCrossing ) override;
    std::vector<EDA_ITEM*> allItems() override;
    void                   setItemSelected( EDA_ITEM* aItem, bool aSelected ) override;
    void                   selectionChanged() override;
    VECTOR2I               snapToGrid( const VECTOR2D& aDelta ) const override;
    VECTOR2I               snapPoint( const VECTOR2D& aPoint ) const override;

    bool    beginMove( bool aWithConnections ) override;
    QString copyItems( const std::vector<EDA_ITEM*>& aItems ) override;
    bool    beginPaste( const QString& aText, const VECTOR2I& aPosition ) override;
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

    /// Set a property of the selected items from the property panel, as one change.
    void onPropertyEdited( const void* aKey, const QString& aValue );
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

    /// The wires and junctions connected to the items being moved: they follow them (drag).
    void findConnectedWires();

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

    /**
     * An end of a wire that follows the items moved.  A horizontal or vertical wire stays so:
     * a new wire (the jog) joins its end to the moved point.
     */
    struct WIRE_END
    {
        SCH_LINE* m_Wire;
        bool      m_Start;              ///< The start of the wire follows (or its end)
        VECTOR2I  m_Origin;             ///< Where the end was
        VECTOR2I  m_Fixed;              ///< The other end
        SCH_LINE* m_Jog = nullptr;      ///< In the preview until the drag ends
    };

    std::vector<WIRE_END>       m_followingEnds;
    VECTOR2I                    m_dragged;          ///< Displacement of the drag so far
    bool                        m_pasting = false;    ///< m_moving are pasted, not on the screen
    bool                        m_leaveSheet = false;   ///< The sheet shown has been removed
};


/// The factory of KIFACE_QT_DOCUMENT_FACTORY.
QT_DOCUMENT* CreateQtSchematicDocument( QWidget* aParent );
