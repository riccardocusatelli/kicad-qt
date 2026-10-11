/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/math.h, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>

#include <climits>
#include <cmath>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef METRIC_CONVERSION_CONSTANT
#define METRIC_CONVERSION_CONSTANT ( 1 / 25.4 )
#endif

#ifndef mm2inches
#define mm2inches ( METRIC_CONVERSION_CONSTANT )
#endif

#ifndef inches2mm
#define inches2mm ( 1 / ( mm2inches ) )
#endif

#ifndef mm2twips
#define mm2twips ( METRIC_CONVERSION_CONSTANT * 1440 )
#endif

#ifndef twips2mm
#define twips2mm ( 25.4 / 1440 )
#endif

#ifndef mm2pt
#define mm2pt ( METRIC_CONVERSION_CONSTANT * 72 )
#endif

#ifndef pt2mm
#define pt2mm ( 1 / ( METRIC_CONVERSION_CONSTANT * 72 ) )
#endif

#define wxFinite( x ) std::isfinite( x )
#define wxIsNaN( x ) std::isnan( x )

inline bool wxIsSameDouble( double x, double y )
{
    return x == y;
}

inline bool wxIsNullDouble( double x )
{
    return wxIsSameDouble( x, 0. );
}

inline int wxRound( double x )
{
    wxASSERT_MSG( x > double( INT_MIN ) - 0.5 && x < double( INT_MAX ) + 0.5,
                  "argument out of supported range" );
    return int( std::lround( x ) );
}

inline int wxRound( float x )
{
    wxASSERT_MSG( x > float( INT_MIN ) && x < float( INT_MAX ), "argument out of supported range" );
    return int( std::lround( x ) );
}

inline int wxRound( long double x )
{
    return wxRound( double( x ) );
}

inline double wxDegToRad( double deg )
{
    return ( deg * M_PI ) / 180.0;
}

inline double wxRadToDeg( double rad )
{
    return ( rad * 180.0 ) / M_PI;
}

#define wxMulDivInt32( a, b, c ) ( wxRound( ( a ) * ( ( (double) b ) / ( (double) c ) ) ) )

#ifndef _WIN32
#pragma GCC visibility pop
#endif
