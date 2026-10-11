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

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/textfile.h>

#include "kiwx_private.h"

#include <cerrno>
#include <cstring>
#include <string>


namespace
{

bool decode( const std::string& aData, const wxMBConv& aConv, wxString* aStr )
{
    std::wstring out;

    if( aData.empty() )
    {
        aStr->clear();
        return true;
    }

    if( !aConv.Decode( aData.data(), aData.size(), out ) )
    {
        wxLogError( _( "Failed to convert file contents to Unicode." ) );
        return false;
    }

    *aStr = wxString( std::move( out ) );
    return true;
}

} // namespace


// wxFile

bool wxFile::Write( const wxString& aStr, const wxMBConv& aConv )
{
    std::string out;

    if( !aConv.Encode( aStr.wc_str(), aStr.length(), out ) )
        return false;

    return Write( out.data(), out.size() ) == out.size();
}


bool wxFile::ReadAll( wxString* aStr, const wxMBConv& aConv )
{
    std::string data;
    char        buf[65536];
    ssize_t     n;

    while( ( n = Read( buf, sizeof( buf ) ) ) > 0 )
        data.append( buf, n );

    if( n < 0 )
        return false;

    return decode( data, aConv, aStr );
}


bool wxFile::Eof() const
{
    return Tell() >= Length();
}


// wxTempFile

wxTempFile::~wxTempFile()
{
    if( IsOpened() )
        Discard();
}


bool wxTempFile::Commit()
{
    m_file.Close();

    if( !kiwx::RenameReplacing( m_tempName, m_name ) )
    {
        wxLogSysError( _( "can't commit changes to file '%s'" ), m_name );
        wxRemoveFile( m_tempName );
        return false;
    }

    return true;
}


void wxTempFile::Discard()
{
    m_file.Close();
    wxRemoveFile( m_tempName );
}


// wxFFile

bool wxFFile::Close()
{
    if( !m_fp )
        return true;

    int rc = std::fclose( m_fp );
    m_fp = nullptr;
    return rc == 0;
}


size_t wxFFile::Read( void* aBuf, size_t aCount )
{
    return std::fread( aBuf, 1, aCount, m_fp );
}


size_t wxFFile::Write( const void* aBuf, size_t aCount )
{
    return std::fwrite( aBuf, 1, aCount, m_fp );
}


bool wxFFile::Write( const wxString& aStr, const wxMBConv& aConv )
{
    std::string out;

    if( !aConv.Encode( aStr.wc_str(), aStr.length(), out ) )
        return false;

    return Write( out.data(), out.size() ) == out.size();
}


bool wxFFile::ReadAll( wxString* aStr, const wxMBConv& aConv )
{
    std::string data;
    char        buf[65536];
    size_t      n;

    std::clearerr( m_fp );

    while( ( n = std::fread( buf, 1, sizeof( buf ), m_fp ) ) > 0 )
        data.append( buf, n );

    if( std::ferror( m_fp ) )
    {
        wxLogSysError( _( "Read error on file '%s'" ), m_name );
        return false;
    }

    return decode( data, aConv, aStr );
}


bool wxFFile::Flush()
{
    return !m_fp || std::fflush( m_fp ) == 0;
}


bool wxFFile::Eof() const
{
    return std::feof( m_fp ) != 0;
}


bool wxFFile::Error() const
{
    return std::ferror( m_fp ) != 0;
}


// wxDir

wxString wxDir::GetName() const
{
    return m_dirName;
}


wxString wxDir::GetNameWithSep() const
{
    wxString name = GetName();

    if( !name.empty() && !wxEndsWithPathSeparator( name ) )
        name += wxFILE_SEP_PATH;

    return name;
}


bool wxDir::GetNext( wxString* aFileName ) const
{
    return m_dir && read( aFileName );
}


bool wxDir::HasFiles( const wxString& aSpec ) const
{
    wxString name;
    return GetFirst( &name, aSpec, wxDIR_FILES | wxDIR_HIDDEN );
}


bool wxDir::HasSubDirs( const wxString& aSpec ) const
{
    wxString name;
    return GetFirst( &name, aSpec, wxDIR_DIRS | wxDIR_HIDDEN );
}


size_t wxDir::Traverse( wxDirTraverser& aSink, const wxString& aFileSpec, int aFlags ) const
{
    if( !m_dir )
        return size_t( -1 );

    const wxString prefix = GetNameWithSep();
    size_t         nFiles = 0;
    bool           cont = true;

    // Subdirectories first (the file specification is for files only).
    if( aFlags & wxDIR_DIRS )
    {
        std::vector<wxString> dirs;
        wxString              dirName;

        for( bool more = GetFirst( &dirName, wxString(), ( aFlags & ~( wxDIR_FILES | wxDIR_DOTDOT ) ) | wxDIR_DIRS );
             more; more = GetNext( &dirName ) )
        {
            dirs.push_back( dirName );
        }

        for( const wxString& name : dirs )
        {
            if( !cont )
                break;

            const wxString fullDirName = prefix + name;

            switch( aSink.OnDir( fullDirName ) )
            {
            case wxDIR_STOP:
                cont = false;
                break;

            case wxDIR_CONTINUE:
            {
                wxDir subdir;
                bool  ok;

                do
                {
                    wxLogNull noLog;
                    ok = subdir.Open( fullDirName );

                    if( !ok )
                    {
                        wxDirTraverseResult res = aSink.OnOpenError( fullDirName );

                        if( res == wxDIR_STOP )
                            cont = false;

                        if( res != wxDIR_CONTINUE )
                            break;
                    }
                } while( !ok );

                if( ok )
                    nFiles += subdir.Traverse( aSink, aFileSpec, aFlags );

                break;
            }

            case wxDIR_IGNORE:
                break;
            }
        }
    }

    if( cont && ( aFlags & wxDIR_FILES ) )
    {
        wxString fileName;
        bool     more = GetFirst( &fileName, aFileSpec, aFlags & ~wxDIR_DIRS );

        while( more )
        {
            if( aSink.OnFile( prefix + fileName ) == wxDIR_STOP )
                break;

            nFiles++;
            more = GetNext( &fileName );
        }
    }

    return nFiles;
}


namespace
{

class SIMPLE_TRAVERSER : public wxDirTraverser
{
public:
    explicit SIMPLE_TRAVERSER( wxArrayString& aFiles ) : m_files( aFiles ) {}

    wxDirTraverseResult OnFile( const wxString& aFileName ) override
    {
        m_files.push_back( aFileName );
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& ) override { return wxDIR_CONTINUE; }

private:
    wxArrayString& m_files;
};


class FIND_FIRST_TRAVERSER : public wxDirTraverser
{
public:
    wxDirTraverseResult OnFile( const wxString& aFileName ) override
    {
        m_file = aFileName;
        return wxDIR_STOP;
    }

    wxDirTraverseResult OnDir( const wxString& ) override { return wxDIR_CONTINUE; }

    wxString m_file;
};

} // namespace


size_t wxDir::GetAllFiles( const wxString& aDirName, wxArrayString* aFiles,
                           const wxString& aFileSpec, int aFlags )
{
    if( !aFiles )
        return size_t( -1 );

    size_t nFiles = 0;
    wxDir  dir( aDirName );

    if( dir.IsOpened() )
    {
        SIMPLE_TRAVERSER traverser( *aFiles );
        nFiles += dir.Traverse( traverser, aFileSpec, aFlags );
    }

    return nFiles;
}


wxString wxDir::FindFirst( const wxString& aDirName, const wxString& aFileSpec, int aFlags )
{
    wxDir dir( aDirName );

    if( !dir.IsOpened() )
        return wxString();

    FIND_FIRST_TRAVERSER traverser;
    dir.Traverse( traverser, aFileSpec, aFlags | wxDIR_FILES );
    return traverser.m_file;
}


bool wxDir::Make( const wxString& aDir, int aPerm, int aFlags )
{
    return wxFileName::Mkdir( aDir, aPerm, aFlags );
}


bool wxDir::Remove( const wxString& aDir, int aFlags )
{
    return wxFileName::Rmdir( aDir, aFlags );
}


// wxTextBuffer and wxTextFile

void kiwxSplitLines( const wxString& aText, std::vector<wxString>& aLines,
                     std::vector<wxTextFileType>& aTypes )
{
    const std::wstring& str = aText.impl();
    size_t              lineStart = 0;

    for( size_t p = 0; p < str.size(); ++p )
    {
        wchar_t ch = str[p];

        if( ch == L'\r' || ch == L'\n' )
        {
            wxTextFileType type;

            if( ch == L'\r' )
                type = ( p + 1 < str.size() && str[p + 1] == L'\n' ) ? wxTextFileType_Dos
                                                                    : wxTextFileType_Mac;
            else
                type = wxTextFileType_Unix;

            aLines.emplace_back( str.substr( lineStart, p - lineStart ) );
            aTypes.push_back( type );

            if( type == wxTextFileType_Dos )
                p++;

            lineStart = p + 1;
        }
    }

    if( lineStart < str.size() )
    {
        aLines.emplace_back( str.substr( lineStart ) );
        aTypes.push_back( wxTextFileType_None );
    }
}


const wxChar* wxTextBuffer::GetEOL( wxTextFileType aType )
{
    switch( aType )
    {
    case wxTextFileType_None: return wxT( "" );
    case wxTextFileType_Unix: return wxT( "\n" );
    case wxTextFileType_Dos:  return wxT( "\r\n" );
    case wxTextFileType_Mac:  return wxT( "\r" );
    default:                  return wxT( "\n" );
    }
}


wxString wxTextBuffer::Translate( const wxString& aText, wxTextFileType aType )
{
    if( aType == wxTextFileType_None )
        return aText;

    std::vector<wxString>       lines;
    std::vector<wxTextFileType> types;
    kiwxSplitLines( aText, lines, types );

    wxString result;

    for( size_t i = 0; i < lines.size(); ++i )
    {
        result += lines[i];

        if( types[i] != wxTextFileType_None )
            result += GetEOL( aType );
    }

    return result;
}


wxTextFileType wxTextBuffer::GuessType() const
{
    size_t unix = 0, dos = 0, mac = 0;

    for( wxTextFileType type : m_types )
    {
        if( type == wxTextFileType_Unix )
            unix++;
        else if( type == wxTextFileType_Dos )
            dos++;
        else if( type == wxTextFileType_Mac )
            mac++;
    }

    if( dos > unix && dos > mac )
        return wxTextFileType_Dos;

    if( mac > unix && mac > dos )
        return wxTextFileType_Mac;

    return unix || ( !dos && !mac ) ? wxTextFileType_Unix : typeDefault;
}


bool wxTextBuffer::Create()
{
    if( Exists() )
        return false;

    if( !OnOpen( m_name, WriteAccess ) )
        return false;

    OnClose();
    return true;
}


bool wxTextBuffer::Create( const wxString& aName )
{
    m_name = aName;
    return Create();
}


bool wxTextBuffer::Open( const wxString& aName, const wxMBConv& aConv )
{
    m_name = aName;

    if( !OnOpen( m_name, ReadAccess ) )
        return false;

    m_isOpened = OnRead( aConv );
    OnClose();
    return m_isOpened;
}


bool wxTextBuffer::Close()
{
    Clear();
    m_isOpened = false;
    return true;
}


bool wxTextBuffer::Write( wxTextFileType aType, const wxMBConv& aConv )
{
    return OnWrite( aType, aConv );
}


bool wxTextFile::OnExists() const
{
    return wxFileExists( m_name );
}


bool wxTextFile::OnOpen( const wxString& aName, int aMode )
{
    return m_file.Open( aName, aMode == ReadAccess ? wxFile::read : wxFile::write );
}


bool wxTextFile::OnClose()
{
    return m_file.Close();
}


bool wxTextFile::OnRead( const wxMBConv& aConv )
{
    wxString text;

    if( !m_file.ReadAll( &text, aConv ) )
        return false;

    Clear();
    kiwxSplitLines( text, m_lines, m_types );
    return true;
}


bool wxTextFile::OnWrite( wxTextFileType aType, const wxMBConv& aConv )
{
    wxFileName fn( m_name );

    if( !fn.IsAbsolute() )
        fn.Normalize( wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_ABSOLUTE );

    wxTempFile tmp( fn.GetFullPath() );

    if( !tmp.IsOpened() )
    {
        wxLogError( _( "can't write buffer '%s' to disk." ), m_name );
        return false;
    }

    wxString text;

    for( size_t n = 0; n < m_lines.size(); n++ )
        text += m_lines[n] + GetEOL( aType == wxTextFileType_None ? m_types[n] : aType );

    return tmp.Write( text, aConv ) && tmp.Commit();
}
