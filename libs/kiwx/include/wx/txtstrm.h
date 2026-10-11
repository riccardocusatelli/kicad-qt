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

#include <wx/stream.h>
#include <wx/strconv.h>

#include <memory>

#define wxEOT wxT( '\x04' )


class wxTextInputStream
{
public:
    wxTextInputStream( wxInputStream& aStream, const wxString& aSep = wxT( " \t" ),
                       const wxMBConv& aConv = wxConvAuto() );

    wxString ReadLine();
    wxString ReadWord();
    wxChar   GetChar();
    wxUint64 Read64( int aBase = 10 );
    wxUint32 Read32( int aBase = 10 );
    wxUint16 Read16( int aBase = 10 ) { return static_cast<wxUint16>( Read32( aBase ) ); }
    wxUint8  Read8( int aBase = 10 ) { return static_cast<wxUint8>( Read32( aBase ) ); }
    wxInt64  Read64S( int aBase = 10 );
    wxInt32  Read32S( int aBase = 10 );
    wxInt16  Read16S( int aBase = 10 ) { return static_cast<wxInt16>( Read32S( aBase ) ); }
    wxInt8   Read8S( int aBase = 10 ) { return static_cast<wxInt8>( Read32S( aBase ) ); }
    double   ReadDouble();

    wxString GetStringSeparators() const { return m_separators; }
    void     SetStringSeparators( const wxString& aSep ) { m_separators = aSep; }

    bool                 IsOk() const { return m_input.IsOk(); }
    const wxInputStream& GetInputStream() const { return m_input; }

    wxTextInputStream& operator>>( wxString& aWord ) { aWord = ReadWord(); return *this; }
    wxTextInputStream& operator>>( char& c );
    wxTextInputStream& operator>>( wchar_t& c ) { c = GetChar(); return *this; }
    wxTextInputStream& operator>>( wxInt16& i ) { i = Read16S(); return *this; }
    wxTextInputStream& operator>>( wxInt32& i ) { i = Read32S(); return *this; }
    wxTextInputStream& operator>>( wxInt64& i ) { i = Read64S(); return *this; }
    wxTextInputStream& operator>>( wxUint16& i ) { i = Read16(); return *this; }
    wxTextInputStream& operator>>( wxUint32& i ) { i = Read32(); return *this; }
    wxTextInputStream& operator>>( wxUint64& i ) { i = Read64(); return *this; }
    wxTextInputStream& operator>>( double& d ) { d = ReadDouble(); return *this; }
    wxTextInputStream& operator>>( float& f ) { f = static_cast<float>( ReadDouble() ); return *this; }

private:
    bool   EatEOL( wxChar aCh );
    void   UngetLast();
    wxChar NextNonSeparators();

    wxInputStream&            m_input;
    wxString                  m_separators;
    std::unique_ptr<wxMBConv> m_conv;

    // Bytes [m_validBegin, m_validEnd) of m_lastBytes are read but not yet decoded, the
    // ones before m_validBegin are those of the last character returned (for UngetLast()).
    char   m_lastBytes[10] = {};
    size_t m_validBegin = 0;
    size_t m_validEnd = 0;
};


class wxTextOutputStream
{
public:
    wxTextOutputStream( wxOutputStream& aStream, wxEOL aMode = wxEOL_NATIVE,
                        const wxMBConv& aConv = wxConvAuto() );
    virtual ~wxTextOutputStream() = default;

    void                WriteString( const wxString& aStr );
    wxTextOutputStream& PutChar( wxChar aCh );
    void                Flush() {}

    template <typename T>
    void Write( const T& aValue )
    {
        wxString str;
        str << aValue;
        WriteString( str );
    }

    void Write64( wxUint64 i ) { WriteString( wxString::Format( wxT( "%llu" ), (unsigned long long) i ) ); }
    void Write32( wxUint32 i ) { WriteString( wxString::Format( wxT( "%u" ), i ) ); }
    void Write16( wxUint16 i ) { WriteString( wxString::Format( wxT( "%u" ), (unsigned) i ) ); }
    void Write8( wxUint8 i ) { WriteString( wxString::Format( wxT( "%u" ), (unsigned) i ) ); }
    void WriteDouble( double d ) { WriteString( wxString::Format( wxT( "%f" ), d ) ); }

    void  SetMode( wxEOL aMode = wxEOL_NATIVE ) { m_mode = aMode == wxEOL_NATIVE ? kiwxNativeEOL : aMode; }
    wxEOL GetMode() { return m_mode; }

    wxTextOutputStream& operator<<( const wxString& s ) { WriteString( s ); return *this; }
    wxTextOutputStream& operator<<( const char* s ) { WriteString( wxString( s ) ); return *this; }
    wxTextOutputStream& operator<<( const wchar_t* s ) { WriteString( wxString( s ) ); return *this; }
    wxTextOutputStream& operator<<( char c ) { WriteString( wxString::FromAscii( c ) ); return *this; }
    wxTextOutputStream& operator<<( wchar_t c ) { return PutChar( c ); }
    wxTextOutputStream& operator<<( wxInt16 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( wxInt32 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( wxInt64 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( wxUint16 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( wxUint32 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( wxUint64 i ) { Write( i ); return *this; }
    wxTextOutputStream& operator<<( double d ) { Write( d ); return *this; }
    wxTextOutputStream& operator<<( float f ) { Write( f ); return *this; }
    wxTextOutputStream& operator<<( wxTextOutputStream& ( *aFunc )( wxTextOutputStream& ) ) { return aFunc( *this ); }

private:
    wxOutputStream&           m_output;
    wxEOL                     m_mode;
    std::unique_ptr<wxMBConv> m_conv;
};


inline wxTextOutputStream& endl( wxTextOutputStream& aStream )
{
    aStream.PutChar( wxT( '\n' ) );
    return aStream;
}

#pragma GCC visibility pop
