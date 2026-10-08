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
 * @file vertex_editor_pane.h
 * PCB_VERTEX_EDITOR_PANE of the Qt build: the table of the vertices of a polygon or a zone (see
 * pcbnew/widgets/vertex_editor_pane.h).
 */

#pragma once

#include <memory>
#include <vector>

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QWidget>

#include <wx/string.h>

#include <geometry/shape_poly_set.h>
#include <origin_transforms.h>
#include <board_commit.h>

class BOARD_ITEM;
class PCB_BASE_EDIT_FRAME;
class PCB_SHAPE;
class ZONE;
class QTableWidget;

class PCB_VERTEX_EDITOR_PANE : public QWidget
{
public:
    PCB_VERTEX_EDITOR_PANE( PCB_BASE_EDIT_FRAME* aFrame );
    ~PCB_VERTEX_EDITOR_PANE() override;

    void SetItem( BOARD_ITEM* aItem );
    void ClearItem();

    /**
     * Check if the pane is currently editing the given item.
     */
    bool IsEditingItem( BOARD_ITEM* aItem ) const { return m_item == aItem; }

    /**
     * Update the pane in response to external selection changes.
     * This should be called when the selection changes to update or clear the editor.
     */
    void OnSelectionChanged( BOARD_ITEM* aNewItem );

    /// Give the focus to the table.
    void SetFocus();

private:
    void refreshGrid();
    void updateRow( int aRow );
    void updateHighlight( int aRow );

    bool parseCellValue( const wxString& aText, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                         int& aResult ) const;
    wxString formatCoord( int aValue, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) const;

    SHAPE_POLY_SET*       getPoly();
    const SHAPE_POLY_SET* getPoly() const;

    void onGridCellChange( int aRow, int aCol );

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    BOARD_ITEM*          m_item;
    ZONE*                m_zone;
    PCB_SHAPE*           m_shape;

    QTableWidget* m_grid;

    std::vector<SHAPE_POLY_SET::VERTEX_INDEX> m_rows;

    bool         m_updatingGrid;
};
