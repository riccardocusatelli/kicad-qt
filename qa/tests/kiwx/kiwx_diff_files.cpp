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
 * @file kiwx_diff_files.cpp
 *
 * Differential test of kiwx: file names, files, directories, text files, environment
 * variables and dates (see run_diff.sh).  Works in a scratch directory under the current one.
 */

#include "kiwx_diff.h"

#include <wx/confbase.h>
#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/utils.h>

#include <algorithm>


static void showFileName( const wxFileName& fn )
{
    wxString dirs;

    for( const wxString& d : fn.GetDirs() )
        dirs += wxT( "<" ) + d + wxT( ">" );

    OUT( wxString::Format( wxT( "full=%s path=%s pathsep=%s name=%s ext=%s hasext=%d fullname=%s "
                                "dirs=%s abs=%d dir=%d ok=%d" ),
                           fn.GetFullPath(), fn.GetPath(), fn.GetPathWithSep(), fn.GetName(),
                           fn.GetExt(), fn.HasExt(), fn.GetFullName(), dirs, fn.IsAbsolute(),
                           fn.IsDir(), fn.IsOk() ) );
}


static void testFileNames()
{
    SECTION( "file names" );

    const wxChar* paths[] = { wxT( "/home/user/board.kicad_pcb" ), wxT( "board.kicad_pcb" ),
                              wxT( "/home/user/" ), wxT( "/home/user" ), wxT( "/" ), wxT( "" ),
                              wxT( "dir/sub/file" ), wxT( ".hidden" ), wxT( "/dir/.hidden" ),
                              wxT( "/dir/.hidden.txt" ), wxT( "file." ), wxT( "a.b.c" ),
                              wxT( "/a.dir/file" ), wxT( "./x/../y.z" ), wxT( "//double//slash//f.e" ),
                              wxT( "~/projects/p.kicad_pro" ), wxT( "${KIPRJMOD}/lib.pretty" ),
                              wxT( "/root.txt" ), wxT( "..." ), wxT( "a/." ) };

    for( const wxChar* p : paths )
    {
        OUT( wxString( p ) );
        showFileName( wxFileName( p ) );
        showFileName( wxFileName::DirName( p ) );

        wxString volume, path, name, ext;
        bool     hasExt;
        wxFileName::SplitPath( p, &volume, &path, &name, &ext, &hasExt );
        OUT( wxString::Format( wxT( "split vol=%s path=%s name=%s ext=%s has=%d" ), volume, path,
                               name, ext, hasExt ) );
    }

    wxFileName fn( wxT( "/a/b/c/file.txt" ) );
    fn.SetExt( wxT( "csv" ) );
    showFileName( fn );
    fn.ClearExt();
    showFileName( fn );
    fn.SetEmptyExt();
    showFileName( fn );
    fn.SetName( wxT( "other" ) );
    fn.SetFullName( wxT( "full.name.ext" ) );
    showFileName( fn );
    fn.AppendDir( wxT( "d" ) );
    showFileName( fn );
    fn.RemoveLastDir();
    fn.InsertDir( 1, wxT( "ins" ) );
    fn.PrependDir( wxT( "pre" ) );
    showFileName( fn );
    fn.SetPath( wxT( "rel/path" ) );
    showFileName( fn );

    wxFileName fn2( wxT( "/base" ), wxT( "name.ext" ) );
    showFileName( fn2 );
    wxFileName fn3( wxT( "/base/" ), wxT( "name" ), wxT( "ext" ) );
    showFileName( fn3 );
    wxFileName fn4( wxT( "/base" ), wxT( "name" ), wxT( "" ) );
    showFileName( fn4 );
    wxFileName fn5;
    showFileName( fn5 );
    fn5.AssignDir( wxT( "some/dir" ) );
    showFileName( fn5 );

    const wxChar* norm[] = { wxT( "/a/b/../c/./d/file.txt" ), wxT( "../../x/y" ), wxT( "/../x" ),
                             wxT( "a/../../b" ), wxT( "./" ), wxT( "a/b/../../.." ) };

    for( const wxChar* p : norm )
    {
        wxFileName n( p );
        n.Normalize( wxPATH_NORM_DOTS );
        showFileName( n );

        wxFileName a( p );
        a.MakeAbsolute( wxT( "/work/dir" ) );
        showFileName( a );
        OUT( wxFileName( p ).GetAbsolutePath( wxT( "/cwd" ) ) );
    }

    const wxChar* rel[][2] = { { wxT( "/a/b/c/file.txt" ), wxT( "/a/b" ) },
                               { wxT( "/a/b/c/file.txt" ), wxT( "/a/x/y" ) },
                               { wxT( "/a/b/file.txt" ), wxT( "/a/b/" ) },
                               { wxT( "/a/b/" ), wxT( "/a/b" ) },
                               { wxT( "/x/y/z" ), wxT( "/" ) },
                               { wxT( "/a/b/c/" ), wxT( "/a/d/e" ) } };

    for( const auto& r : rel )
    {
        wxFileName f( r[0] );
        OUT( f.MakeRelativeTo( r[1] ) );
        showFileName( f );
    }

    OUT( wxFileName( wxT( "/a/b/../c" ) ).SameAs( wxFileName( wxT( "/a/c" ) ) ) );
    OUT( wxFileName( wxT( "/a/b" ) ) == wxFileName( wxT( "/a/c" ) ) );
    OUT( wxFileName::GetPathSeparator() );
    OUT( wxFileName::GetForbiddenChars() );
    OUT( wxFileName::StripExtension( wxT( "/x/y.tar.gz" ) ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 0 ) ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 512 ) ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 1536 ) ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 5000000 ), wxT( "-" ), 2 ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 5000000 ), wxT( "-" ), 1, wxSIZE_CONV_SI ) );
    OUT( wxFileName::GetHumanReadableSize( wxULongLong( 5000000 ), wxT( "-" ), 1, wxSIZE_CONV_IEC ) );
    OUT( wxPathOnly( wxT( "/a/b/c.txt" ) ) );
    OUT( wxPathOnly( wxT( "c.txt" ) ) );
    OUT( wxPathOnly( wxT( "/c.txt" ) ) );
    OUT( wxFileNameFromPath( wxString( wxT( "/a/b/c.txt" ) ) ) );
    OUT( wxIsAbsolutePath( wxT( "/a" ) ) );
    OUT( wxIsAbsolutePath( wxT( "a" ) ) );
    OUT( wxMatchWild( wxT( "*.txt" ), wxT( "a.txt" ) ) );
    OUT( wxMatchWild( wxT( "*.txt" ), wxT( ".a.txt" ) ) );
    OUT( wxMatchWild( wxT( "*.txt" ), wxT( ".a.txt" ), false ) );
}


static void testEnv()
{
    SECTION( "environment" );

    wxSetEnv( wxT( "KIWX_TEST_VAR" ), wxT( "/value/path" ) );
    wxSetEnv( wxT( "KIWX_EMPTY" ), wxT( "" ) );

    wxString value;
    OUT( wxGetEnv( wxT( "KIWX_TEST_VAR" ), &value ) );
    OUT( value );
    OUT( wxGetEnv( wxT( "KIWX_NOT_SET" ), &value ) );

    const wxChar* inputs[] = { wxT( "$KIWX_TEST_VAR/x" ), wxT( "${KIWX_TEST_VAR}/x" ),
                               wxT( "$(KIWX_TEST_VAR)/x" ), wxT( "${KIWX_NOT_SET}/x" ),
                               wxT( "$KIWX_NOT_SET/x" ), wxT( "\\$KIWX_TEST_VAR" ),
                               wxT( "a$" ), wxT( "${KIWX_EMPTY}x" ), wxT( "$1abc" ),
                               wxT( "100%" ), wxT( "${KIWX_TEST_VAR" ), wxT( "$" ) };

    for( const wxChar* in : inputs )
        OUT( wxExpandEnvVars( in ) );

    wxFileName fn( wxT( "${KIWX_TEST_VAR}/sub/file.txt" ) );
    fn.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS );
    showFileName( fn );

    OUT( wxUnsetEnv( wxT( "KIWX_TEST_VAR" ) ) );
    OUT( wxGetEnv( wxT( "KIWX_TEST_VAR" ), nullptr ) );
}


static void testFiles()
{
    SECTION( "files" );

    wxString base = wxGetCwd() + wxT( "/kiwx_scratch" );
    wxFileName::Rmdir( base, wxPATH_RMDIR_RECURSIVE );

    OUT( wxFileName::Mkdir( base + wxT( "/a/b/c" ), wxS_DIR_DEFAULT ) );
    OUT( wxFileName::Mkdir( base + wxT( "/a/b/c" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    OUT( wxFileName::Mkdir( base + wxT( "/a/b/c" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    OUT( wxDirExists( base + wxT( "/a/b/c" ) ) );
    OUT( wxDirExists( base + wxT( "/a/b/c/" ) ) );
    OUT( wxFileExists( base + wxT( "/a/b/c" ) ) );

    wxString file = base + wxT( "/a/test.txt" );
    {
        wxFFile f( file, wxT( "w" ) );
        OUT( f.IsOpened() );
        OUT( f.Write( wxString( wxT( "line1\nline2\r\nline3\rlène4" ) ) ) );
    }

    OUT( wxFileExists( file ) );
    OUT( wxFileName( file ).GetSize().GetValue() );
    OUT( wxFileName::FileExists( file ) );
    OUT( wxFileName( file ).FileExists() );
    OUT( wxFileName( file ).IsFileReadable() );
    OUT( wxFileName( file ).IsDirWritable() );
    OUT( wxFileName::Exists( file ) );
    OUT( wxFileName::Exists( file, wxFILE_EXISTS_DIR ) );

    {
        wxFFile  f( file );
        wxString content;
        OUT( f.ReadAll( &content ) );
        OUT( content );
        OUT( f.Length() );
    }

    {
        wxFile   f( file );
        wxString content;
        OUT( f.IsOpened() );
        OUT( f.Length() );
        OUT( f.ReadAll( &content ) );
        OUT( content );
    }

    {
        wxTextFile tf( file );
        OUT( tf.Open() );
        OUT( tf.GetLineCount() );

        for( size_t i = 0; i < tf.GetLineCount(); i++ )
            OUT( wxString::Format( wxT( "%s|%d" ), tf.GetLine( i ), (int) tf.GetLineType( i ) ) );

        tf.AddLine( wxT( "added" ) );
        tf.InsertLine( wxT( "first" ), 0 );
        tf.RemoveLine( 2 );
        OUT( tf.Write() );
        tf.Close();

        wxFFile  f( file );
        wxString content;
        f.ReadAll( &content );
        OUT( content );

        wxTextFile created( base + wxT( "/a/created.txt" ) );
        OUT( created.Create() );
        created.AddLine( wxT( "x" ) );
        created.AddLine( wxT( "y" ), wxTextFileType_Dos );
        OUT( created.Write() );
        OUT( created.Create() );
        wxFFile  g( base + wxT( "/a/created.txt" ) );
        g.ReadAll( &content );
        OUT( content );
    }

    {
        // Latin-1 content: wxConvAuto falls back to ISO-8859-1.
        wxFFile f( base + wxT( "/a/latin1.txt" ), wxT( "wb" ) );
        f.Write( "caf\xe9", 4 );
        f.Close();
        wxFFile  r( base + wxT( "/a/latin1.txt" ) );
        wxString content;
        OUT( r.ReadAll( &content ) );
        OUT( content );

        wxFFile b( base + wxT( "/a/bom.txt" ), wxT( "wb" ) );
        b.Write( "\xef\xbb\xbf" "bom", 6 );
        b.Close();
        wxFFile rb( base + wxT( "/a/bom.txt" ) );
        OUT( rb.ReadAll( &content ) );
        OUT( content );
    }

    OUT( wxCopyFile( file, base + wxT( "/a/copy.txt" ) ) );
    OUT( wxCopyFile( file, base + wxT( "/a/copy.txt" ), false ) );
    OUT( wxRenameFile( base + wxT( "/a/copy.txt" ), base + wxT( "/a/b/moved.txt" ) ) );
    OUT( wxFileExists( base + wxT( "/a/copy.txt" ) ) );
    OUT( wxFileExists( base + wxT( "/a/b/moved.txt" ) ) );
    OUT( wxRemoveFile( base + wxT( "/a/b/moved.txt" ) ) );
    OUT( wxRemoveFile( base + wxT( "/a/b/moved.txt" ) ) );

    {
        wxLogNull noLog;
        wxString  dir = base + wxT( "/mk" );
        OUT( wxMkdir( dir ) );
        OUT( wxMkdir( dir ) );
        OUT( wxDirExists( dir ) );
        OUT( wxRemoveFile( dir ) );
        OUT( wxDirExists( dir ) );
        OUT( wxRmdir( file ) );
        OUT( wxFileExists( file ) );
        OUT( wxRmdir( dir ) );
        OUT( wxRmdir( dir ) );
        OUT( wxDirExists( dir ) );
        OUT( wxMkdir( base + wxT( "/no/such/parent" ) ) );

        wxString cwd = wxGetCwd();
        OUT( wxSetWorkingDirectory( base + wxT( "/a" ) ) );
        OUT( wxGetCwd().Replace( base, wxT( "BASE" ) ) );
        OUT( wxFileExists( wxT( "test.txt" ) ) );
        OUT( wxSetWorkingDirectory( base + wxT( "/none" ) ) );
        OUT( wxSetWorkingDirectory( cwd ) );
        OUT( wxGetCwd() == cwd );

        time_t mtime = wxFileModificationTime( file );
        OUT( mtime > 0 );
        OUT( mtime == wxFileName( file ).GetModificationTime().GetTicks() );
        OUT( wxFileModificationTime( base + wxT( "/none" ) ) );

        wxLongLong total, free;
        OUT( wxGetDiskSpace( base, &total, &free ) );
        OUT( total > 0 && free >= 0 && free <= total );
        OUT( wxGetDiskSpace( base + wxT( "/none" ), &total, &free ) );
        OUT( wxGetDiskSpace( wxEmptyString, &total, &free ) );

        OUT( wxNow().length() );
        OUT( wxNow().Mid( 19 ) == wxDateTime::Now().Format( wxT( " %Y" ) ) );
    }

    wxFFile( base + wxT( "/a/b/c/deep.kicad_sch" ), wxT( "w" ) ).Write( wxString( wxT( "x" ) ) );
    wxFFile( base + wxT( "/a/b/.hidden.kicad_sch" ), wxT( "w" ) ).Write( wxString( wxT( "x" ) ) );
    wxFFile( base + wxT( "/a/b/vis.kicad_pcb" ), wxT( "w" ) ).Write( wxString( wxT( "x" ) ) );

    wxArrayString files;
    OUT( wxDir::GetAllFiles( base, &files ) );
    std::sort( files.begin(), files.end() );

    for( wxString& f : files )
        f.Replace( base, wxT( "BASE" ) );

    OUT( files );

    files.clear();
    OUT( wxDir::GetAllFiles( base, &files, wxT( "*.kicad_sch" ) ) );
    OUT( files.size() );
    files.clear();
    OUT( wxDir::GetAllFiles( base, &files, wxT( "*.kicad_sch" ), wxDIR_FILES | wxDIR_DIRS ) );
    OUT( files.size() );
    files.clear();
    OUT( wxDir::GetAllFiles( base + wxT( "/a" ), &files, wxEmptyString, wxDIR_FILES ) );
    OUT( files.size() );

    wxDir dir( base + wxT( "/a/b" ) );
    OUT( dir.IsOpened() );
    OUT( dir.GetName().Replace( base, wxT( "BASE" ) ) );
    OUT( dir.HasFiles() );
    OUT( dir.HasSubDirs() );
    OUT( dir.HasFiles( wxT( "*.kicad_pcb" ) ) );
    OUT( dir.HasFiles( wxT( "*.xyz" ) ) );

    wxArrayString names;
    wxString      name;

    for( bool cont = dir.GetFirst( &name, wxEmptyString, wxDIR_DEFAULT ); cont; cont = dir.GetNext( &name ) )
        names.Add( name );

    std::sort( names.begin(), names.end() );
    OUT( names );

    names.clear();

    for( bool cont = dir.GetFirst( &name, wxT( "*" ), wxDIR_FILES ); cont; cont = dir.GetNext( &name ) )
        names.Add( name );

    std::sort( names.begin(), names.end() );
    OUT( names );

    {
        wxLogNull nolog;
        wxFile    missing;
        OUT( missing.Open( base + wxT( "/missing/file" ) ) );
        wxFFile missing2;
        OUT( missing2.Open( base + wxT( "/missing/file" ) ) );
        wxDir missing3;
        OUT( missing3.Open( base + wxT( "/missing" ) ) );
    }

    wxString tmp = wxFileName::CreateTempFileName( base + wxT( "/tmp" ) );
    OUT( wxFileExists( tmp ) );
    OUT( wxFileName( tmp ).GetName().StartsWith( wxT( "tmp" ) ) );
    OUT( wxFileName( tmp ).GetPath().Replace( base, wxT( "BASE" ) ) );

    wxFileName modfile( file );
    wxDateTime mod = modfile.GetModificationTime();
    OUT( mod.IsValid() );
    OUT( ( wxDateTime::Now() - mod ).GetSeconds().GetValue() < 60 );

    OUT( wxFileName::Rmdir( base + wxT( "/a/b/c" ) ) );
    OUT( wxFileName::Rmdir( base, wxPATH_RMDIR_RECURSIVE ) );
    OUT( wxDirExists( base ) );
}


static void testDates()
{
    SECTION( "dates" );

    wxDateTime dt( 15, wxDateTime::Mar, 2024, 13, 45, 30, 123 );
    OUT( dt.IsValid() );
    OUT( dt.FormatISODate() );
    OUT( dt.FormatISOTime() );
    OUT( dt.FormatISOCombined() );
    OUT( dt.FormatISOCombined( ' ' ) );
    OUT( dt.Format( wxT( "%Y-%m-%d %H:%M:%S.%l" ) ) );
    OUT( dt.Format( wxT( "%Hh%Mm%Ss" ) ) );
    OUT( dt.Format( wxT( "%Y-%b-%d %A" ) ) );
    OUT( dt.GetYear() );
    OUT( (int) dt.GetMonth() );
    OUT( dt.GetDay() );
    OUT( dt.GetHour() );
    OUT( dt.GetMillisecond() );
    OUT( dt.GetDayOfYear() );
    OUT( (int) wxDateTime::GetNumberOfDays( wxDateTime::Feb, 2024 ) );
    OUT( (int) wxDateTime::GetNumberOfDays( wxDateTime::Feb, 2023 ) );

    wxDateTime later = dt + wxTimeSpan::Hours( 25 );
    OUT( later.FormatISOCombined() );
    OUT( ( later - dt ).GetMinutes() );
    OUT( later.IsLaterThan( dt ) );
    OUT( dt.IsSameDate( later ) );
    OUT( wxTimeSpan::Seconds( 3725 ).Format() );
    OUT( wxTimeSpan::Seconds( 61 ).IsShorterThan( wxTimeSpan::Minutes( 1 ) ) );

    wxDateTime parsed;
    OUT( parsed.ParseISOCombined( wxT( "2023-07-01T08:09:10" ) ) );
    OUT( parsed.FormatISOCombined() );
    OUT( parsed.ParseISODate( wxT( "2020-02-29" ) ) );
    OUT( parsed.FormatISODate() );
    OUT( parsed.ParseISODate( wxT( "not a date" ) ) );

    wxDateTime fromTicks( (time_t) 1700000000 );
    OUT( fromTicks.GetTicks() );
    OUT( fromTicks.FormatISOCombined() );
    OUT( fromTicks.Format( wxT( "%Y-%m-%d %H:%M:%S" ), wxDateTime::UTC ) );
    OUT( wxDefaultDateTime.IsValid() );
    OUT( wxDateTime().IsValid() );
}


int main()
{
    testFileNames();
    testEnv();
    testFiles();
    testDates();
    return 0;
}
