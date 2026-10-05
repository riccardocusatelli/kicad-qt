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

#include <wx/arrstr.h>
#include <wx/tokenzr.h>

#include <algorithm>


wxArrayString::wxArrayString( size_t aCount, const char** aStrings )
{
    for( size_t i = 0; i < aCount; ++i )
        push_back( wxString( aStrings[i] ) );
}


wxArrayString::wxArrayString( size_t aCount, const wchar_t** aStrings )
{
    for( size_t i = 0; i < aCount; ++i )
        push_back( wxString( aStrings[i] ) );
}


wxArrayString::wxArrayString( size_t aCount, const wxString* aStrings )
{
    for( size_t i = 0; i < aCount; ++i )
        push_back( aStrings[i] );
}


int wxArrayString::Index( const wxString& aStr, bool aCaseSensitive, bool aFromEnd ) const
{
    if( m_sorted && aCaseSensitive && !aFromEnd )
    {
        auto it = std::lower_bound( begin(), end(), aStr,
                                    []( const wxString& a, const wxString& b )
                                    {
                                        return a.Cmp( b ) < 0;
                                    } );

        if( it != end() && *it == aStr )
            return static_cast<int>( it - begin() );

        return wxNOT_FOUND;
    }

    if( aFromEnd )
    {
        for( size_t i = size(); i > 0; --i )
        {
            if( ( *this )[i - 1].IsSameAs( aStr, aCaseSensitive ) )
                return static_cast<int>( i - 1 );
        }
    }
    else
    {
        for( size_t i = 0; i < size(); ++i )
        {
            if( ( *this )[i].IsSameAs( aStr, aCaseSensitive ) )
                return static_cast<int>( i );
        }
    }

    return wxNOT_FOUND;
}


size_t wxArrayString::Add( const wxString& aStr, size_t aCopies )
{
    if( m_sorted )
    {
        auto it = std::upper_bound( begin(), end(), aStr,
                                    []( const wxString& a, const wxString& b )
                                    {
                                        return a.Cmp( b ) < 0;
                                    } );
        size_t index = it - begin();
        insert( it, aCopies, aStr );
        return index;
    }

    insert( end(), aCopies, aStr );
    return size() - aCopies;
}


void wxArrayString::Insert( const wxString& aStr, size_t aIndex, size_t aCopies )
{
    insert( begin() + std::min( aIndex, size() ), aCopies, aStr );
}


void wxArrayString::Remove( const wxString& aStr )
{
    int index = Index( aStr );

    if( index != wxNOT_FOUND )
        RemoveAt( index );
}


void wxArrayString::RemoveAt( size_t aIndex, size_t aCount )
{
    if( aIndex >= size() )
        return;

    erase( begin() + aIndex, begin() + std::min( size(), aIndex + aCount ) );
}


void wxArrayString::Sort( bool aReverseOrder )
{
    if( aReverseOrder )
        std::sort( begin(), end(), []( const wxString& a, const wxString& b ) { return a.Cmp( b ) > 0; } );
    else
        std::sort( begin(), end(), []( const wxString& a, const wxString& b ) { return a.Cmp( b ) < 0; } );
}


void wxArrayString::Sort( CompareFunction aCompare )
{
    std::sort( begin(), end(),
               [aCompare]( const wxString& a, const wxString& b ) { return aCompare( a, b ) < 0; } );
}


void wxArrayString::Sort( CompareFunction2 aCompare )
{
    std::sort( begin(), end(),
               [aCompare]( wxString& a, wxString& b ) { return aCompare( &a, &b ) < 0; } );
}


wxSortedArrayString::wxSortedArrayString( const wxArrayString& aArray ) : wxArrayString( 1 )
{
    for( const wxString& str : aArray )
        Add( str );
}


size_t wxSortedArrayString::Add( const wxString& aStr, size_t aCopies )
{
    if( !m_compare )
        return wxArrayString::Add( aStr, aCopies );

    auto it = std::upper_bound( begin(), end(), aStr,
                                [this]( const wxString& a, const wxString& b )
                                {
                                    return m_compare( a, b ) < 0;
                                } );
    size_t index = it - begin();
    insert( it, aCopies, aStr );
    return index;
}


int wxSortedArrayString::Index( const wxString& aStr, bool aCaseSensitive, bool aFromEnd ) const
{
    if( !m_compare || !aCaseSensitive || aFromEnd )
        return wxArrayString::Index( aStr, aCaseSensitive, aFromEnd );

    auto it = std::lower_bound( begin(), end(), aStr,
                                [this]( const wxString& a, const wxString& b )
                                {
                                    return m_compare( a, b ) < 0;
                                } );

    if( it != end() && m_compare( *it, aStr ) == 0 )
        return static_cast<int>( it - begin() );

    return wxNOT_FOUND;
}


int wxArrayInt::Index( int aValue, bool aFromEnd ) const
{
    if( aFromEnd )
    {
        for( size_t i = size(); i > 0; --i )
        {
            if( ( *this )[i - 1] == aValue )
                return static_cast<int>( i - 1 );
        }

        return wxNOT_FOUND;
    }

    auto it = std::find( begin(), end(), aValue );
    return it == end() ? wxNOT_FOUND : static_cast<int>( it - begin() );
}


void wxArrayInt::Sort( int ( *aCompare )( int*, int* ) )
{
    std::sort( begin(), end(), [aCompare]( int a, int b ) { return aCompare( &a, &b ) < 0; } );
}


wxArrayString wxSplit( const wxString& aStr, const wxChar aSep, const wxChar aEscape )
{
    if( aEscape == 0 )
        return wxStringTokenize( aStr, wxString( aSep ), wxTOKEN_RET_EMPTY_ALL );

    // The escape character escapes the separator and itself; before any other character it
    // is kept.
    wxArrayString       result;
    wxString            current;
    bool                endsWithSep = false;
    const std::wstring& str = aStr.impl();

    for( size_t i = 0; i < str.size(); ++i )
    {
        wxChar ch = str[i];
        endsWithSep = false;

        if( ch == aSep )
        {
            result.push_back( current );
            current.clear();
            endsWithSep = true;
        }
        else if( ch == aEscape && i + 1 < str.size()
                 && ( str[i + 1] == aSep || str[i + 1] == aEscape ) )
        {
            current += str[++i];
        }
        else
        {
            current += ch;
        }
    }

    if( !current.empty() || endsWithSep )
        result.push_back( current );

    return result;
}


wxString wxJoin( const wxArrayString& aArray, const wxChar aSep, const wxChar aEscape )
{
    wxString result;

    for( size_t n = 0; n < aArray.size(); ++n )
    {
        if( n )
            result += aSep;

        if( aEscape == 0 )
        {
            result += aArray[n];
            continue;
        }

        for( wxChar ch : aArray[n].impl() )
        {
            if( ch == aSep )
                result += aEscape;

            result += ch;
        }
    }

    return result;
}


// wxStringTokenizer

void wxStringTokenizer::SetString( const wxString& aStr, const wxString& aDelims,
                                   wxStringTokenizerMode aMode )
{
    if( aMode == wxTOKEN_DEFAULT )
    {
        // strtok() behaviour if the delimiters are all whitespace, else RET_EMPTY.
        aMode = wxTOKEN_STRTOK;

        for( wxChar c : aDelims.impl() )
        {
            if( !std::iswspace( c ) )
            {
                aMode = wxTOKEN_RET_EMPTY;
                break;
            }
        }
    }

    m_delims = aDelims;
    m_mode = aMode;
    Reinit( aStr );
}


void wxStringTokenizer::Reinit( const wxString& aStr )
{
    m_string = aStr;
    m_pos = 0;
    m_lastDelim = 0;
}


bool wxStringTokenizer::HasMoreTokens() const
{
    if( m_mode == wxTOKEN_INVALID )
        return false;

    // Non-delimiter characters left: there are more tokens.
    if( m_string.find_first_not_of( m_delims, m_pos ) != wxString::npos )
        return true;

    switch( m_mode )
    {
    case wxTOKEN_RET_EMPTY:
    case wxTOKEN_RET_DELIMS:
        // The initial empty token is returned even if only delimiters follow.
        return !m_string.empty() && m_pos == 0;

    case wxTOKEN_RET_EMPTY_ALL:
        // The trailing empty token after the last delimiter is still to come while
        // m_lastDelim is set.
        return m_pos < m_string.length() || m_lastDelim != 0;

    default:
        return false;
    }
}


wxString wxStringTokenizer::GetNextToken()
{
    wxString token;

    do
    {
        if( !HasMoreTokens() )
            break;

        size_t pos = m_string.find_first_of( m_delims, m_pos );

        if( pos == wxString::npos )
        {
            token = m_string.Mid( m_pos );
            m_pos = m_string.length();
            m_lastDelim = 0;
        }
        else
        {
            size_t len = pos - m_pos;

            if( m_mode == wxTOKEN_RET_DELIMS )
                len++;

            token = m_string.Mid( m_pos, len );
            m_lastDelim = m_string.impl()[pos];
            m_pos = pos + 1;
        }
    } while( !AllowEmpty() && token.empty() );

    return token;
}


size_t wxStringTokenizer::CountTokens() const
{
    wxStringTokenizer copy( *this );
    size_t            count = 0;

    while( copy.HasMoreTokens() )
    {
        count++;
        copy.GetNextToken();
    }

    return count;
}


wxArrayString wxStringTokenize( const wxString& aStr, const wxString& aDelims,
                                wxStringTokenizerMode aMode )
{
    wxArrayString     tokens;
    wxStringTokenizer tk( aStr, aDelims, aMode );

    while( tk.HasMoreTokens() )
        tokens.Add( tk.GetNextToken() );

    return tokens;
}
