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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file wx_menubar.h
 * WX_MENUBAR of the Qt build: the menu bar of a frame, made of ACTION_MENUs (the class of
 * KiCad is kept, so that the code creating the menu bars is the same).
 */

#ifndef WX_MENUBAR_H_
#define WX_MENUBAR_H_

#include <QMenuBar>
#include <wx/kiwx_menu.h>

#include <tool/action_menu.h>
#include <qt_strings.h>


/**
 * The menu bar of a frame.
 *
 * The shortcuts of the menus are shown but not used by the menu bar: the hotkeys go to the
 * tools (TOOL_DISPATCHER), as with the menu bar of wxWidgets without accelerator table.
 */
class WX_MENUBAR : public wxMenuBar
{
public:
    WX_MENUBAR() = default;

    /**
     * Append a menu, with its title (a & marks the mnemonic).
     *
     * The menu becomes a child of the menu bar: it is deleted with it, and the frame finds its
     * items (the update of their state).
     */
    void Append( ACTION_MENU* aMenu, const wxString& aTitle )
    {
        aMenu->setParent( this, aMenu->windowFlags() );
        aMenu->setTitle( ToQString( aTitle ) );
        addMenu( aMenu );
    }

    /// The number of menus.
    size_t GetMenuCount() const { return menus().size(); }

    /// The menu at @a aPos.
    ACTION_MENU* GetMenu( size_t aPos ) const
    {
        QList<ACTION_MENU*> list = menus();
        return aPos < (size_t) list.size() ? list[aPos] : nullptr;
    }

private:
    QList<ACTION_MENU*> menus() const
    {
        QList<ACTION_MENU*> list;

        for( QAction* action : actions() )
        {
            if( ACTION_MENU* menu = dynamic_cast<ACTION_MENU*>( action->menu() ) )
                list.append( menu );
        }

        return list;
    }
};

#endif // WX_MENUBAR_H_
