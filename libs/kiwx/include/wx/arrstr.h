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

#include <initializer_list>
#include <vector>


/**
 * An array of strings (a std::vector with the wxArrayString API).  A sorted array
 * (wxSortedArrayString, or constructed with autoSort) keeps its strings in order.
 */
class wxArrayString : public std::vector<wxString>
{
public:
    typedef int ( *CompareFunction )( const wxString& aFirst, const wxString& aSecond );
    typedef int ( *CompareFunction2 )( wxString* aFirst, wxString* aSecond );

    wxArrayString() = default;
    explicit wxArrayString( int aAutoSort ) : m_sorted( aAutoSort != 0 ) {}
    wxArrayString( size_t aCount, const char** aStrings );
    wxArrayString( size_t aCount, const wchar_t** aStrings );
    wxArrayString( size_t aCount, const wxString* aStrings );

    template <typename U>
    wxArrayString( std::initializer_list<U> aList )
    {
        for( const U& item : aList )
            push_back( wxString( item ) );
    }

    template <typename IT,
              typename = typename std::iterator_traits<IT>::iterator_category>
    wxArrayString( IT aFirst, IT aLast ) : std::vector<wxString>( aFirst, aLast )
    {
    }

    size_t GetCount() const { return size(); }
    size_t Count() const { return size(); }
    bool   IsEmpty() const { return empty(); }
    void   Empty() { clear(); }
    void   Clear() { clear(); }
    void   Alloc( size_t aCount ) { reserve( aCount ); }
    void   Shrink() { shrink_to_fit(); }
    void   SetCount( size_t aCount ) { resize( aCount ); }

    wxString&       Item( size_t aIndex ) { return ( *this )[aIndex]; }
    const wxString& Item( size_t aIndex ) const { return ( *this )[aIndex]; }
    wxString&       Last() { return back(); }
    const wxString& Last() const { return back(); }

    int    Index( const wxString& aStr, bool aCaseSensitive = true, bool aFromEnd = false ) const;
    size_t Add( const wxString& aStr, size_t aCopies = 1 );
    void   Insert( const wxString& aStr, size_t aIndex, size_t aCopies = 1 );
    void   Remove( const wxString& aStr );
    void   RemoveAt( size_t aIndex, size_t aCount = 1 );

    void Sort( bool aReverseOrder = false );
    void Sort( CompareFunction aCompare );
    void Sort( CompareFunction2 aCompare );

    bool operator==( const wxArrayString& aOther ) const
    {
        return static_cast<const std::vector<wxString>&>( *this )
               == static_cast<const std::vector<wxString>&>( aOther );
    }

    bool operator!=( const wxArrayString& aOther ) const { return !( *this == aOther ); }

protected:
    bool m_sorted = false;
};


class wxSortedArrayString : public wxArrayString
{
public:
    wxSortedArrayString() : wxArrayString( 1 ) {}
    explicit wxSortedArrayString( CompareFunction aCompare ) : wxArrayString( 1 ), m_compare( aCompare ) {}
    wxSortedArrayString( const wxArrayString& aArray );

    size_t Add( const wxString& aStr, size_t aCopies = 1 );
    int    Index( const wxString& aStr, bool aCaseSensitive = true, bool aFromEnd = false ) const;

private:
    CompareFunction m_compare = nullptr;
};


typedef wxSortedArrayString wxArrayStringSorted;

/// Split @a aStr at @a aSep; @a aSep preceded by @a aEscape is not a separator.
wxArrayString wxSplit( const wxString& aStr, const wxChar aSep, const wxChar aEscape = wxT( '\\' ) );

/// Join with @a aSep, escaping the separators in the strings with @a aEscape.
wxString wxJoin( const wxArrayString& aArray, const wxChar aSep,
                 const wxChar aEscape = wxT( '\\' ) );


/// An array of ints (wxArrayInt of wxWidgets).
class wxArrayInt : public std::vector<int>
{
public:
    using std::vector<int>::vector;

    size_t GetCount() const { return size(); }
    size_t Count() const { return size(); }
    bool   IsEmpty() const { return empty(); }
    void   Empty() { clear(); }
    void   Clear() { clear(); }
    void   Alloc( size_t aCount ) { reserve( aCount ); }
    size_t Add( int aValue, size_t aCopies = 1 )
    {
        insert( end(), aCopies, aValue );
        return size() - aCopies;
    }
    void Insert( int aValue, size_t aIndex, size_t aCopies = 1 )
    {
        insert( begin() + aIndex, aCopies, aValue );
    }
    void RemoveAt( size_t aIndex, size_t aCount = 1 )
    {
        erase( begin() + aIndex, begin() + aIndex + aCount );
    }
    void Remove( int aValue )
    {
        int i = Index( aValue );

        if( i != wxNOT_FOUND )
            RemoveAt( i );
    }
    int Index( int aValue, bool aFromEnd = false ) const;
    int&       Item( size_t aIndex ) { return ( *this )[aIndex]; }
    const int& Item( size_t aIndex ) const { return ( *this )[aIndex]; }
    int&       Last() { return back(); }
    const int& Last() const { return back(); }
    void       Sort( int ( *aCompare )( int*, int* ) );
};


typedef wxArrayInt wxArrayLong;

#ifndef _WIN32
#pragma GCC visibility pop
#endif
