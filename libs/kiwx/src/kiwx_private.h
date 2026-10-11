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
 * @file kiwx_private.h
 *
 * Helpers shared by the kiwx sources (not part of the wx API).
 */

#pragma once

#include <wx/arrstr.h>
#include <wx/mstream.h>
#include <wx/txtstrm.h>

#include <cerrno>
#include <filesystem>
#include <string>
#include <system_error>

namespace kiwx
{

/// The std::filesystem path of a file name (UTF-8 on Unix, UTF-16 on Windows).
inline std::filesystem::path FsPath( const wxString& aPath )
{
#ifdef _WIN32
    return std::filesystem::path( aPath.ToStdWstring() );
#else
    return std::filesystem::path( aPath.utf8_string() );
#endif
}


inline wxString FromFsPath( const std::filesystem::path& aPath )
{
#ifdef _WIN32
    return wxString( aPath.wstring() );
#else
    return wxString::FromUTF8( aPath.native() );
#endif
}


/// errno set from a std::filesystem error (for wxSysErrorCode(), as the C library calls).
inline void SetErrno( const std::error_code& aError )
{
    errno = aError.default_error_condition().value();
}


/// Rename @a aFrom to @a aTo, replacing @a aTo if it exists (atomically where possible).
bool RenameReplacing( const wxString& aFrom, const wxString& aTo );

/// The lines of a captured output, as wxWidgets reads them (wxTextInputStream::ReadLine()).
inline void OutputLines( const std::string& aData, wxArrayString& aLines )
{
    wxMemoryInputStream in( aData.data(), aData.size() );
    wxTextInputStream   text( in );

    while( in.CanRead() && !in.Eof() )
    {
        wxString line = text.ReadLine();

        if( in.Eof() && line.empty() )
            break;

        aLines.Add( line );
    }
}

} // namespace kiwx
