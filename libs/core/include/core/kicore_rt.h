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
 * @file kicore_rt.h
 * Export macro for kicore_rt, the small shared library that owns the process-wide state of
 * the low level libraries (assert handler, log backend, geometry tuning).
 *
 * core_base and kimath are static libraries linked into kicommon, kigal and every kiface.
 * KiCad builds with hidden visibility, so state kept in those static libraries would be
 * duplicated per module: a backend installed by kicommon would not be seen by the copy inside
 * _pcbnew.kiface.  Keeping the state in one shared library gives a single instance.
 * Standalone (headless) builds link it statically and define KICORE_RT_STATIC.
 */

#ifndef KICORE_RT_H
#define KICORE_RT_H

#if defined( KICORE_RT_STATIC ) || defined( SWIG )
    #define KICORE_RT_API
#elif defined( _WIN32 )
    #if defined( KICORE_RT_DLL )
        #define KICORE_RT_API __declspec( dllexport )
    #else
        #define KICORE_RT_API __declspec( dllimport )
    #endif
#elif defined( __GNUC__ ) || defined( __clang__ )
    #define KICORE_RT_API __attribute__( ( visibility( "default" ) ) )
#else
    #define KICORE_RT_API
#endif

#endif // KICORE_RT_H
