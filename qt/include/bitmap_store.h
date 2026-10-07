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
 * @file bitmap_store.h
 * BITMAP_STORE of the Qt build: the icons of the image archive of KiCad, as Qt images.
 */

#ifndef KICAD_BITMAP_STORE_H
#define KICAD_BITMAP_STORE_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <QIcon>
#include <QImage>
#include <QPixmap>

#include <bitmaps/bitmap_info.h>
#include <kicommon.h>

class ASSET_ARCHIVE;

namespace std
{
    template<> struct KICOMMON_API hash<std::pair<BITMAPS, int>>
    {
        size_t operator()( const std::pair<BITMAPS, int>& aPair ) const;
    };
}

/**
 * Helper to retrieve bitmaps while handling icon themes and scaling
 */
class KICOMMON_API BITMAP_STORE
{
public:
    BITMAP_STORE();

    ~BITMAP_STORE();

    /**
     * Retrieve a bitmap from the given bitmap id.
     *
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h.
     * @param aHeight is the requested height in pixels of the source image, or -1 for any height.
     */
    QPixmap GetBitmap( BITMAPS aBitmapId, int aHeight = -1 );

    /// An icon with the images of all the heights from @a aMinHeight (all if -1).
    QIcon GetBitmapBundle( BITMAPS aBitmapId, int aMinHeight = -1 );

    /// An icon with the images of all the heights, and @a aDefHeight.
    QIcon GetBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight );

    /// The disabled look of an icon.
    QIcon GetDisabledBitmapBundle( BITMAPS aBitmapId, int aMinHeight = -1 );

    /// The disabled look of an icon, with @a aDefHeight.
    QIcon GetDisabledBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight );

    /**
     * Notify the store that the icon theme has been changed by the user, so caches must be
     * invalidated.
     */
    void ThemeChanged();

    bool IsDarkTheme() const { return m_theme == BITMAP_INFO::THEME::DARK; }

private:
    QImage getImage( BITMAPS aBitmapId, int aHeight = -1 );

    /// The images of the current theme, from @a aMinHeight, and @a aDefHeight if > 0.
    std::vector<QImage> images( BITMAPS aBitmapId, int aMinHeight, int aDefHeight );

    const wxString& bitmapName( BITMAPS aBitmapId, int aHeight = -1 );

    wxString computeBitmapName( BITMAPS aBitmapId, int aHeight = -1 );

    void buildBitmapInfoCache();

    std::unique_ptr<ASSET_ARCHIVE> m_archive;

    std::unordered_map<std::pair<BITMAPS, int>, wxString> m_bitmapNameCache;

    std::unordered_map<BITMAPS, std::vector<BITMAP_INFO>> m_bitmapInfoCache;

    BITMAP_INFO::THEME m_theme;
};

#endif // KICAD_BITMAP_STORE_H
