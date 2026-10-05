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

#include <wx/datstrm.h>
#include <wx/debug.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/stdstream.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include <cmath>
#include <cstring>
#include <zlib.h>


// wxStreamBase

size_t wxStreamBase::GetSize() const
{
    wxFileOffset length = GetLength();
    return length == wxInvalidOffset ? 0 : static_cast<size_t>( length );
}


// wxInputStream

bool wxInputStream::Eof() const
{
    // The base class only knows it after trying to read beyond the end.
    return GetLastError() == wxSTREAM_EOF;
}


bool wxInputStream::CanRead() const
{
    // optimistic: there may be data unless we know there is not
    return m_lastError != wxSTREAM_EOF;
}


char wxInputStream::Peek()
{
    char c;
    Read( &c, 1 );

    if( m_lastError == wxSTREAM_NO_ERROR )
    {
        Ungetch( c );
        return c;
    }

    return 0;
}


int wxInputStream::GetC()
{
    unsigned char c;
    Read( &c, 1 );
    return LastRead() ? c : -1;
}


size_t wxInputStream::getWBack( void* aBuffer, size_t aSize )
{
    std::memset( aBuffer, 0, aSize );

    if( m_wbackPos >= m_wback.size() )
        return 0;

    size_t n = std::min( aSize, m_wback.size() - m_wbackPos );
    std::memcpy( aBuffer, m_wback.data() + m_wbackPos, n );
    m_wbackPos += n;

    if( m_wbackPos >= m_wback.size() )
    {
        m_wback.clear();
        m_wbackPos = 0;
    }

    return n;
}


wxInputStream& wxInputStream::Read( void* aBuffer, size_t aSize )
{
    char* p = static_cast<char*>( aBuffer );
    m_lastCount = 0;

    size_t read = getWBack( aBuffer, aSize );

    for( ;; )
    {
        aSize -= read;
        m_lastCount += read;
        p += read;

        if( !aSize )
            break;

        // something read and reading more would block (or fail): stop here
        if( p != aBuffer && !CanRead() )
            break;

        read = OnSysRead( p, aSize );

        if( !read )
            break;
    }

    return *this;
}


bool wxInputStream::ReadAll( void* aBuffer, size_t aSize )
{
    char*  buffer = static_cast<char*>( aBuffer );
    size_t total = 0;

    for( ;; )
    {
        size_t n = Read( buffer, aSize ).LastRead();

        if( !n )
            break;

        total += n;

        if( !IsOk() )
            break;

        if( n >= aSize )
        {
            aSize = 0;
            break;
        }

        aSize -= n;
        buffer += n;
    }

    m_lastCount = total;
    return aSize == 0;
}


wxInputStream& wxInputStream::Read( wxOutputStream& aStream )
{
    char   buf[4096];
    size_t total = 0;

    for( ;; )
    {
        size_t n = Read( buf, sizeof( buf ) ).LastRead();

        if( !n )
            break;

        if( aStream.Write( buf, n ).LastWrite() != n )
            break;

        total += n;
    }

    m_lastCount = total;
    return *this;
}


size_t wxInputStream::Ungetch( const void* aBuffer, size_t aSize )
{
    if( m_lastError != wxSTREAM_NO_ERROR && m_lastError != wxSTREAM_EOF )
        return 0;

    const char*       p = static_cast<const char*>( aBuffer );
    std::vector<char> rest( m_wback.begin() + m_wbackPos, m_wback.end() );

    m_wback.assign( p, p + aSize );
    m_wback.insert( m_wback.end(), rest.begin(), rest.end() );
    m_wbackPos = 0;

    if( m_lastError == wxSTREAM_EOF )
        m_lastError = wxSTREAM_NO_ERROR;

    return aSize;
}


wxFileOffset wxInputStream::SeekI( wxFileOffset aPos, wxSeekMode aMode )
{
    if( m_lastError == wxSTREAM_EOF )
        m_lastError = wxSTREAM_NO_ERROR;

    // avoid useless seeks
    wxFileOffset current = TellI();
    wxFileOffset size = GetLength();

    if( ( aMode == wxFromStart && current == aPos ) || ( aMode == wxFromCurrent && aPos == 0 )
        || ( aMode == wxFromEnd && size != wxInvalidOffset && current == size - aPos ) )
    {
        return current;
    }

    if( !IsSeekable() && aMode == wxFromCurrent && aPos > 0 )
    {
        // forward "seek" of a non seekable stream: read and discard
        char buf[4096];

        for( ; aPos >= (wxFileOffset) sizeof( buf ); aPos -= sizeof( buf ) )
        {
            Read( buf, sizeof( buf ) );

            if( m_lastError != wxSTREAM_NO_ERROR )
                return wxInvalidOffset;
        }

        Read( buf, static_cast<size_t>( aPos ) );

        if( m_lastError != wxSTREAM_NO_ERROR )
            return wxInvalidOffset;

        return TellI();
    }

    m_wback.clear();
    m_wbackPos = 0;
    return OnSysSeek( aPos, aMode );
}


wxFileOffset wxInputStream::TellI() const
{
    wxFileOffset pos = OnSysTell();

    if( pos != wxInvalidOffset )
        pos -= static_cast<wxFileOffset>( m_wback.size() - m_wbackPos );

    return pos;
}


// wxOutputStream

wxOutputStream& wxOutputStream::Write( const void* aBuffer, size_t aSize )
{
    m_lastCount = OnSysWrite( aBuffer, aSize );
    return *this;
}


bool wxOutputStream::WriteAll( const void* aBuffer, size_t aSize )
{
    const char* p = static_cast<const char*>( aBuffer );
    size_t      total = 0;

    for( ;; )
    {
        size_t n = Write( p, aSize ).LastWrite();

        if( !n )
            break;

        total += n;

        if( !IsOk() )
            break;

        if( n >= aSize )
        {
            aSize = 0;
            break;
        }

        aSize -= n;
        p += n;
    }

    m_lastCount = total;
    return aSize == 0;
}


wxOutputStream& wxOutputStream::Write( wxInputStream& aStream )
{
    aStream.Read( *this );
    return *this;
}


wxFileOffset wxOutputStream::SeekO( wxFileOffset aPos, wxSeekMode aMode )
{
    return OnSysSeek( aPos, aMode );
}


wxFileOffset wxOutputStream::TellO() const
{
    return OnSysTell();
}


// File streams

wxFileInputStream::wxFileInputStream( const wxString& aFileName ) :
        m_file( new wxFile( aFileName, wxFile::read ) ),
        m_owns( true )
{
    if( !m_file->IsOpened() )
        m_lastError = wxSTREAM_READ_ERROR;
}


wxFileInputStream::~wxFileInputStream()
{
    if( m_owns )
        delete m_file;
}


size_t wxFileInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    ssize_t n = m_file->Read( aBuffer, aSize );

    if( n == 0 )
    {
        m_lastError = wxSTREAM_EOF;
    }
    else if( n == wxInvalidOffset )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        n = 0;
    }
    else
    {
        m_lastError = wxSTREAM_NO_ERROR;
    }

    return n;
}


wxFileOutputStream::wxFileOutputStream( const wxString& aFileName ) :
        m_file( new wxFile( aFileName, wxFile::write ) ),
        m_owns( true )
{
    if( !m_file->IsOpened() )
        m_lastError = wxSTREAM_WRITE_ERROR;
}


wxFileOutputStream::~wxFileOutputStream()
{
    if( m_owns )
        delete m_file;
}


size_t wxFileOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    size_t n = m_file->Write( aBuffer, aSize );
    m_lastError = m_file->Error() ? wxSTREAM_WRITE_ERROR : wxSTREAM_NO_ERROR;
    return n;
}


wxFFileInputStream::wxFFileInputStream( const wxString& aFileName, const wxString& aMode ) :
        m_file( new wxFFile( aFileName, aMode ) ),
        m_owns( true )
{
    if( !m_file->IsOpened() )
        m_lastError = wxSTREAM_READ_ERROR;
}


wxFFileInputStream::~wxFFileInputStream()
{
    if( m_owns )
        delete m_file;
}


size_t wxFFileInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    ssize_t n = m_file->Read( aBuffer, aSize );

    // as wxWidgets: the error is not reset by a successful read
    if( !m_file->IsOpened() || m_file->Eof() )
        m_lastError = wxSTREAM_EOF;

    if( n == wxInvalidOffset )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        n = 0;
    }

    return n;
}


wxFFileOutputStream::wxFFileOutputStream( const wxString& aFileName, const wxString& aMode ) :
        m_file( new wxFFile( aFileName, aMode ) ),
        m_owns( true )
{
    if( !m_file->IsOpened() )
        m_lastError = wxSTREAM_WRITE_ERROR;
}


wxFFileOutputStream::~wxFFileOutputStream()
{
    if( m_owns )
    {
        m_file->Flush();
        delete m_file;
    }
}


size_t wxFFileOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    size_t n = m_file->Write( aBuffer, aSize );
    m_lastError = !m_file->IsOpened() || m_file->Error() ? wxSTREAM_WRITE_ERROR : wxSTREAM_NO_ERROR;
    return n;
}


// Memory streams

wxMemoryInputStream::wxMemoryInputStream( const void* aData, size_t aLen ) :
        m_data( static_cast<const char*>( aData ), static_cast<const char*>( aData ) + aLen )
{
    m_buffer = std::make_unique<wxStreamBuffer>( m_data, &m_pos );
}


wxMemoryInputStream::wxMemoryInputStream( const wxMemoryOutputStream& aStream ) :
        m_data( aStream.GetData() )
{
    m_buffer = std::make_unique<wxStreamBuffer>( m_data, &m_pos );
}


wxMemoryInputStream::wxMemoryInputStream( wxInputStream& aStream, wxFileOffset aLen )
{
    char buf[16384];

    while( aLen == wxInvalidOffset || static_cast<wxFileOffset>( m_data.size() ) < aLen )
    {
        size_t want = sizeof( buf );

        if( aLen != wxInvalidOffset )
            want = std::min<size_t>( want, aLen - m_data.size() );

        size_t n = aStream.Read( buf, want ).LastRead();

        if( !n )
            break;

        m_data.insert( m_data.end(), buf, buf + n );
    }

    m_buffer = std::make_unique<wxStreamBuffer>( m_data, &m_pos );
}


size_t wxMemoryInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    if( m_pos == m_data.size() )
    {
        m_lastError = wxSTREAM_EOF;
        return 0;
    }

    size_t n = std::min( aSize, m_data.size() - m_pos );
    std::memcpy( aBuffer, m_data.data() + m_pos, n );
    m_pos += n;
    m_lastError = wxSTREAM_NO_ERROR;
    return n;
}


char wxMemoryInputStream::Peek()
{
    if( m_pos == m_data.size() )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        return 0;
    }

    return m_data[m_pos];
}


wxFileOffset wxMemoryInputStream::OnSysSeek( wxFileOffset aPos, wxSeekMode aMode )
{
    wxFileOffset base = aMode == wxFromStart ? 0
                        : aMode == wxFromCurrent ? static_cast<wxFileOffset>( m_pos )
                                                 : static_cast<wxFileOffset>( m_data.size() );
    wxFileOffset pos = base + aPos;

    if( pos < 0 || pos > static_cast<wxFileOffset>( m_data.size() ) )
        return wxInvalidOffset;

    m_pos = static_cast<size_t>( pos );
    return pos;
}


wxMemoryOutputStream::wxMemoryOutputStream( void* aData, size_t aLen )
{
    if( aData && aLen )
        m_data.assign( static_cast<char*>( aData ), static_cast<char*>( aData ) + aLen );

    m_buffer = std::make_unique<wxStreamBuffer>( m_data, &m_pos );
}


size_t wxMemoryOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    const char* p = static_cast<const char*>( aBuffer );

    if( m_pos + aSize > m_data.size() )
        m_data.resize( m_pos + aSize );

    std::memcpy( m_data.data() + m_pos, p, aSize );
    m_pos += aSize;
    m_lastError = wxSTREAM_NO_ERROR;
    return aSize;
}


wxFileOffset wxMemoryOutputStream::OnSysSeek( wxFileOffset aPos, wxSeekMode aMode )
{
    wxFileOffset base = aMode == wxFromStart ? 0
                        : aMode == wxFromCurrent ? static_cast<wxFileOffset>( m_pos )
                                                 : static_cast<wxFileOffset>( m_data.size() );
    wxFileOffset pos = base + aPos;

    if( pos < 0 || pos > static_cast<wxFileOffset>( m_data.size() ) )
        return wxInvalidOffset;

    m_pos = static_cast<size_t>( pos );
    return pos;
}


size_t wxMemoryOutputStream::CopyTo( void* aBuffer, size_t aLen ) const
{
    size_t n = std::min( aLen, m_data.size() );
    std::memcpy( aBuffer, m_data.data(), n );
    return n;
}


// String streams

size_t wxStringInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    size_t left = m_data.size() - m_pos;

    if( !left )
    {
        m_lastError = wxSTREAM_EOF;
        return 0;
    }

    size_t n = std::min( aSize, left );
    std::memcpy( aBuffer, m_data.data() + m_pos, n );
    m_pos += n;
    return n;
}


wxFileOffset wxStringInputStream::OnSysSeek( wxFileOffset aPos, wxSeekMode aMode )
{
    wxFileOffset base = aMode == wxFromStart ? 0
                        : aMode == wxFromCurrent ? static_cast<wxFileOffset>( m_pos )
                                                 : static_cast<wxFileOffset>( m_data.size() );
    wxFileOffset pos = base + aPos;

    if( pos < 0 || pos > static_cast<wxFileOffset>( m_data.size() ) )
        return wxInvalidOffset;

    m_pos = static_cast<size_t>( pos );
    return pos;
}


wxStringOutputStream::wxStringOutputStream( wxString* aStr, const wxMBConv& aConv ) :
        m_str( aStr ? aStr : &m_ownStr ),
        m_conv( aConv.Clone() )
{
    if( aStr )
        m_conv->Encode( aStr->wc_str(), aStr->length(), m_bytes );
}


const wxString& wxStringOutputStream::GetString() const
{
    return *m_str;
}


size_t wxStringOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    m_bytes.append( static_cast<const char*>( aBuffer ), aSize );

    // Decode what is complete (a UTF-8 sequence may be split between writes).
    for( size_t trim = 0; trim < 4 && trim <= m_bytes.size(); ++trim )
    {
        std::wstring out;

        if( m_conv->Decode( m_bytes.data(), m_bytes.size() - trim, out ) )
        {
            *m_str = wxString( std::move( out ) );
            break;
        }
    }

    m_lastError = wxSTREAM_NO_ERROR;
    return aSize;
}


// wxTextInputStream (the logic of wxWidgets, which decodes one character at a time)

wxTextInputStream::wxTextInputStream( wxInputStream& aStream, const wxString& aSep,
                                      const wxMBConv& aConv ) :
        m_input( aStream ),
        m_separators( aSep ),
        m_conv( aConv.Clone() )
{
}


void wxTextInputStream::UngetLast()
{
    if( m_validEnd )
    {
        m_input.Ungetch( m_lastBytes, m_validEnd );
        m_validBegin = 0;
        m_validEnd = 0;
    }
}


wxChar wxTextInputStream::GetChar()
{
    if( m_validBegin < m_validEnd )
    {
        m_validEnd -= m_validBegin;
        std::memmove( m_lastBytes, m_lastBytes + m_validBegin, m_validEnd );
    }
    else
    {
        m_validEnd = 0;
    }

    for( size_t inlen = 0; inlen < sizeof( m_lastBytes ); inlen++ )
    {
        if( inlen >= m_validEnd )
        {
            m_lastBytes[inlen] = static_cast<char>( m_input.GetC() );

            if( m_input.LastRead() == 0 )
                return 0;

            m_validEnd++;
        }

        wchar_t wbuf[6];
        size_t  res = m_conv->ToWChar( wbuf, 6, m_lastBytes, inlen + 1 );

        if( res == 0 || res == wxCONV_FAILED )
            continue;

        if( res == 1 )
        {
            m_validBegin = inlen + 1;
            return wbuf[0];
        }

        // more than one character from more bytes: a BOM detection finally decided, restart
        inlen = static_cast<size_t>( -1 );
    }

    m_validBegin = 0;
    m_validEnd = sizeof( m_lastBytes );
    return 0;
}


wxChar wxTextInputStream::NextNonSeparators()
{
    for( ;; )
    {
        wxChar c = GetChar();

        if( !c )
            return c;

        if( c != wxT( '\n' ) && c != wxT( '\r' ) && m_separators.Find( c ) < 0 )
            return c;
    }
}


bool wxTextInputStream::EatEOL( wxChar aCh )
{
    if( aCh == wxT( '\n' ) )
        return true;

    if( aCh == wxT( '\r' ) )
    {
        wxChar c2 = GetChar();

        if( !c2 )
            return true;

        if( c2 != wxT( '\n' ) )
            UngetLast();

        return true;
    }

    return false;
}


wxUint64 wxTextInputStream::Read64( int aBase )
{
    if( !m_input )
        return 0;

    wxString           word = ReadWord();
    unsigned long long res = 0;

    if( word.empty() || !word.ToULongLong( &res, aBase ) )
        return 0;

    return res;
}


wxUint32 wxTextInputStream::Read32( int aBase )
{
    if( !m_input )
        return 0;

    wxString word = ReadWord();

    if( word.empty() )
        return 0;

    return static_cast<wxUint32>( std::wcstoul( word.wc_str(), nullptr, aBase ) );
}


wxInt64 wxTextInputStream::Read64S( int aBase )
{
    if( !m_input )
        return 0;

    wxString  word = ReadWord();
    long long res = 0;

    if( word.empty() || !word.ToLongLong( &res, aBase ) )
        return 0;

    return res;
}


wxInt32 wxTextInputStream::Read32S( int aBase )
{
    if( !m_input )
        return 0;

    wxString word = ReadWord();

    if( word.empty() )
        return 0;

    return static_cast<wxInt32>( std::wcstol( word.wc_str(), nullptr, aBase ) );
}


double wxTextInputStream::ReadDouble()
{
    if( !m_input )
        return 0;

    wxString word = ReadWord();

    if( word.empty() )
        return 0;

    return std::wcstod( word.wc_str(), nullptr );
}


wxString wxTextInputStream::ReadLine()
{
    wxString line;

    for( ;; )
    {
        wxChar c = GetChar();

        if( m_input.Eof() )
            break;

        if( !c )
        {
            // not at the end but nothing decoded: a conversion error
            m_input.Reset( wxSTREAM_READ_ERROR );
            break;
        }

        if( EatEOL( c ) )
            break;

        line += c;
    }

    return line;
}


wxString wxTextInputStream::ReadWord()
{
    wxString word;

    if( !m_input )
        return word;

    wxChar c = NextNonSeparators();

    if( !c )
        return word;

    word += c;

    while( !m_input.Eof() )
    {
        c = GetChar();

        if( !c || m_separators.Find( c ) >= 0 || EatEOL( c ) )
            break;

        word += c;
    }

    return word;
}


wxTextInputStream& wxTextInputStream::operator>>( char& c )
{
    c = static_cast<char>( m_input.GetC() );

    if( m_input.LastRead() == 0 )
        c = 0;

    if( EatEOL( static_cast<unsigned char>( c ) ) )
        c = '\n';

    return *this;
}


// wxTextOutputStream

wxTextOutputStream::wxTextOutputStream( wxOutputStream& aStream, wxEOL aMode, const wxMBConv& aConv ) :
        m_output( aStream ),
        m_mode( aMode == wxEOL_NATIVE ? wxEOL_UNIX : aMode ),
        m_conv( aConv.Clone() )
{
}


void wxTextOutputStream::WriteString( const wxString& aStr )
{
    wxString out;
    out.reserve( aStr.length() );

    for( wxChar c : aStr.impl() )
    {
        if( c == wxT( '\n' ) )
        {
            switch( m_mode )
            {
            case wxEOL_DOS: out += wxT( "\r\n" ); continue;
            case wxEOL_MAC: out += wxT( '\r' ); continue;
            default:        break;
            }
        }

        out += c;
    }

    std::string bytes;

    if( m_conv->Encode( out.wc_str(), out.length(), bytes ) )
        m_output.Write( bytes.data(), bytes.size() );
}


wxTextOutputStream& wxTextOutputStream::PutChar( wxChar aCh )
{
    // as wxWidgets with a 4 byte wchar_t: through WriteString(), so with the EOL translation
    WriteString( wxString( aCh ) );
    return *this;
}


// wxDataOutputStream / wxDataInputStream

wxDataOutputStream::wxDataOutputStream( wxOutputStream& aStream, const wxMBConv& aConv ) :
        m_output( aStream ),
        m_conv( aConv.Clone() )
{
}


void wxDataOutputStream::writeBytes( const unsigned char* aBytes, size_t aLen )
{
    // aBytes is little endian
    if( m_bigEndian )
    {
        unsigned char buf[8];

        for( size_t i = 0; i < aLen; ++i )
            buf[i] = aBytes[aLen - 1 - i];

        m_output.Write( buf, aLen );
    }
    else
    {
        m_output.Write( aBytes, aLen );
    }
}


void wxDataOutputStream::Write64( wxUint64 i )
{
    unsigned char buf[8];

    for( int b = 0; b < 8; ++b )
        buf[b] = static_cast<unsigned char>( i >> ( 8 * b ) );

    writeBytes( buf, 8 );
}


void wxDataOutputStream::Write32( wxUint32 i )
{
    unsigned char buf[4];

    for( int b = 0; b < 4; ++b )
        buf[b] = static_cast<unsigned char>( i >> ( 8 * b ) );

    writeBytes( buf, 4 );
}


void wxDataOutputStream::Write16( wxUint16 i )
{
    unsigned char buf[2] = { static_cast<unsigned char>( i ), static_cast<unsigned char>( i >> 8 ) };
    writeBytes( buf, 2 );
}


// 80 bit IEEE extended numbers (the default format of wxDataStream doubles, from Apple's code)

static void kiwxToIeeeExtended( double aNum, unsigned char* aBytes )
{
    int32_t  sign = 0;
    int      expon = 0;
    uint32_t hiMant = 0, loMant = 0;

    if( aNum < 0 )
    {
        sign = 0x8000;
        aNum = -aNum;
    }

    if( aNum != 0 )
    {
        double fMant = std::frexp( aNum, &expon );

        if( expon > 16384 || !( fMant < 1 ) )
        {
            expon = sign | 0x7FFF;
            hiMant = 0;
            loMant = 0;
        }
        else
        {
            expon += 16382;

            if( expon < 0 )
            {
                fMant = std::ldexp( fMant, expon );
                expon = 0;
            }

            expon |= sign;
            fMant = std::ldexp( fMant, 32 );
            double fsMant = std::floor( fMant );
            hiMant = static_cast<uint32_t>( fsMant );
            fMant = std::ldexp( fMant - fsMant, 32 );
            fsMant = std::floor( fMant );
            loMant = static_cast<uint32_t>( fsMant );
        }
    }

    aBytes[0] = static_cast<unsigned char>( expon >> 8 );
    aBytes[1] = static_cast<unsigned char>( expon );
    aBytes[2] = static_cast<unsigned char>( hiMant >> 24 );
    aBytes[3] = static_cast<unsigned char>( hiMant >> 16 );
    aBytes[4] = static_cast<unsigned char>( hiMant >> 8 );
    aBytes[5] = static_cast<unsigned char>( hiMant );
    aBytes[6] = static_cast<unsigned char>( loMant >> 24 );
    aBytes[7] = static_cast<unsigned char>( loMant >> 16 );
    aBytes[8] = static_cast<unsigned char>( loMant >> 8 );
    aBytes[9] = static_cast<unsigned char>( loMant );
}


static double kiwxFromIeeeExtended( const unsigned char* aBytes )
{
    int32_t  expon = ( ( aBytes[0] & 0x7F ) << 8 ) | aBytes[1];
    uint32_t hiMant = ( uint32_t( aBytes[2] ) << 24 ) | ( uint32_t( aBytes[3] ) << 16 )
                      | ( uint32_t( aBytes[4] ) << 8 ) | aBytes[5];
    uint32_t loMant = ( uint32_t( aBytes[6] ) << 24 ) | ( uint32_t( aBytes[7] ) << 16 )
                      | ( uint32_t( aBytes[8] ) << 8 ) | aBytes[9];
    double   f;

    if( expon == 0 && hiMant == 0 && loMant == 0 )
    {
        f = 0;
    }
    else if( expon == 0x7FFF )
    {
        f = HUGE_VAL;
    }
    else
    {
        expon -= 16383;
        f = std::ldexp( double( hiMant ), expon -= 31 );
        f += std::ldexp( double( loMant ), expon -= 32 );
    }

    return ( aBytes[0] & 0x80 ) ? -f : f;
}


void wxDataOutputStream::WriteDouble( double d )
{
    if( m_useExtended )
    {
        unsigned char buf[10];
        kiwxToIeeeExtended( d, buf );
        m_output.Write( buf, 10 );
        return;
    }

    wxUint64 bits;
    std::memcpy( &bits, &d, 8 );
    Write64( bits );
}


void wxDataOutputStream::WriteFloat( float f )
{
    if( m_useExtended )
    {
        WriteDouble( f );
        return;
    }

    wxUint32 bits;
    std::memcpy( &bits, &f, 4 );
    Write32( bits );
}


void wxDataOutputStream::WriteString( const wxString& aStr )
{
    std::string bytes;
    m_conv->Encode( aStr.wc_str(), aStr.length(), bytes );
    Write32( static_cast<wxUint32>( bytes.size() ) );

    if( !bytes.empty() )
        m_output.Write( bytes.data(), bytes.size() );
}


wxDataInputStream::wxDataInputStream( wxInputStream& aStream, const wxMBConv& aConv ) :
        m_input( aStream ),
        m_conv( aConv.Clone() )
{
}


wxUint64 wxDataInputStream::readBytes( size_t aLen )
{
    unsigned char buf[8] = {};
    m_input.Read( buf, aLen );
    wxUint64 value = 0;

    for( size_t i = 0; i < aLen; ++i )
    {
        size_t idx = m_bigEndian ? i : aLen - 1 - i;
        value = ( value << 8 ) | buf[idx];
    }

    return value;
}


wxUint64 wxDataInputStream::Read64()
{
    return readBytes( 8 );
}


wxUint32 wxDataInputStream::Read32()
{
    return static_cast<wxUint32>( readBytes( 4 ) );
}


wxUint16 wxDataInputStream::Read16()
{
    return static_cast<wxUint16>( readBytes( 2 ) );
}


wxUint8 wxDataInputStream::Read8()
{
    return static_cast<wxUint8>( readBytes( 1 ) );
}


double wxDataInputStream::ReadDouble()
{
    if( m_useExtended )
    {
        unsigned char buf[10] = {};
        m_input.Read( buf, 10 );
        return kiwxFromIeeeExtended( buf );
    }

    wxUint64 bits = Read64();
    double   d;
    std::memcpy( &d, &bits, 8 );
    return d;
}


float wxDataInputStream::ReadFloat()
{
    if( m_useExtended )
        return static_cast<float>( ReadDouble() );

    wxUint32 bits = Read32();
    float    f;
    std::memcpy( &f, &bits, 4 );
    return f;
}


wxString wxDataInputStream::ReadString()
{
    wxUint32 len = Read32();

    if( !len )
        return wxString();

    std::string bytes( len, '\0' );
    m_input.Read( bytes.data(), len );
    std::wstring out;
    m_conv->Decode( bytes.data(), m_input.LastRead(), out );
    return wxString( std::move( out ) );
}


// std::streambuf adapters

wxStdInputStreamBuffer::int_type wxStdInputStreamBuffer::underflow()
{
    if( gptr() < egptr() )
        return traits_type::to_int_type( *gptr() );

    size_t n = m_stream.Read( m_buffer, sizeof( m_buffer ) ).LastRead();

    if( !n )
        return traits_type::eof();

    setg( m_buffer, m_buffer, m_buffer + n );
    return traits_type::to_int_type( *gptr() );
}


wxStdInputStreamBuffer::pos_type wxStdInputStreamBuffer::seekoff( off_type aOff, std::ios_base::seekdir aDir,
                                                                  std::ios_base::openmode )
{
    wxSeekMode mode = aDir == std::ios_base::beg ? wxFromStart
                      : aDir == std::ios_base::cur ? wxFromCurrent
                                                   : wxFromEnd;

    if( mode == wxFromCurrent )
        aOff -= egptr() - gptr();

    wxFileOffset pos = m_stream.SeekI( aOff, mode );
    setg( m_buffer, m_buffer, m_buffer );
    return pos == wxInvalidOffset ? pos_type( off_type( -1 ) ) : pos_type( pos );
}


wxStdInputStreamBuffer::pos_type wxStdInputStreamBuffer::seekpos( pos_type aPos, std::ios_base::openmode aMode )
{
    return seekoff( off_type( aPos ), std::ios_base::beg, aMode );
}


std::streamsize wxStdInputStreamBuffer::showmanyc()
{
    return m_stream.CanRead() ? 1 : -1;
}


wxStdOutputStreamBuffer::int_type wxStdOutputStreamBuffer::overflow( int_type aCh )
{
    if( traits_type::eq_int_type( aCh, traits_type::eof() ) )
        return traits_type::not_eof( aCh );

    char c = traits_type::to_char_type( aCh );
    return m_stream.Write( &c, 1 ).LastWrite() == 1 ? aCh : traits_type::eof();
}


std::streamsize wxStdOutputStreamBuffer::xsputn( const char* aData, std::streamsize aCount )
{
    return m_stream.Write( aData, aCount ).LastWrite();
}


wxStdOutputStreamBuffer::pos_type wxStdOutputStreamBuffer::seekoff( off_type aOff, std::ios_base::seekdir aDir,
                                                                    std::ios_base::openmode )
{
    wxSeekMode mode = aDir == std::ios_base::beg ? wxFromStart
                      : aDir == std::ios_base::cur ? wxFromCurrent
                                                   : wxFromEnd;

    if( mode == wxFromCurrent && aOff == 0 )
    {
        wxFileOffset pos = m_stream.TellO();
        return pos == wxInvalidOffset ? pos_type( off_type( -1 ) ) : pos_type( pos );
    }

    wxFileOffset pos = m_stream.SeekO( aOff, mode );
    return pos == wxInvalidOffset ? pos_type( off_type( -1 ) ) : pos_type( pos );
}


wxStdOutputStreamBuffer::pos_type wxStdOutputStreamBuffer::seekpos( pos_type aPos, std::ios_base::openmode aMode )
{
    return seekoff( off_type( aPos ), std::ios_base::beg, aMode );
}


// zlib streams

static constexpr size_t ZSTREAM_BUFFER_SIZE = 16384;


wxZlibInputStream::wxZlibInputStream( wxInputStream& aStream, int aFlags ) :
        wxFilterInputStream( aStream )
{
    init( aFlags );
}


wxZlibInputStream::wxZlibInputStream( wxInputStream* aStream, int aFlags ) :
        wxFilterInputStream( aStream )
{
    init( aFlags );
}


void wxZlibInputStream::init( int aFlags )
{
    m_buffer.resize( ZSTREAM_BUFFER_SIZE );
    m_inflate = new z_stream_s;
    std::memset( m_inflate, 0, sizeof( z_stream_s ) );

    int windowBits = MAX_WBITS;

    switch( aFlags )
    {
    case wxZLIB_NO_HEADER: windowBits = -MAX_WBITS; break;
    case wxZLIB_ZLIB:      windowBits = MAX_WBITS; break;
    case wxZLIB_GZIP:      windowBits = MAX_WBITS | 0x10; break;
    case wxZLIB_AUTO:      windowBits = MAX_WBITS | 0x20; break;
    default:               wxFAIL_MSG( wxT( "Invalid zlib flag" ) );
    }

    if( inflateInit2( m_inflate, windowBits ) == Z_OK )
        return;

    wxLogError( _( "Can't initialize zlib inflate stream." ) );
    m_lastError = wxSTREAM_READ_ERROR;
}


wxZlibInputStream::~wxZlibInputStream()
{
    inflateEnd( m_inflate );
    delete m_inflate;
}


bool wxZlibInputStream::SetDictionary( const char* aData, size_t aSize )
{
    return inflateSetDictionary( m_inflate, reinterpret_cast<const Bytef*>( aData ),
                                 static_cast<uInt>( aSize ) ) == Z_OK;
}


size_t wxZlibInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    if( !IsOk() || !aSize )
        return 0;

    int err = Z_OK;
    m_inflate->next_out = static_cast<unsigned char*>( aBuffer );
    m_inflate->avail_out = static_cast<uInt>( aSize );

    while( err == Z_OK && m_inflate->avail_out > 0 )
    {
        if( m_inflate->avail_in == 0 && m_parent->IsOk() )
        {
            m_parent->Read( m_buffer.data(), m_buffer.size() );
            m_inflate->next_in = m_buffer.data();
            m_inflate->avail_in = static_cast<uInt>( m_parent->LastRead() );
        }

        err = inflate( m_inflate, Z_SYNC_FLUSH );
    }

    switch( err )
    {
    case Z_OK:
        break;

    case Z_STREAM_END:
        if( m_inflate->avail_out )
        {
            // give back what follows the compressed data
            if( m_inflate->avail_in )
            {
                m_parent->Reset();
                m_parent->Ungetch( m_inflate->next_in, m_inflate->avail_in );
                m_inflate->avail_in = 0;
            }

            m_lastError = wxSTREAM_EOF;
        }

        break;

    case Z_BUF_ERROR:
        m_lastError = wxSTREAM_READ_ERROR;

        if( m_parent->Eof() )
            wxLogError( _( "Can't read inflate stream: unexpected EOF in underlying stream." ) );

        break;

    default:
    {
        wxString msg = m_inflate->msg ? wxString( m_inflate->msg ) : wxString();

        if( msg.empty() )
            msg = wxString::Format( _( "zlib error %d" ), err );

        wxLogError( _( "Can't read from inflate stream: %s" ), msg );
        m_lastError = wxSTREAM_READ_ERROR;
    }
    }

    aSize -= m_inflate->avail_out;
    m_pos += aSize;
    return aSize;
}


wxZlibOutputStream::wxZlibOutputStream( wxOutputStream& aStream, int aLevel, int aFlags ) :
        wxFilterOutputStream( aStream )
{
    init( aLevel, aFlags );
}


wxZlibOutputStream::wxZlibOutputStream( wxOutputStream* aStream, int aLevel, int aFlags ) :
        wxFilterOutputStream( aStream )
{
    init( aLevel, aFlags );
}


void wxZlibOutputStream::init( int aLevel, int aFlags )
{
    m_buffer.resize( ZSTREAM_BUFFER_SIZE );

    if( aLevel == -1 )
        aLevel = Z_DEFAULT_COMPRESSION;

    m_deflate = new z_stream_s;
    std::memset( m_deflate, 0, sizeof( z_stream_s ) );
    m_deflate->next_out = m_buffer.data();
    m_deflate->avail_out = static_cast<uInt>( m_buffer.size() );

    int windowBits = MAX_WBITS;

    switch( aFlags )
    {
    case wxZLIB_NO_HEADER: windowBits = -MAX_WBITS; break;
    case wxZLIB_ZLIB:      windowBits = MAX_WBITS; break;
    case wxZLIB_GZIP:      windowBits = MAX_WBITS | 0x10; break;
    default:               wxFAIL_MSG( wxT( "Invalid zlib flag" ) );
    }

    if( deflateInit2( m_deflate, aLevel, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY ) == Z_OK )
        return;

    wxLogError( _( "Can't initialize zlib deflate stream." ) );
    m_lastError = wxSTREAM_WRITE_ERROR;
}


bool wxZlibOutputStream::SetDictionary( const char* aData, size_t aSize )
{
    return deflateSetDictionary( m_deflate, reinterpret_cast<const Bytef*>( aData ),
                                 static_cast<uInt>( aSize ) ) == Z_OK;
}


bool wxZlibOutputStream::Close()
{
    doFlush( true );

    if( m_deflate )
    {
        deflateEnd( m_deflate );
        delete m_deflate;
        m_deflate = nullptr;
    }

    m_buffer.clear();
    m_buffer.shrink_to_fit();
    return wxFilterOutputStream::Close() && IsOk();
}


void wxZlibOutputStream::doFlush( bool aFinal )
{
    if( !m_deflate || m_buffer.empty() )
        m_lastError = wxSTREAM_WRITE_ERROR;

    if( !IsOk() )
        return;

    int  err = Z_OK;
    bool done = false;

    while( err == Z_OK || err == Z_STREAM_END )
    {
        size_t len = m_buffer.size() - m_deflate->avail_out;

        if( len )
        {
            if( m_parent->Write( m_buffer.data(), len ).LastWrite() != len )
            {
                m_lastError = wxSTREAM_WRITE_ERROR;
                break;
            }

            m_deflate->next_out = m_buffer.data();
            m_deflate->avail_out = static_cast<uInt>( m_buffer.size() );
        }

        if( done )
            break;

        err = deflate( m_deflate, aFinal ? Z_FINISH : Z_FULL_FLUSH );
        done = m_deflate->avail_out != 0 || err == Z_STREAM_END;
    }
}


size_t wxZlibOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    if( !m_deflate || m_buffer.empty() )
        m_lastError = wxSTREAM_WRITE_ERROR;

    if( !IsOk() || !aSize )
        return 0;

    int err = Z_OK;
    m_deflate->next_in = const_cast<unsigned char*>( static_cast<const unsigned char*>( aBuffer ) );
    m_deflate->avail_in = static_cast<uInt>( aSize );

    while( err == Z_OK && m_deflate->avail_in > 0 )
    {
        if( m_deflate->avail_out == 0 )
        {
            m_parent->Write( m_buffer.data(), m_buffer.size() );

            if( m_parent->LastWrite() != m_buffer.size() )
            {
                m_lastError = wxSTREAM_WRITE_ERROR;
                break;
            }

            m_deflate->next_out = m_buffer.data();
            m_deflate->avail_out = static_cast<uInt>( m_buffer.size() );
        }

        err = deflate( m_deflate, Z_NO_FLUSH );
    }

    if( err != Z_OK )
    {
        m_lastError = wxSTREAM_WRITE_ERROR;
        wxString msg = m_deflate->msg ? wxString( m_deflate->msg ) : wxString();

        if( msg.empty() )
            msg = wxString::Format( _( "zlib error %d" ), err );

        wxLogError( _( "Can't write to deflate stream: %s" ), msg );
    }

    aSize -= m_deflate->avail_in;
    m_pos += aSize;
    return aSize;
}
