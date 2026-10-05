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
 * @file kiwx_diff_string.cpp
 *
 * Differential test of kiwx: strings, formatting, arrays, tokenizers and conversions.  The
 * program is built twice, against wxWidgets and against kiwx, and the outputs must be equal
 * (qa/tests/kiwx/run_diff.sh).
 */

#include "kiwx_diff.h"

#include <wx/arrstr.h>
#include <wx/string.h>
#include <wx/tokenzr.h>

#include <climits>


static void testBasics()
{
    SECTION( "basics" );

    wxString s( wxT( "Hello, World" ) );
    OUT( s.Len() );
    OUT( s.Upper() );
    OUT( s.Lower() );
    OUT( s.Mid( 7 ) );
    OUT( s.Mid( 7, 3 ) );
    OUT( s.Mid( 50 ) );
    OUT( s.Mid( 12 ) );
    OUT( s.Left( 5 ) );
    OUT( s.Left( 50 ) );
    OUT( s.Right( 5 ) );
    OUT( s.Right( 50 ) );
    OUT( s.SubString( 2, 4 ) );
    OUT( s.BeforeFirst( ',' ) );
    OUT( s.BeforeFirst( 'x' ) );
    OUT( s.AfterFirst( ',' ) );
    OUT( s.AfterFirst( 'x' ) );
    OUT( s.BeforeLast( 'o' ) );
    OUT( s.BeforeLast( 'x' ) );
    OUT( s.AfterLast( 'o' ) );
    OUT( s.AfterLast( 'x' ) );

    wxString rest = wxT( "unchanged" );
    OUT( s.BeforeFirst( 'x', &rest ) );
    OUT( rest );
    rest = wxT( "unchanged" );
    OUT( s.BeforeLast( 'x', &rest ) );
    OUT( rest );
    OUT( s.BeforeLast( 'l', &rest ) );
    OUT( rest );
    OUT( s.BeforeFirst( 'l', &rest ) );
    OUT( rest );

    OUT( s.Find( 'o' ) );
    OUT( s.Find( 'o', true ) );
    OUT( s.Find( 'z' ) );
    OUT( s.Find( wxT( "World" ) ) );
    OUT( s.Find( wxT( "" ) ) );
    OUT( s.Find( wxT( "xyz" ) ) );
    OUT( s.Freq( 'l' ) );
    OUT( s.Contains( wxT( "lo, W" ) ) );
    OUT( s.StartsWith( wxT( "Hell" ), &rest ) );
    OUT( rest );
    OUT( s.EndsWith( wxT( "World" ), &rest ) );
    OUT( rest );
    OUT( s.StartsWith( wxT( "World" ) ) );
    OUT( s.EndsWith( wxT( "" ) ) );

    OUT( s.IsSameAs( wxT( "hello, world" ) ) );
    OUT( s.IsSameAs( wxT( "hello, world" ), false ) );
    OUT( s.CmpNoCase( wxT( "HELLO, WORLD" ) ) );
    OUT( ( s.CmpNoCase( wxT( "HELLO" ) ) > 0 ) );
    OUT( ( s.Cmp( wxT( "Hello" ) ) > 0 ) );
    OUT( ( s.Cmp( wxT( "Iello" ) ) < 0 ) );
    OUT( wxString( wxT( "a" ) ).IsSameAs( 'A', false ) );
    OUT( wxString( wxT( "a" ) ).IsSameAs( 'A' ) );

    wxString t = wxT( "  \t trim me \n " );
    OUT( wxString( t ).Trim() );
    OUT( wxString( t ).Trim( false ) );
    OUT( wxString( t ).Trim().Trim( false ) );
    OUT( t.Strip( wxString::both ) );
    OUT( t.Strip( wxString::leading ) );
    OUT( t.Strip() );
    OUT( wxString( wxT( "ab" ) ).Pad( 3, '*' ) );
    OUT( wxString( wxT( "ab" ) ).Pad( 3, '*', false ) );
    OUT( wxString( wxT( "abcdef" ) ).Truncate( 3 ) );
    OUT( wxString( wxT( "abcdef" ) ).Truncate( 30 ) );
    OUT( wxString( wxT( "abcdef" ) ).Remove( 2 ) );
    OUT( wxString( wxT( "abcdef" ) ).Remove( 2, 2 ) );
    OUT( wxString( wxT( "abcdef" ) ).RemoveLast() );
    OUT( wxString( wxT( "abcdef" ) ).RemoveLast( 3 ) );
    OUT( wxString( wxT( "hELLO wORLD" ) ).Capitalize() );
    OUT( wxString( wxT( "" ) ).Capitalize() );

    wxString r = wxT( "a-b-c--d" );
    OUT( r.Replace( wxT( "-" ), wxT( "+" ), false ) );
    OUT( r );
    OUT( r.Replace( wxT( "-" ), wxT( "==" ) ) );
    OUT( r );
    OUT( r.Replace( wxT( "zz" ), wxT( "x" ) ) );
    wxString rr = wxT( "aaa" );
    OUT( rr.Replace( wxT( "a" ), wxT( "aa" ) ) );
    OUT( rr );

    OUT( wxString( wxT( "file.kicad_pcb" ) ).Matches( wxT( "*.kicad_pcb" ) ) );
    OUT( wxString( wxT( "file.kicad_pcb" ) ).Matches( wxT( "*.KICAD_PCB" ) ) );
    OUT( wxString( wxT( "R12" ) ).Matches( wxT( "R?2" ) ) );
    OUT( wxString( wxT( "R12" ) ).Matches( wxT( "R*" ) ) );
    OUT( wxString( wxT( "R12" ) ).Matches( wxT( "*1*2*" ) ) );
    OUT( wxString( wxT( "R12" ) ).Matches( wxT( "*3*" ) ) );
    OUT( wxString( wxT( "" ) ).Matches( wxT( "*" ) ) );
    OUT( wxString( wxT( "" ) ).Matches( wxT( "?" ) ) );
    OUT( wxString( wxT( "abc" ) ).Matches( wxT( "" ) ) );

    OUT( wxString( wxT( "123" ) ).IsNumber() );
    OUT( wxString( wxT( "-123" ) ).IsNumber() );
    OUT( wxString( wxT( "+12" ) ).IsNumber() );
    OUT( wxString( wxT( "12a" ) ).IsNumber() );
    OUT( wxString( wxT( "" ) ).IsNumber() );
    OUT( wxString( wxT( "-" ) ).IsNumber() );
    OUT( wxString( wxT( "abc" ) ).IsWord() );
    OUT( wxString( wxT( "" ) ).IsWord() );
    OUT( wxString( wxT( "ab1" ) ).IsWord() );
    OUT( wxString( wxT( "abc" ) ).IsAscii() );

    wxString c( 'x', 3 );
    OUT( c );
    OUT( wxString( wxT( "abc" ) ) + wxT( "def" ) + 'g' + wxString( wxT( "h" ) ) );
    wxString app;
    app << wxT( "n=" ) << 42 << wxT( " d=" ) << 2.5 << wxT( " l=" ) << 123456789012LL << ' '
        << 'c' << wxT( " u=" ) << 7u << wxT( " f=" ) << 0.1f;
    OUT( app );
    OUT( wxString( wxT( "abc" ) ).Prepend( wxT( "xy" ) ) );
    OUT( wxString( wxT( "abc" ) ).Append( 'z', 3 ) );
    OUT( wxString( wxT( "abc" ) ).Last() );
    OUT( wxString( wxT( "abc" ) ).GetChar( 1 ) );
}


static void testNumbers()
{
    SECTION( "numbers" );

    const wchar_t* inputs[] = { L"42", L"-42", L"  42", L"42  ", L"42abc", L"", L"abc", L"0x1F",
                                L"1F", L"99999999999999999999", L"-1", L"+7", L"3.14", L"1e3",
                                L"-0.5", L"1,5", L".5", L"5.", L"inf", L"nan", L"  -3.25e-2" };

    for( const wchar_t* in : inputs )
    {
        wxString      s( in );
        long          l = -999;
        unsigned long ul = 999;
        long long     ll = -999;
        double        d = -999.0;
        double        cd = -999.0;
        int           i = -999;

        bool rl = s.ToLong( &l );
        bool rul = s.ToULong( &ul );
        bool rll = s.ToLongLong( &ll );
        bool rd = s.ToDouble( &d );
        bool rcd = s.ToCDouble( &cd );
        bool ri = s.ToInt( &i );

        OUT( wxString::Format( wxT( "[%s] long %d %ld ulong %d %lu ll %d %lld int %d %d dbl %d %g cdbl %d %g" ),
                               s, rl, l, rul, ul, rll, ll, ri, i, rd, d, rcd, cd ) );
    }

    long hex = 0;
    OUT( wxString( wxT( "1F" ) ).ToLong( &hex, 16 ) );
    OUT( hex );
    OUT( wxString( wxT( "0x1F" ) ).ToLong( &hex, 0 ) );
    OUT( hex );
    OUT( wxString( wxT( "777" ) ).ToLong( &hex, 8 ) );
    OUT( hex );

    OUT( wxString::FromDouble( 3.14159 ) );
    OUT( wxString::FromDouble( 3.14159, 2 ) );
    OUT( wxString::FromDouble( 1e20 ) );
    OUT( wxString::FromDouble( 0.0001 ) );
    OUT( wxString::FromCDouble( 3.14159 ) );
    OUT( wxString::FromCDouble( 3.14159, 3 ) );
    OUT( wxString::FromCDouble( 1234567.0 ) );
    OUT( wxString::FromCDouble( 0.1, 0 ) );
}


static void testFormat()
{
    SECTION( "format" );

    OUT( wxString::Format( wxT( "%d|%5d|%-5d|%05d|%+d|% d" ), 42, 42, 42, 42, 42, 42 ) );
    OUT( wxString::Format( wxT( "%x|%X|%o|%#x|%#o" ), 255, 255, 8, 255, 8 ) );
    OUT( wxString::Format( wxT( "%u|%d" ), -1, UINT_MAX ) );
    OUT( wxString::Format( wxT( "%ld|%lld|%lu|%llu" ), -5L, -6LL, 7UL, 8ULL ) );
    OUT( wxString::Format( wxT( "%zu|%zd" ), size_t( 9 ), ssize_t( -9 ) ) );
    OUT( wxString::Format( wxT( "%hd|%hu|%hhd" ), (short) -3, (unsigned short) 65535, (signed char) -1 ) );
    OUT( wxString::Format( wxT( "%f|%.2f|%10.3f|%-10.1f|%e|%E|%g|%G" ), 3.14159, 3.14159,
                           3.14159, 3.14159, 31415.9, 31415.9, 0.0000314, 314159265.0 ) );
    OUT( wxString::Format( wxT( "%.0f|%.10g|%#g|%a" ), 2.5, 1.0 / 3.0, 1.0, 1.0 ) );
    OUT( wxString::Format( wxT( "%s|%10s|%-10s|%.3s|%5.2s" ), wxT( "abc" ), wxT( "abc" ),
                           wxT( "abc" ), wxT( "abcdef" ), wxT( "abcdef" ) ) );
    OUT( wxString::Format( wxT( "%s %s %s" ), wxString( wxT( "wx" ) ), "char*", std::string( "std" ) ) );
    OUT( wxString::Format( wxT( "%s" ), std::wstring( L"wide" ) ) );
    OUT( wxString::Format( wxT( "%c%c%c" ), 'a', wxUniChar( 'b' ), wxT( 'c' ) ) );
    OUT( wxString::Format( wxT( "%c" ), 65 ) );
    OUT( wxString::Format( wxT( "100%% %d%%" ), 5 ) );
    OUT( wxString::Format( wxT( "%*d|%-*d|%.*f" ), 6, 1, 6, 2, 3, 2.0 ) );
    OUT( wxString::Format( wxT( "%2$s %1$s" ), wxT( "world" ), wxT( "hello" ) ) );
    OUT( wxString::Format( wxT( "%d %s" ), true, wxT( "x" ) ) );
    OUT( wxString::Format( wxT( "no args" ) ) );
    OUT( wxString::Format( wxT( "%s" ), wxT( "été 中" ) ) );
    OUT( wxString::Format( wxT( "%5s|%-5s|" ), wxT( "é" ), wxT( "中" ) ) );
    OUT( wxString::Format( "%s=%d", "narrow", 3 ) );
    OUT( wxString::Format( wxT( "%i|%3i" ), 7, 8 ) );
    OUT( wxString::Format( wxT( "%.3d|%8.3d" ), 7, 8 ) );

    enum E { E1 = 3 };
    OUT( wxString::Format( wxT( "%d" ), E1 ) );

    wxString p;
    OUT( p.Printf( wxT( "%s-%d" ), wxT( "x" ), 1 ) );
    OUT( p );
}


static void testConversions()
{
    SECTION( "conversions" );

    wxString utf = wxString::FromUTF8( "\xc3\xa9t\xc3\xa9 \xe4\xb8\xad \xf0\x9f\x98\x80" );
    OUT( utf.Len() );
    OUT( utf.utf8_str().length() );
    OUT( std::string( utf.utf8_str() ) );
    OUT( utf.ToStdString( wxConvUTF8 ) );
    OUT( wxString::FromUTF8( "\xff\xfe bad" ).Len() );
    OUT( wxString::FromUTF8( "" ).Len() );
    OUT( wxString::FromUTF8( nullptr ).Len() );
    OUT( wxString::FromUTF8( "abc\0def", 7 ).Len() );
    OUT( wxString( "plain ascii" ) );
    OUT( wxString( "abcdef", 3 ) );
    OUT( wxString( std::string( "std string" ) ) );
    OUT( wxString( L"wide", 2 ) );
    OUT( wxString( "\xc3\xa9", wxConvUTF8 ).Len() );
    OUT( wxString::From8BitData( "\xe9" ).Len() );
    OUT( (int) wxString::From8BitData( "\xe9" )[0].GetValue() );
    OUT( std::string( wxString( wxT( "éa" ) ).ToAscii() ) );
    OUT( std::string( wxString( wxT( "abc" ) ).mb_str() ) );
    OUT( std::string( wxString( wxT( "abc" ) ).mb_str( wxConvUTF8 ) ) );
    OUT( wxString( wxT( "é" ) ).ToStdString( wxConvUTF8 ).size() );
    OUT( wxString( wxT( "é" ) ).utf8_string().size() );

    wxCSConv latin1( wxT( "ISO-8859-1" ) );
    OUT( wxString( "\xe9t\xe9", latin1 ) );
    OUT( wxString( "\xe9t\xe9", latin1 ).Len() );
    wxCSConv cp1252( wxFONTENCODING_CP1252 );
    OUT( (int) wxString( "\x80", cp1252 )[0].GetValue() );
    OUT( std::string( wxString( wxT( "€" ) ).mb_str( cp1252 ) ).size() );
    OUT( latin1.IsOk() );

    wxString cstr( wxT( "éx" ) );
    OUT( std::string( (const char*) cstr.c_str() ) );
    OUT( std::wstring( (const wchar_t*) cstr.c_str() ).size() );
    OUT( wxString( cstr.c_str() ) );
}


static void testArrays()
{
    SECTION( "arrays" );

    wxArrayString a;
    a.Add( wxT( "pear" ) );
    a.Add( wxT( "Apple" ) );
    a.Add( wxT( "banana" ) );
    a.Add( wxT( "apple" ) );
    a.Insert( wxT( "fig" ), 1 );
    OUT( a );
    OUT( a.Index( wxT( "apple" ) ) );
    OUT( a.Index( wxT( "APPLE" ), false ) );
    OUT( a.Index( wxT( "APPLE" ), false, true ) );
    OUT( a.Index( wxT( "kiwi" ) ) );
    a.Sort();
    OUT( a );
    a.Sort( true );
    OUT( a );
    a.Remove( wxT( "fig" ) );
    a.RemoveAt( 0 );
    OUT( a );
    OUT( a.GetCount() );
    OUT( a.Last() );

    wxSortedArrayString sorted;
    sorted.Add( wxT( "m" ) );
    sorted.Add( wxT( "c" ) );
    sorted.Add( wxT( "x" ) );
    sorted.Add( wxT( "a" ) );
    OUT( sorted );
    OUT( sorted.Index( wxT( "m" ) ) );
    OUT( sorted.Index( wxT( "q" ) ) );

    const wxChar* splits[] = { wxT( "a,b,c" ), wxT( "a,,c" ), wxT( "a,b," ), wxT( ",a" ),
                               wxT( "" ), wxT( "a\\,b,c" ), wxT( "abc" ), wxT( "," ),
                               wxT( "a\\\\,b" ) };

    for( const wxChar* sp : splits )
    {
        OUT( wxSplit( sp, ',' ) );
        OUT( wxSplit( sp, ',', 0 ) );
    }

    wxArrayString j;
    j.Add( wxT( "a" ) );
    j.Add( wxT( "b,c" ) );
    j.Add( wxT( "" ) );
    OUT( wxJoin( j, ',' ) );
    OUT( wxJoin( j, ',', 0 ) );
    OUT( wxJoin( wxArrayString(), ',' ) );
}


static void testTokenizer()
{
    SECTION( "tokenizer" );

    const wxChar* inputs[] = { wxT( "a b  c" ), wxT( "  lead trail  " ), wxT( "a,b,,c," ),
                               wxT( "" ), wxT( "," ), wxT( "x" ), wxT( "a:b;c" ) };
    const wxChar* delims[] = { wxT( " " ), wxT( "," ), wxT( ":;" ), wxT( " \t" ) };
    wxStringTokenizerMode modes[] = { wxTOKEN_DEFAULT, wxTOKEN_RET_EMPTY, wxTOKEN_RET_EMPTY_ALL,
                                      wxTOKEN_RET_DELIMS, wxTOKEN_STRTOK };

    for( const wxChar* in : inputs )
    {
        for( const wxChar* d : delims )
        {
            for( wxStringTokenizerMode m : modes )
            {
                wxStringTokenizer tk( in, d, m );
                wxString          line = wxString::Format( wxT( "[%s|%s|%d] n=%d:" ), in, d,
                                                           (int) m, (int) tk.CountTokens() );

                while( tk.HasMoreTokens() )
                {
                    wxString tok = tk.GetNextToken();
                    line += wxString::Format( wxT( " <%s>%d@%d" ), tok,
                                              (int) tk.GetLastDelimiter(),
                                              (int) tk.GetPosition() );
                }

                OUT( line );
            }
        }
    }

    OUT( wxStringTokenize( wxT( "one two  three" ) ) );
}


int main()
{
    testBasics();
    testNumbers();
    testFormat();
    testConversions();
    testArrays();
    testTokenizer();
    return 0;
}
