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
 * @file registry.h
 *
 * wxRegKey: a key of the registry of Windows, with the part of the API of wxWidgets 3.2.4
 * (include/wx/msw/registry.h) the code of KiCad uses (reading values and enumerating keys).
 */

#pragma once

#ifndef _WIN32
#error "wx/msw/registry.h is for Windows only"
#endif

#include <wx/defs.h>
#include <wx/string.h>

struct HKEY__;


class wxRegKey
{
public:
    /// The predefined keys.
    enum StdKey
    {
        HKCR,
        HKCU,
        HKLM,
        HKUSR,
        HKPD,
        HKCC,
        HKDD,
        HKMAX
    };

    /// The view of the registry (32 or 64 bit) for a 64 bit system.
    enum WOW64ViewMode
    {
        WOW64ViewMode_Default,
        WOW64ViewMode_32,
        WOW64ViewMode_64
    };

    enum AccessMode
    {
        Read,
        Write
    };

    wxRegKey( StdKey aKeyParent, const wxString& aStrKey,
              WOW64ViewMode aViewMode = WOW64ViewMode_Default );
    ~wxRegKey();

    wxRegKey( const wxRegKey& ) = delete;
    wxRegKey& operator=( const wxRegKey& ) = delete;

    wxString GetName( bool aShortPrefix = true ) const;

    bool Exists() const;
    bool IsOpened() const { return m_hKey != nullptr; }
    bool Open( AccessMode aMode = Write );
    bool Close();

    bool HasValue( const wxString& aValue ) const;
    bool HasSubkey( const wxString& aKey ) const;
    bool HasSubkeys() const;
    bool HasValues() const;

    bool QueryValue( const wxString& aValue, wxString& aStrValue, bool aRaw = false ) const;
    bool QueryValue( const wxString& aValue, long* aLongValue ) const;
    bool QueryValue64( const wxString& aValue, wxLongLong_t* aLongValue ) const;

    /// The enumeration of the subkeys (aIndex is the state of the enumeration).
    bool GetFirstKey( wxString& aStrKeyName, long& aIndex );
    bool GetNextKey( wxString& aStrKeyName, long& aIndex ) const;

    /// The enumeration of the values.
    bool GetFirstValue( wxString& aStrValueName, long& aIndex );
    bool GetNextValue( wxString& aStrValueName, long& aIndex ) const;

private:
    /// Open the key for reading if it isn't open yet.
    bool openForRead() const;

    HKEY__*       m_hRootKey;
    wxString      m_strKey;
    WOW64ViewMode m_viewMode;
    mutable HKEY__* m_hKey = nullptr;
    mutable bool    m_readOnly = true;
};
