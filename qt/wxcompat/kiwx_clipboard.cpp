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

#include <wx/kiwx_clipboard.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QUrl>

#include <qt_strings.h>


wxClipboard* wxClipboard::Get()
{
    static wxClipboard s_clipboard;
    return &s_clipboard;
}


static QClipboard::Mode clipboardMode( bool aPrimary )
{
    QClipboard* clipboard = QGuiApplication::clipboard();

    if( aPrimary && clipboard->supportsSelection() )
        return QClipboard::Selection;

    return QClipboard::Clipboard;
}


bool wxClipboard::Open()
{
    if( !QGuiApplication::clipboard() )
        return false;

    m_open = true;
    return true;
}


void wxClipboard::Close()
{
    m_open = false;
}


bool wxClipboard::SetData( wxDataObject* aData )
{
    std::unique_ptr<wxDataObject> data( aData );
    QClipboard*                   clipboard = QGuiApplication::clipboard();

    if( !clipboard || !data )
        return false;

    QMimeData* mime = new QMimeData;
    data->ToMimeDataCompat( *mime );
    clipboard->setMimeData( mime, clipboardMode( m_primary ) );
    return true;
}


bool wxClipboard::GetData( wxDataObject& aData )
{
    QClipboard* clipboard = QGuiApplication::clipboard();

    if( !clipboard )
        return false;

    const QMimeData* mime = clipboard->mimeData( clipboardMode( m_primary ) );

    return mime && aData.FromMimeDataCompat( *mime );
}


bool wxClipboard::IsSupported( const wxDataFormat& aFormat )
{
    QClipboard* clipboard = QGuiApplication::clipboard();

    if( !clipboard )
        return false;

    const QMimeData* mime = clipboard->mimeData( clipboardMode( m_primary ) );

    if( !mime )
        return false;

    switch( aFormat.GetType() )
    {
    case wxDF_TEXT:
    case wxDF_UNICODETEXT:
    case wxDF_OEMTEXT:      return mime->hasText();
    case wxDF_BITMAP:
    case wxDF_DIB:
    case wxDF_PNG:          return mime->hasImage();
    case wxDF_FILENAME:     return mime->hasUrls();
    case wxDF_HTML:         return mime->hasHtml();
    default:
    {
        wxString type = aFormat.GetMimeTypeCompat();
        return !type.empty() && mime->hasFormat( ToQString( type ) );
    }
    }
}


void wxClipboard::Clear()
{
    if( QClipboard* clipboard = QGuiApplication::clipboard() )
        clipboard->clear( clipboardMode( m_primary ) );
}


// The formats and the data objects ---------------------------------------------------------

wxString wxDataFormat::GetMimeTypeCompat() const
{
    switch( m_id )
    {
    case wxDF_TEXT:
    case wxDF_UNICODETEXT:
    case wxDF_OEMTEXT:  return wxS( "text/plain" );
    case wxDF_BITMAP:
    case wxDF_DIB:
    case wxDF_PNG:      return wxS( "image/png" );
    case wxDF_FILENAME: return wxS( "text/uri-list" );
    case wxDF_HTML:     return wxS( "text/html" );
    default:            return m_mimeType;
    }
}


void wxTextDataObject::ToMimeDataCompat( QMimeData& aMime ) const
{
    aMime.setText( ToQString( m_text ) );
}


bool wxTextDataObject::FromMimeDataCompat( const QMimeData& aMime )
{
    if( !aMime.hasText() )
        return false;

    m_text = ToWxString( aMime.text() );
    return true;
}


void wxBitmapDataObject::ToMimeDataCompat( QMimeData& aMime ) const
{
    aMime.setImageData( m_bitmap.toImage() );
}


bool wxBitmapDataObject::FromMimeDataCompat( const QMimeData& aMime )
{
    if( !aMime.hasImage() )
        return false;

    m_bitmap = wxBitmap( QPixmap::fromImage( qvariant_cast<QImage>( aMime.imageData() ) ) );
    return m_bitmap.IsOk();
}


void wxFileDataObject::ToMimeDataCompat( QMimeData& aMime ) const
{
    QList<QUrl> urls;

    for( const wxString& file : m_filenames )
        urls.append( QUrl::fromLocalFile( ToQString( file ) ) );

    aMime.setUrls( urls );
}


bool wxFileDataObject::FromMimeDataCompat( const QMimeData& aMime )
{
    m_filenames.Clear();

    for( const QUrl& url : aMime.urls() )
    {
        if( url.isLocalFile() )
            m_filenames.Add( ToWxString( url.toLocalFile() ) );
    }

    return !m_filenames.empty();
}


void wxCustomDataObject::ToMimeDataCompat( QMimeData& aMime ) const
{
    wxString type = m_format.GetMimeTypeCompat();

    if( !type.empty() )
        aMime.setData( ToQString( type ), QByteArray( m_data.data(), qsizetype( m_data.size() ) ) );
}


bool wxCustomDataObject::FromMimeDataCompat( const QMimeData& aMime )
{
    QString type = ToQString( m_format.GetMimeTypeCompat() );

    if( type.isEmpty() || !aMime.hasFormat( type ) )
        return false;

    QByteArray bytes = aMime.data( type );
    m_data.assign( bytes.constData(), bytes.constData() + bytes.size() );
    return true;
}


void wxDataObjectComposite::Add( wxDataObject* aData, bool aPreferred )
{
    // The preferred object is read first
    if( aPreferred )
        m_objects.emplace( m_objects.begin(), aData );
    else
        m_objects.emplace_back( aData );
}


bool wxDataObjectComposite::IsSupported( const wxDataFormat& aFormat ) const
{
    for( const std::unique_ptr<wxDataObject>& object : m_objects )
    {
        if( object->IsSupported( aFormat ) )
            return true;
    }

    return false;
}


void wxDataObjectComposite::ToMimeDataCompat( QMimeData& aMime ) const
{
    for( const std::unique_ptr<wxDataObject>& object : m_objects )
        object->ToMimeDataCompat( aMime );
}


bool wxDataObjectComposite::FromMimeDataCompat( const QMimeData& aMime )
{
    for( const std::unique_ptr<wxDataObject>& object : m_objects )
    {
        if( object->FromMimeDataCompat( aMime ) )
        {
            if( wxCustomDataObject* custom = dynamic_cast<wxCustomDataObject*>( object.get() ) )
                m_receivedFormat = custom->GetFormat();
            else if( dynamic_cast<wxTextDataObject*>( object.get() ) )
                m_receivedFormat = wxDF_UNICODETEXT;
            else if( dynamic_cast<wxBitmapDataObject*>( object.get() ) )
                m_receivedFormat = wxDF_BITMAP;
            else if( dynamic_cast<wxFileDataObject*>( object.get() ) )
                m_receivedFormat = wxDF_FILENAME;

            return true;
        }
    }

    return false;
}
