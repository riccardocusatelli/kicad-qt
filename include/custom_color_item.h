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

#pragma once

#include <vector>

#include <gal/color4d.h>
#include <wx/string.h>


/**
 * A class to handle a custom color (predefined color) for the color picker dialog.
 */
struct CUSTOM_COLOR_ITEM
{
    KIGFX::COLOR4D m_Color;
    wxString m_ColorName;

    CUSTOM_COLOR_ITEM( double red, double green, double blue, const wxString& aName )
    {
        m_Color.r = red;
        m_Color.g = green;
        m_Color.b = blue;
        m_ColorName = aName;
    }

    CUSTOM_COLOR_ITEM( double red, double green, double blue, double alpha, const wxString& aName )
    {
        m_Color.r = red;
        m_Color.g = green;
        m_Color.b = blue;
        m_Color.a = alpha;
        m_ColorName = aName;
    }

    CUSTOM_COLOR_ITEM( const KIGFX::COLOR4D& aColor, const wxString& aName )
        : m_Color( aColor ), m_ColorName( aName)
    {}
};


typedef std::vector<CUSTOM_COLOR_ITEM> CUSTOM_COLORS_LIST;
