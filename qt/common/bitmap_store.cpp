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
 * @file bitmap_store.cpp
 * BITMAP_STORE of the Qt build, and the functions of bitmap_types.h.
 */

#include <bitmap_store.h>

#include <set>

#include <asset_archive.h>
#include <bitmaps.h>
#include <hash.h>
#include <kiplatform/ui.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_strings.h>

// s_imageNotFound, s_dummyItem
#include "../../common/bitmap_store_images.h"

static const wxString traceBitmaps = wxT( "KICAD_BITMAPS" );

static const wxString IMAGE_ARCHIVE = wxT( "images.tar.gz" );


size_t std::hash<std::pair<BITMAPS, int>>::operator()( const std::pair<BITMAPS, int>& aPair ) const
{
    std::size_t seed = 0xa82de1c0;
    hash_combine( seed, static_cast<size_t>( aPair.first ), static_cast<size_t>( aPair.second ) );

    return seed;
}


BITMAP_STORE::BITMAP_STORE() :
    m_theme( BITMAP_INFO::THEME::LIGHT )
{
    wxFileName path( PATHS::GetStockDataPath() + wxT( "/resources" ), IMAGE_ARCHIVE );

    KI_LOG_TRACE( traceBitmaps, "Loading bitmaps from " + path.GetFullPath() );

    m_archive = std::make_unique<ASSET_ARCHIVE>( path.GetFullPath() );

    buildBitmapInfoCache();

    ThemeChanged();
}


BITMAP_STORE::~BITMAP_STORE() = default;


QPixmap BITMAP_STORE::GetBitmap( BITMAPS aBitmapId, int aHeight )
{
    return QPixmap::fromImage( getImage( aBitmapId, aHeight ) );
}


/// An image scaled to a height, with the quality of the images of the archive.
static QImage scaledImage( const QImage& aImage, int aHeight )
{
    return aImage.scaled( aHeight, aHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation );
}


std::vector<QImage> BITMAP_STORE::images( BITMAPS aBitmapId, int aMinHeight, int aDefHeight )
{
    std::vector<QImage> imgs;
    std::set<int>       sizes;
    QImage              largestImage;

    for( const BITMAP_INFO& info : m_bitmapInfoCache[aBitmapId] )
    {
        if( info.theme != m_theme )
            continue;

        QImage img = getImage( info.id, info.height );

        if( largestImage.isNull() || img.height() > largestImage.height() )
            largestImage = img;

        if( aMinHeight > 0 && info.height < aMinHeight )
            continue;

        sizes.emplace( info.height );
        imgs.push_back( img );
    }

    if( aDefHeight > 0 )
    {
        if( !sizes.contains( aDefHeight ) && !largestImage.isNull() )
            imgs.push_back( scaledImage( largestImage, aDefHeight ) );

        if( imgs.empty() )
        {
            KI_FAIL_MSG( wxString::Format( "Missing bitmap for ID %d", static_cast<int>( aBitmapId ) ) );
            QImage notFound = QImage::fromData( s_imageNotFound, sizeof( s_imageNotFound ), "PNG" );
            imgs.push_back( scaledImage( notFound, aDefHeight ) );
        }
    }

    return imgs;
}


QIcon BITMAP_STORE::GetBitmapBundle( BITMAPS aBitmapId, int aMinHeight )
{
    QIcon icon;

    for( const QImage& img : images( aBitmapId, aMinHeight, -1 ) )
        icon.addPixmap( QPixmap::fromImage( img ) );

    return icon;
}


QIcon BITMAP_STORE::GetBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight )
{
    QIcon icon;

    for( const QImage& img : images( aBitmapId, -1, aDefHeight ) )
        icon.addPixmap( QPixmap::fromImage( img ) );

    return icon;
}


/// The disabled look of an image (as wxImage::ConvertToDisabled()): grey, lighter on a light
/// theme and darker on a dark one.
static QImage disabledImage( const QImage& aImage, bool aDarkTheme )
{
    QImage img = aImage.convertToFormat( QImage::Format_ARGB32 );
    int    brightness = aDarkTheme ? 70 : 255;

    for( int y = 0; y < img.height(); ++y )
    {
        QRgb* line = reinterpret_cast<QRgb*>( img.scanLine( y ) );

        for( int x = 0; x < img.width(); ++x )
        {
            int grey = qGray( line[x] );
            int value = ( grey + brightness ) / 2;

            line[x] = qRgba( value, value, value, qAlpha( line[x] ) );
        }
    }

    return img;
}


QIcon BITMAP_STORE::GetDisabledBitmapBundle( BITMAPS aBitmapId, int aMinHeight )
{
    QIcon icon;
    bool  dark = KIPLATFORM::UI::IsDarkTheme();

    for( const QImage& img : images( aBitmapId, aMinHeight, -1 ) )
        icon.addPixmap( QPixmap::fromImage( disabledImage( img, dark ) ) );

    return icon;
}


QIcon BITMAP_STORE::GetDisabledBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight )
{
    QIcon icon;
    bool  dark = KIPLATFORM::UI::IsDarkTheme();

    for( const QImage& img : images( aBitmapId, -1, aDefHeight ) )
        icon.addPixmap( QPixmap::fromImage( disabledImage( img, dark ) ) );

    return icon;
}


QImage BITMAP_STORE::getImage( BITMAPS aBitmapId, int aHeight )
{
    const unsigned char* data = nullptr;
    long                 count;

    if( aBitmapId == BITMAPS::dummy_item )
    {
        data = s_dummyItem;
        count = sizeof( s_dummyItem );
    }
    else
    {
        const wxString& name = bitmapName( aBitmapId, aHeight );
        count = m_archive->GetFilePointer( name, &data );

        if( count < 0 )
        {
            KI_LOG_TRACE( traceBitmaps,
                        "Bitmap for %d, %d, %d has an info tag with file %s,"
                        "but that file could not be found in the archive!",
                        static_cast<int>( aBitmapId ), aHeight, static_cast<int>( m_theme ), name );
            data = s_imageNotFound;
            count = sizeof( s_imageNotFound );
        }
    }

    return QImage::fromData( data, static_cast<int>( count ), "PNG" );
}


void BITMAP_STORE::ThemeChanged()
{
    BITMAP_INFO::THEME oldTheme = m_theme;

    if( COMMON_SETTINGS* settings = Pgm().GetCommonSettings() )
    {
        switch( settings->m_Appearance.icon_theme )
        {
        case ICON_THEME::LIGHT: m_theme = BITMAP_INFO::THEME::LIGHT; break;
        case ICON_THEME::DARK:  m_theme = BITMAP_INFO::THEME::DARK; break;
        case ICON_THEME::AUTO:  m_theme = KIPLATFORM::UI::IsDarkTheme() ? BITMAP_INFO::THEME::DARK : BITMAP_INFO::THEME::LIGHT; break;
        }
    }
    else
    {
        m_theme = KIPLATFORM::UI::IsDarkTheme() ? BITMAP_INFO::THEME::DARK : BITMAP_INFO::THEME::LIGHT;
    }

    if( oldTheme != m_theme )
        m_bitmapNameCache.clear();
}


const wxString& BITMAP_STORE::bitmapName( BITMAPS aBitmapId, int aHeight )
{
    std::pair<BITMAPS, int> key = std::make_pair( aBitmapId, aHeight );

    if( !m_bitmapNameCache.count( key ) )
        m_bitmapNameCache[key] = computeBitmapName( aBitmapId, aHeight );

    return m_bitmapNameCache.at( key );
}


wxString BITMAP_STORE::computeBitmapName( BITMAPS aBitmapId, int aHeight )
{
    if( !m_bitmapInfoCache.count( aBitmapId ) || m_bitmapInfoCache.at( aBitmapId ).empty() )
    {
        KI_LOG_TRACE( traceBitmaps, "No bitmap info available for %d", aBitmapId );
        return wxEmptyString;
    }

    wxString fn;

    for( const BITMAP_INFO& info : m_bitmapInfoCache.at( aBitmapId ) )
    {
        if( info.theme != m_theme )
            continue;

        if( aHeight < 0 || info.height == aHeight )
        {
            fn = info.filename;
            break;
        }
    }

    if( fn.IsEmpty() )
    {
        KI_LOG_TRACE( traceBitmaps, "No bitmap found matching ID %d, height %d, theme %s",
                      static_cast<int>( aBitmapId ), aHeight, m_theme );
        return m_bitmapInfoCache.at( aBitmapId ).begin()->filename;
    }

    return fn;
}


void BITMAP_STORE::buildBitmapInfoCache()
{
    BuildBitmapInfo( m_bitmapInfoCache );
}


// The functions of bitmap_types.h

static std::unique_ptr<BITMAP_STORE> s_BitmapStore;


BITMAP_STORE* GetBitmapStore()
{
    if( !s_BitmapStore )
        s_BitmapStore = std::make_unique<BITMAP_STORE>();

    return s_BitmapStore.get();
}


QPixmap KiBitmap( BITMAPS aBitmap, int aHeightTag )
{
    return GetBitmapStore()->GetBitmap( aBitmap, aHeightTag );
}


QIcon KiBitmapBundle( BITMAPS aBitmap, int aMinHeight )
{
    return GetBitmapStore()->GetBitmapBundle( aBitmap, aMinHeight );
}


QIcon KiBitmapBundleDef( BITMAPS aBitmap, int aDefHeight )
{
    return GetBitmapStore()->GetBitmapBundleDef( aBitmap, aDefHeight );
}


QIcon KiDisabledBitmapBundle( BITMAPS aBitmap, int aMinHeight )
{
    return GetBitmapStore()->GetDisabledBitmapBundle( aBitmap, aMinHeight );
}


QIcon KiDisabledBitmapBundleDef( BITMAPS aBitmap, int aDefHeight )
{
    return GetBitmapStore()->GetDisabledBitmapBundleDef( aBitmap, aDefHeight );
}


void ClearScaledBitmapCache()
{
    // The icons are made on demand: there is no cache of scaled bitmaps
}
