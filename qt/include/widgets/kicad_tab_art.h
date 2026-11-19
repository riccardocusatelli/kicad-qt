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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// The Qt build of include/widgets/kicad_tab_art.h: only the visual state of the tabs, which
// EDITOR_TABS_PANEL applies to its QTabBar (there is no wxAuiTabArt).

#ifndef KICAD_TAB_ART_H
#define KICAD_TAB_ART_H

/// Visual decorations derived from document state: preview is italic, modified is bold with a
/// leading asterisk, and modified wins over preview.
struct TAB_VISUAL_STATE
{
    bool italic       = false;
    bool bold         = false;
    bool showAsterisk = false;
};


/**
 * Resolve a tab's decorations from its document state flags.
 */
inline TAB_VISUAL_STATE ResolveTabVisualState( bool aPreview, bool aModified )
{
    TAB_VISUAL_STATE state;

    // An edit promotes a preview to a permanent tab, so modified wins.
    if( aModified )
    {
        state.bold = true;
        state.showAsterisk = true;
    }
    else if( aPreview )
    {
        state.italic = true;
    }

    return state;
}

#endif // KICAD_TAB_ART_H
