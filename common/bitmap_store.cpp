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

#include <wx/bitmap.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <advanced_config.h>
#include <asset_archive.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <bitmaps/bitmap_info.h>
#include <hash.h>
#include <kiplatform/ui.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>


// s_imageNotFound, s_dummyItem
#include "bitmap_store_images.h"

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


wxBitmap BITMAP_STORE::GetBitmap( BITMAPS aBitmapId, int aHeight )
{
    return wxBitmap( getImage( aBitmapId, aHeight ) );
}


wxBitmapBundle BITMAP_STORE::GetBitmapBundle( BITMAPS aBitmapId, int aMinHeight )
{
    wxVector<wxBitmap> bmps;

    for( const BITMAP_INFO& info : m_bitmapInfoCache[aBitmapId] )
    {
        if( info.theme != m_theme )
            continue;

        if( aMinHeight > 0 && info.height < aMinHeight )
            continue;

        bmps.push_back( wxBitmap( getImage( info.id, info.height ) ) );
    }

    return wxBitmapBundle::FromBitmaps( bmps );
}


// Better scaling algorithm, taken from wx 3.3
static wxImage resampleImage( const wxImage& aImg, int aWidth, int aHeight )
{
    wxImage image;
    int     old_width = aImg.GetWidth();
    int     old_height = aImg.GetHeight();

    // When downscaling, use bilinear algorithm for shrinking to an
    // integer multiple of the target size and then box average by the
    // integer part.
    if( aWidth <= old_width && aHeight <= old_height )
    {
        const double shrinkFactorX = double( old_width ) / aWidth;
        const double shrinkFactorY = double( old_height ) / aHeight;

        const int shrinkInt( wxMin( shrinkFactorX, shrinkFactorY ) );

        image = aImg.ResampleBilinear( aWidth * shrinkInt, aHeight * shrinkInt );
        if( shrinkInt != 1 )
            image = image.ResampleBox( aWidth, aHeight );
    }
    else // Use box average algorithm for upscaling.
    {
        image = aImg.ResampleBox( aWidth, aHeight );
    }

    return image;
}


wxBitmapBundle BITMAP_STORE::GetBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight )
{
    wxVector<wxBitmap> bmps;
    std::set<int>      sizes;
    int                largestHeight = 0;
    wxImage            largestImage;

    for( const BITMAP_INFO& info : m_bitmapInfoCache[aBitmapId] )
    {
        if( info.theme != m_theme )
            continue;

        wxImage img = getImage( info.id, info.height );

        if( info.height > largestHeight )
        {
            largestHeight = info.height;
            largestImage = img;
        }

        if( info.height >= aDefHeight )
        {
            sizes.emplace( info.height );
            bmps.push_back( wxBitmap( img ) );
        }
    }

    if( !sizes.contains( aDefHeight ) && largestImage.IsOk() )
        bmps.push_back( wxBitmap( resampleImage( largestImage, aDefHeight, aDefHeight ) ) );

    if( bmps.empty() )
    {
        KI_FAIL_MSG( wxString::Format( "Missing bitmap for ID %d", static_cast<int>( aBitmapId ) ) );
        wxMemoryInputStream is( s_imageNotFound, sizeof( s_imageNotFound ) );
        largestImage = resampleImage( wxImage( is, wxBITMAP_TYPE_PNG ), aDefHeight, aDefHeight );
        bmps.push_back( wxBitmap( largestImage ) );
    }

#ifdef __WXOSX__
    // OSX doesn't align text in trees properly when 2x bitmaps are not provided, apparently
    int size2x = aDefHeight * 2;

    if( !sizes.contains( size2x ) && largestImage.IsOk() )
        bmps.push_back( wxBitmap( resampleImage( largestImage, size2x, size2x ) ) );
#endif

    return wxBitmapBundle::FromBitmaps( bmps );
}


wxBitmapBundle BITMAP_STORE::GetDisabledBitmapBundle( BITMAPS aBitmapId, int aMinHeight )
{
    wxVector<wxBitmap> bmps;

    for( const BITMAP_INFO& info : m_bitmapInfoCache[aBitmapId] )
    {
        if( info.theme != m_theme )
            continue;

        if( aMinHeight > 0 && info.height < aMinHeight )
            continue;

        wxBitmap bmp( getImage( info.id, info.height )
                              .ConvertToDisabled( KIPLATFORM::UI::IsDarkTheme() ? 70 : 255 ) );
        bmps.push_back( bmp );
    }

    return wxBitmapBundle::FromBitmaps( bmps );
}


wxBitmapBundle BITMAP_STORE::GetDisabledBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight )
{
    wxVector<wxBitmap> bmps;
    std::set<int>      sizes;
    int                largestHeight = 0;
    wxImage            largestImage;

    for( const BITMAP_INFO& info : m_bitmapInfoCache[aBitmapId] )
    {
        if( info.theme != m_theme )
            continue;

        wxImage img = getImage( info.id, info.height ).ConvertToDisabled( KIPLATFORM::UI::IsDarkTheme() ? 70 : 255 );

        if( info.height > largestHeight )
        {
            largestHeight = info.height;
            largestImage = img;
        }

        if( info.height >= aDefHeight )
        {
            sizes.emplace( info.height );
            bmps.push_back( wxBitmap( img ) );
        }
    }

    if( !sizes.contains( aDefHeight ) && largestImage.IsOk() )
        bmps.push_back( wxBitmap( resampleImage( largestImage, aDefHeight, aDefHeight ) ) );

    if( bmps.empty() )
    {
        KI_FAIL_MSG( wxString::Format( "Missing bitmap for ID %d", static_cast<int>( aBitmapId ) ) );
        wxMemoryInputStream is( s_imageNotFound, sizeof( s_imageNotFound ) );
        largestImage = resampleImage( wxImage( is, wxBITMAP_TYPE_PNG ), aDefHeight, aDefHeight );
        bmps.push_back( wxBitmap( largestImage ) );
    }

#ifdef __WXOSX__
    // OSX doesn't align text in trees properly when 2x bitmaps are not provided, apparently
    int size2x = aDefHeight * 2;

    if( !sizes.contains( size2x ) && largestImage.IsOk() )
        bmps.push_back( wxBitmap( resampleImage( largestImage, size2x, size2x ) ) );
#endif

    return wxBitmapBundle::FromBitmaps( bmps );
}


wxBitmap BITMAP_STORE::GetBitmapScaled( BITMAPS aBitmapId, int aScaleFactor, int aHeight )
{
    wxImage image = getImage( aBitmapId, aHeight );

    // Bilinear seems to genuinely look better for these line-drawing icons
    // than bicubic, despite claims in the wx documentation that bicubic is
    // "highest quality". I don't recommend changing this. Bicubic looks
    // blurry and makes me want an eye exam.
    image.Rescale( aScaleFactor * image.GetWidth() / 4, aScaleFactor * image.GetHeight() / 4,
                   wxIMAGE_QUALITY_BILINEAR );

    return wxBitmap( image );
}


wxImage BITMAP_STORE::getImage( BITMAPS aBitmapId, int aHeight )
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

    wxMemoryInputStream is( data, count );
    wxImage             image( is, wxBITMAP_TYPE_PNG );

    return image;
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
        KI_LOG_TRACE( traceBitmaps, "No bitmap found matching ID %d, height %d, theme %s", static_cast<int>( aBitmapId ),
                    aHeight, m_theme );
        return m_bitmapInfoCache.at( aBitmapId ).begin()->filename;
    }

    return fn;
}


void BITMAP_STORE::buildBitmapInfoCache()
{
    BuildBitmapInfo( m_bitmapInfoCache );
}
