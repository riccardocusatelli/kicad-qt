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
 * @file kiwx_diff_streams.cpp
 *
 * Differential test of kiwx: file, memory, string, text, data, std and zlib streams, zip and
 * tar archives (see run_diff.sh).  Compressed output is compared byte for byte, archives are
 * compared by what reading them back gives (the reference archives are from Python).
 */

#include "kiwx_diff.h"
#include "kiwx_diff_archdata.h"

#include <wx/datstrm.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/stdstream.h>
#include <wx/tarstrm.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/zstream.h>

#include <memory>
#include <sstream>


static std::string hexDigest( const void* aData, size_t aSize )
{
    // FNV-1a: enough to tell if two byte streams are the same
    uint64_t           h = 1469598103934665603ULL;
    const unsigned char* p = static_cast<const unsigned char*>( aData );

    for( size_t i = 0; i < aSize; ++i )
        h = ( h ^ p[i] ) * 1099511628211ULL;

    char buf[64];
    std::snprintf( buf, sizeof( buf ), "%zu:%016llx", aSize, (unsigned long long) h );
    return buf;
}


static std::string readAll( wxInputStream& aIn )
{
    std::string out;
    char        buf[37];

    while( true )
    {
        aIn.Read( buf, sizeof( buf ) );
        size_t n = aIn.LastRead();

        if( n == 0 )
            break;

        out.append( buf, n );
    }

    return out;
}


static void testMemoryStreams()
{
    SECTION( "memory streams" );

    const char          data[] = "0123456789abcdefghij";
    wxMemoryInputStream in( data, 20 );

    OUT( in.IsOk() );
    OUT( (long long) in.GetLength() );
    OUT( in.GetSize() );
    OUT( in.IsSeekable() );
    OUT( in.GetC() );
    OUT( in.Peek() );
    OUT( (long long) in.TellI() );

    char buf[8] = {};
    in.Read( buf, 5 );
    OUT( in.LastRead() );
    OUT( std::string( buf, 5 ) );
    OUT( (long long) in.TellI() );
    OUT( (long long) in.SeekI( 3, wxFromCurrent ) );
    OUT( in.GetC() );
    OUT( (long long) in.SeekI( -2, wxFromEnd ) );
    in.Read( buf, 8 );
    OUT( in.LastRead() );
    OUT( in.Eof() );
    OUT( (int) in.GetLastError() );
    OUT( in.IsOk() );
    in.Read( buf, 8 );
    OUT( in.LastRead() );
    OUT( (int) in.GetLastError() );
    OUT( (long long) in.SeekI( 0 ) );
    OUT( (int) in.GetLastError() );
    OUT( in.Ungetch( 'Z' ) );
    OUT( in.GetC() );
    OUT( in.GetC() );
    OUT( in.CanRead() );

    wxMemoryOutputStream out;
    out.Write( "hello", 5 );
    out.PutC( ' ' );
    out.Write( "world", 5 );
    OUT( out.LastWrite() );
    OUT( (long long) out.TellO() );
    OUT( (long long) out.GetLength() );

    char copy[32] = {};
    OUT( out.CopyTo( copy, sizeof( copy ) ) );
    OUT( std::string( copy ) );
    OUT( out.GetOutputStreamBuffer()->GetBufferSize() );
    OUT( std::string( static_cast<char*>( out.GetOutputStreamBuffer()->GetBufferStart() ),
                      out.GetOutputStreamBuffer()->GetBufferSize() ) );

    wxMemoryInputStream fromOut( out );
    OUT( readAll( fromOut ) );

    wxMemoryInputStream copyIn( data, 20 );
    wxMemoryOutputStream copyOut;
    copyOut << copyIn;
    OUT( (long long) copyOut.GetLength() );

    wxMemoryInputStream copyIn2( data, 20 );
    wxMemoryOutputStream copyOut2;
    copyIn2.Read( copyOut2 );
    OUT( (long long) copyOut2.GetLength() );
}


static void testStringStreams()
{
    SECTION( "string streams" );

    wxStringInputStream sin( wxString::FromUTF8( "line one\nlìne two\r\nthree" ) );
    OUT( (long long) sin.GetLength() );
    OUT( readAll( sin ) );

    wxString             str;
    wxStringOutputStream sout( &str );
    sout.Write( "abc", 3 );
    sout.Write( "\xc3\xa8", 2 );
    sout.Write( "\xc3", 1 );
    OUT( str );
    sout.Write( "\xa0z", 2 );
    OUT( str );
    OUT( sout.GetString() );

    wxStringOutputStream own;
    own.Write( "own", 3 );
    OUT( own.GetString() );
}


static void testTextStreams()
{
    SECTION( "text streams" );

    wxString             text;
    wxStringOutputStream sout( &text );
    wxTextOutputStream   tout( sout );

    tout << wxT( "number " ) << 42 << wxT( " " ) << 3.5 << wxT( " " ) << 'c' << endl;
    tout.WriteString( wxT( "second\n" ) );
    tout.Write32( 7 );
    tout << wxT( "\n" );
    tout.PutChar( wxT( 'x' ) );
    tout << wxString::FromUTF8( "àè\n" );
    tout.Flush();
    OUT( text );

    wxTextOutputStream dosOut( sout, wxEOL_DOS );
    dosOut << wxT( "a\nb\n" );
    dosOut.Flush();
    OUT( text );

    wxMemoryInputStream mem( "first line\nsecond\r\nthird\rfourth\n\nlast 12 -5 2.5 word",
                             56 );
    wxTextInputStream   tin( mem );

    for( int i = 0; i < 6; ++i )
    {
        OUT( tin.ReadLine() );
        OUT( mem.Eof() );
    }

    OUT( tin.ReadWord() );
    OUT( (long) tin.Read32() );
    OUT( (long) tin.Read32S() );
    OUT( tin.ReadDouble() );
    OUT( tin.ReadWord() );
    OUT( mem.Eof() );
    OUT( tin.ReadLine() );

    const char          utf[] = "caf\xc3\xa9 ok\nsecond";
    wxMemoryInputStream mem2( utf, sizeof( utf ) - 1 );
    wxTextInputStream   tin2( mem2, wxT( " \t" ), wxConvUTF8 );
    OUT( tin2.ReadLine() );
    OUT( tin2.GetChar() );
    OUT( tin2.ReadLine() );
    OUT( mem2.Eof() );

    wxMemoryInputStream mem3( "x;y z", 5 );
    wxTextInputStream   tin3( mem3, wxT( ";" ) );
    OUT( tin3.ReadWord() );
    OUT( tin3.ReadWord() );
}


static void testDataStreams()
{
    SECTION( "data streams" );

    wxMemoryOutputStream mout;
    wxDataOutputStream   dout( mout );

    dout.Write8( 0xAB );
    dout.Write16( 0x1234 );
    dout.Write32( 0xDEADBEEF );
    dout.Write64( (wxUint64) 0x0102030405060708ULL );
    dout.WriteDouble( 1.5 );
    dout.WriteFloat( 2.25f );
    dout.WriteString( wxString::FromUTF8( "hé" ) );
    dout << (wxUint8) 1 << (wxUint16) 2 << (wxUint32) 3;
    dout.BigEndianOrdered( true );
    dout.Write16( 0x1234 );
    dout.Write32( 0xDEADBEEF );
    dout.WriteDouble( 1.5 );

    size_t      len = mout.GetLength();
    std::string bytes( len, '\0' );
    mout.CopyTo( bytes.data(), len );

    std::string hex;

    for( unsigned char c : bytes )
    {
        char b[4];
        std::snprintf( b, sizeof( b ), "%02x", c );
        hex += b;
    }

    OUT( hex );

    wxMemoryInputStream min( bytes.data(), bytes.size() );
    wxDataInputStream   din( min );
    OUT( (int) din.Read8() );
    OUT( (int) din.Read16() );
    OUT( (unsigned long) din.Read32() );
    OUT( (unsigned long long) din.Read64() );
    OUT( din.ReadDouble() );
    OUT( (double) din.ReadFloat() );
    OUT( din.ReadString() );
}


static void testStdStreams()
{
    SECTION( "std streams" );

    const char          data[] = "word1 word2 12 3.5\nline two\n";
    wxMemoryInputStream mem( data, sizeof( data ) - 1 );
    wxStdInputStream    sin( mem );
    std::string         word;
    int                 i = 0;
    double              d = 0;
    sin >> word;
    OUT( word );
    sin >> word >> i >> d;
    OUT( word );
    OUT( i );
    OUT( d );
    std::getline( sin, word );
    OUT( word );
    std::getline( sin, word );
    OUT( word );
    OUT( sin.eof() );
    std::getline( sin, word );
    OUT( sin.eof() );

    wxMemoryInputStream mem2( data, sizeof( data ) - 1 );
    wxStdInputStream    sin2( mem2 );
    std::stringstream   all;
    all << sin2.rdbuf();
    OUT( all.str() );

    wxMemoryOutputStream mout;
    {
        wxStdOutputStream sout( mout );
        sout << "value " << 42 << ' ' << 1.25 << std::endl;
        sout.write( "raw", 3 );
        sout.flush();
    }
    std::string out( mout.GetLength(), '\0' );
    mout.CopyTo( out.data(), out.size() );
    OUT( out );
}


static void testFileStreams()
{
    SECTION( "file streams" );

    wxFileName::Mkdir( wxT( "streams_scratch" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    {
        wxFileOutputStream out( wxT( "streams_scratch/f1.bin" ) );
        OUT( out.IsOk() );
        out.Write( "file stream data\n", 17 );
        OUT( out.LastWrite() );
        OUT( (long long) out.TellO() );
        OUT( out.Close() );
    }

    {
        wxFileInputStream in( wxT( "streams_scratch/f1.bin" ) );
        OUT( in.IsOk() );
        OUT( (long long) in.GetLength() );
        OUT( in.GetSize() );
        OUT( readAll( in ) );
        OUT( in.Eof() );
        OUT( (long long) in.SeekI( 5 ) );
        OUT( in.GetC() );
        OUT( in.GetFile()->IsOpened() );
    }

    {
        wxFileInputStream bad( wxT( "streams_scratch/missing.bin" ) );
        OUT( bad.IsOk() );
    }

    {
        wxFFileOutputStream out( wxT( "streams_scratch/f2.bin" ), wxT( "wb" ) );
        OUT( out.IsOk() );
        out.Write( "ffile", 5 );
        out.Close();

        wxFFileInputStream in( wxT( "streams_scratch/f2.bin" ) );
        OUT( in.IsOk() );
        OUT( (long long) in.GetLength() );
        OUT( readAll( in ) );
        OUT( in.Eof() );
    }

    {
        wxFFileOutputStream out( wxT( "streams_scratch/f2.bin" ), wxT( "ab" ) );
        out.Write( "+more", 5 );
    }

    {
        wxFFileInputStream in( wxT( "streams_scratch/f2.bin" ), wxT( "rb" ) );
        OUT( readAll( in ) );
    }

    {
        wxTempFileOutputStream tmp( wxT( "streams_scratch/f3.txt" ) );
        tmp.Write( "temp", 4 );
        OUT( wxFileName::FileExists( wxT( "streams_scratch/f3.txt" ) ) );
        OUT( tmp.Commit() );
        OUT( wxFileName::FileExists( wxT( "streams_scratch/f3.txt" ) ) );
    }

    {
        wxTempFileOutputStream tmp( wxT( "streams_scratch/f4.txt" ) );
        tmp.Write( "never", 5 );
        tmp.Discard();
    }

    OUT( wxFileName::FileExists( wxT( "streams_scratch/f4.txt" ) ) );

    {
        wxFFile f( wxT( "streams_scratch/f3.txt" ) );
        wxString content;
        f.ReadAll( &content );
        OUT( content );
    }

    wxFileName::Rmdir( wxT( "streams_scratch" ), wxPATH_RMDIR_RECURSIVE );
}


static void testZlib()
{
    SECTION( "zlib" );

    std::string plain;

    for( int i = 0; i < 200; ++i )
        plain += "zlib round trip line " + std::to_string( i ) + "\n";

    const int flags[] = { wxZLIB_ZLIB, wxZLIB_GZIP, wxZLIB_NO_HEADER };
    const int levels[] = { -1, 0, 1, 9 };

    for( int flag : flags )
    {
        for( int level : levels )
        {
            wxMemoryOutputStream mout;

            {
                wxZlibOutputStream zout( mout, level, flag );
                zout.Write( plain.data(), 1000 );
                zout.Write( plain.data() + 1000, plain.size() - 1000 );
                OUT( (long long) zout.TellO() );
                zout.Close();
            }

            std::string comp( mout.GetLength(), '\0' );
            mout.CopyTo( comp.data(), comp.size() );
            OUT( flag * 100 + level );
            OUT( hexDigest( comp.data(), comp.size() ) );

            wxMemoryInputStream min( comp.data(), comp.size() );
            wxZlibInputStream   zin( min, flag == wxZLIB_GZIP ? wxZLIB_GZIP
                                          : flag == wxZLIB_NO_HEADER ? wxZLIB_NO_HEADER
                                                                     : wxZLIB_AUTO );
            std::string back = readAll( zin );
            OUT( back == plain );
            OUT( zin.Eof() );
            OUT( (int) zin.GetLastError() );
            OUT( (long long) zin.TellI() );

            // trailing data after the compressed stream must stay readable from the parent
        }
    }

    // a zlib stream owning its parent, as KiCad's gzip readers use it
    {
        wxMemoryOutputStream mout;
        {
            wxZlibOutputStream zout( mout, -1, wxZLIB_GZIP );
            zout.Write( "gz", 2 );
        }
        std::string comp( mout.GetLength(), '\0' );
        mout.CopyTo( comp.data(), comp.size() );
        wxZlibInputStream zin( new wxMemoryInputStream( comp.data(), comp.size() ), wxZLIB_AUTO );
        OUT( readAll( zin ) );
    }

    // corrupt input
    {
        const char          junk[] = "this is not compressed";
        wxMemoryInputStream min( junk, sizeof( junk ) - 1 );
        wxZlibInputStream   zin( min );
        char                buf[16];
        zin.Read( buf, sizeof( buf ) );
        OUT( zin.LastRead() );
        OUT( zin.IsOk() );
    }

    OUT( wxZlibInputStream::CanHandleGZip() );
}


static void listZip( wxInputStream& aIn )
{
    wxZipInputStream zip( aIn );
    OUT( zip.IsOk() );
    OUT( zip.GetTotalEntries() );

    std::unique_ptr<wxZipEntry> entry;

    while( entry.reset( zip.GetNextEntry() ), entry )
    {
        OUT( entry->GetName() );
        OUT( entry->GetInternalName() );
        OUT( entry->IsDir() );
        OUT( (long long) entry->GetSize() );
        OUT( entry->GetMethod() );
        OUT( zip.OpenEntry( *entry ) );
        OUT( zip.CanRead() );
        OUT( readAll( zip ) );
        OUT( zip.Eof() );
        OUT( (int) zip.GetLastError() );
    }

    OUT( (int) zip.GetLastError() );
}


static void testZip()
{
    SECTION( "zip read (python)" );

    {
        wxMemoryInputStream mem( kZipData, sizeof( kZipData ) );
        listZip( mem );
    }

    SECTION( "zip by name" );

    {
        wxMemoryInputStream mem( kZipData, sizeof( kZipData ) );
        wxZipInputStream    zip( mem );
        std::unique_ptr<wxZipEntry> entry;

        while( entry.reset( zip.GetNextEntry() ), entry )
        {
            if( entry->GetName() == wxT( "dir/deflated.txt" ) )
            {
                OUT( readAll( zip ).size() );
                break;
            }
        }
    }

    SECTION( "zip round trip" );

    wxMemoryOutputStream mout;

    {
        wxZipOutputStream zout( mout, 9 );
        OUT( zout.PutNextDirEntry( wxT( "folder" ) ) );
        OUT( zout.PutNextEntry( wxT( "folder/a.txt" ) ) );
        zout.Write( "content of a\n", 13 );
        OUT( zout.PutNextEntry( wxString::FromUTF8( "folder/ü.txt" ) ) );

        for( int i = 0; i < 100; ++i )
            zout.Write( "repeated ", 9 );

        wxZipEntry* e = new wxZipEntry( wxT( "stored.bin" ) );
        e->SetMethod( wxZIP_METHOD_STORE );
        OUT( zout.PutNextEntry( e ) );
        zout.Write( "raw", 3 );
        OUT( zout.CloseEntry() );
        zout.SetComment( wxT( "archive comment" ) );
        OUT( zout.Close() );
    }

    std::string bytes( mout.GetLength(), '\0' );
    mout.CopyTo( bytes.data(), bytes.size() );
    wxMemoryInputStream rin( bytes.data(), bytes.size() );
    listZip( rin );

    // from a file, as the library importers open it
    {
        wxFFileOutputStream f( wxT( "kiwx_zip_test.zip" ) );
        f.Write( bytes.data(), bytes.size() );
    }

    {
        wxFFileInputStream f( wxT( "kiwx_zip_test.zip" ) );
        wxZipInputStream   zip( f );
        OUT( zip.GetComment() );
        OUT( zip.GetTotalEntries() );
    }

    wxRemoveFile( wxT( "kiwx_zip_test.zip" ) );

    SECTION( "zip factory" );

    const wxArchiveClassFactory* factory = wxArchiveClassFactory::Find( wxT( ".zip" ),
                                                                         wxSTREAM_FILEEXT );
    OUT( factory != nullptr );

    if( factory )
    {
        wxMemoryInputStream mem( kZipData, sizeof( kZipData ) );
        std::unique_ptr<wxArchiveInputStream> arch( factory->NewStream( mem ) );
        std::unique_ptr<wxArchiveEntry>       entry;
        int                                   count = 0;

        while( entry.reset( arch->GetNextEntry() ), entry )
            ++count;

        OUT( count );
    }

    OUT( wxArchiveClassFactory::Find( wxT( ".tar" ), wxSTREAM_FILEEXT ) != nullptr );
    OUT( wxArchiveClassFactory::Find( wxT( ".foo" ), wxSTREAM_FILEEXT ) != nullptr );
}


static void listTar( wxInputStream& aIn )
{
    wxTarInputStream tar( aIn );
    std::unique_ptr<wxTarEntry> entry;

    while( entry.reset( tar.GetNextEntry() ), entry )
    {
        OUT( entry->GetName() );
        OUT( entry->IsDir() );
        OUT( (long long) entry->GetSize() );
        OUT( readAll( tar ) );
    }

    OUT( (int) tar.GetLastError() );
}


static void testTar()
{
    SECTION( "tar read (python)" );

    {
        wxMemoryInputStream mem( kTarData, sizeof( kTarData ) );
        listTar( mem );
    }

    SECTION( "tar round trip" );

    wxMemoryOutputStream mout;

    {
        wxTarOutputStream tout( mout );
        OUT( tout.PutNextEntry( wxT( "short.txt" ) ) );
        tout.Write( "short", 5 );
        wxString longName = wxT( "deep/" ) + wxString( 'n', 150 ) + wxT( ".txt" );
        OUT( tout.PutNextEntry( longName ) );
        tout.Write( "long", 4 );
        OUT( tout.PutNextDirEntry( wxT( "adir" ) ) );
        OUT( tout.Close() );
    }

    std::string bytes( mout.GetLength(), '\0' );
    mout.CopyTo( bytes.data(), bytes.size() );
    OUT( bytes.size() % 512 );
    wxMemoryInputStream rin( bytes.data(), bytes.size() );
    listTar( rin );
}


int main()
{
    testMemoryStreams();
    testStringStreams();
    testTextStreams();
    testDataStreams();
    testStdStreams();
    testFileStreams();
    testZlib();
    testZip();
    testTar();
    return 0;
}
