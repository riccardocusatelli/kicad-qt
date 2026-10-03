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
 * @file color4d_wx.cpp
 * Conversions between COLOR4D and wxColour, for the GUI (the rest of COLOR4D is in color4d.cpp
 * and uses no GUI).
 */

#include <wx/colour.h>

#include <gal/color4d.h>

namespace KIGFX
{

COLOR4D::COLOR4D( const wxColour& aColor )
{
    r = aColor.Red() / 255.0;
    g = aColor.Green() / 255.0;
    b = aColor.Blue() / 255.0;
    a = aColor.Alpha() / 255.0;
    m_text = nullptr;
}


wxColour COLOR4D::ToColour() const
{
    using CHAN_T = wxColourBase::ChannelType;

    const wxColour colour(
            static_cast<CHAN_T>( r * 255 + 0.5 ), static_cast<CHAN_T>( g * 255 + 0.5 ),
            static_cast<CHAN_T>( b * 255 + 0.5 ), static_cast<CHAN_T>( a * 255 + 0.5 ) );
    return colour;
}

} // namespace KIGFX
