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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>


/**
 * A shared buffer of characters (the result of string conversions).  Copies share the data,
 * as in wxWidgets.  A default-constructed buffer is null: data() returns nullptr.
 */
template <typename T>
class wxScopedCharTypeBuffer
{
public:
    typedef T CharType;

    wxScopedCharTypeBuffer() = default;

    static wxScopedCharTypeBuffer CreateNonOwned( const T* aStr, size_t aLen = size_t( -1 ) )
    {
        return CreateOwned( aStr, aLen );
    }

    static wxScopedCharTypeBuffer CreateOwned( const T* aStr, size_t aLen = size_t( -1 ) )
    {
        wxScopedCharTypeBuffer buf;

        if( aStr )
            buf.m_str = std::make_shared<std::basic_string<T>>(
                    aStr, aLen == size_t( -1 ) ? std::char_traits<T>::length( aStr ) : aLen );

        return buf;
    }

    const T* data() const { return m_str ? m_str->c_str() : nullptr; }
    T*       data() { return m_str ? m_str->data() : nullptr; }
    size_t   length() const { return m_str ? m_str->size() : 0; }

    operator const T*() const { return data(); }

    T operator[]( size_t aIndex ) const { return ( *m_str )[aIndex]; }

    void reset() { m_str.reset(); }

protected:
    std::shared_ptr<std::basic_string<T>> m_str;
};


template <typename T>
class wxCharTypeBuffer : public wxScopedCharTypeBuffer<T>
{
public:
    wxCharTypeBuffer() = default;

    wxCharTypeBuffer( const T* aStr )
    {
        if( aStr )
            this->m_str = std::make_shared<std::basic_string<T>>( aStr );
    }

    wxCharTypeBuffer( const T* aStr, size_t aLen )
    {
        if( aStr )
            this->m_str = std::make_shared<std::basic_string<T>>( aStr, aLen );
    }

    explicit wxCharTypeBuffer( size_t aLen )
    {
        this->m_str = std::make_shared<std::basic_string<T>>( aLen, T( 0 ) );
    }

    wxCharTypeBuffer( const wxScopedCharTypeBuffer<T>& aOther ) :
            wxScopedCharTypeBuffer<T>( aOther )
    {
    }

    explicit wxCharTypeBuffer( const std::basic_string<T>& aStr )
    {
        this->m_str = std::make_shared<std::basic_string<T>>( aStr );
    }

    bool extend( size_t aLen )
    {
        if( !this->m_str )
            this->m_str = std::make_shared<std::basic_string<T>>();

        this->m_str->resize( aLen );
        return true;
    }

    bool shrink( size_t aLen ) { return extend( aLen ); }
};


typedef wxScopedCharTypeBuffer<char>    wxScopedCharBuffer;
typedef wxScopedCharTypeBuffer<wchar_t> wxScopedWCharBuffer;
typedef wxCharTypeBuffer<char>          wxCharBuffer;
typedef wxCharTypeBuffer<wchar_t>       wxWCharBuffer;
typedef wxCharBuffer                    wxWritableCharBuffer;
typedef wxWCharBuffer                   wxWritableWCharBuffer;
typedef wxWCharBuffer                   wxWxCharBuffer;


/**
 * A growable buffer of bytes.  Copies share the data, as in wxWidgets.
 */
class wxMemoryBuffer
{
public:
    explicit wxMemoryBuffer( size_t aSize = 1024 ) : m_data( std::make_shared<DATA>() )
    {
        m_data->bytes.reserve( aSize );
    }

    void*  GetData() const { return m_data->bytes.data(); }
    size_t GetBufSize() const { return std::max( m_data->bytes.capacity(), m_data->bytes.size() ); }
    size_t GetDataLen() const { return m_data->len; }

    bool IsEmpty() const { return m_data->len == 0; }

    void SetBufSize( size_t aSize )
    {
        if( aSize > m_data->bytes.size() )
            m_data->bytes.resize( aSize );
    }

    void SetDataLen( size_t aLen )
    {
        SetBufSize( aLen );
        m_data->len = aLen;
    }

    void Clear() { SetDataLen( 0 ); }

    void* GetWriteBuf( size_t aSize )
    {
        SetBufSize( aSize );
        return m_data->bytes.data();
    }

    void UngetWriteBuf( size_t aSize ) { SetDataLen( aSize ); }

    void* GetAppendBuf( size_t aSize )
    {
        SetBufSize( m_data->len + aSize );
        return m_data->bytes.data() + m_data->len;
    }

    void UngetAppendBuf( size_t aSize ) { SetDataLen( m_data->len + aSize ); }

    void AppendByte( char aByte )
    {
        SetBufSize( m_data->len + 1 );
        m_data->bytes[m_data->len++] = aByte;
    }

    void AppendData( const void* aSrc, size_t aLen )
    {
        if( !aLen )
            return;

        SetBufSize( m_data->len + aLen );
        std::memcpy( m_data->bytes.data() + m_data->len, aSrc, aLen );
        m_data->len += aLen;
    }

    operator const char*() const { return m_data->bytes.data(); }

    char& operator[]( size_t aIndex ) { return m_data->bytes[aIndex]; }

    void* release()
    {
        void* copy = std::malloc( std::max<size_t>( m_data->len, 1 ) );
        std::memcpy( copy, m_data->bytes.data(), m_data->len );
        m_data = std::make_shared<DATA>();
        return copy;
    }

private:
    struct DATA
    {
        std::vector<char> bytes;
        size_t            len = 0;
    };

    std::shared_ptr<DATA> m_data;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
