/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file footprint_wizard_frame_qt.cpp
 * The parts of FOOTPRINT_WIZARD_FRAME with Qt: the layout of its panes and its events (see
 * pcbnew/footprint_wizard_frame_wx.cpp).  The page list and the parameter grid give their events
 * to the frame through the handlers bound on them.
 */

#include <QDockWidget>

#include <footprint_wizard_frame.h>
#include <pcbnew_settings.h>
#include <widgets/wx_grid.h>


void FOOTPRINT_WIZARD_FRAME::setupLayout()
{
    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The parameters, and the output of the wizard below them, on the left side
    QDockWidget* parameters = addPane( m_parametersPanel, wxS( "Params" ), _( "Parameters" ),
                                       PANE_DOCK::LEFT );
    QDockWidget* output = addPane( m_buildMessageBox, wxS( "Output" ), wxEmptyString,
                                   PANE_DOCK::LEFT );

    splitDockWidget( parameters, output, Qt::Vertical );

    // No caption for the output, as the wxWidgets build
    output->setTitleBarWidget( new QWidget( output ) );

    m_parametersPanel->setMinimumSize( 360, 180 );
    m_buildMessageBox->setMinimumWidth( 360 );
}


void FOOTPRINT_WIZARD_FRAME::setupToolkitEvents()
{
    m_pageList->Bind( wxEVT_LISTBOX, &FOOTPRINT_WIZARD_FRAME::ClickOnPageList, this );
    m_parameterGrid->Bind( wxEVT_GRID_CELL_CHANGED, &FOOTPRINT_WIZARD_FRAME::ParametersUpdated,
                           this );
}


void FOOTPRINT_WIZARD_FRAME::saveLayout( PCBNEW_SETTINGS* aCfg )
{
    // The frame stores the state of its docks with its window settings
}


void FOOTPRINT_WIZARD_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    // The dock widgets of Qt follow the size of the frame
    SizeEv.Skip();
}
