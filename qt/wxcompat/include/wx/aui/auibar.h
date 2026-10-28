/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file auibar.h
 * The tools of the toolbars of the wxWidgets compatibility layer (wxAuiToolBarItem): the item of
 * an action of a QToolBar (ACTION_TOOLBAR::AddTool()).
 */

#pragma once

#include <wx/defs.h>
#include <wx/string.h>

class QAction;


/// The styles of an AUI toolbar (the toolbars of the layer are QToolBars: only the orientation
/// matters).
enum wxAuiToolBarStyle
{
    wxAUI_TB_TEXT = 1 << 0,
    wxAUI_TB_NO_TOOLTIPS = 1 << 1,
    wxAUI_TB_NO_AUTORESIZE = 1 << 2,
    wxAUI_TB_GRIPPER = 1 << 3,
    wxAUI_TB_OVERFLOW = 1 << 4,
    wxAUI_TB_VERTICAL = 1 << 5,
    wxAUI_TB_HORZ_LAYOUT = 1 << 6,
    wxAUI_TB_HORIZONTAL = 1 << 7,
    wxAUI_TB_PLAIN_BACKGROUND = 1 << 8,
    wxAUI_TB_HORZ_TEXT = ( wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_TEXT ),
    wxAUI_TB_DEFAULT_STYLE = 0
};


class wxAuiToolBarItem
{
public:
    explicit wxAuiToolBarItem( int aId = wxID_ANY, QAction* aAction = nullptr ) :
            m_id( aId ),
            m_action( aAction )
    {
    }

    int GetId() const { return m_id; }
    void SetId( int aId ) { m_id = aId; }

    /// The action of the tool in the toolbar.
    QAction* GetActionCompat() const { return m_action; }

private:
    int      m_id;
    QAction* m_action;
};
