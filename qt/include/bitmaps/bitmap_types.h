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


/**
 * @file bitmap_types.h
 * The icons of KiCad in the Qt build: the interface of include/bitmaps/bitmap_types.h, with
 * the image types of Qt.
 */

#ifndef BITMAP_TYPES_H_
#define BITMAP_TYPES_H_

#include <kicommon.h>

class QIcon;
class QPixmap;
class BITMAP_STORE;

enum class BITMAPS : unsigned int;

enum class BITMAP_TYPE
{
    PNG,
    JPG,
    BMP
};

/// The store of the icons (of the image archive of KiCad).
KICOMMON_API BITMAP_STORE* GetBitmapStore();

/**
 * An icon at one of its sizes.
 *
 * @param aHeightTag is the height of the image, or -1 for the default size.
 */
KICOMMON_API QPixmap KiBitmap( BITMAPS aBitmap, int aHeightTag = -1 );

/// An icon with all its sizes from @a aMinHeight (all if -1).
KICOMMON_API QIcon KiBitmapBundle( BITMAPS aBitmap, int aMinHeight = -1 );

/// An icon with all its sizes, and @a aDefHeight (scaled from the largest one if needed).
KICOMMON_API QIcon KiBitmapBundleDef( BITMAPS aBitmap, int aDefHeight );

/// The disabled look of an icon, with all its sizes from @a aMinHeight (all if -1).
KICOMMON_API QIcon KiDisabledBitmapBundle( BITMAPS aBitmap, int aMinHeight = -1 );

/// The disabled look of an icon, with all its sizes and @a aDefHeight.
KICOMMON_API QIcon KiDisabledBitmapBundleDef( BITMAPS aBitmap, int aDefHeight );

/// Forget the icons made for the theme or the scale used so far.
KICOMMON_API void ClearScaledBitmapCache();

#endif  // BITMAP_TYPES_H_
