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

#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/stream.h>

#include <memory>


class wxArchiveEntry
{
public:
    virtual ~wxArchiveEntry() = default;

    wxArchiveEntry* Clone() const { return DoClone(); }

    virtual wxDateTime   GetDateTime() const = 0;
    virtual wxFileOffset GetSize() const = 0;
    virtual wxFileOffset GetOffset() const = 0;
    virtual bool         IsDir() const = 0;
    virtual bool         IsReadOnly() const = 0;
    virtual wxString     GetInternalName() const = 0;
    virtual wxPathFormat GetInternalFormat() const = 0;
    virtual wxString     GetName( wxPathFormat aFormat = wxPATH_NATIVE ) const = 0;

    virtual void SetDateTime( const wxDateTime& aDt ) = 0;
    virtual void SetSize( wxFileOffset aSize ) = 0;
    virtual void SetIsDir( bool aIsDir = true ) = 0;
    virtual void SetIsReadOnly( bool aIsReadOnly = true ) = 0;
    virtual void SetName( const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE ) = 0;

protected:
    virtual wxArchiveEntry* DoClone() const = 0;
};


class wxArchiveInputStream : public wxFilterInputStream
{
public:
    wxArchiveEntry* GetNextEntry() { return DoGetNextEntry(); }

    virtual bool OpenEntry( wxArchiveEntry& aEntry ) = 0;
    virtual bool CloseEntry() = 0;

protected:
    wxArchiveInputStream( wxInputStream& aStream ) : wxFilterInputStream( aStream ) {}
    wxArchiveInputStream( wxInputStream* aStream ) : wxFilterInputStream( aStream ) {}

    virtual wxArchiveEntry* DoGetNextEntry() = 0;
};


class wxArchiveOutputStream : public wxFilterOutputStream
{
public:
    virtual bool PutNextEntry( wxArchiveEntry* aEntry ) = 0;
    virtual bool PutNextEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now(),
                               wxFileOffset aSize = wxInvalidOffset ) = 0;
    virtual bool PutNextDirEntry( const wxString& aName, const wxDateTime& aDt = wxDateTime::Now() ) = 0;
    virtual bool CloseEntry() = 0;

protected:
    wxArchiveOutputStream( wxOutputStream& aStream ) : wxFilterOutputStream( aStream ) {}
    wxArchiveOutputStream( wxOutputStream* aStream ) : wxFilterOutputStream( aStream ) {}
};


class wxArchiveClassFactory
{
public:
    virtual ~wxArchiveClassFactory() = default;

    /// The factory of the archives with this protocol or file extension (zip and tar).
    static const wxArchiveClassFactory* Find( const wxString& aProtocol,
                                              wxStreamProtocolType aType = wxSTREAM_PROTOCOL );
    static const wxArchiveClassFactory* GetFirst();
    const wxArchiveClassFactory*        GetNext() const { return m_next; }

    virtual wxArchiveInputStream*  NewStream( wxInputStream& aStream ) const = 0;
    virtual wxArchiveOutputStream* NewStream( wxOutputStream& aStream ) const = 0;
    virtual wxArchiveInputStream*  NewStream( wxInputStream* aStream ) const = 0;
    virtual wxArchiveOutputStream* NewStream( wxOutputStream* aStream ) const = 0;
    virtual wxArchiveEntry*        NewEntry() const = 0;

    virtual const wxChar* const* GetProtocols( wxStreamProtocolType aType = wxSTREAM_PROTOCOL ) const = 0;
    bool CanHandle( const wxString& aProtocol, wxStreamProtocolType aType = wxSTREAM_PROTOCOL ) const;

protected:
    const wxArchiveClassFactory* m_next = nullptr;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
