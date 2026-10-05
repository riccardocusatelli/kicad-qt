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


class wxDataOutputStream
{
public:
    explicit wxDataOutputStream( wxOutputStream& aStream, const wxMBConv& aConv = wxConvUTF8 );

    void BigEndianOrdered( bool aBigEndian ) { m_bigEndian = aBigEndian; }
    void UseBasicPrecisions() { m_useExtended = false; }
    void UseExtendedPrecision() { m_useExtended = true; }

    void Write64( wxUint64 i );
    void Write32( wxUint32 i );
    void Write16( wxUint16 i );
    void Write8( wxUint8 i ) { m_output.Write( &i, 1 ); }
    void WriteDouble( double d );
    void WriteFloat( float f );
    void WriteString( const wxString& aStr );

    void Write64( const wxUint64* aBuf, size_t aCount ) { for( size_t i = 0; i < aCount; ++i ) Write64( aBuf[i] ); }
    void Write32( const wxUint32* aBuf, size_t aCount ) { for( size_t i = 0; i < aCount; ++i ) Write32( aBuf[i] ); }
    void Write16( const wxUint16* aBuf, size_t aCount ) { for( size_t i = 0; i < aCount; ++i ) Write16( aBuf[i] ); }
    void Write8( const wxUint8* aBuf, size_t aCount ) { m_output.Write( aBuf, aCount ); }
    void WriteDouble( const double* aBuf, size_t aCount ) { for( size_t i = 0; i < aCount; ++i ) WriteDouble( aBuf[i] ); }
    void WriteFloat( const float* aBuf, size_t aCount ) { for( size_t i = 0; i < aCount; ++i ) WriteFloat( aBuf[i] ); }

    wxDataOutputStream& operator<<( const wxString& s ) { WriteString( s ); return *this; }
    wxDataOutputStream& operator<<( wxInt8 c ) { Write8( static_cast<wxUint8>( c ) ); return *this; }
    wxDataOutputStream& operator<<( wxUint8 c ) { Write8( c ); return *this; }
    wxDataOutputStream& operator<<( wxInt16 i ) { Write16( static_cast<wxUint16>( i ) ); return *this; }
    wxDataOutputStream& operator<<( wxUint16 i ) { Write16( i ); return *this; }
    wxDataOutputStream& operator<<( wxInt32 i ) { Write32( static_cast<wxUint32>( i ) ); return *this; }
    wxDataOutputStream& operator<<( wxUint32 i ) { Write32( i ); return *this; }
    wxDataOutputStream& operator<<( wxInt64 i ) { Write64( static_cast<wxUint64>( i ) ); return *this; }
    wxDataOutputStream& operator<<( wxUint64 i ) { Write64( i ); return *this; }
    wxDataOutputStream& operator<<( double d ) { WriteDouble( d ); return *this; }
    wxDataOutputStream& operator<<( float f ) { WriteFloat( f ); return *this; }

private:
    void writeBytes( const unsigned char* aBytes, size_t aLen );

    wxOutputStream&           m_output;
    std::unique_ptr<wxMBConv> m_conv;
    bool                      m_bigEndian = false;
    bool                      m_useExtended = true;
};


class wxDataInputStream
{
public:
    explicit wxDataInputStream( wxInputStream& aStream, const wxMBConv& aConv = wxConvUTF8 );

    void BigEndianOrdered( bool aBigEndian ) { m_bigEndian = aBigEndian; }
    void UseBasicPrecisions() { m_useExtended = false; }
    void UseExtendedPrecision() { m_useExtended = true; }

    bool IsOk() { return m_input.IsOk(); }

    wxUint64 Read64();
    wxUint32 Read32();
    wxUint16 Read16();
    wxUint8  Read8();
    double   ReadDouble();
    float    ReadFloat();
    wxString ReadString();

    wxDataInputStream& operator>>( wxString& s ) { s = ReadString(); return *this; }
    wxDataInputStream& operator>>( wxUint8& c ) { c = Read8(); return *this; }
    wxDataInputStream& operator>>( wxUint16& i ) { i = Read16(); return *this; }
    wxDataInputStream& operator>>( wxUint32& i ) { i = Read32(); return *this; }
    wxDataInputStream& operator>>( wxUint64& i ) { i = Read64(); return *this; }
    wxDataInputStream& operator>>( double& d ) { d = ReadDouble(); return *this; }
    wxDataInputStream& operator>>( float& f ) { f = ReadFloat(); return *this; }

private:
    wxUint64 readBytes( size_t aLen );

    wxInputStream&            m_input;
    std::unique_ptr<wxMBConv> m_conv;
    bool                      m_bigEndian = false;
    bool                      m_useExtended = true;
};

#pragma GCC visibility pop
