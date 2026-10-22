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
 * @file hotkey_cycle_popup.h
 * The feedback of the hotkeys that cycle through options of the Qt build (see
 * common/dialogs/hotkey_cycle_popup.h): the layout of eda_view_switcher_base.fbp, with the widgets
 * of Qt.
 */

#ifndef  HOTKEY_CYCLE_POPUP_H
#define  HOTKEY_CYCLE_POPUP_H

#include <dialogs/eda_view_switcher_base.h>
#include <wx/arrstr.h>

class EDA_DRAW_FRAME;
class QTimer;


/**
 * A popup that shows the option chosen by a single-stroke hotkey, for a short time.  It is shown
 * without being activated: the keys still go to the canvas.
 */
class HOTKEY_CYCLE_POPUP : public EDA_VIEW_SWITCHER_BASE
{
public:
    HOTKEY_CYCLE_POPUP( EDA_DRAW_FRAME* aParent );

    void Popup( const wxString& aTitle, const wxArrayString& aItems, int aSelection );

private:
    QTimer*         m_showTimer;
    EDA_DRAW_FRAME* m_drawFrame;
};

#endif    // HOTKEY_CYCLE_POPUP_H
