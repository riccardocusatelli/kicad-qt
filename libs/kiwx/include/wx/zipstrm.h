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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/archive.h>
#include <wx/strconv.h>

#include <map>
#include <vector>

enum wxZipMethod
{
    wxZIP_METHOD_STORE = 0,
    wxZIP_METHOD_DEFLATE = 8,
    wxZIP_METHOD_DEFAULT = 0xffff
};

enum wxZipSystem
{
    wxZIP_SYSTEM_MSDOS = 0,
    wxZIP_SYSTEM_UNIX = 3
};

enum wxZipAttributes
{
    wxZIP_A_RDONLY = 0x01,
    wxZIP_A_HIDDEN = 0x02,
    wxZIP_A_SYSTEM = 0x04,
    wxZIP_A_SUBDIR = 0x10,
    wxZIP_A_ARCH = 0x20,
    wxZIP_A_MASK = 0x37
};


class wxZipEntry : public wxArchiveEntry
{
public:
    wxZipEntry( const wxString& aName = wxString(), const wxDateTime& aDt = wxDateTime::Now(),
                wxFileOffset aSize = wxInvalidOffset );

    wxZipEntry* Clone() const { return new wxZipEntry( *this ); }

    wxDateTime   GetDateTime() const override { return m_dateTime; }
    wxFileOffset GetSize() const override { return m_size; }
    wxFileOffset GetOffset() const override { return m_offset; }
    bool         IsDir() const override { return m_isDir; }
    bool         IsReadOnly() const override { return ( m_externalAttributes & wxZIP_A_RDONLY ) != 0; }
    wxString     GetInternalName() const override { return m_name; }
    wxPathFormat GetInternalFormat() const override { return wxPATH_UNIX; }
    wxString     GetName( wxPathFormat aFormat = wxPATH_NATIVE ) const override;

    void SetDateTime( const wxDateTime& aDt ) override { m_dateTime = aDt; }
    void SetSize( wxFileOffset aSize ) override { m_size = aSize; }
    void SetIsDir( bool aIsDir = true ) override { m_isDir = aIsDir; }
    void SetIsReadOnly( bool aIsReadOnly = true ) override;
    void SetName( const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE ) override;

    int          GetMethod() const { return m_method; }
    void         SetMethod( int aMethod ) { m_method = aMethod; }
    wxFileOffset GetCompressedSize() const { return m_compressedSize; }
    wxUint32     GetCrc() const { return m_crc; }
    wxString     GetComment() const { return m_comment; }
    void         SetComment( const wxString& aComment ) { m_comment = aComment; }
    wxUint32     GetExternalAttributes() const { return m_externalAttributes; }
    void         SetExternalAttributes( wxUint32 aAttr ) { m_externalAttributes = aAttr; }
    int          GetMode() const { return ( m_externalAttributes >> 16 ) & 0xFFFF; }
    void         SetMode( int aMode );
    bool         IsText() const { return m_isText; }
    void         SetIsText( bool aIsText = true ) { m_isText = aIsText; }

    static wxString GetInternalName( const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE,
                                     bool* aIsDir = nullptr );

    // Internal (wxZipInputStream / wxZipOutputStream)
    void SetCrc( wxUint32 aCrc ) { m_crc = aCrc; }
    void SetCompressedSize( wxFileOffset aSize ) { m_compressedSize = aSize; }
    void SetOffset( wxFileOffset aOffset ) { m_offset = aOffset; }

protected:
    wxArchiveEntry* DoClone() const override { return Clone(); }

private:
    wxString     m_name;
    wxString     m_comment;
    wxDateTime   m_dateTime;
    wxFileOffset m_size = wxInvalidOffset;
    wxFileOffset m_compressedSize = wxInvalidOffset;
    wxFileOffset m_offset = wxInvalidOffset;
    wxUint32     m_crc = 0;
    wxUint32     m_externalAttributes = 0;
    int          m_method = wxZIP_METHOD_DEFAULT;
    bool         m_isDir = false;
    bool         m_isText = false;
};


/// Reads a zip archive (the whole archive is read from the stream first).
class wxZipInputStream : public wxArchiveInputStream
{
public:
    explicit wxZipInputStream( wxInputStream& aStream, wxMBConv& aConv = wxConvLocal );
    explicit wxZipInputStream( wxInputStream* aStream, wxMBConv& aConv = wxConvLocal );
    ~wxZipInputStream() override;

    wxZipEntry* GetNextEntry() { return static_cast<wxZipEntry*>( DoGetNextEntry() ); }
    bool        OpenEntry( wxArchiveEntry& aEntry ) override;
    bool        CloseEntry() override;
    int         GetTotalEntries();
    wxString    GetComment() { return m_comment; }

    bool         Eof() const override;
    wxFileOffset GetLength() const override;

protected:
    size_t          OnSysRead( void* aBuffer, size_t aSize ) override;
    wxArchiveEntry* DoGetNextEntry() override;

private:
    struct ENTRY;

    void load( wxMBConv& aConv );

    std::vector<ENTRY>         m_entries;
    std::vector<unsigned char> m_archive;
    size_t                     m_next = 0;
    std::vector<unsigned char> m_data;     ///< the data of the open entry
    size_t                     m_dataPos = 0;
    bool                       m_entryOpen = false;
    wxString                   m_comment;
};


/// Writes a zip archive.  The entries are written with their sizes in the local headers.
class wxZipOutputStream : public wxArchiveOutputStream
{
public:
    explicit wxZipOutputStream( wxOutputStream& aStream, int aLevel = -1, wxMBConv& aConv = wxConvUTF8 );
    explicit wxZipOutputStream( wxOutputStream* aStream, int aLevel = -1, wxMBConv& aConv = wxConvUTF8 );
    ~wxZipOutputStream() override;

    bool PutNextEntry( wxZipEntry* aEntry );
    bool PutNextEntry( wxArchiveEntry* aEntry ) override;
    bool PutNextEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now(),
                       wxFileOffset aSize = wxInvalidOffset ) override;
    bool PutNextDirEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now() ) override;
    bool CopyEntry( wxZipEntry* aEntry, wxZipInputStream& aInput );
    bool CloseEntry() override;
    bool Close() override;

    void SetLevel( int aLevel ) { m_level = aLevel; }
    int  GetLevel() const { return m_level; }
    void SetComment( const wxString& aComment ) { m_comment = aComment; }

protected:
    size_t OnSysWrite( const void* aBuffer, size_t aSize ) override;

private:
    void writeEntry();

    int                                      m_level;
    std::unique_ptr<wxZipEntry>              m_entry;
    std::vector<unsigned char>               m_data;
    std::vector<std::unique_ptr<wxZipEntry>> m_written;
    std::vector<std::vector<unsigned char>>  m_centralNames;
    wxFileOffset                             m_offset = 0;
    wxString                                 m_comment;
    bool                                     m_closed = false;
};


class wxZipClassFactory : public wxArchiveClassFactory
{
public:
    wxArchiveInputStream*  NewStream( wxInputStream& aStream ) const override { return new wxZipInputStream( aStream ); }
    wxArchiveOutputStream* NewStream( wxOutputStream& aStream ) const override { return new wxZipOutputStream( aStream ); }
    wxArchiveInputStream*  NewStream( wxInputStream* aStream ) const override { return new wxZipInputStream( aStream ); }
    wxArchiveOutputStream* NewStream( wxOutputStream* aStream ) const override { return new wxZipOutputStream( aStream ); }
    wxArchiveEntry*        NewEntry() const override { return new wxZipEntry(); }

    const wxChar* const* GetProtocols( wxStreamProtocolType aType = wxSTREAM_PROTOCOL ) const override;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
