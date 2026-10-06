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

#include <math/vector2d.h>
#include <sch_view.h>

#include <qt_document.h>

class PROPERTY_PANEL;
class QTreeWidget;
class QTreeWidgetItem;
class SCH_ITEM;
class SCHEMATIC;


/**
 * A schematic in the Qt shell: the canvas, the sheets of the hierarchy and the properties of
 * the selected item.  It hosts the view (SCH_VIEW_HOST): the drawing sheet shows the sheet
 * displayed.
 */
class SCHEMATIC_DOCUMENT : public QT_DOCUMENT, public SCH_VIEW_HOST
{
    Q_OBJECT

public:
    explicit SCHEMATIC_DOCUMENT( QWidget* aParent = nullptr );
    ~SCHEMATIC_DOCUMENT() override;

    bool        Open( const QString& aFileName, QString& aError ) override;
    QString     FileName() const override { return m_fileName; }
    GAL_CANVAS* Canvas() const override { return m_canvas; }
    void        ZoomToFit() override;

    bool GetCurrentSheetInfo( SCH_CURRENT_SHEET_INFO& aInfo ) const override;

private:
    void onCursorMoved( double aX, double aY );
    void onClicked( double aX, double aY, Qt::KeyboardModifiers aModifiers );
    void onSheetActivated( QTreeWidgetItem* aItem );

    /// Fill the list of sheets, by page number.
    void fillSheets();

    /// Show the sheet of a page (index in the list of sheets by page number).
    void showSheet( int aPage );

    /// The item under a point, the one drawn on top (null if none).
    SCH_ITEM* itemAt( const VECTOR2I& aPosition ) const;

    /// Select an item (null to clear the selection) and show its properties.
    void select( SCH_ITEM* aItem );

    KIGFX::SCH_VIEW* view() const;

    GAL_CANVAS*                m_canvas;
    QTreeWidget*               m_sheets;
    PROPERTY_PANEL*            m_properties;

    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_ITEM*                  m_selected = nullptr;
    QString                    m_fileName;
};


/// The factory of KIFACE_QT_DOCUMENT_FACTORY.
QT_DOCUMENT* CreateQtSchematicDocument( QWidget* aParent );
