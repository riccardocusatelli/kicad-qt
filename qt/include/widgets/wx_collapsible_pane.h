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
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file wx_collapsible_pane.h
 * WX_COLLAPSIBLE_PANE of the Qt build: the collapsible pane of the wxWidgets compatibility layer
 * (a tool button over the pane), which sends WX_COLLAPSIBLE_PANE_CHANGED when it is toggled.
 */

#ifndef KICAD_WX_COLLAPSIBLE_PANE_H
#define KICAD_WX_COLLAPSIBLE_PANE_H

#include <wx/collpane.h>

/// The pane was collapsed or expanded (a wxCollapsiblePaneEvent, a wxCommandEvent).
#define WX_COLLAPSIBLE_PANE_CHANGED wxEVT_COLLAPSIBLEPANE_CHANGED
#define WX_COLLAPSIBLE_PANE_HEADER_CHANGED wxEVT_COLLAPSIBLEPANE_CHANGED


class WX_COLLAPSIBLE_PANE : public wxCollapsiblePane
{
public:
    WX_COLLAPSIBLE_PANE( wxWidgetArgCompat aParent, int aId, const wxString& aLabel ) :
            wxCollapsiblePane( aParent, aId, aLabel )
    {
    }

    /// The margin of the pane under the header (the layout of Qt has none).
    int GetMargin() const { return 0; }
};

#endif // KICAD_WX_COLLAPSIBLE_PANE_H
