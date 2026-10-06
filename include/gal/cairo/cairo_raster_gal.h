/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#pragma once

#include <gal/cairo/cairo_gal_base.h>

namespace KIGFX
{

/**
 * The Cairo GAL of an interactive canvas, without the window: draws into an ARGB32 raster
 * (premultiplied alpha, native byte order) through a compositor with the main, overlay and
 * temporary buffers.  At the end of each drawing the subclass shows the raster with present().
 */
class GAL_API CAIRO_RASTER_GAL : public CAIRO_GAL_BASE
{
public:
    CAIRO_RASTER_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions );

    ~CAIRO_RASTER_GAL();

    void ResizeScreen( int aWidth, int aHeight ) override;

    int BeginGroup() override;

    void EndGroup() override;

    void SetTarget( RENDER_TARGET aTarget ) override;

    RENDER_TARGET GetTarget() const override;

    void ClearTarget( RENDER_TARGET aTarget ) override;

    /// @copydoc GAL::StartDiffLayer()
    void StartDiffLayer() override;

    /// @copydoc GAL::EndDiffLayer()
    void EndDiffLayer() override;

    /// @copydoc GAL::StartNegativesLayer()
    void StartNegativesLayer() override;

    /// @copydoc GAL::EndNegativesLayer()
    void EndNegativesLayer() override;

    /// @copydoc GAL::BeginDrawing()
    void BeginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    void EndDrawing() override;

    /// The raster of the last drawing: m_screenSize.y rows of RasterStride() bytes.
    const unsigned char* Raster() const { return m_bitmapBuffer; }
    int                  RasterStride() const { return m_stride; }

protected:
    /// Show the raster, which holds the composited image (called by EndDrawing()).
    virtual void present() {}

    /// Prepare Cairo surfaces for drawing
    void initSurface();

    /// Destroy Cairo surfaces when are not needed anymore
    void deinitSurface();

    /// Allocate the raster for the current screen size
    void allocateBitmaps();

    /// Free the raster
    void deleteBitmaps();

    /// Prepare the compositor
    void setCompositor();

    ///< Cairo-specific update handlers
    bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions ) override;

    // Compositor related variables
    std::shared_ptr<CAIRO_COMPOSITOR> m_compositor;  ///< Object for layers compositing
    unsigned int        m_mainBuffer;          ///< Handle to the main buffer
    unsigned int        m_overlayBuffer;       ///< Handle to the overlay buffer
    unsigned int        m_tempBuffer;          ///< Handle to the temp buffer
    unsigned int        m_savedBuffer;         ///< Handle to buffer to restore after rendering to temp buffer
    RENDER_TARGET       m_currentTarget;       ///< Current rendering target
    bool                m_validCompositor;     ///< Compositor initialization flag

    unsigned int        m_bufferSize;          ///< Size of the raster
    unsigned char*      m_bitmapBuffer;        ///< Storage of the Cairo image
    int                 m_stride;              ///< Stride value for Cairo
    int                 m_bufferWidth;         ///< Width of the raster in pixels
    bool                m_isInitialized;       ///< Are Cairo image & surface ready to use
};

} // namespace KIGFX
