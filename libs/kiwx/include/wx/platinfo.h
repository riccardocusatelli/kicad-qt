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

#include <wx/string.h>

enum wxPortId
{
    wxPORT_UNKNOWN = 0,
    wxPORT_BASE = 1 << 0,
    wxPORT_MSW = 1 << 1,
    wxPORT_MOTIF = 1 << 2,
    wxPORT_GTK = 1 << 3,
    wxPORT_DFB = 1 << 4,
    wxPORT_X11 = 1 << 5,
    wxPORT_OSX = 1 << 6,
    wxPORT_MAC = wxPORT_OSX,
    wxPORT_COCOA = 1 << 7,
    wxPORT_WINCE = 1 << 8,
    wxPORT_QT = 1 << 9
};

enum wxBitness
{
    wxBITNESS_INVALID = -1,
    wxBITNESS_32,
    wxBITNESS_64,
    wxBITNESS_MAX
};

enum wxEndianness
{
    wxENDIAN_INVALID = -1,
    wxENDIAN_BIG,
    wxENDIAN_LITTLE,
    wxENDIAN_PDP,
    wxENDIAN_MAX
};


/// The platform information of a program without user interface (the wxBase port).
/// Header-only: no export macro, since MSVC would import the inline members from a DLL.
class wxPlatformInfo
{
public:
    static const wxPlatformInfo& Get()
    {
        static const wxPlatformInfo s_info;
        return s_info;
    }

    wxPortId GetPortId() const { return wxPORT_BASE; }
    wxString GetPortIdName() const { return wxT( "wxBase" ); }
    wxString GetPortIdShortName() const { return wxT( "base" ); }

    wxBitness GetBitness() const { return sizeof( void* ) == 8 ? wxBITNESS_64 : wxBITNESS_32; }
    wxString  GetBitnessName() const { return GetBitness() == wxBITNESS_64 ? wxT( "64 bit" ) : wxT( "32 bit" ); }
    bool      Is64Bit() const { return GetBitness() == wxBITNESS_64; }

    wxEndianness GetEndianness() const
    {
        const unsigned short probe = 1;
        return *reinterpret_cast<const unsigned char*>( &probe ) ? wxENDIAN_LITTLE : wxENDIAN_BIG;
    }

    wxString GetEndiannessName() const
    {
        return GetEndianness() == wxENDIAN_LITTLE ? wxT( "Little endian" ) : wxT( "Big endian" );
    }

    int GetToolkitMajorVersion() const { return 0; }
    int GetToolkitMinorVersion() const { return 0; }
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
