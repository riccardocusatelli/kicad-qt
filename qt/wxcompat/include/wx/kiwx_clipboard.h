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
 * on QClipboard, with the data objects of wxWidgets on QMimeData.
 */

#ifndef KIWX_CLIPBOARD_H
#define KIWX_CLIPBOARD_H

#include <kiqt.h>

#include <memory>
#include <vector>

#include <wx/arrstr.h>
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/kiwx_gdi.h>

class QMimeData;


/// The format of the data (wxDataFormat): an id of wxWidgets (wxDF_TEXT...) or a MIME type.
class KIQT_API wxDataFormat
{
public:
    wxDataFormat( wxDataFormatId aId = wxDF_INVALID ) : m_id( aId ) {}
    wxDataFormat( const wxString& aMimeType ) : m_id( wxDF_PRIVATE ), m_mimeType( aMimeType ) {}
    wxDataFormat( const wchar_t* aMimeType ) : wxDataFormat( wxString( aMimeType ) ) {}
    wxDataFormat( const char* aMimeType ) : wxDataFormat( wxString( aMimeType ) ) {}

    wxDataFormatId GetType() const { return m_id; }
    wxString GetId() const { return m_mimeType; }
    void SetId( const wxString& aMimeType )
    {
        m_id = wxDF_PRIVATE;
        m_mimeType = aMimeType;
    }

    /// The MIME type of the format on the clipboard of Qt.
    wxString GetMimeTypeCompat() const;

    bool operator==( const wxDataFormat& aOther ) const
    {
        return m_id == aOther.m_id && m_mimeType == aOther.m_mimeType;
    }

    bool operator!=( const wxDataFormat& aOther ) const { return !( *this == aOther ); }
    bool operator==( wxDataFormatId aId ) const { return m_id == aId; }
    bool operator!=( wxDataFormatId aId ) const { return m_id != aId; }

private:
    wxDataFormatId m_id;
    wxString       m_mimeType;
};


/**
 * The data of the clipboard (wxDataObject): an object puts its formats into the data of Qt
 * (QMimeData), and reads them back.
 */
class KIQT_API wxDataObject
{
public:
    virtual ~wxDataObject() = default;

    virtual bool IsSupported( const wxDataFormat& aFormat ) const = 0;

    /// Put the data into @a aMime.
    virtual void ToMimeDataCompat( QMimeData& aMime ) const = 0;

    /// Read the data from @a aMime: false if it has none of the formats.
    virtual bool FromMimeDataCompat( const QMimeData& aMime ) = 0;
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

    void ToMimeDataCompat( QMimeData& aMime ) const override;
    bool FromMimeDataCompat( const QMimeData& aMime ) override;

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

    void ToMimeDataCompat( QMimeData& aMime ) const override;
    bool FromMimeDataCompat( const QMimeData& aMime ) override;

private:
    wxBitmap m_bitmap;
};


/// The names of files (wxFileDataObject): the URLs of the data of Qt.
class KIQT_API wxFileDataObject : public wxDataObject
{
public:
    const wxArrayString& GetFilenames() const { return m_filenames; }
    void AddFile( const wxString& aFilename ) { m_filenames.Add( aFilename ); }

    bool IsSupported( const wxDataFormat& aFormat ) const override
    {
        return aFormat == wxDF_FILENAME;
    }

    void ToMimeDataCompat( QMimeData& aMime ) const override;
    bool FromMimeDataCompat( const QMimeData& aMime ) override;

private:
    wxArrayString m_filenames;
};


/// Bytes in a format (wxCustomDataObject): the data of its MIME type (wxDF_BITMAP: a PNG).
class KIQT_API wxCustomDataObject : public wxDataObject
{
public:
    explicit wxCustomDataObject( const wxDataFormat& aFormat = wxDF_PRIVATE ) :
            m_format( aFormat )
    {
    }

    const wxDataFormat& GetFormat() const { return m_format; }
    void SetFormat( const wxDataFormat& aFormat ) { m_format = aFormat; }

    bool Alloc( size_t aSize )
    {
        m_data.resize( aSize );
        return true;
    }

    void Free() { m_data.clear(); }

    bool SetData( size_t aSize, const void* aData )
    {
        const char* bytes = static_cast<const char*>( aData );
        m_data.assign( bytes, bytes + aSize );
        return true;
    }

    void TakeData( size_t aSize, void* aData )
    {
        SetData( aSize, aData );
        delete[] static_cast<char*>( aData );
    }

    const void* GetData() const { return m_data.data(); }
    size_t GetSize() const { return m_data.size(); }
    size_t GetDataSize() const { return m_data.size(); }

    bool IsSupported( const wxDataFormat& aFormat ) const override { return aFormat == m_format; }

    void ToMimeDataCompat( QMimeData& aMime ) const override;
    bool FromMimeDataCompat( const QMimeData& aMime ) override;

private:
    wxDataFormat      m_format;
    std::vector<char> m_data;
};


/// Several data objects (wxDataObjectComposite), each with its formats.
class KIQT_API wxDataObjectComposite : public wxDataObject
{
public:
    /// Add a data object (the composite takes it).
    void Add( wxDataObject* aData, bool aPreferred = false );

    /// The format of the object that got data in the last FromMimeDataCompat().
    wxDataFormat GetReceivedFormat() const { return m_receivedFormat; }

    bool IsSupported( const wxDataFormat& aFormat ) const override;
    void ToMimeDataCompat( QMimeData& aMime ) const override;
    bool FromMimeDataCompat( const QMimeData& aMime ) override;

private:
    std::vector<std::unique_ptr<wxDataObject>> m_objects;
    wxDataFormat                               m_receivedFormat;
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
