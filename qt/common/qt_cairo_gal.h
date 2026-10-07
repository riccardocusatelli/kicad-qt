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

#include <functional>

#include <gal/cairo/cairo_raster_gal.h>

#include "kiqt.h"

class QPainter;

namespace KIGFX
{

/**
 * The Cairo GAL of a Qt canvas: draws into the raster of CAIRO_RASTER_GAL, which the canvas
 * shows when a drawing ends.
 */
class KIQT_API QT_CAIRO_GAL : public CAIRO_RASTER_GAL
{
public:
    /// @param aPresent is called at the end of each drawing, while the raster is valid.
    QT_CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, std::function<void()> aPresent );

    /**
     * Paint the cursor of the GAL (the crosshair) over the drawing, as CAIRO_GAL blits it.
     *
     * @param aPainter paints on the canvas, in logical pixels.
     * @param aRatio is the ratio of the device pixels of the GAL to the logical pixels.
     */
    void PaintCursor( QPainter& aPainter, double aRatio ) const;

protected:
    void present() override;

private:
    std::function<void()> m_present;
};

} // namespace KIGFX
