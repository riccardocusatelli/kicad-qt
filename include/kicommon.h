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

#include <import_export.h>

#ifndef SWIG
	#if defined( KICOMMON_DLL )
		#define KICOMMON_API APIEXPORT
	#else
		#define KICOMMON_API APIIMPORT
	#endif
#else
#define KICOMMON_API
#endif

// The classes that the Qt build compiles into libkiqt_common (a static library, in each KIFACE and
// in kicad-qt) instead of kicommon: not exported, not imported
#if defined( KICAD_WX_FREE )
    #define KIQT_COMMON_API
#else
    #define KIQT_COMMON_API KICOMMON_API
#endif