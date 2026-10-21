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
 * @file wx_aui_art_providers.h
 * The art providers of AUI of the Qt build (include/widgets/wx_aui_art_providers.h with
 * wxWidgets): Qt draws the tabs of the notebooks, the provider only keeps the API.
 */

#pragma once

#include <wx/aui/auibook.h>


class WX_AUI_TAB_ART : public wxAuiTabArt
{
public:
    WX_AUI_TAB_ART() = default;

    wxAuiTabArt* Clone() override { return new WX_AUI_TAB_ART(); }
};
