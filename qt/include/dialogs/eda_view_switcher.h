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
 * @file eda_view_switcher.h
 * The switcher of the layer presets and of the viewports (Ctrl+Tab) of the Qt build: the
 * layout of eda_view_switcher_base.fbp, with the widgets of Qt.
 */

#ifndef  EDA_VIEW_SWITCHER_H
#define  EDA_VIEW_SWITCHER_H

#include <dialogs/eda_view_switcher_base.h>
#include <toolkit/toolkit_types.h>
#include <wx/arrstr.h>
#include <wx/defs.h>


#ifdef __APPLE__
    #define PRESET_SWITCH_KEY WXK_RAW_CONTROL
    #define VIEWPORT_SWITCH_KEY WXK_ALT
#else
    #define PRESET_SWITCH_KEY WXK_CONTROL
    #define VIEWPORT_SWITCH_KEY WXK_SHIFT
#endif


/**
 * The list of the views; Tab (with the switch key held down) goes to the next one, and
 * releasing the switch key chooses it (Qt version of include/dialogs/eda_view_switcher.h).
 */
class EDA_VIEW_SWITCHER : public EDA_VIEW_SWITCHER_BASE
{
public:
    EDA_VIEW_SWITCHER( TOOLKIT_WINDOW* aParent, const wxArrayString& aItems, wxKeyCode aCtrlKey );

    int GetSelection() const { return m_listBox->currentRow(); }

protected:
    /// Tab and the release of the switch key, before the navigation between the widgets.
    bool event( QEvent* aEvent ) override;

private:
    /// Select the next view (or the previous one).
    void advance( bool aBackward );

    wxKeyCode m_ctrlKey;
};

#endif    // EDA_VIEW_SWITCHER_H
