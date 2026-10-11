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
#include <vector>

/// The array API of the wxWidgets WX_DEFINE_ARRAY() classes, on a std::vector.
template <typename T>
class kiwxArray : public std::vector<T>
{
public:
    using std::vector<T>::vector;

    size_t GetCount() const { return this->size(); }
    size_t Count() const { return this->size(); }
    bool   IsEmpty() const { return this->empty(); }
    void   Empty() { this->clear(); }
    void   Clear() { this->clear(); }
    void   Alloc( size_t aCount ) { this->reserve( aCount ); }
    void   Shrink() { this->shrink_to_fit(); }

    T&       Item( size_t aIndex ) { return ( *this )[aIndex]; }
    const T& Item( size_t aIndex ) const { return ( *this )[aIndex]; }
    T&       Last() { return this->back(); }
    const T& Last() const { return this->back(); }

    void Add( const T& aItem, size_t aCount = 1 ) { this->insert( this->end(), aCount, aItem ); }
    void Insert( const T& aItem, size_t aIndex, size_t aCount = 1 )
    {
        this->insert( this->begin() + aIndex, aCount, aItem );
    }

    int Index( const T& aItem, bool aFromEnd = false ) const
    {
        if( aFromEnd )
        {
            for( size_t i = this->size(); i > 0; --i )
            {
                if( ( *this )[i - 1] == aItem )
                    return static_cast<int>( i - 1 );
            }

            return wxNOT_FOUND;
        }

        auto it = std::find( this->begin(), this->end(), aItem );
        return it == this->end() ? wxNOT_FOUND : static_cast<int>( it - this->begin() );
    }

    void RemoveAt( size_t aIndex, size_t aCount = 1 )
    {
        this->erase( this->begin() + aIndex, this->begin() + aIndex + aCount );
    }

    void Remove( const T& aItem )
    {
        int i = Index( aItem );

        if( i != wxNOT_FOUND )
            RemoveAt( i );
    }
};


/// A WX_DEFINE_SORTED_ARRAY() class: kept sorted by a comparison function.
template <typename T>
class kiwxSortedArray : public kiwxArray<T>
{
public:
    typedef int ( *CMPFUNC )( T, T );

    explicit kiwxSortedArray( CMPFUNC aCompare ) : m_compare( aCompare ) {}

    /// Inserts before the equal items (as wxWidgets), returns the index.
    size_t Add( const T& aItem )
    {
        auto it = std::lower_bound( this->begin(), this->end(), aItem,
                                    [this]( const T& a, const T& b )
                                    {
                                        return m_compare( a, b ) < 0;
                                    } );
        size_t n = it - this->begin();
        this->insert( it, aItem );
        return n;
    }

private:
    CMPFUNC m_compare;
};


#define WX_DEFINE_ARRAY( T, name )                                                               \
    class name : public kiwxArray<T>                                                             \
    {                                                                                            \
    public:                                                                                      \
        using kiwxArray<T>::kiwxArray;                                                           \
    }
#define WX_DEFINE_ARRAY_PTR( T, name ) WX_DEFINE_ARRAY( T, name )
#define WX_DEFINE_SORTED_ARRAY( T, name )                                                        \
    class name : public kiwxSortedArray<T>                                                       \
    {                                                                                            \
    public:                                                                                      \
        using kiwxSortedArray<T>::kiwxSortedArray;                                               \
    }

typedef kiwxArray<void*> wxArrayPtrVoid;

#ifndef _WIN32
#pragma GCC visibility pop
#endif
