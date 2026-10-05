/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/regex.cpp, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

#include <wx/regex.h>

#include <wx/debug.h>
#include <wx/log.h>
#include <wx/translation.h>

#include <cwchar>
#include <cwctype>
#include <vector>

// wxString holds wchar_t, 32 bit here: the matching is done on it directly.
static_assert( sizeof( wchar_t ) == 4, "kiwx regex needs a 32 bit wchar_t" );
#define PCRE2_CODE_UNIT_WIDTH 32
#include <pcre2.h>


class wxRegExImpl
{
public:
    ~wxRegExImpl()
    {
        if( m_matchData )
            pcre2_match_data_free( m_matchData );

        if( m_code )
            pcre2_code_free( m_code );
    }

    bool   Compile( wxString aExpr, int aFlags );
    bool   Matches( const wchar_t* aStr, int aFlags, size_t aLen ) const;
    bool   GetMatch( size_t* aStart, size_t* aLen, size_t aIndex = 0 ) const;
    size_t GetMatchCount() const;
    int    Replace( wxString* aText, const wxString& aReplacement, size_t aMaxMatches ) const;

private:
    wxString errorMessage( int aCode ) const;

    pcre2_code*       m_code = nullptr;
    pcre2_match_data* m_matchData = nullptr;
    size_t            m_nMatches = 0;

    // offsets of the last match: start and end of each sub-expression, -1 if unset
    mutable std::vector<size_t> m_matches;
    mutable bool                m_matched = false;
};


wxString wxRegExImpl::errorMessage( int aCode ) const
{
    PCRE2_UCHAR buffer[256];
    int         len = pcre2_get_error_message( aCode, buffer, 256 );

    if( len < 0 )
        return wxString::Format( wxS( "PCRE error %d" ), aCode );

    return wxString( reinterpret_cast<const wchar_t*>( buffer ), static_cast<size_t>( len ) );
}


// Advances from the '[' starting a bracket expression to its closing ']' (or the end).
static wxString::const_iterator skipBracketExpression( wxString::const_iterator it,
                                                       wxString::const_iterator end )
{
    ++it;

    if( it != end && *it == '^' )
        ++it;

    if( it != end && *it == ']' )
        ++it;

    for( ; it != end; ++it )
    {
        const wxUniChar c = *it;

        if( c == ']' )
            break;

        if( c == '[' )
        {
            if( ++it == end )
                break;

            const wxUniChar c2 = *it;

            if( c2 == ':' || c2 == '.' || c2 == '=' )
            {
                for( ++it; it != end; ++it )
                {
                    if( *it == c2 )
                    {
                        if( ++it == end )
                            break;

                        if( *it == ']' )
                            break;
                    }
                }

                if( it == end )
                    break;
            }
        }
    }

    return it;
}


wxString wxRegEx::ConvertFromBasic( const wxString& aBre )
{
    wxString ere;
    ere.reserve( aBre.length() );

    enum SinceStart
    {
        SinceStart_None,
        SinceStart_OnlyCaret,
        SinceStart_Some
    };

    struct State
    {
        explicit State( SinceStart aSinceStart ) : isBackslash( false ), sinceStart( aSinceStart ) {}

        bool       isBackslash;
        SinceStart sinceStart;
    };

    State previous( SinceStart_None );

    for( wxString::const_iterator it = aBre.begin(), end = aBre.end(); it != end; ++it )
    {
        const wxUniChar c = *it;

        enum Disposition
        {
            Disposition_Skip,
            Disposition_Append,
            Disposition_Escape
        } disposition = Disposition_Append;

        State current( SinceStart_Some );

        if( previous.isBackslash )
        {
            disposition = Disposition_Escape;

            switch( c.GetValue() )
            {
            case '(':
                current.sinceStart = SinceStart_None;
                [[fallthrough]];

            case ')':
            case '{':
            case '}':
                disposition = Disposition_Append;
                break;

            case '<':
            case '>':
                ere += "[[:";
                ere += c;
                ere += ":]]";
                disposition = Disposition_Skip;
                break;
            }
        }
        else
        {
            switch( c.GetValue() )
            {
            case '\\':
                current.isBackslash = true;
                disposition = Disposition_Skip;
                break;

            case '^':
                if( previous.sinceStart == SinceStart_None )
                    current.sinceStart = SinceStart_OnlyCaret;
                else
                    disposition = Disposition_Escape;

                break;

            case '*':
                if( previous.sinceStart != SinceStart_Some )
                    disposition = Disposition_Escape;

                break;

            case '$':
            {
                disposition = Disposition_Escape;
                wxString::const_iterator next = it;
                ++next;

                if( next == end )
                {
                    disposition = Disposition_Append;
                }
                else if( *next == '\\' )
                {
                    ++next;

                    if( next != end && *next == ')' )
                        disposition = Disposition_Append;
                }

                break;
            }

            case '|':
            case '+':
            case '?':
            case '(':
            case ')':
            case '{':
            case '}':
                disposition = Disposition_Escape;
                break;

            case '[':
            {
                const wxString::const_iterator start = it;
                it = skipBracketExpression( it, end );
                ere += wxString( start, it );

                if( it == end )
                    return ere;

                break;
            }
            }
        }

        switch( disposition )
        {
        case Disposition_Skip:
            break;

        case Disposition_Escape:
            ere += '\\';
            [[fallthrough]];

        case Disposition_Append:
            ere += *it;
            break;
        }

        previous = current;
    }

    if( previous.isBackslash )
        ere += '\\';

    return ere;
}


static wxString pcreOptionsToString( int aOpts )
{
    wxString s;

    if( aOpts & PCRE2_CASELESS )
        s += 'i';

    if( aOpts & PCRE2_MULTILINE )
        s += 'm';

    if( aOpts & PCRE2_DOTALL )
        s += 's';

    if( aOpts & PCRE2_EXTENDED )
        s += 'x';

    return s;
}


// Converts the Tcl directors and embedded options to the PCRE syntax.
static wxString convertMetasyntax( wxString aExpr, int& aFlags )
{
    const int DIRECTOR_PREFIX_LEN = 3;

    if( aExpr.length() > DIRECTOR_PREFIX_LEN && aExpr.StartsWith( "***" ) )
    {
        switch( aExpr[DIRECTOR_PREFIX_LEN].GetValue() )
        {
        case ':':
            aFlags &= ~wxRE_BASIC;
            aFlags |= wxRE_ADVANCED;
            aExpr.erase( 0, DIRECTOR_PREFIX_LEN + 1 );
            break;

        case '=':
            aFlags &= ~( wxRE_BASIC | wxRE_ADVANCED );
            aExpr.replace( 0, DIRECTOR_PREFIX_LEN + 1, "\\Q" );
            break;

        default:
            break;
        }
    }

    if( ( aFlags & wxRE_ADVANCED ) && aExpr.StartsWith( "(?" ) )
    {
        wxString optsString;
        int      opts = 0;
        int      negopts = 0;

        enum Syntax
        {
            Syntax_None,
            Syntax_Basic,
            Syntax_Extended,
            Syntax_Literal
        } syntax = Syntax_None;

        const size_t start = 2;

        for( size_t i = start; i < aExpr.length(); ++i )
        {
            const wxUniChar c = aExpr[i];

            if( c == ')' )
            {
                optsString += pcreOptionsToString( opts );

                if( negopts )
                {
                    optsString += "-";
                    optsString += pcreOptionsToString( negopts );
                }

                size_t posAfterOpts;

                if( optsString.empty() )
                {
                    aExpr.erase( 0, i + 1 );
                    posAfterOpts = 0;
                }
                else
                {
                    aExpr.replace( start, i - start, optsString );
                    posAfterOpts = optsString.length() + 3;
                }

                aFlags &= ~wxRE_ADVANCED;

                switch( syntax )
                {
                case Syntax_None:     aFlags |= wxRE_ADVANCED; break;
                case Syntax_Basic:    aFlags |= wxRE_BASIC; break;
                case Syntax_Extended: aFlags |= wxRE_EXTENDED; break;
                case Syntax_Literal:  aExpr.insert( posAfterOpts, "\\Q" ); break;
                }

                break;
            }

            // only letters are options: other "(?" constructs are left alone
            if( c < 'a' || c > 'z' )
                break;

            switch( c.GetValue() )
            {
            case 'b': syntax = Syntax_Basic; break;
            case 'e': syntax = Syntax_Extended; break;
            case 'q': syntax = Syntax_Literal; break;

            case 'm':
            case 'n':
                negopts &= ~PCRE2_MULTILINE;
                opts |= PCRE2_MULTILINE;
                [[fallthrough]];

            case 'p':
                negopts |= PCRE2_DOTALL;
                break;

            case 'w':
                negopts &= ~( PCRE2_MULTILINE | PCRE2_DOTALL );
                opts |= PCRE2_MULTILINE | PCRE2_DOTALL;
                break;

            case 'c': negopts |= PCRE2_CASELESS; break;
            case 't': negopts |= PCRE2_EXTENDED; break;

            case 's':
                negopts &= ~PCRE2_DOTALL;
                opts |= PCRE2_DOTALL;
                break;

            case 'i':
                negopts &= ~PCRE2_CASELESS;
                opts |= PCRE2_CASELESS;
                break;

            case 'x':
                negopts &= ~PCRE2_EXTENDED;
                opts |= PCRE2_EXTENDED;
                break;

            default:
                optsString += c;
                break;
            }
        }
    }

    return aExpr;
}


// Converts the Tcl word boundaries (\m \M \y \Y) to the PCRE syntax.
static wxString convertWordBoundaries( const wxString& aExpr )
{
    wxString out;
    out.reserve( aExpr.length() );

    for( wxString::const_iterator it = aExpr.begin(), end = aExpr.end(); it != end; ++it )
    {
        if( *it == '\\' )
        {
            ++it;

            if( it == end )
            {
                out.append( '\\' );
                break;
            }

            const char* replacement = nullptr;

            switch( wxUniChar( *it ).GetValue() )
            {
            case 'm': replacement = "[[:<:]]"; break;
            case 'M': replacement = "[[:>:]]"; break;
            case 'y': replacement = "\\b"; break;
            case 'Y': replacement = "\\B"; break;
            }

            if( replacement )
            {
                out.append( replacement );
                continue;
            }

            out.append( '\\' );
        }

        out.append( *it );
    }

    return out;
}


bool wxRegExImpl::Compile( wxString aExpr, int aFlags )
{
    aExpr = convertMetasyntax( aExpr, aFlags );

    if( aFlags & wxRE_BASIC )
    {
        aExpr = wxRegEx::ConvertFromBasic( aExpr );
        aFlags &= ~wxRE_BASIC;
    }
    else if( aFlags & wxRE_ADVANCED )
    {
        aExpr = convertWordBoundaries( aExpr );
    }

    // \uXXXX as in the Tcl library; the old library's '.' matched '\n'
    uint32_t options = PCRE2_UTF | PCRE2_ALT_BSUX;

    if( aFlags & wxRE_ICASE )
        options |= PCRE2_CASELESS;

    if( aFlags & wxRE_NEWLINE )
        options |= PCRE2_MULTILINE;
    else
        options |= PCRE2_DOTALL;

    int        errorCode = 0;
    PCRE2_SIZE errorOffset = 0;

    m_code = pcre2_compile( reinterpret_cast<PCRE2_SPTR>( aExpr.wc_str() ), PCRE2_ZERO_TERMINATED,
                            options, &errorCode, &errorOffset, nullptr );

    if( !m_code )
    {
        wxLogError( _( "Invalid regular expression '%s': %s" ), aExpr, errorMessage( errorCode ) );
        return false;
    }

    m_matchData = pcre2_match_data_create_from_pattern( m_code, nullptr );

    if( aFlags & wxRE_NOSUB )
    {
        m_nMatches = 0;
    }
    else
    {
        // the whole expression and the groups (counted the way wxWidgets does)
        m_nMatches = 1;

        for( const wchar_t* p = aExpr.wc_str(); *p; p++ )
        {
            if( *p == '\\' )
            {
                if( !*++p )
                    break;
            }
            else if( *p == '(' && p[1] != '?' && p[1] != '*' )
            {
                m_nMatches++;
            }
        }
    }

    return true;
}


bool wxRegExImpl::Matches( const wchar_t* aStr, int aFlags, size_t aLen ) const
{
    uint32_t options = 0;

    if( aFlags & wxRE_NOTBOL )
        options |= PCRE2_NOTBOL;

    if( aFlags & wxRE_NOTEOL )
        options |= PCRE2_NOTEOL;

    if( aFlags & wxRE_NOTEMPTY )
        options |= PCRE2_NOTEMPTY;

    const int rc = pcre2_match( m_code, reinterpret_cast<PCRE2_SPTR>( aStr ), aLen, 0, options,
                                m_matchData, nullptr );

    if( rc == PCRE2_ERROR_NOMATCH )
        return false;

    if( rc < 0 )
    {
        wxLogError( _( "Failed to find match for regular expression: %s" ), errorMessage( rc ) );
        return false;
    }

    const PCRE2_SIZE* ovector = pcre2_get_ovector_pointer( m_matchData );
    const size_t      actual = static_cast<size_t>( rc );

    m_matches.assign( m_nMatches * 2, static_cast<size_t>( -1 ) );

    for( size_t n = 0; n < m_nMatches && n < actual; ++n )
    {
        m_matches[n * 2] = ovector[n * 2] == PCRE2_UNSET ? static_cast<size_t>( -1 ) : ovector[n * 2];
        m_matches[n * 2 + 1] =
                ovector[n * 2 + 1] == PCRE2_UNSET ? static_cast<size_t>( -1 ) : ovector[n * 2 + 1];
    }

    m_matched = true;
    return true;
}


bool wxRegExImpl::GetMatch( size_t* aStart, size_t* aLen, size_t aIndex ) const
{
    wxCHECK_MSG( m_nMatches, false, wxT( "can't use with wxRE_NOSUB" ) );
    wxCHECK_MSG( m_matched, false, wxT( "must call Matches() first" ) );
    wxCHECK_MSG( aIndex < m_nMatches, false, wxT( "invalid match index" ) );

    if( aStart )
        *aStart = m_matches[aIndex * 2];

    if( aLen )
        *aLen = m_matches[aIndex * 2 + 1] - m_matches[aIndex * 2];

    return true;
}


size_t wxRegExImpl::GetMatchCount() const
{
    wxCHECK_MSG( m_nMatches, 0, wxT( "can't use with wxRE_NOSUB" ) );
    return m_nMatches;
}


int wxRegExImpl::Replace( wxString* aText, const wxString& aReplacement, size_t aMaxMatches ) const
{
    wxCHECK_MSG( aText, wxNOT_FOUND, wxT( "NULL text in wxRegEx::Replace" ) );

    // copy: the text is replaced at the end
    const std::wstring text = aText->ToStdWstring();
    const wchar_t*     textstr = text.c_str();
    const size_t       textlen = text.length();

    wxString textNew;
    wxString result;
    result.reserve( 5 * textlen / 4 );

    bool mayHaveBackrefs = aReplacement.find_first_of( wxT( "\\&" ) ) != wxString::npos;

    if( !mayHaveBackrefs )
        textNew = aReplacement;

    size_t matchStart = 0;
    size_t countRepl = 0;

    // '^' must not match after the first match
    while( ( !aMaxMatches || countRepl < aMaxMatches )
           && Matches( textstr + matchStart, countRepl ? wxRE_NOTBOL : 0, textlen - matchStart ) )
    {
        if( mayHaveBackrefs )
        {
            mayHaveBackrefs = false;
            textNew.clear();
            textNew.reserve( aReplacement.length() );

            for( const wchar_t* p = aReplacement.wc_str(); *p; p++ )
            {
                size_t index = static_cast<size_t>( -1 );

                if( *p == '\\' )
                {
                    if( std::iswdigit( *++p ) )
                    {
                        wchar_t* end;
                        index = static_cast<size_t>( std::wcstoul( p, &end, 10 ) );
                        p = end - 1;
                    }
                }
                else if( *p == '&' )
                {
                    index = 0;
                }

                if( index != static_cast<size_t>( -1 ) )
                {
                    size_t start, len;

                    if( !GetMatch( &start, &len, index ) )
                    {
                        wxFAIL_MSG( wxT( "invalid back reference" ) );
                    }
                    else
                    {
                        textNew += wxString( textstr + matchStart + start, len );
                        mayHaveBackrefs = true;
                    }
                }
                else
                {
                    // also the char after a backslash, or the end of the string
                    if( !*p )
                        break;

                    textNew += *p;
                }
            }
        }

        size_t start, len;

        if( !GetMatch( &start, &len ) )
        {
            wxFAIL_MSG( wxT( "internal logic error in wxRegEx::Replace" ) );
            return wxNOT_FOUND;
        }

        result.append( wxString( textstr + matchStart, start ) );
        matchStart += start;
        result.append( textNew );

        countRepl++;
        matchStart += len;
    }

    result.append( wxString( textstr + matchStart ) );
    *aText = result;

    return static_cast<int>( countRepl );
}


// wxRegEx

wxRegEx::~wxRegEx()
{
    delete m_impl;
}


bool wxRegEx::Compile( const wxString& aExpr, int aFlags )
{
    delete m_impl;
    m_impl = new wxRegExImpl;

    if( !m_impl->Compile( aExpr, aFlags ) )
    {
        delete m_impl;
        m_impl = nullptr;
        return false;
    }

    return true;
}


bool wxRegEx::Matches( const wxString& aStr, int aFlags ) const
{
    wxCHECK_MSG( IsValid(), false, wxT( "must successfully Compile() first" ) );
    return m_impl->Matches( aStr.wc_str(), aFlags, aStr.length() );
}


bool wxRegEx::GetMatch( size_t* aStart, size_t* aLen, size_t aIndex ) const
{
    wxCHECK_MSG( IsValid(), false, wxT( "must successfully Compile() first" ) );
    return m_impl->GetMatch( aStart, aLen, aIndex );
}


wxString wxRegEx::GetMatch( const wxString& aText, size_t aIndex ) const
{
    size_t start, len;

    if( !GetMatch( &start, &len, aIndex ) )
        return wxEmptyString;

    return aText.Mid( start, len );
}


size_t wxRegEx::GetMatchCount() const
{
    wxCHECK_MSG( IsValid(), 0, wxT( "must successfully Compile() first" ) );
    return m_impl->GetMatchCount();
}


int wxRegEx::Replace( wxString* aText, const wxString& aReplacement, size_t aMaxMatches ) const
{
    wxCHECK_MSG( IsValid(), wxNOT_FOUND, wxT( "must successfully Compile() first" ) );
    return m_impl->Replace( aText, aReplacement, aMaxMatches );
}


wxString wxRegEx::QuoteMeta( const wxString& aStr )
{
    static const wxString metaChars = wxS( "\\^$.|?*+()[]{}" );

    wxString escaped;
    escaped.reserve( aStr.length() * 2 );

    for( wxUniChar c : aStr )
    {
        if( metaChars.find( c ) != wxString::npos )
            escaped += wxS( '\\' );

        escaped += c;
    }

    return escaped;
}
