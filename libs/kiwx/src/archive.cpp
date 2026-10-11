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

#include <wx/log.h>
#include <wx/tarstrm.h>
#include <wx/zipstrm.h>

#include <cstring>
#include <zlib.h>


namespace
{

uint32_t rd16( const unsigned char* p )
{
    return p[0] | ( p[1] << 8 );
}


uint32_t rd32( const unsigned char* p )
{
    return p[0] | ( p[1] << 8 ) | ( p[2] << 16 ) | ( static_cast<uint32_t>( p[3] ) << 24 );
}


void wr16( std::vector<unsigned char>& v, uint32_t x )
{
    v.push_back( x & 0xFF );
    v.push_back( ( x >> 8 ) & 0xFF );
}


void wr32( std::vector<unsigned char>& v, uint32_t x )
{
    for( int i = 0; i < 4; ++i )
        v.push_back( ( x >> ( 8 * i ) ) & 0xFF );
}


void dosDateTime( const wxDateTime& aDt, uint32_t& aTime, uint32_t& aDate )
{
    wxDateTime dt = aDt.IsValid() ? aDt : wxDateTime::Now();
    int        year = std::max( dt.GetYear(), 1980 );

    aTime = ( dt.GetHour() << 11 ) | ( dt.GetMinute() << 5 ) | ( dt.GetSecond() / 2 );
    aDate = ( ( year - 1980 ) << 9 ) | ( ( dt.GetMonth() + 1 ) << 5 ) | dt.GetDay();
}


wxDateTime fromDos( uint32_t aTime, uint32_t aDate )
{
    int year = ( ( aDate >> 9 ) & 0x7F ) + 1980;
    int month = ( ( aDate >> 5 ) & 0x0F ) - 1;
    int day = aDate & 0x1F;

    if( month < 0 || month > 11 || day < 1 )
        return wxDateTime();

    return wxDateTime( static_cast<wxDateTime::wxDateTime_t>( day ), static_cast<wxDateTime::Month>( month ),
                       year, static_cast<wxDateTime::wxDateTime_t>( ( aTime >> 11 ) & 0x1F ),
                       static_cast<wxDateTime::wxDateTime_t>( ( aTime >> 5 ) & 0x3F ),
                       static_cast<wxDateTime::wxDateTime_t>( ( aTime & 0x1F ) * 2 ) );
}


bool inflateRaw( const unsigned char* aData, size_t aLen, size_t aOutLen, std::vector<unsigned char>& aOut )
{
    z_stream z;
    std::memset( &z, 0, sizeof( z ) );

    if( inflateInit2( &z, -MAX_WBITS ) != Z_OK )
        return false;

    aOut.resize( aOutLen );
    z.next_in = const_cast<Bytef*>( aData );
    z.avail_in = static_cast<uInt>( aLen );
    z.next_out = aOut.data();
    z.avail_out = static_cast<uInt>( aOutLen );

    int rc = inflate( &z, Z_FINISH );
    inflateEnd( &z );

    return rc == Z_STREAM_END || ( rc == Z_BUF_ERROR && z.avail_out == 0 );
}


wxString zipName( const unsigned char* aName, size_t aLen, bool aUtf8, wxMBConv& aConv )
{
    if( aUtf8 )
        return wxString::FromUTF8( reinterpret_cast<const char*>( aName ), aLen );

    std::wstring out;

    if( aConv.Decode( reinterpret_cast<const char*>( aName ), aLen, out ) )
        return wxString( out );

    // Not valid in the conversion: ISO-8859-1 always works.
    return wxString::From8BitData( reinterpret_cast<const char*>( aName ), aLen );
}

} // namespace


// wxZipEntry

wxZipEntry::wxZipEntry( const wxString& aName, const wxDateTime& aDt, wxFileOffset aSize ) :
        m_dateTime( aDt ),
        m_size( aSize )
{
    SetName( aName );
}


wxString wxZipEntry::GetInternalName( const wxString& aName, wxPathFormat aFormat, bool* aIsDir )
{
    wxString internal = wxFileName::GetFormat( aFormat ) != wxPATH_UNIX
                                ? wxFileName( aName, aFormat ).GetFullPath( wxPATH_UNIX )
                                : aName;

    // both separators, whatever the format (as wxWidgets)
    auto isSep =
            []( wxUniChar aCh )
            {
                return aCh == wxT( '/' ) || aCh == wxT( '\\' );
            };

    bool isDir = !internal.empty() && isSep( internal.Last() );

    if( aIsDir )
        *aIsDir = isDir;

    if( isDir )
        internal.RemoveLast();

    while( !internal.empty() && isSep( internal[0] ) )
        internal.erase( 0, 1 );

    while( internal.length() >= 2 && internal[0] == wxT( '.' ) && isSep( internal[1] ) )
        internal.erase( 0, 2 );

    if( internal == wxT( "." ) || internal == wxT( ".." ) )
        internal.clear();

    return internal;
}


namespace
{

/// An internal name (with '/') in the given format.
wxString externalName( const wxString& aName, bool aIsDir, wxPathFormat aFormat )
{
    bool isDir = aIsDir && !aName.empty();

    if( wxFileName::GetFormat( aFormat ) == wxPATH_DOS )
    {
        wxString name( aName );
        name.Replace( wxT( "/" ), wxT( "\\" ) );
        return isDir ? name + wxT( "\\" ) : name;
    }

    return isDir ? aName + wxT( "/" ) : aName;
}

} // namespace


wxString wxZipEntry::GetName( wxPathFormat aFormat ) const
{
    return externalName( m_name, m_isDir, aFormat );
}


void wxZipEntry::SetName( const wxString& aName, wxPathFormat aFormat )
{
    bool isDir;
    m_name = GetInternalName( aName, aFormat, &isDir );
    SetIsDir( isDir );
}


void wxZipEntry::SetIsReadOnly( bool aIsReadOnly )
{
    if( aIsReadOnly )
        m_externalAttributes |= wxZIP_A_RDONLY;
    else
        m_externalAttributes &= ~wxZIP_A_RDONLY;
}


void wxZipEntry::SetMode( int aMode )
{
    m_externalAttributes = ( m_externalAttributes & 0xFFFF ) | ( static_cast<wxUint32>( aMode & 0xFFFF ) << 16 );
}


// wxZipInputStream

struct wxZipInputStream::ENTRY
{
    wxZipEntry entry;
    size_t     dataOffset;
};


wxZipInputStream::wxZipInputStream( wxInputStream& aStream, wxMBConv& aConv ) :
        wxArchiveInputStream( aStream )
{
    load( aConv );
}


wxZipInputStream::wxZipInputStream( wxInputStream* aStream, wxMBConv& aConv ) :
        wxArchiveInputStream( aStream )
{
    load( aConv );
}


wxZipInputStream::~wxZipInputStream() = default;


void wxZipInputStream::load( wxMBConv& aConv )
{
    if( !m_parent )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        return;
    }

    char buf[65536];

    for( ;; )
    {
        size_t n = m_parent->Read( buf, sizeof( buf ) ).LastRead();

        if( !n )
            break;

        m_archive.insert( m_archive.end(), buf, buf + n );
    }

    // End of central directory: the last signature in the final 64 kB.
    const size_t size = m_archive.size();
    size_t       eocd = std::string::npos;

    for( size_t i = size >= 22 ? size - 22 : 0; size >= 22; --i )
    {
        if( rd32( &m_archive[i] ) == 0x06054b50 )
        {
            eocd = i;
            break;
        }

        if( i == 0 || size - i > 65557 )
            break;
    }

    if( eocd == std::string::npos )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        wxLogError( _( "invalid zip file" ) );
        return;
    }

    const unsigned char* e = &m_archive[eocd];
    uint32_t             count = rd16( e + 10 );
    uint32_t             cdOffset = rd32( e + 16 );
    uint32_t             commentLen = rd16( e + 20 );

    if( eocd + 22 + commentLen <= size )
        m_comment = wxString::FromUTF8( reinterpret_cast<const char*>( e + 22 ), commentLen );

    size_t pos = cdOffset;

    for( uint32_t i = 0; i < count && pos + 46 <= size; ++i )
    {
        const unsigned char* c = &m_archive[pos];

        if( rd32( c ) != 0x02014b50 )
            break;

        uint32_t flags = rd16( c + 8 );
        uint32_t method = rd16( c + 10 );
        uint32_t time = rd16( c + 12 );
        uint32_t date = rd16( c + 14 );
        uint32_t crc = rd32( c + 16 );
        uint32_t csize = rd32( c + 20 );
        uint32_t usize = rd32( c + 24 );
        uint32_t nameLen = rd16( c + 28 );
        uint32_t extraLen = rd16( c + 30 );
        uint32_t cmtLen = rd16( c + 32 );
        uint32_t extAttr = rd32( c + 38 );
        uint32_t localOffset = rd32( c + 42 );

        if( pos + 46 + nameLen > size || localOffset + 30 > size )
            break;

        ENTRY ent;
        ent.entry.SetName( zipName( c + 46, nameLen, flags & 0x0800, aConv ), wxPATH_UNIX );
        ent.entry.SetMethod( method );
        ent.entry.SetDateTime( fromDos( time, date ) );
        ent.entry.SetCrc( crc );
        ent.entry.SetCompressedSize( csize );
        ent.entry.SetSize( usize );
        ent.entry.SetExternalAttributes( extAttr );
        ent.entry.SetOffset( localOffset );

        if( extAttr & wxZIP_A_SUBDIR )
            ent.entry.SetIsDir( true );

        if( cmtLen )
        {
            ent.entry.SetComment( wxString::FromUTF8(
                    reinterpret_cast<const char*>( c + 46 + nameLen + extraLen ), cmtLen ) );
        }

        const unsigned char* l = &m_archive[localOffset];
        ent.dataOffset = localOffset + 30 + rd16( l + 26 ) + rd16( l + 28 );

        m_entries.push_back( std::move( ent ) );
        pos += 46 + nameLen + extraLen + cmtLen;
    }
}


int wxZipInputStream::GetTotalEntries()
{
    return static_cast<int>( m_entries.size() );
}


wxArchiveEntry* wxZipInputStream::DoGetNextEntry()
{
    CloseEntry();

    if( m_next >= m_entries.size() )
    {
        m_lastError = wxSTREAM_EOF;
        return nullptr;
    }

    wxZipEntry* entry = new wxZipEntry( m_entries[m_next].entry );
    m_next++;

    if( !OpenEntry( *entry ) )
    {
        delete entry;
        return nullptr;
    }

    return entry;
}


bool wxZipInputStream::OpenEntry( wxArchiveEntry& aEntry )
{
    CloseEntry();

    wxZipEntry* zipEntry = dynamic_cast<wxZipEntry*>( &aEntry );

    if( !zipEntry )
        return false;

    const ENTRY* found = nullptr;

    for( const ENTRY& ent : m_entries )
    {
        if( ent.entry.GetOffset() == zipEntry->GetOffset() )
        {
            found = &ent;
            break;
        }
    }

    if( !found )
        return false;

    size_t csize = static_cast<size_t>( found->entry.GetCompressedSize() );
    size_t usize = static_cast<size_t>( found->entry.GetSize() );

    if( found->dataOffset + csize > m_archive.size() )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        return false;
    }

    const unsigned char* data = m_archive.data() + found->dataOffset;

    if( found->entry.GetMethod() == wxZIP_METHOD_STORE )
    {
        m_data.assign( data, data + csize );
    }
    else if( found->entry.GetMethod() == wxZIP_METHOD_DEFLATE )
    {
        if( !inflateRaw( data, csize, usize, m_data ) )
        {
            m_lastError = wxSTREAM_READ_ERROR;
            return false;
        }
    }
    else
    {
        wxLogError( _( "unsupported Zip compression method" ) );
        m_lastError = wxSTREAM_READ_ERROR;
        return false;
    }

    m_dataPos = 0;
    m_entryOpen = true;
    m_lastError = wxSTREAM_NO_ERROR;
    m_wback.clear();
    m_wbackPos = 0;
    return true;
}


bool wxZipInputStream::CloseEntry()
{
    m_data.clear();
    m_dataPos = 0;
    m_entryOpen = false;
    return true;
}


bool wxZipInputStream::Eof() const
{
    return !m_entryOpen || ( m_dataPos >= m_data.size() && m_wbackPos >= m_wback.size() );
}


wxFileOffset wxZipInputStream::GetLength() const
{
    return m_entryOpen ? static_cast<wxFileOffset>( m_data.size() ) : wxInvalidOffset;
}


size_t wxZipInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    if( !m_entryOpen )
    {
        m_lastError = wxSTREAM_READ_ERROR;
        return 0;
    }

    size_t n = std::min( aSize, m_data.size() - m_dataPos );
    std::memcpy( aBuffer, m_data.data() + m_dataPos, n );
    m_dataPos += n;
    m_lastError = n < aSize ? wxSTREAM_EOF : wxSTREAM_NO_ERROR;
    return n;
}


// wxZipOutputStream

wxZipOutputStream::wxZipOutputStream( wxOutputStream& aStream, int aLevel, wxMBConv& ) :
        wxArchiveOutputStream( aStream ),
        m_level( aLevel )
{
}


wxZipOutputStream::wxZipOutputStream( wxOutputStream* aStream, int aLevel, wxMBConv& ) :
        wxArchiveOutputStream( aStream ),
        m_level( aLevel )
{
}


wxZipOutputStream::~wxZipOutputStream()
{
    Close();
}


bool wxZipOutputStream::PutNextEntry( wxZipEntry* aEntry )
{
    CloseEntry();

    if( !aEntry )
        return false;

    m_entry.reset( aEntry );
    m_data.clear();
    return true;
}


bool wxZipOutputStream::PutNextEntry( wxArchiveEntry* aEntry )
{
    wxZipEntry* zipEntry = dynamic_cast<wxZipEntry*>( aEntry );

    if( !zipEntry )
    {
        delete aEntry;
        return false;
    }

    return PutNextEntry( zipEntry );
}


bool wxZipOutputStream::PutNextEntry( const wxString& aName, const wxDateTime& aDt, wxFileOffset aSize )
{
    return PutNextEntry( new wxZipEntry( aName, aDt, aSize ) );
}


bool wxZipOutputStream::PutNextDirEntry( const wxString& aName, const wxDateTime& aDt )
{
    wxZipEntry* entry = new wxZipEntry( aName, aDt );
    entry->SetIsDir();
    return PutNextEntry( entry );
}


bool wxZipOutputStream::CopyEntry( wxZipEntry* aEntry, wxZipInputStream& aInput )
{
    if( !PutNextEntry( aEntry ) )
        return false;

    if( !aInput.OpenEntry( *aEntry ) )
        return false;

    char buf[16384];

    for( ;; )
    {
        size_t n = aInput.Read( buf, sizeof( buf ) ).LastRead();

        if( !n )
            break;

        Write( buf, n );
    }

    return CloseEntry();
}


size_t wxZipOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    if( !m_entry )
    {
        m_lastError = wxSTREAM_WRITE_ERROR;
        return 0;
    }

    const unsigned char* p = static_cast<const unsigned char*>( aBuffer );
    m_data.insert( m_data.end(), p, p + aSize );
    m_lastError = wxSTREAM_NO_ERROR;
    return aSize;
}


void wxZipOutputStream::writeEntry()
{
    wxZipEntry& entry = *m_entry;

    uint32_t crc = static_cast<uint32_t>( crc32( 0L, m_data.data(), static_cast<uInt>( m_data.size() ) ) );

    std::vector<unsigned char> compressed;
    int                        method = entry.GetMethod();

    // The choices of wxWidgets: stored at level 0 or for up to 6 bytes, deflated otherwise
    // (the output is seekable or the entry is fully buffered, as here)
    if( method == wxZIP_METHOD_DEFAULT )
    {
        method = ( m_level == 0 || entry.IsDir() || m_data.size() <= 6 ) ? wxZIP_METHOD_STORE
                                                                          : wxZIP_METHOD_DEFLATE;
    }

    if( method == wxZIP_METHOD_DEFLATE )
    {
        z_stream z;
        std::memset( &z, 0, sizeof( z ) );
        deflateInit2( &z, m_level < 0 ? Z_DEFAULT_COMPRESSION : m_level, Z_DEFLATED, -MAX_WBITS, 8,
                      Z_DEFAULT_STRATEGY );
        compressed.resize( deflateBound( &z, static_cast<uLong>( m_data.size() ) ) );
        z.next_in = m_data.data();
        z.avail_in = static_cast<uInt>( m_data.size() );
        z.next_out = compressed.data();
        z.avail_out = static_cast<uInt>( compressed.size() );
        deflate( &z, Z_FINISH );
        compressed.resize( z.total_out );
        deflateEnd( &z );
    }
    // wxWidgets sees the first 4 KiB before writing the entry: a short entry that does not
    // shrink is stored
    if( method == wxZIP_METHOD_DEFLATE && m_data.size() < 4096
        && !( compressed.size() > 0 && compressed.size() < m_data.size() ) )
    {
        method = wxZIP_METHOD_STORE;
    }

    if( method != wxZIP_METHOD_DEFLATE )
        compressed = m_data;

    entry.SetMethod( method );
    entry.SetCrc( crc );
    entry.SetSize( m_data.size() );
    entry.SetCompressedSize( compressed.size() );
    entry.SetOffset( m_offset );

    wxString    name = entry.GetName( wxPATH_UNIX );
    std::string nameBytes = name.utf8_string();
    uint32_t    time, date;
    dosDateTime( entry.GetDateTime(), time, date );

    std::vector<unsigned char> header;
    wr32( header, 0x04034b50 );
    wr16( header, 20 );                     // version needed
    wr16( header, name.IsAscii() ? 0 : 0x0800 );   // UTF-8 names
    wr16( header, method );
    wr16( header, time );
    wr16( header, date );
    wr32( header, crc );
    wr32( header, static_cast<uint32_t>( compressed.size() ) );
    wr32( header, static_cast<uint32_t>( m_data.size() ) );
    wr16( header, static_cast<uint32_t>( nameBytes.size() ) );
    wr16( header, 0 );
    header.insert( header.end(), nameBytes.begin(), nameBytes.end() );

    m_parent->Write( header.data(), header.size() );

    if( !compressed.empty() )
        m_parent->Write( compressed.data(), compressed.size() );

    m_offset += header.size() + compressed.size();

    if( !entry.GetExternalAttributes() )
    {
        if( entry.IsDir() )
            entry.SetExternalAttributes( ( 040755u << 16 ) | wxZIP_A_SUBDIR );
        else
            entry.SetExternalAttributes( 0100644u << 16 );
    }

    m_centralNames.emplace_back( nameBytes.begin(), nameBytes.end() );
    m_written.push_back( std::move( m_entry ) );
}


bool wxZipOutputStream::CloseEntry()
{
    if( !m_entry )
        return true;

    writeEntry();
    m_data.clear();
    return m_parent->IsOk();
}


bool wxZipOutputStream::Close()
{
    if( m_closed )
        return true;

    CloseEntry();
    m_closed = true;

    std::vector<unsigned char> cd;

    for( size_t i = 0; i < m_written.size(); ++i )
    {
        const wxZipEntry&                 entry = *m_written[i];
        const std::vector<unsigned char>& name = m_centralNames[i];
        uint32_t                          time, date;
        dosDateTime( entry.GetDateTime(), time, date );

        std::string comment = entry.GetComment().utf8_string();

        wr32( cd, 0x02014b50 );
        wr16( cd, ( wxZIP_SYSTEM_UNIX << 8 ) | 20 );   // version made by
        wr16( cd, 20 );                                // version needed
        wr16( cd, entry.GetName().IsAscii() ? 0 : 0x0800 );
        wr16( cd, entry.GetMethod() );
        wr16( cd, time );
        wr16( cd, date );
        wr32( cd, entry.GetCrc() );
        wr32( cd, static_cast<uint32_t>( entry.GetCompressedSize() ) );
        wr32( cd, static_cast<uint32_t>( entry.GetSize() ) );
        wr16( cd, static_cast<uint32_t>( name.size() ) );
        wr16( cd, 0 );                                 // extra
        wr16( cd, static_cast<uint32_t>( comment.size() ) );
        wr16( cd, 0 );                                 // disk
        wr16( cd, 0 );                                 // internal attributes
        wr32( cd, entry.GetExternalAttributes() );
        wr32( cd, static_cast<uint32_t>( entry.GetOffset() ) );
        cd.insert( cd.end(), name.begin(), name.end() );
        cd.insert( cd.end(), comment.begin(), comment.end() );
    }

    std::string comment = m_comment.utf8_string();

    std::vector<unsigned char> eocd;
    wr32( eocd, 0x06054b50 );
    wr16( eocd, 0 );
    wr16( eocd, 0 );
    wr16( eocd, static_cast<uint32_t>( m_written.size() ) );
    wr16( eocd, static_cast<uint32_t>( m_written.size() ) );
    wr32( eocd, static_cast<uint32_t>( cd.size() ) );
    wr32( eocd, static_cast<uint32_t>( m_offset ) );
    wr16( eocd, static_cast<uint32_t>( comment.size() ) );
    eocd.insert( eocd.end(), comment.begin(), comment.end() );

    if( !cd.empty() )
        m_parent->Write( cd.data(), cd.size() );

    m_parent->Write( eocd.data(), eocd.size() );

    bool ok = m_parent->IsOk();
    return wxArchiveOutputStream::Close() && ok;
}


const wxChar* const* wxZipClassFactory::GetProtocols( wxStreamProtocolType aType ) const
{
    static const wxChar* protocols[] = { wxT( "zip" ), nullptr };
    static const wxChar* mimetypes[] = { wxT( "application/zip" ), nullptr };
    static const wxChar* fileexts[] = { wxT( ".zip" ), wxT( ".htb" ), nullptr };
    static const wxChar* empty[] = { nullptr };

    switch( aType )
    {
    case wxSTREAM_PROTOCOL: return protocols;
    case wxSTREAM_MIMETYPE: return mimetypes;
    case wxSTREAM_FILEEXT:  return fileexts;
    default:                return empty;
    }
}


// wxTarEntry

wxTarEntry::wxTarEntry( const wxString& aName, const wxDateTime& aDt, wxFileOffset aSize ) :
        m_dateTime( aDt ),
        m_size( aSize )
{
    SetName( aName );
}


wxString wxTarEntry::GetName( wxPathFormat aFormat ) const
{
    return externalName( m_name, IsDir(), aFormat );
}


void wxTarEntry::SetName( const wxString& aName, wxPathFormat aFormat )
{
    bool isDir;
    m_name = wxZipEntry::GetInternalName( aName, aFormat, &isDir );

    if( isDir )
        SetIsDir( true );
}


void wxTarEntry::SetIsDir( bool aIsDir )
{
    if( aIsDir )
    {
        m_typeFlag = wxTAR_DIRTYPE;
        m_mode |= 0111;
    }
    else if( m_typeFlag == wxTAR_DIRTYPE )
    {
        m_typeFlag = wxTAR_REGTYPE;
    }
}


void wxTarEntry::SetIsReadOnly( bool aIsReadOnly )
{
    if( aIsReadOnly )
        m_mode &= ~0222;
    else
        m_mode |= 0200;
}


// wxTarInputStream

namespace
{

long long parseOctal( const unsigned char* aField, size_t aLen )
{
    // GNU base-256 for large sizes
    if( aField[0] & 0x80 )
    {
        long long v = aField[0] & 0x7F;

        for( size_t i = 1; i < aLen; ++i )
            v = ( v << 8 ) | aField[i];

        return v;
    }

    long long v = 0;

    for( size_t i = 0; i < aLen && aField[i]; ++i )
    {
        if( aField[i] >= '0' && aField[i] <= '7' )
            v = v * 8 + ( aField[i] - '0' );
    }

    return v;
}


std::string field( const unsigned char* aField, size_t aLen )
{
    size_t n = 0;

    while( n < aLen && aField[n] )
        ++n;

    return std::string( reinterpret_cast<const char*>( aField ), n );
}

} // namespace


wxTarInputStream::wxTarInputStream( wxInputStream& aStream, wxMBConv& ) :
        wxArchiveInputStream( aStream )
{
}


wxTarInputStream::wxTarInputStream( wxInputStream* aStream, wxMBConv& ) :
        wxArchiveInputStream( aStream )
{
}


bool wxTarInputStream::readBlock( unsigned char* aBlock )
{
    return m_parent->Read( aBlock, 512 ).LastRead() == 512;
}


void wxTarInputStream::skip( wxFileOffset aBytes )
{
    char buf[4096];

    while( aBytes > 0 )
    {
        size_t n = m_parent->Read( buf, static_cast<size_t>( std::min<wxFileOffset>( aBytes, sizeof( buf ) ) ) ).LastRead();

        if( !n )
            break;

        aBytes -= n;
    }
}


wxArchiveEntry* wxTarInputStream::DoGetNextEntry()
{
    CloseEntry();

    std::string longName;
    std::string paxPath;
    long long   paxSize = -1;

    for( ;; )
    {
        unsigned char block[512];

        if( !readBlock( block ) )
        {
            m_lastError = wxSTREAM_EOF;
            return nullptr;
        }

        bool zero = true;

        for( unsigned char b : block )
        {
            if( b )
            {
                zero = false;
                break;
            }
        }

        if( zero )
        {
            m_lastError = wxSTREAM_EOF;
            return nullptr;
        }

        long long size = parseOctal( block + 124, 12 );
        char      type = static_cast<char>( block[156] );

        if( type == 'L' || type == 'x' || type == 'g' )
        {
            // GNU long name, or pax extended header: the data is the name or the records.
            std::string data( static_cast<size_t>( size ), '\0' );

            if( size )
                m_parent->Read( data.data(), static_cast<size_t>( size ) );

            skip( ( 512 - size % 512 ) % 512 );

            if( type == 'L' )
            {
                longName = data.c_str();
            }
            else if( type == 'x' )
            {
                size_t pos = 0;

                while( pos < data.size() )
                {
                    size_t sp = data.find( ' ', pos );

                    if( sp == std::string::npos )
                        break;

                    size_t      len = std::stoul( data.substr( pos, sp - pos ) );
                    std::string record = data.substr( sp + 1, len - ( sp - pos ) - 2 );
                    size_t      eq = record.find( '=' );

                    if( eq != std::string::npos )
                    {
                        std::string key = record.substr( 0, eq );
                        std::string value = record.substr( eq + 1 );

                        if( key == "path" )
                            paxPath = value;
                        else if( key == "size" )
                            paxSize = std::stoll( value );
                    }

                    pos += len;
                }
            }

            continue;
        }

        std::string name = field( block, 100 );
        std::string prefix = field( block + 345, 155 );

        if( std::memcmp( block + 257, "ustar", 5 ) == 0 && !prefix.empty() )
            name = prefix + "/" + name;

        if( !longName.empty() )
            name = longName;

        if( !paxPath.empty() )
            name = paxPath;

        if( paxSize >= 0 )
            size = paxSize;

        wxTarEntry* entry = new wxTarEntry( wxString::FromUTF8( name ),
                                            wxDateTime( static_cast<time_t>( parseOctal( block + 136, 12 ) ) ),
                                            size );
        entry->SetMode( static_cast<int>( parseOctal( block + 100, 8 ) ) );
        entry->SetTypeFlag( type ? type : wxTAR_REGTYPE );
        entry->SetLinkName( wxString::FromUTF8( field( block + 157, 100 ) ) );

        if( type == wxTAR_DIRTYPE )
            entry->SetIsDir( true );

        m_remaining = ( type == wxTAR_REGTYPE || type == 0 || type == wxTAR_CONTTYPE ) ? size : 0;
        m_padding = ( 512 - m_remaining % 512 ) % 512;
        m_entryOpen = true;
        m_lastError = wxSTREAM_NO_ERROR;
        return entry;
    }
}


bool wxTarInputStream::OpenEntry( wxArchiveEntry& )
{
    // Entries are read in order: the entry returned by GetNextEntry() is open.
    return m_entryOpen;
}


bool wxTarInputStream::CloseEntry()
{
    if( m_entryOpen )
    {
        skip( m_remaining + m_padding );
        m_remaining = 0;
        m_padding = 0;
        m_entryOpen = false;
    }

    return true;
}


size_t wxTarInputStream::OnSysRead( void* aBuffer, size_t aSize )
{
    if( !m_entryOpen || m_remaining <= 0 )
    {
        m_lastError = wxSTREAM_EOF;
        return 0;
    }

    size_t want = static_cast<size_t>( std::min<wxFileOffset>( aSize, m_remaining ) );
    size_t n = m_parent->Read( aBuffer, want ).LastRead();
    m_remaining -= n;

    if( n < aSize )
        m_lastError = m_remaining == 0 ? wxSTREAM_EOF : wxSTREAM_READ_ERROR;
    else
        m_lastError = wxSTREAM_NO_ERROR;

    return n;
}


// wxTarOutputStream

wxTarOutputStream::wxTarOutputStream( wxOutputStream& aStream, int, wxMBConv& ) :
        wxArchiveOutputStream( aStream )
{
}


wxTarOutputStream::wxTarOutputStream( wxOutputStream* aStream, int, wxMBConv& ) :
        wxArchiveOutputStream( aStream )
{
}


wxTarOutputStream::~wxTarOutputStream()
{
    Close();
}


bool wxTarOutputStream::PutNextEntry( wxTarEntry* aEntry )
{
    CloseEntry();

    if( !aEntry )
        return false;

    m_entry.reset( aEntry );
    m_data.clear();
    return true;
}


bool wxTarOutputStream::PutNextEntry( wxArchiveEntry* aEntry )
{
    wxTarEntry* tarEntry = dynamic_cast<wxTarEntry*>( aEntry );

    if( !tarEntry )
    {
        delete aEntry;
        return false;
    }

    return PutNextEntry( tarEntry );
}


bool wxTarOutputStream::PutNextEntry( const wxString& aName, const wxDateTime& aDt, wxFileOffset aSize )
{
    return PutNextEntry( new wxTarEntry( aName, aDt, aSize ) );
}


bool wxTarOutputStream::PutNextDirEntry( const wxString& aName, const wxDateTime& aDt )
{
    wxTarEntry* entry = new wxTarEntry( aName, aDt );
    entry->SetIsDir( true );
    return PutNextEntry( entry );
}


size_t wxTarOutputStream::OnSysWrite( const void* aBuffer, size_t aSize )
{
    if( !m_entry )
    {
        m_lastError = wxSTREAM_WRITE_ERROR;
        return 0;
    }

    const unsigned char* p = static_cast<const unsigned char*>( aBuffer );
    m_data.insert( m_data.end(), p, p + aSize );
    m_lastError = wxSTREAM_NO_ERROR;
    return aSize;
}


void wxTarOutputStream::writeHeader( const wxTarEntry& aEntry, wxFileOffset aSize )
{
    std::string name = aEntry.GetName( wxPATH_UNIX ).utf8_string();

    auto octal =
            []( unsigned char* aField, size_t aLen, long long aValue )
            {
                std::snprintf( reinterpret_cast<char*>( aField ), aLen, "%0*llo",
                               static_cast<int>( aLen - 1 ), aValue );
            };

    auto writeBlock =
            [&]( const std::string& aName, char aType, long long aSize, int aMode, time_t aTime )
            {
                unsigned char block[512];
                std::memset( block, 0, sizeof( block ) );

                std::string n = aName;
                std::string prefix;

                if( n.size() > 100 )
                {
                    // Split at a '/' so that the name fits in 100 and the prefix in 155.
                    size_t slash = n.rfind( '/', 155 );

                    while( slash != std::string::npos && n.size() - slash - 1 > 100 )
                        slash = slash ? n.rfind( '/', slash - 1 ) : std::string::npos;

                    if( slash != std::string::npos && slash <= 155 )
                    {
                        prefix = n.substr( 0, slash );
                        n = n.substr( slash + 1 );
                    }
                    else
                    {
                        n = n.substr( 0, 100 );
                    }
                }

                std::memcpy( block, n.data(), std::min<size_t>( n.size(), 100 ) );
                octal( block + 100, 8, aMode );
                octal( block + 108, 8, 0 );
                octal( block + 116, 8, 0 );
                octal( block + 124, 12, aSize );
                octal( block + 136, 12, static_cast<long long>( aTime ) );
                std::memset( block + 148, ' ', 8 );
                block[156] = static_cast<unsigned char>( aType );
                std::memcpy( block + 257, "ustar", 6 );
                std::memcpy( block + 263, "00", 2 );
                std::memcpy( block + 345, prefix.data(), std::min<size_t>( prefix.size(), 155 ) );

                unsigned sum = 0;

                for( unsigned char b : block )
                    sum += b;

                std::snprintf( reinterpret_cast<char*>( block + 148 ), 8, "%06o", sum );
                block[155] = ' ';
                m_parent->Write( block, sizeof( block ) );
            };

    time_t mtime = aEntry.GetDateTime().IsValid() ? aEntry.GetDateTime().GetTicks() : std::time( nullptr );

    // Names that do not fit the ustar fields go in a pax extended header.
    bool fits = name.size() <= 100;

    if( !fits && name.size() <= 256 )
    {
        size_t slash = name.rfind( '/', 155 );
        fits = slash != std::string::npos && name.size() - slash - 1 <= 100;
    }

    if( !fits )
    {
        std::string record = " path=" + name + "\n";
        size_t      len = record.size();
        size_t      total = len + std::to_string( len ).size();

        if( std::to_string( total ).size() != std::to_string( len ).size() )
            total++;

        std::string pax = std::to_string( total ) + record;

        writeBlock( "PaxHeaders/" + name.substr( name.size() > 80 ? name.size() - 80 : 0 ), 'x',
                    static_cast<long long>( pax.size() ), 0644, mtime );
        m_parent->Write( pax.data(), pax.size() );

        static const char zeros[512] = {};
        size_t            pad = ( 512 - pax.size() % 512 ) % 512;
        m_parent->Write( zeros, pad );
    }

    writeBlock( name, static_cast<char>( aEntry.GetTypeFlag() ), aEntry.IsDir() ? 0 : aSize,
                aEntry.GetMode(), mtime );
}


bool wxTarOutputStream::CloseEntry()
{
    if( !m_entry )
        return true;

    writeHeader( *m_entry, static_cast<wxFileOffset>( m_data.size() ) );

    if( !m_data.empty() && !m_entry->IsDir() )
    {
        m_parent->Write( m_data.data(), m_data.size() );

        static const char zeros[512] = {};
        size_t            pad = ( 512 - m_data.size() % 512 ) % 512;
        m_parent->Write( zeros, pad );
    }

    m_entry.reset();
    m_data.clear();
    return m_parent->IsOk();
}


bool wxTarOutputStream::Close()
{
    if( m_closed )
        return true;

    CloseEntry();
    m_closed = true;

    static const char zeros[1024] = {};
    m_parent->Write( zeros, sizeof( zeros ) );

    bool ok = m_parent->IsOk();
    return wxArchiveOutputStream::Close() && ok;
}


const wxChar* const* wxTarClassFactory::GetProtocols( wxStreamProtocolType aType ) const
{
    static const wxChar* protocols[] = { wxT( "tar" ), nullptr };
    static const wxChar* mimetypes[] = { wxT( "application/x-tar" ), nullptr };
    static const wxChar* fileexts[] = { wxT( ".tar" ), nullptr };
    static const wxChar* empty[] = { nullptr };

    switch( aType )
    {
    case wxSTREAM_PROTOCOL: return protocols;
    case wxSTREAM_MIMETYPE: return mimetypes;
    case wxSTREAM_FILEEXT:  return fileexts;
    default:                return empty;
    }
}


// wxArchiveClassFactory

namespace
{

const wxArchiveClassFactory* const* factories()
{
    static const wxZipClassFactory           zip;
    static const wxTarClassFactory           tar;
    static const wxArchiveClassFactory* const list[] = { &zip, &tar, nullptr };
    return list;
}

} // namespace


bool wxArchiveClassFactory::CanHandle( const wxString& aProtocol, wxStreamProtocolType aType ) const
{
    for( const wxChar* const* p = GetProtocols( aType ); *p; ++p )
    {
        if( aType == wxSTREAM_FILEEXT )
        {
            if( aProtocol.Lower().EndsWith( *p ) )
                return true;
        }
        else if( aProtocol.IsSameAs( *p, false ) )
        {
            return true;
        }
    }

    return false;
}


const wxArchiveClassFactory* wxArchiveClassFactory::Find( const wxString& aProtocol, wxStreamProtocolType aType )
{
    for( const wxArchiveClassFactory* const* f = factories(); *f; ++f )
    {
        if( ( *f )->CanHandle( aProtocol, aType ) )
            return *f;
    }

    return nullptr;
}


const wxArchiveClassFactory* wxArchiveClassFactory::GetFirst()
{
    return factories()[0];
}
