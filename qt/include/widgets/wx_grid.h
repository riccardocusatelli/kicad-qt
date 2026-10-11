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
 * @file wx_grid.h
 * WX_GRID of the Qt build: the grids of the panels and dialogs, as a QTableWidget (the rows and
 * columns are set by the code of each grid).  The wxWidgets build has its WX_GRID on wxGrid
 * (common/widgets/wx_grid.h).
 */

#pragma once

#include <QHeaderView>
#include <QTableWidget>


class WX_GRID : public QTableWidget
{
public:
    explicit WX_GRID( QWidget* aParent = nullptr ) :
            QTableWidget( aParent )
    {
        setSelectionBehavior( QAbstractItemView::SelectRows );
        setEditTriggers( QAbstractItemView::NoEditTriggers );
        setShowGrid( false );
        verticalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    }
};
