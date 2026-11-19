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
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// The CoreFoundation references of wxWidgets (wx/osx/core/cfref.h, cfdataref.h) used by the
// macOS code of kiplatform: a reference owns its object and releases it.

#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include <utility>


template <typename REF>
class wxCFRef
{
public:
    wxCFRef() = default;

    /// Takes the ownership of @a aRef (as wxWidgets).
    explicit wxCFRef( REF aRef ) : m_ref( aRef ) {}

    wxCFRef( const wxCFRef& aOther ) : m_ref( aOther.m_ref )
    {
        if( m_ref )
            CFRetain( m_ref );
    }

    wxCFRef( wxCFRef&& aOther ) noexcept : m_ref( aOther.m_ref ) { aOther.m_ref = nullptr; }

    wxCFRef& operator=( wxCFRef aOther )
    {
        std::swap( m_ref, aOther.m_ref );
        return *this;
    }

    ~wxCFRef()
    {
        if( m_ref )
            CFRelease( m_ref );
    }

    operator REF() const { return m_ref; }
    REF get() const { return m_ref; }

private:
    REF m_ref = nullptr;
};


class wxCFDataRef : public wxCFRef<CFDataRef>
{
public:
    wxCFDataRef() = default;

    /// Takes the ownership of @a aRef (as wxWidgets).
    explicit wxCFDataRef( CFDataRef aRef ) : wxCFRef<CFDataRef>( aRef ) {}

    wxCFDataRef( const UInt8* aData, CFIndex aLength ) :
            wxCFRef<CFDataRef>( CFDataCreate( kCFAllocatorDefault, aData, aLength ) )
    {
    }

    CFIndex GetLength() const { return get() ? CFDataGetLength( get() ) : 0; }
};
