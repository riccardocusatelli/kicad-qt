/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file footprint_edit_frame_qt.cpp
 * The parts of FOOTPRINT_EDIT_FRAME with Qt: the layout of its panes and its events (see
 * pcbnew/footprint_edit_frame_wx.cpp).
 *
 * The keys that the library tree doesn't use reach the tools through
 * EDA_DRAW_FRAME::keyPressEvent(), and the dropped files through EDA_BASE_FRAME::dropEvent().
 */

#include <QDockWidget>

#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <pcbnew_id.h>
#include <widgets/appearance_controls.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_properties_panel.h>


void FOOTPRINT_EDIT_FRAME::setupLayout()
{
    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The libraries and the properties on the left side
    QDockWidget* libraries = addPane( m_treePane, wxS( "Footprints" ), _( "Libraries" ),
                                      PANE_DOCK::LEFT );
    QDockWidget* properties = addPane( m_propertiesPanel, PropertiesPaneName(),
                                       _( "Properties" ), PANE_DOCK::LEFT,
                                       GetSettings()->m_AuiPanels.show_properties );

    splitDockWidget( libraries, properties, Qt::Horizontal );

    m_treePane->setMinimumSize( 250, 80 );
    m_propertiesPanel->setMinimumSize( 240, 60 );

    // The appearance and the selection filter on the right side, one above the other
    QDockWidget* appearance = addPane( m_appearancePanel, wxS( "LayersManager" ),
                                       _( "Appearance" ), PANE_DOCK::RIGHT,
                                       m_show_layer_manager_tools );
    QDockWidget* selectionFilter = addPane( m_selectionFilterPanel, wxS( "SelectionFilter" ),
                                            _( "Selection Filter" ), PANE_DOCK::RIGHT,
                                            m_show_layer_manager_tools );

    splitDockWidget( appearance, selectionFilter, Qt::Vertical );

    // The appearance and the selection filter are always shown together (they are hidden
    // with the layers manager tools, not closed)
    for( QDockWidget* dock : { appearance, selectionFilter } )
        dock->setFeatures( dock->features() & ~QDockWidget::DockWidgetClosable );

    m_appearancePanel->setMinimumWidth( 180 );

    // The best sizes of the panels (as the wxWidgets build)
    resizeDocks( { libraries, properties }, { 250, 300 }, Qt::Horizontal );

    // The selection filter doesn't need to grow in the vertical direction when docked
    resizeDocks( { selectionFilter }, { m_selectionFilterPanel->sizeHint().height() },
                 Qt::Vertical );
}


void FOOTPRINT_EDIT_FRAME::setupToolkitEvents()
{
    BindMenuCommand( wxID_CLOSE, [this]() { CloseFootprintEditor(); } );
    BindMenuCommand( wxID_EXIT, [this]() { OnExitKiCad(); } );
    BindMenuCommand( ID_FPEDIT_SAVE_PNG, [this]() { OnSaveFootprintAsPng(); } );
}
