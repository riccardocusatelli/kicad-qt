/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * CairoGal - Graphics Abstraction Layer for Cairo
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
#include <gal/cairo/cairo_raster_gal.h>
#include <gal/cairo/cairo_compositor.h>

#include <core/ki_assert.h>

using namespace KIGFX;


CAIRO_RASTER_GAL::CAIRO_RASTER_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions ) :
        CAIRO_GAL_BASE( aDisplayOptions ),
        m_mainBuffer( 0 ),
        m_overlayBuffer( 0 ),
        m_tempBuffer( 0 ),
        m_savedBuffer( 0 ),
        m_currentTarget( TARGET_NONCACHED ),
        m_validCompositor( false ),
        m_bufferSize( 0 ),
        m_bitmapBuffer( nullptr ),
        m_stride( 0 ),
        m_bufferWidth( 0 ),
        m_isInitialized( false )
{
}


CAIRO_RASTER_GAL::~CAIRO_RASTER_GAL()
{
    deleteBitmaps();
}


void CAIRO_RASTER_GAL::BeginDrawing()
{
    initSurface();

    CAIRO_GAL_BASE::BeginDrawing();

    if( !m_validCompositor )
        setCompositor();

    m_compositor->SetMainContext( m_context );
    m_compositor->SetBuffer( m_mainBuffer );
}


void CAIRO_RASTER_GAL::EndDrawing()
{
    CAIRO_GAL_BASE::EndDrawing();

    // Merge the buffers into the image
    m_compositor->DrawBuffer( m_mainBuffer );
    m_compositor->DrawBuffer( m_overlayBuffer );

    present();

    deinitSurface();
}


void CAIRO_RASTER_GAL::ResizeScreen( int aWidth, int aHeight )
{
    CAIRO_GAL_BASE::ResizeScreen( aWidth, aHeight );

    // Recreate the bitmaps
    deleteBitmaps();
    allocateBitmaps();

    if( m_validCompositor )
        m_compositor->Resize( aWidth, aHeight );

    m_validCompositor = false;
}


void CAIRO_RASTER_GAL::StartDiffLayer()
{
    SetTarget( TARGET_TEMP );
    ClearTarget( TARGET_TEMP );
}

void CAIRO_RASTER_GAL::EndDiffLayer()
{
    m_compositor->DrawBuffer( m_tempBuffer, m_mainBuffer, CAIRO_OPERATOR_DIFFERENCE );
}

void CAIRO_RASTER_GAL::StartNegativesLayer()
{
    SetTarget( TARGET_TEMP );
    ClearTarget( TARGET_TEMP );
}

void CAIRO_RASTER_GAL::EndNegativesLayer()
{
    m_compositor->DrawBuffer( m_tempBuffer, m_mainBuffer, CAIRO_OPERATOR_OVER );
}

int CAIRO_RASTER_GAL::BeginGroup()
{
    initSurface();
    return CAIRO_GAL_BASE::BeginGroup();
}

void CAIRO_RASTER_GAL::EndGroup()
{
    CAIRO_GAL_BASE::EndGroup();
    deinitSurface();
}

void CAIRO_RASTER_GAL::SetTarget( RENDER_TARGET aTarget )
{
    // If the compositor is not set, that means that there is a recaching process going on
    // and we do not need the compositor now
    if( !m_validCompositor )
        return;

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    if( m_isInitialized )
        storePath();

    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED: m_compositor->SetBuffer( m_mainBuffer );    break;
    case TARGET_OVERLAY:   m_compositor->SetBuffer( m_overlayBuffer ); break;
    case TARGET_TEMP:      m_compositor->SetBuffer( m_tempBuffer );    break;
    }

    m_currentTarget = aTarget;
}

RENDER_TARGET CAIRO_RASTER_GAL::GetTarget() const
{
    return m_currentTarget;
}

void CAIRO_RASTER_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int currentBuffer = m_compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED: m_compositor->SetBuffer( m_mainBuffer );    break;
    case TARGET_OVERLAY:   m_compositor->SetBuffer( m_overlayBuffer ); break;
    case TARGET_TEMP:      m_compositor->SetBuffer( m_tempBuffer );    break;
    }

    m_compositor->ClearBuffer( COLOR4D::BLACK );

    // Restore the previous state
    m_compositor->SetBuffer( currentBuffer );
}

void CAIRO_RASTER_GAL::initSurface()
{
    if( m_isInitialized )
        return;

    m_surface = cairo_image_surface_create_for_data( m_bitmapBuffer, GAL_FORMAT, m_bufferWidth,
                                                     m_screenSize.y, m_stride );

    m_context = cairo_create( m_surface );

#ifdef DEBUG
    cairo_status_t status = cairo_status( m_context );
    KI_ASSERT_MSG( status == CAIRO_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* DEBUG */

    m_currentContext = m_context;

    m_isInitialized = true;
}

void CAIRO_RASTER_GAL::deinitSurface()
{
    if( !m_isInitialized )
        return;

    cairo_destroy( m_context );
    m_context = nullptr;
    cairo_surface_destroy( m_surface );
    m_surface = nullptr;

    m_isInitialized = false;
}

void CAIRO_RASTER_GAL::setCompositor()
{
    // Recreate the compositor with the new Cairo context
    m_compositor.reset( new CAIRO_COMPOSITOR( &m_currentContext ) );
    m_compositor->Resize( m_screenSize.x, m_screenSize.y );
    m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );

    // Prepare buffers
    m_mainBuffer = m_compositor->CreateBuffer();
    m_overlayBuffer = m_compositor->CreateBuffer();
    m_tempBuffer = m_compositor->CreateBuffer();

    m_validCompositor = true;
}


void CAIRO_RASTER_GAL::allocateBitmaps()
{
    m_bufferWidth = m_screenSize.x;

    // Create buffer, use the system independent Cairo context backend
    m_stride = cairo_format_stride_for_width( GAL_FORMAT, m_bufferWidth );
    m_bufferSize = m_stride * m_screenSize.y;

    KI_ASSERT( m_bitmapBuffer == nullptr );
    m_bitmapBuffer = new unsigned char[m_bufferSize];
}


void CAIRO_RASTER_GAL::deleteBitmaps()
{
    delete[] m_bitmapBuffer;
    m_bitmapBuffer = nullptr;
}


bool CAIRO_RASTER_GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    bool refresh = false;

    if( m_validCompositor && aOptions.antialiasing_mode != m_compositor->GetAntialiasingMode() )
    {
        m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );
        m_validCompositor = false;
        deinitSurface();

        refresh = true;
    }

    if( super::updatedGalDisplayOptions( aOptions ) )
        refresh = true;

    return refresh;
}
