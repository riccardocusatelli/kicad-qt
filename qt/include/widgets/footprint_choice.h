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
 * @file footprint_choice.h
 * FOOTPRINT_CHOICE of the Qt build: the choice of the footprints of the footprint selector, a
 * wxChoice of the compatibility layer (include/widgets/footprint_choice.h draws its items with
 * wxOwnerDrawnComboBox, greying the library names).
 */

#ifndef FOOTPRINT_CHOICE_H
#define FOOTPRINT_CHOICE_H

#include <wx/choice.h>


class FOOTPRINT_CHOICE : public wxChoice
{
public:
    FOOTPRINT_CHOICE( wxWidgetArgCompat aParent, int aId ) :
            wxChoice( aParent, aId )
    {
    }
};

#endif // FOOTPRINT_CHOICE_H
