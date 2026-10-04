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

/**
 * @file raster_image_wx.h
 * @brief Conversions between RASTER_IMAGE (data model) and wxImage, for the GUI.
 */

#include <kicommon.h>
#include <raster_image.h>

class wxImage;


/// The wxImage of @a aImage (pixels, alpha or mask, resolution).
KICOMMON_API wxImage ToWxImage( const RASTER_IMAGE& aImage );

/// The RASTER_IMAGE of @a aImage (pixels, alpha or mask, resolution).
KICOMMON_API RASTER_IMAGE FromWxImage( const wxImage& aImage );

/// Let RASTER_IMAGE read the image formats that wxImage reads (BMP, GIF, TIFF...).
KICOMMON_API void RegisterWxImageDecoder();
