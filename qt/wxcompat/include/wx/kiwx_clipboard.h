/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file kiwx_clipboard.h
 * The clipboard of the wxWidgets compatibility layer of the Qt build (wxcompat): wxTheClipboard
 * on QClipboard, with the text and bitmap data objects.
 */

#ifndef KIWX_CLIPBOARD_H
#define KIWX_CLIPBOARD_H

#include <kiqt.h>

#include <memory>

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/kiwx_gdi.h>


/// The format of the data (wxDataFormat): the ids of wxWidgets (wxDF_TEXT...).
class wxDataFormat
{
public:
    wxDataFormat( wxDataFormatId aId = wxDF_INVALID ) : m_id( aId ) {}

    wxDataFormatId GetType() const { return m_id; }
    bool operator==( const wxDataFormat& aOther ) const { return m_id == aOther.m_id; }
    bool operator==( wxDataFormatId aId ) const { return m_id == aId; }

private:
    wxDataFormatId m_id;
};


/// The data of the clipboard (wxDataObject).
class KIQT_API wxDataObject
{
public:
    virtual ~wxDataObject() = default;

    virtual bool IsSupported( const wxDataFormat& aFormat ) const = 0;
};


class KIQT_API wxTextDataObject : public wxDataObject
{
public:
    explicit wxTextDataObject( const wxString& aText = wxEmptyString ) : m_text( aText ) {}

    wxString GetText() const { return m_text; }
    void SetText( const wxString& aText ) { m_text = aText; }
    size_t GetTextLength() const { return m_text.length(); }

    bool IsSupported( const wxDataFormat& aFormat ) const override
    {
        return aFormat == wxDF_TEXT || aFormat == wxDF_UNICODETEXT;
    }

private:
    wxString m_text;
};


class KIQT_API wxBitmapDataObject : public wxDataObject
{
public:
    explicit wxBitmapDataObject( const wxBitmap& aBitmap = wxBitmap() ) : m_bitmap( aBitmap ) {}

    wxBitmap GetBitmap() const { return m_bitmap; }
    void SetBitmap( const wxBitmap& aBitmap ) { m_bitmap = aBitmap; }

    bool IsSupported( const wxDataFormat& aFormat ) const override
    {
        return aFormat == wxDF_BITMAP;
    }

private:
    wxBitmap m_bitmap;
};


/// The clipboard of the system (wxClipboard): always available, Open() and Close() are kept.
class KIQT_API wxClipboard
{
public:
    bool Open();
    void Close();
    bool IsOpened() const { return m_open; }

    /// Put the data on the clipboard (the clipboard takes the object).
    bool SetData( wxDataObject* aData );
    bool AddData( wxDataObject* aData ) { return SetData( aData ); }
    bool GetData( wxDataObject& aData );
    bool IsSupported( const wxDataFormat& aFormat );
    void Clear();

    /// The data stays after the application ends (Qt keeps it with the clipboard manager).
    bool Flush() { return true; }

    void UsePrimarySelection( bool aPrimary = false ) { m_primary = aPrimary; }
    bool IsUsingPrimarySelection() const { return m_primary; }

    static wxClipboard* Get();

private:
    bool m_open = false;
    bool m_primary = false;
};

#define wxTheClipboard ( wxClipboard::Get() )


/// Open the clipboard while the locker lives (wxClipboardLocker).
class wxClipboardLocker
{
public:
    explicit wxClipboardLocker( wxClipboard* aClipboard = nullptr ) :
            m_clipboard( aClipboard ? aClipboard : wxTheClipboard )
    {
        m_open = m_clipboard->Open();
    }

    ~wxClipboardLocker()
    {
        if( m_open )
            m_clipboard->Close();
    }

    bool operator!() const { return !m_open; }
    operator bool() const { return m_open; }

private:
    wxClipboard* m_clipboard;
    bool         m_open;
};

#endif // KIWX_CLIPBOARD_H
