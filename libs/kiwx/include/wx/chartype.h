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

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>

// String literals: the strings are wide (UTF-32) as in the wxWidgets build of KiCad.
// As wxWidgets, through a helper: the argument is macro expanded before L is prepended.
#define wxT( x ) wxCONCAT_HELPER( L, x )
#define wxS( x ) wxCONCAT_HELPER( L, x )
#ifdef _T
#undef _T
#endif
#define _T( x ) wxT( x )
#define wxT_2( x ) L##x
#define wxTRANSLATE( x ) x
#define wxTRANSLATE_IN_CONTEXT( c, x ) x
#define wxCHAR_TYPE( x ) L##x

#ifndef _WIN32
#pragma GCC visibility pop
#endif
