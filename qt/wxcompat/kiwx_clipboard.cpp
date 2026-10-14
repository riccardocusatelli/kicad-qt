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
#include <QMimeData>

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

    if( wxTextDataObject* text = dynamic_cast<wxTextDataObject*>( data.get() ) )
    {
        clipboard->setText( ToQString( text->GetText() ), clipboardMode( m_primary ) );
        return true;
    }

    if( wxBitmapDataObject* bitmap = dynamic_cast<wxBitmapDataObject*>( data.get() ) )
    {
        clipboard->setPixmap( bitmap->GetBitmap(), clipboardMode( m_primary ) );
        return true;
    }

    return false;
}


bool wxClipboard::GetData( wxDataObject& aData )
{
    QClipboard* clipboard = QGuiApplication::clipboard();

    if( !clipboard )
        return false;

    const QMimeData* mime = clipboard->mimeData( clipboardMode( m_primary ) );

    if( !mime )
        return false;

    if( wxTextDataObject* text = dynamic_cast<wxTextDataObject*>( &aData ) )
    {
        if( !mime->hasText() )
            return false;

        text->SetText( ToWxString( mime->text() ) );
        return true;
    }

    if( wxBitmapDataObject* bitmap = dynamic_cast<wxBitmapDataObject*>( &aData ) )
    {
        if( !mime->hasImage() )
            return false;

        bitmap->SetBitmap( wxBitmap( QPixmap::fromImage( clipboard->image( clipboardMode( m_primary ) ) ) ) );
        return true;
    }

    return false;
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
    default:                return false;
    }
}


void wxClipboard::Clear()
{
    if( QClipboard* clipboard = QGuiApplication::clipboard() )
        clipboard->clear( clipboardMode( m_primary ) );
}
