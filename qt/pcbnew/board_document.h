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

#include <layer_ids.h>
#include <math/vector2d.h>

#include <qt_document.h>

class BOARD;
class BOARD_ITEM;
class PROPERTY_PANEL;
class QListWidget;
class QListWidgetItem;

namespace KIGFX
{
class PCB_VIEW;
}


/**
 * A board in the Qt shell: the canvas, the layers (visibility, active layer) and the
 * properties of the selected item.
 */
class BOARD_DOCUMENT : public QT_DOCUMENT
{
    Q_OBJECT

public:
    explicit BOARD_DOCUMENT( QWidget* aParent = nullptr );
    ~BOARD_DOCUMENT() override;

    bool        Open( const QString& aFileName, QString& aError ) override;
    QString     FileName() const override { return m_fileName; }
    GAL_CANVAS* Canvas() const override { return m_canvas; }
    void        ZoomToFit() override;

private:
    void onCursorMoved( double aX, double aY );
    void onClicked( double aX, double aY, Qt::KeyboardModifiers aModifiers );
    void onLayerChanged( QListWidgetItem* aItem );
    void onCurrentLayerChanged( QListWidgetItem* aCurrent );

    /// Fill the layer list from the board.
    void fillLayers();

    /// The item under a point, the one drawn on top (null if none).
    BOARD_ITEM* itemAt( const VECTOR2I& aPosition ) const;

    /// Select an item (null to clear the selection) and show its properties.
    void select( BOARD_ITEM* aItem );

    KIGFX::PCB_VIEW* view() const;

    GAL_CANVAS*            m_canvas;
    QListWidget*           m_layers;
    PROPERTY_PANEL*        m_properties;

    std::unique_ptr<BOARD> m_board;
    BOARD_ITEM*            m_selected = nullptr;
    QString                m_fileName;
};


/// The factory of KIFACE_QT_DOCUMENT_FACTORY.
QT_DOCUMENT* CreateQtBoardDocument( QWidget* aParent );
