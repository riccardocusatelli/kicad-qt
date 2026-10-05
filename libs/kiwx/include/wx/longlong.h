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
#pragma GCC visibility push( default )

#include <wx/string.h>

#include <type_traits>



/// A 64 bit integer with the wxLongLong API.
template <typename T>
class kiwxLongLongBase
{
public:
    constexpr kiwxLongLongBase( T aValue = 0 ) : m_value( aValue ) {}
    kiwxLongLongBase( long aHi, unsigned long aLo ) :
            m_value( static_cast<T>( ( static_cast<unsigned long long>( aHi ) << 32 ) | ( aLo & 0xFFFFFFFFUL ) ) )
    {
    }

    constexpr T GetValue() const { return m_value; }
    long          GetHi() const { return static_cast<long>( m_value >> 32 ); }
    unsigned long GetLo() const { return static_cast<unsigned long>( m_value & 0xFFFFFFFF ); }
    long          ToLong() const { return static_cast<long>( m_value ); }
    unsigned long ToULong() const { return static_cast<unsigned long>( m_value ); }
    double        ToDouble() const { return static_cast<double>( m_value ); }
    wxString      ToString() const
    {
        if constexpr( std::is_signed_v<T> )
            return wxString::Format( wxT( "%lld" ), static_cast<long long>( m_value ) );
        else
            return wxString::Format( wxT( "%llu" ), static_cast<unsigned long long>( m_value ) );
    }

    kiwxLongLongBase& Assign( double aValue )
    {
        m_value = static_cast<T>( aValue );
        return *this;
    }

    /// Between signed and unsigned, as the wxWidgets native long long classes.
    template <typename U, typename = std::enable_if_t<!std::is_same_v<U, T>>>
    kiwxLongLongBase& operator=( const kiwxLongLongBase<U>& aOther )
    {
        m_value = static_cast<T>( aOther.GetValue() );
        return *this;
    }
    kiwxLongLongBase Abs() const { return m_value < 0 ? -m_value : m_value; }

    operator T() const { return m_value; }

    kiwxLongLongBase& operator+=( const kiwxLongLongBase& o ) { m_value += o.m_value; return *this; }
    kiwxLongLongBase& operator-=( const kiwxLongLongBase& o ) { m_value -= o.m_value; return *this; }
    kiwxLongLongBase& operator*=( const kiwxLongLongBase& o ) { m_value *= o.m_value; return *this; }
    kiwxLongLongBase& operator/=( const kiwxLongLongBase& o ) { m_value /= o.m_value; return *this; }

private:
    T m_value;
};

typedef kiwxLongLongBase<long long>          wxLongLong;
typedef kiwxLongLongBase<unsigned long long> wxULongLong;

inline const wxULongLong wxInvalidSize( static_cast<wxULongLong_t>( -1 ) );

#pragma GCC visibility pop
