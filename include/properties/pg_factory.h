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


#ifndef PG_FACTORY_H
#define PG_FACTORY_H

/**
 * @file pg_factory.h
 * The properties of the property grid of the properties panels, as the toolkit-independent code
 * of the panels makes and updates them.  Each GUI toolkit implements these functions
 * (common/properties/pg_properties.cpp with wxWidgets, qt/common/properties with Qt).
 */

#include <functional>
#include <vector>
#include <utility>

#include <wx/string.h>
#include <wx/variant.h>

#include <gal/color4d.h>
#include <properties/property_choices.h>
#include <toolkit/pg_types.h>

class EDA_DRAW_FRAME;
class PROPERTY_BASE;


/// The custom editors of the properties that the panels choose for some properties.
enum class PG_EDITOR
{
    FPID,      ///< A footprint identifier, with a button opening the footprint chooser
    URL        ///< A link or a file (the datasheets), with a button opening it
};


/**
 * Make the property of the grid showing @a aProperty, as its display type says.
 */
TOOLKIT_PG_PROPERTY* PGPropertyFactory( const PROPERTY_BASE* aProperty, EDA_DRAW_FRAME* aFrame );

/**
 * Make a property choosing one of @a aChoices (as their labels and values), each shown with a
 * color swatch (the colors of the layers).
 *
 * @param aColorFunc gives the color of a value.
 */
TOOLKIT_PG_PROPERTY* PGColorEnumPropertyFactory( const PROPERTY_BASE* aProperty,
                                                 const std::vector<std::pair<wxString, int>>& aChoices,
                                                 std::function<KIGFX::COLOR4D( int aValue )> aColorFunc );

/// Edit @a aProperty with a custom editor of the frame @a aFrame.
void PGSetEditor( TOOLKIT_PG_PROPERTY* aProperty, PG_EDITOR aEditor, EDA_DRAW_FRAME* aFrame );

/**
 * Set the background color of the swatch of a color property (the color under a transparent
 * color).
 *
 * @return false if @a aProperty isn't a color property.
 */
bool PGSetColorBackground( TOOLKIT_PG_PROPERTY* aProperty, const KIGFX::COLOR4D& aBackground );

/// @return the name of the property (the name of the PROPERTY_BASE it shows).
wxString PGGetName( const TOOLKIT_PG_PROPERTY* aProperty );

/// Set the choices of an enumeration property.
void PGSetChoices( TOOLKIT_PG_PROPERTY* aProperty, const PROPERTY_CHOICES& aChoices );

/// Set the value of a property (a null variant shows the different values of a selection).
void PGSetValue( TOOLKIT_PG_PROPERTY* aProperty, const wxVariant& aValue );

/// Allow or forbid the edition of a property.
void PGSetReadOnly( TOOLKIT_PG_PROPERTY* aProperty, bool aReadOnly );

#endif // PG_FACTORY_H
