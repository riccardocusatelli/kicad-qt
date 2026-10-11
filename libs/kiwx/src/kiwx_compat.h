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
 * @file kiwx_compat.h
 *
 * The few C library functions that are POSIX only, with their Windows (UCRT) equivalents.
 */

#pragma once

#include <cstring>
#include <ctime>

namespace kiwx
{

inline void LocalTime( time_t aTime, struct tm& aOut )
{
#ifdef _WIN32
    localtime_s( &aOut, &aTime );
#else
    localtime_r( &aTime, &aOut );
#endif
}


inline void GmTime( time_t aTime, struct tm& aOut )
{
#ifdef _WIN32
    gmtime_s( &aOut, &aTime );
#else
    gmtime_r( &aTime, &aOut );
#endif
}


/// The offset of the local time from UTC at @a aTime, in seconds (east positive).
inline long GmtOffset( time_t aTime )
{
    struct tm local;
    LocalTime( aTime, local );

#ifdef _WIN32
    return static_cast<long>( _mkgmtime( &local ) - aTime );
#else
    return local.tm_gmtoff;
#endif
}


/// The text of the C library error @a aErrno.
inline const char* StrError( int aErrno, char* aBuf, size_t aLen )
{
#ifdef _WIN32
    strerror_s( aBuf, aLen, aErrno );
    return aBuf;
#elif defined( __GLIBC__ ) && defined( _GNU_SOURCE )
    return strerror_r( aErrno, aBuf, aLen );
#else
    if( strerror_r( aErrno, aBuf, aLen ) != 0 )
        aBuf[0] = 0;

    return aBuf;
#endif
}

} // namespace kiwx
