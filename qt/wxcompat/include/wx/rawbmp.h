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
 * @file rawbmp.h
 * The access to the pixels of a bitmap (wxNativePixelData, wxAlphaPixelData) of the wxWidgets
 * compatibility layer: the pixels are the ones of an image (RGBA, 8 bits by channel) copied
 * from the bitmap, and copied back to it when the data is destroyed.
 */

#pragma once

#include <wx/kiwx_gdi.h>

#include <QImage>
#include <QPixmap>


class wxPixelDataCompat
{
public:
    explicit wxPixelDataCompat( wxBitmap& aBitmap ) :
            m_bitmap( aBitmap ),
            m_image( aBitmap.toImage().convertToFormat( QImage::Format_RGBA8888 ) )
    {
    }

    ~wxPixelDataCompat() { m_bitmap = wxBitmap( QPixmap::fromImage( m_image ) ); }

    wxPixelDataCompat( const wxPixelDataCompat& ) = delete;
    wxPixelDataCompat& operator=( const wxPixelDataCompat& ) = delete;

    explicit operator bool() const { return !m_image.isNull(); }

    int GetWidth() const { return m_image.width(); }
    int GetHeight() const { return m_image.height(); }
    wxSize GetSize() const { return wxSize( m_image.width(), m_image.height() ); }

    /// A pixel: its 4 bytes in the image (R, G, B, A).
    class Iterator
    {
    public:
        explicit Iterator( wxPixelDataCompat& aData ) :
                m_pixel( aData.m_image.bits() )
        {
        }

        Iterator() : m_pixel( nullptr ) {}

        bool IsOk() const { return m_pixel != nullptr; }

        unsigned char& Red() { return m_pixel[0]; }
        unsigned char& Green() { return m_pixel[1]; }
        unsigned char& Blue() { return m_pixel[2]; }
        unsigned char& Alpha() { return m_pixel[3]; }

        Iterator& operator++()
        {
            m_pixel += 4;
            return *this;
        }

        Iterator operator++( int )
        {
            Iterator previous = *this;
            ++*this;
            return previous;
        }

        void OffsetX( const wxPixelDataCompat& aData, int aX ) { m_pixel += 4 * aX; }

        void OffsetY( const wxPixelDataCompat& aData, int aY )
        {
            m_pixel += aData.m_image.bytesPerLine() * aY;
        }

        void Offset( const wxPixelDataCompat& aData, int aX, int aY )
        {
            OffsetY( aData, aY );
            OffsetX( aData, aX );
        }

        void MoveTo( wxPixelDataCompat& aData, int aX, int aY )
        {
            m_pixel = aData.m_image.bits();
            Offset( aData, aX, aY );
        }

    private:
        unsigned char* m_pixel;
    };

private:
    wxBitmap& m_bitmap;
    QImage    m_image;
};


using wxNativePixelData = wxPixelDataCompat;
using wxAlphaPixelData = wxPixelDataCompat;
