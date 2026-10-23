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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file raster_image_wx.h
 * The Qt version of include/raster_image_wx.h: the wxImage of the compatibility layer is a QImage,
 * the conversions are the ones of raster_image_qt.h.
 */

#pragma once

#include <raster_image_qt.h>
#include <wx/image.h>


/// The wxImage of @a aImage (pixels, alpha or mask, resolution).
inline wxImage ToWxImage( const RASTER_IMAGE& aImage )
{
    return wxImage( ToQImage( aImage ) );
}

/// The RASTER_IMAGE of @a aImage (pixels, alpha or mask, resolution).
inline RASTER_IMAGE FromWxImage( const wxImage& aImage )
{
    return FromQImage( aImage );
}
