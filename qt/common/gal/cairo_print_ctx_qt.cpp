/*
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file cairo_print_ctx_qt.cpp
 * The Cairo context of the printouts with Qt (see common/gal/cairo/cairo_print_ctx_wx.cpp): Cairo
 * draws a page on an image, at the resolution of the device up to 300 DPI, which the painter of
 * the DC of the printout (a printer, a preview) draws on the page.  Or Cairo draws on an image.
 */

#include <gal/cairo/cairo_print.h>

#include <algorithm>
#include <stdexcept>

#include <QPaintDevice>
#include <QPainter>

#include <core/ki_assert.h>
#include <wx/kiwx_gdi.h>

using namespace KIGFX;


/// The resolution of the images of the pages, at most.
static constexpr double MAX_PRINT_DPI = 300.0;


CAIRO_PRINT_CTX::CAIRO_PRINT_CTX( wxImage* aImage, double aDPI ) :
        m_targetImage( aImage ),
        m_dpi( aDPI )
{
    KI_CHECK( aImage, /* void */ );

    m_surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, aImage->GetWidth(),
                                            aImage->GetHeight() );

    if( !m_surface || cairo_surface_status( m_surface ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo surface" );

    m_ctx = cairo_create( m_surface );

    if( !m_ctx || cairo_status( m_ctx ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo context" );

    cairo_set_antialias( m_ctx, CAIRO_ANTIALIAS_GOOD );
}


CAIRO_PRINT_CTX::CAIRO_PRINT_CTX( wxDC* aDC )
{
    QPainter*     painter = aDC ? aDC->GetPainter() : nullptr;
    QPaintDevice* device = painter ? painter->device() : nullptr;

    if( !device )
        throw std::runtime_error( "Unhandled wxDC type" );

    // The page is drawn later by the painter (the one of the DC)
    m_gcdc = new wxGCDC( painter );

    double deviceDPI = device->logicalDpiX() > 0 ? device->logicalDpiX() : 96.0;
    m_dpi = std::min( deviceDPI, MAX_PRINT_DPI );

    double scale = m_dpi / deviceDPI;
    int    width = std::max( 1, int( device->width() * scale + 0.5 ) );
    int    height = std::max( 1, int( device->height() * scale + 0.5 ) );

    m_surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );

    if( !m_surface || cairo_surface_status( m_surface ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo surface" );

    m_ctx = cairo_create( m_surface );

    if( !m_ctx || cairo_status( m_ctx ) != CAIRO_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo context" );

    cairo_set_antialias( m_ctx, CAIRO_ANTIALIAS_GOOD );
}


CAIRO_PRINT_CTX::~CAIRO_PRINT_CTX()
{
    if( m_surface )
    {
        cairo_surface_flush( m_surface );

        // The pixels of Cairo (ARGB32, premultiplied) are the ones of QImage
        QImage page( cairo_image_surface_get_data( m_surface ),
                     cairo_image_surface_get_width( m_surface ),
                     cairo_image_surface_get_height( m_surface ),
                     cairo_image_surface_get_stride( m_surface ),
                     QImage::Format_ARGB32_Premultiplied );

        if( m_targetImage )
        {
            *m_targetImage = wxImage( page.convertToFormat( QImage::Format_ARGB32 ) );
        }
        else if( m_gcdc && m_gcdc->GetPainter() )
        {
            QPainter*     painter = m_gcdc->GetPainter();
            QPaintDevice* device = painter->device();

            // On the whole page, in the coordinates of the device
            painter->save();
            painter->resetTransform();
            painter->setRenderHint( QPainter::SmoothPixmapTransform );
            painter->drawImage( QRect( 0, 0, device->width(), device->height() ), page );
            painter->restore();
        }

        cairo_surface_destroy( m_surface );
    }

    if( m_ctx )
        cairo_destroy( m_ctx );

    delete m_gcdc;
}
