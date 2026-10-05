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

#include <wx/crt.h>   // as wxWidgets, through wx/hashmap.h
#include <wx/archive.h>
#include <wx/strconv.h>

#include <vector>

enum wxTarType
{
    wxTAR_REGTYPE = '0',
    wxTAR_LNKTYPE = '1',
    wxTAR_SYMTYPE = '2',
    wxTAR_CHRTYPE = '3',
    wxTAR_BLKTYPE = '4',
    wxTAR_DIRTYPE = '5',
    wxTAR_FIFOTYPE = '6',
    wxTAR_CONTTYPE = '7'
};


class wxTarEntry : public wxArchiveEntry
{
public:
    wxTarEntry( const wxString& aName = wxString(), const wxDateTime& aDt = wxDateTime::Now(),
                wxFileOffset aSize = wxInvalidOffset );

    wxTarEntry* Clone() const { return new wxTarEntry( *this ); }

    wxDateTime   GetDateTime() const override { return m_dateTime; }
    wxFileOffset GetSize() const override { return m_size; }
    wxFileOffset GetOffset() const override { return m_offset; }
    bool         IsDir() const override { return m_typeFlag == wxTAR_DIRTYPE; }
    bool         IsReadOnly() const override { return !( m_mode & 0222 ); }
    wxString     GetInternalName() const override { return m_name; }
    wxPathFormat GetInternalFormat() const override { return wxPATH_UNIX; }
    wxString     GetName( wxPathFormat aFormat = wxPATH_NATIVE ) const override;

    void SetDateTime( const wxDateTime& aDt ) override { m_dateTime = aDt; }
    void SetSize( wxFileOffset aSize ) override { m_size = aSize; }
    void SetIsDir( bool aIsDir = true ) override;
    void SetIsReadOnly( bool aIsReadOnly = true ) override;
    void SetName( const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE ) override;

    int  GetMode() const { return m_mode; }
    void SetMode( int aMode ) { m_mode = aMode & 07777; }
    int  GetTypeFlag() const { return m_typeFlag; }
    void SetTypeFlag( int aType ) { m_typeFlag = aType; }
    wxString GetLinkName() const { return m_linkName; }

    // Internal
    void SetOffset( wxFileOffset aOffset ) { m_offset = aOffset; }
    void SetLinkName( const wxString& aLink ) { m_linkName = aLink; }

protected:
    wxArchiveEntry* DoClone() const override { return Clone(); }

private:
    wxString     m_name;
    wxString     m_linkName;
    wxDateTime   m_dateTime;
    wxFileOffset m_size = wxInvalidOffset;
    wxFileOffset m_offset = wxInvalidOffset;
    int          m_mode = 0644;
    int          m_typeFlag = wxTAR_REGTYPE;
};


class wxTarInputStream : public wxArchiveInputStream
{
public:
    explicit wxTarInputStream( wxInputStream& aStream, wxMBConv& aConv = wxConvLocal );
    explicit wxTarInputStream( wxInputStream* aStream, wxMBConv& aConv = wxConvLocal );

    wxTarEntry* GetNextEntry() { return static_cast<wxTarEntry*>( DoGetNextEntry() ); }
    bool        OpenEntry( wxArchiveEntry& aEntry ) override;
    bool        CloseEntry() override;

    bool Eof() const override { return m_remaining == 0 && m_wbackPos >= m_wback.size(); }

protected:
    size_t          OnSysRead( void* aBuffer, size_t aSize ) override;
    wxArchiveEntry* DoGetNextEntry() override;

private:
    bool readBlock( unsigned char* aBlock );
    void skip( wxFileOffset aBytes );

    wxFileOffset m_remaining = 0;   ///< bytes of the current entry still to read
    wxFileOffset m_padding = 0;     ///< padding after the current entry
    bool         m_entryOpen = false;
};


class wxTarOutputStream : public wxArchiveOutputStream
{
public:
    explicit wxTarOutputStream( wxOutputStream& aStream, int aFormat = 0, wxMBConv& aConv = wxConvLocal );
    explicit wxTarOutputStream( wxOutputStream* aStream, int aFormat = 0, wxMBConv& aConv = wxConvLocal );
    ~wxTarOutputStream() override;

    bool PutNextEntry( wxTarEntry* aEntry );
    bool PutNextEntry( wxArchiveEntry* aEntry ) override;
    bool PutNextEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now(),
                       wxFileOffset aSize = wxInvalidOffset ) override;
    bool PutNextDirEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now() ) override;
    bool CloseEntry() override;
    bool Close() override;

protected:
    size_t OnSysWrite( const void* aBuffer, size_t aSize ) override;

private:
    void writeHeader( const wxTarEntry& aEntry, wxFileOffset aSize );

    std::unique_ptr<wxTarEntry> m_entry;
    std::vector<unsigned char>  m_data;
    bool                        m_closed = false;
};


class wxTarClassFactory : public wxArchiveClassFactory
{
public:
    wxArchiveInputStream*  NewStream( wxInputStream& aStream ) const override { return new wxTarInputStream( aStream ); }
    wxArchiveOutputStream* NewStream( wxOutputStream& aStream ) const override { return new wxTarOutputStream( aStream ); }
    wxArchiveInputStream*  NewStream( wxInputStream* aStream ) const override { return new wxTarInputStream( aStream ); }
    wxArchiveOutputStream* NewStream( wxOutputStream* aStream ) const override { return new wxTarOutputStream( aStream ); }
    wxArchiveEntry*        NewEntry() const override { return new wxTarEntry(); }

    const wxChar* const* GetProtocols( wxStreamProtocolType aType = wxSTREAM_PROTOCOL ) const override;
};

#pragma GCC visibility pop
