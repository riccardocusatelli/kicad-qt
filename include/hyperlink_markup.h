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

#pragma once

#include <vector>

#include <kicommon.h>
#include <wx/string.h>

/**
 * The [label](url) markup of the violation messages, without the GUI (the renderer of the
 * GUI, HYPERLINK_DV_RENDERER, lays out these runs).
 */
namespace HYPERLINK_MARKUP
{

struct RUN
{
    wxString text;
    wxString href; // empty for plain text
};

/// Split aValue into plain text and link runs.
KICOMMON_API void ParseRuns( const wxString& aValue, std::vector<RUN>& aRuns );

/// Accept only schemes that wxLaunchDefaultBrowser cannot turn into program execution.
KICOMMON_API bool IsSafeUrl( const wxString& aHref );

/// Flatten [label](url) markup, keeping just the label.
KICOMMON_API wxString StripMarkup( const wxString& aValue );

} // namespace HYPERLINK_MARKUP
