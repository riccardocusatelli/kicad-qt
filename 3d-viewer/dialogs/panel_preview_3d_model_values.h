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
 * @file panel_preview_3d_model_values.h
 * The values of the controls of the 3D model preview (PANEL_PREVIEW_3D_MODEL): the evaluation
 * of the expressions and the rotations, without the GUI toolkit.
 */

#pragma once

#include <cmath>

#include <base_units.h>
#include <libeval/numeric_evaluator.h>
#include <wx/string.h>

#ifndef MAX_ROTATION
#define MAX_ROTATION       180.0
#endif


/// The value of an expression typed in a control, or the text if it is not an expression.
inline wxString PreviewEvaluateText( const wxString& aValue )
{
    // NUMERIC_EVALUATOR doesn't handle UTF-8 multi-byte characters properly,
    // so skip evaluation if the string contains non-ASCII characters (e.g., degree symbols)
    for( wxUniChar c : aValue )
    {
        if( !c.IsAscii() )
            return aValue;
    }

    // Attempt to evaluate formula; if successful return result, otherwise return original
    NUMERIC_EVALUATOR eval( EDA_UNITS::UNSCALED );

    if( eval.Process( aValue ) )
        return eval.Result();

    return aValue;
}


/**
 * Normalize a rotation in degrees to the half-open range (-MAX_ROTATION, MAX_ROTATION].
 *
 * Matches the convention used by EDA_ANGLE::Normalize180(), so 198 maps to -162
 * and 540 maps to 180.
 */
inline double PreviewNormalizeRotation( double aRotation )
{
    double normalized = std::fmod( aRotation, 2.0 * MAX_ROTATION );

    if( normalized <= -MAX_ROTATION )
        normalized += 2.0 * MAX_ROTATION;
    else if( normalized > MAX_ROTATION )
        normalized -= 2.0 * MAX_ROTATION;

    if( normalized == -0.0 )
        normalized = 0.0;

    return normalized;
}


/// The rotation (in degrees) of the text of a control.
inline double PreviewRotationFromString( const wxString& aValue )
{
    double rotation = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::DEGREES,
                                                                 aValue );

    return PreviewNormalizeRotation( rotation );
}
