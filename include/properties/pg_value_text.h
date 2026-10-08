/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PG_VALUE_TEXT_H
#define PG_VALUE_TEXT_H

/**
 * @file pg_value_text.h
 * The texts of the values of the properties of the property grids, and the values given by
 * their editors: the toolkit-independent part of the properties of the grid
 * (common/properties/pg_properties.cpp and pg_editors.cpp with wxWidgets, qt/common/properties
 * with Qt).
 */

#include <wx/string.h>
#include <wx/variant.h>

#include <origin_transforms.h>

class EDA_DRAW_FRAME;
class UNIT_BINDER;


/// The quantity edited by the unit editor of a property (the unit binder of its text).
enum class PG_UNIT_KIND
{
    DISTANCE,   ///< A length or a coordinate (the coordinate type of the property)
    AREA,
    ANGLE,      ///< In degrees, scaled by the property (PT_DECIDEGREE)
    TIME,
    OTHER       ///< A number of internal units
};


/// The text of a distance (an int, a std::optional<int>, a long long or a double), in the user
/// units of the frame and with its origin transforms.
wxString PGDistanceToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant,
                             ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType );

/// The text of an area, in the user units of the frame.
wxString PGAreaToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant );

/// The text of a time (a delay), in the user units of the frame.
wxString PGTimeToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant );

/// The text of a scale-free ratio (a double or a std::optional<double>).
wxString PGRatioToString( const wxVariant& aVariant );

/// The text of an angle (a double scaled by @a aScale, an EDA_ANGLE or a long), in degrees.
wxString PGAngleToString( const wxVariant& aVariant, double aScale );

/**
 * The value of an angle from its text, in degrees.
 *
 * @return true if the value changed.
 */
bool PGAngleFromString( wxVariant& aVariant, const wxString& aText, double aScale );

/// The text of a string property (an unescaped string).
wxString PGStringToString( const wxVariant& aVariant );

/// The value of a string property from its text (an escaped string).
wxVariant PGStringFromString( const wxString& aText );

/// The text of a color property (the CSS string of the color).
wxString PGColorToString( const wxVariant& aVariant );

/// The value of a color property from its text (a CSS string).
wxVariant PGColorFromString( const wxString& aText );

/**
 * Show the value of a property in the unit binder of its unit editor.
 *
 * @param aUnspecified is true when the selection has different values.
 */
void PGUnitBinderShowValue( UNIT_BINDER& aBinder, const wxVariant& aValue, PG_UNIT_KIND aKind,
                            double aAngleScale, bool aUnspecified );

/**
 * The value of a property from the text of the unit binder of its unit editor.
 *
 * @param aText is the text of the editor.
 * @return true if the value changed.
 */
bool PGUnitBinderGetValue( UNIT_BINDER& aBinder, const wxString& aText, wxVariant& aVariant,
                           PG_UNIT_KIND aKind, double aAngleScale );

/**
 * The value of a ratio property from the text of its editor.
 *
 * @param aNormalizedText is set to the text of the new value (when it changed).
 * @return true if the value changed.
 */
bool PGRatioFromText( const wxString& aText, wxVariant& aVariant, wxString& aNormalizedText );

#endif // PG_VALUE_TEXT_H
