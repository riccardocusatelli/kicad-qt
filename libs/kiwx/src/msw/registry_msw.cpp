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
 * @file registry_msw.cpp
 *
 * wxRegKey on Windows: the functions of the registry of Windows, as wxWidgets 3.2.4
 * (src/msw/registry.cpp).
 */

#include <wx/msw/registry.h>

#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


namespace
{
HKEY rootKey( wxRegKey::StdKey aKey )
{
    switch( aKey )
    {
    case wxRegKey::HKCR: return HKEY_CLASSES_ROOT;
    case wxRegKey::HKCU: return HKEY_CURRENT_USER;
    case wxRegKey::HKLM: return HKEY_LOCAL_MACHINE;
    case wxRegKey::HKUSR: return HKEY_USERS;
    case wxRegKey::HKPD: return HKEY_PERFORMANCE_DATA;
    case wxRegKey::HKCC: return HKEY_CURRENT_CONFIG;
    case wxRegKey::HKDD: return HKEY_DYN_DATA;
    default: return HKEY_LOCAL_MACHINE;
    }
}


const wchar_t* rootName( HKEY aKey )
{
    if( aKey == HKEY_CLASSES_ROOT )
        return L"HKCR";
    else if( aKey == HKEY_CURRENT_USER )
        return L"HKCU";
    else if( aKey == HKEY_LOCAL_MACHINE )
        return L"HKLM";
    else if( aKey == HKEY_USERS )
        return L"HKU";
    else if( aKey == HKEY_CURRENT_CONFIG )
        return L"HKCC";

    return L"HK??";
}


REGSAM viewFlags( wxRegKey::WOW64ViewMode aMode )
{
    switch( aMode )
    {
    case wxRegKey::WOW64ViewMode_32: return KEY_WOW64_32KEY;
    case wxRegKey::WOW64ViewMode_64: return KEY_WOW64_64KEY;
    default: return 0;
    }
}
} // namespace


wxRegKey::wxRegKey( StdKey aKeyParent, const wxString& aStrKey, WOW64ViewMode aViewMode ) :
        m_hRootKey( rootKey( aKeyParent ) ),
        m_strKey( aStrKey ),
        m_viewMode( aViewMode )
{
    // No trailing separator (as wxWidgets)
    while( m_strKey.EndsWith( wxS( "\\" ) ) )
        m_strKey.RemoveLast();
}


wxRegKey::~wxRegKey()
{
    Close();
}


wxString wxRegKey::GetName( bool aShortPrefix ) const
{
    wxString name( rootName( m_hRootKey ) );

    if( !m_strKey.empty() )
        name << wxS( "\\" ) << m_strKey;

    return name;
}


bool wxRegKey::Exists() const
{
    if( IsOpened() )
        return true;

    HKEY key;

    if( ::RegOpenKeyExW( m_hRootKey, m_strKey.wc_str(), 0, KEY_READ | viewFlags( m_viewMode ),
                         &key ) != ERROR_SUCCESS )
    {
        return false;
    }

    ::RegCloseKey( key );
    return true;
}


bool wxRegKey::Open( AccessMode aMode )
{
    if( IsOpened() )
    {
        if( aMode == Read || !m_readOnly )
            return true;

        Close();
    }

    REGSAM access = ( aMode == Read ? KEY_READ : KEY_ALL_ACCESS ) | viewFlags( m_viewMode );
    HKEY   key;

    if( ::RegOpenKeyExW( m_hRootKey, m_strKey.wc_str(), 0, access, &key ) != ERROR_SUCCESS )
        return false;

    m_hKey = key;
    m_readOnly = ( aMode == Read );
    return true;
}


bool wxRegKey::Close()
{
    if( !IsOpened() )
        return true;

    bool ok = ::RegCloseKey( m_hKey ) == ERROR_SUCCESS;
    m_hKey = nullptr;
    return ok;
}


bool wxRegKey::openForRead() const
{
    return const_cast<wxRegKey*>( this )->Open( Read );
}


bool wxRegKey::HasValue( const wxString& aValue ) const
{
    if( !openForRead() )
        return false;

    return ::RegQueryValueExW( m_hKey, aValue.wc_str(), nullptr, nullptr, nullptr, nullptr )
           == ERROR_SUCCESS;
}


bool wxRegKey::HasSubkey( const wxString& aKey ) const
{
    if( !openForRead() )
        return false;

    HKEY key;

    if( ::RegOpenKeyExW( m_hKey, aKey.wc_str(), 0, KEY_READ, &key ) != ERROR_SUCCESS )
        return false;

    ::RegCloseKey( key );
    return true;
}


bool wxRegKey::HasSubkeys() const
{
    if( !openForRead() )
        return false;

    DWORD subKeys = 0;

    if( ::RegQueryInfoKeyW( m_hKey, nullptr, nullptr, nullptr, &subKeys, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr ) != ERROR_SUCCESS )
    {
        return false;
    }

    return subKeys > 0;
}


bool wxRegKey::HasValues() const
{
    if( !openForRead() )
        return false;

    DWORD values = 0;

    if( ::RegQueryInfoKeyW( m_hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            &values, nullptr, nullptr, nullptr, nullptr ) != ERROR_SUCCESS )
    {
        return false;
    }

    return values > 0;
}


bool wxRegKey::QueryValue( const wxString& aValue, wxString& aStrValue, bool aRaw ) const
{
    if( !openForRead() )
        return false;

    DWORD type = 0;
    DWORD size = 0;

    if( ::RegQueryValueExW( m_hKey, aValue.wc_str(), nullptr, &type, nullptr, &size )
        != ERROR_SUCCESS )
    {
        return false;
    }

    if( type != REG_SZ && type != REG_EXPAND_SZ )
        return false;

    std::vector<wchar_t> buffer( size / sizeof( wchar_t ) + 1, 0 );

    if( ::RegQueryValueExW( m_hKey, aValue.wc_str(), nullptr, &type,
                            reinterpret_cast<LPBYTE>( buffer.data() ), &size ) != ERROR_SUCCESS )
    {
        return false;
    }

    aStrValue = wxString( buffer.data() );

    // The environment variables of an expandable string (unless the raw value is asked)
    if( type == REG_EXPAND_SZ && !aRaw )
    {
        DWORD length = ::ExpandEnvironmentStringsW( buffer.data(), nullptr, 0 );

        if( length > 0 )
        {
            std::vector<wchar_t> expanded( length + 1, 0 );
            ::ExpandEnvironmentStringsW( buffer.data(), expanded.data(), length );
            aStrValue = wxString( expanded.data() );
        }
    }

    return true;
}


bool wxRegKey::QueryValue( const wxString& aValue, long* aLongValue ) const
{
    if( !openForRead() || !aLongValue )
        return false;

    DWORD type = 0;
    DWORD value = 0;
    DWORD size = sizeof( value );

    if( ::RegQueryValueExW( m_hKey, aValue.wc_str(), nullptr, &type,
                            reinterpret_cast<LPBYTE>( &value ), &size ) != ERROR_SUCCESS
        || type != REG_DWORD )
    {
        return false;
    }

    *aLongValue = static_cast<long>( value );
    return true;
}


bool wxRegKey::QueryValue64( const wxString& aValue, wxLongLong_t* aLongValue ) const
{
    if( !openForRead() || !aLongValue )
        return false;

    DWORD     type = 0;
    ULONGLONG value = 0;
    DWORD     size = sizeof( value );

    if( ::RegQueryValueExW( m_hKey, aValue.wc_str(), nullptr, &type,
                            reinterpret_cast<LPBYTE>( &value ), &size ) != ERROR_SUCCESS
        || type != REG_QWORD )
    {
        return false;
    }

    *aLongValue = static_cast<wxLongLong_t>( value );
    return true;
}


bool wxRegKey::GetFirstKey( wxString& aStrKeyName, long& aIndex )
{
    if( !Open( Read ) )
        return false;

    aIndex = 0;
    return GetNextKey( aStrKeyName, aIndex );
}


bool wxRegKey::GetNextKey( wxString& aStrKeyName, long& aIndex ) const
{
    if( !IsOpened() || aIndex < 0 )
        return false;

    // The longest name of a key is 255 characters
    wchar_t name[256];
    DWORD   length = 256;

    LONG status = ::RegEnumKeyExW( m_hKey, static_cast<DWORD>( aIndex++ ), name, &length,
                                   nullptr, nullptr, nullptr, nullptr );

    if( status != ERROR_SUCCESS )
    {
        aIndex = -1;
        return false;
    }

    aStrKeyName = wxString( name, length );
    return true;
}


bool wxRegKey::GetFirstValue( wxString& aStrValueName, long& aIndex )
{
    if( !Open( Read ) )
        return false;

    aIndex = 0;
    return GetNextValue( aStrValueName, aIndex );
}


bool wxRegKey::GetNextValue( wxString& aStrValueName, long& aIndex ) const
{
    if( !IsOpened() || aIndex < 0 )
        return false;

    // The longest name of a value is 16383 characters
    std::vector<wchar_t> name( 16384, 0 );
    DWORD                length = static_cast<DWORD>( name.size() );

    LONG status = ::RegEnumValueW( m_hKey, static_cast<DWORD>( aIndex++ ), name.data(), &length,
                                   nullptr, nullptr, nullptr, nullptr );

    if( status != ERROR_SUCCESS )
    {
        aIndex = -1;
        return false;
    }

    aStrValueName = wxString( name.data(), length );
    return true;
}
