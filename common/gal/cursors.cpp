/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <vector>
#include <map>

#include <gal/cursors.h>
#include <kiplatform/ui.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

#include <gal/cursor_defs.h>
#include <core/ki_assert.h>

#include <wx/bitmap.h>
#include <wx/debug.h>
#include <wx/bmpbndl.h>


CURSOR_STORE::CURSOR_STORE()
{
    for( const auto& [cursorId, defs] : CursorDefinitions() )
    {
        KI_CHECK2( !defs.empty(), continue );

#if wxCHECK_VERSION( 3, 3, 0 )
        // For wx 3.3+, create cursor bundles from the cursor definitions
        std::vector<wxBitmap> bitmaps;

        for( const auto& [xpm, hotspot_def] : defs )
        {
            KI_CHECK2( xpm, continue );
            bitmaps.push_back( wxBitmap( xpm ) );
        }

        wxBitmapBundle bitmapBundle = wxBitmapBundle::FromBitmaps( bitmaps );

        wxPoint hotspot( defs[0].m_hotspot.x, defs[0].m_hotspot.y ); // Use hotspot from standard cursor
        m_bundleMap[cursorId] = wxCursorBundle( bitmapBundle, hotspot );
#else
        auto constructCursor = []( const CURSOR_DEF& aDef ) -> wxCursor
        {
            KI_CHECK( aDef.m_xpm, wxNullCursor );
            wxImage xpmImage = wxImage( aDef.m_xpm );

            xpmImage.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, aDef.m_hotspot.x );
            xpmImage.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, aDef.m_hotspot.y );

            return wxCursor( xpmImage );
        };

        // Add standard cursor (first definition)
        m_standardCursorMap[cursorId] = constructCursor( defs[0] );

        // Add HiDPI cursor (second definition if available, otherwise fallback to standard)
        if( defs.size() > 1 )
            m_hidpiCursorMap[cursorId] = constructCursor( defs[1] );
        else
            m_hidpiCursorMap[cursorId] = m_standardCursorMap[cursorId];
#endif
    }
}

#if wxCHECK_VERSION( 3, 3, 0 )
const wxCursorBundle& CURSOR_STORE::storeGetBundle( KICURSOR aIdKey ) const
{
    const auto find_iter = m_bundleMap.find( aIdKey );

    if( find_iter != m_bundleMap.end() )
        return find_iter->second;

    KI_ASSERT_MSG( false, wxString::Format( "Could not find cursor bundle with ID %d",
                                           static_cast<int>( aIdKey ) ) );

    static const wxCursorBundle invalid;

    return invalid;
}
#else
const wxCursor& CURSOR_STORE::storeGetCursor( KICURSOR aIdKey, bool aHiDPI ) const
{
    const auto& store = aHiDPI ? m_hidpiCursorMap : m_standardCursorMap;
    const auto  find_iter = store.find( aIdKey );

    if( find_iter != store.end() )
        return find_iter->second;

    KI_ASSERT_MSG( false, wxString::Format( "Could not find cursor with ID %d", static_cast<int>( aIdKey ) ) );

    return wxNullCursor;
}
#endif

/* static */
const WX_CURSOR_TYPE CURSOR_STORE::GetCursor( KICURSOR aCursorType, bool aHiDPI )
{
    // Use a single cursor store instance
    static CURSOR_STORE store;

    bool useCustomCursors = true;

    if( COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings() )
        useCustomCursors = commonSettings->m_Appearance.use_custom_cursors;

    if( !useCustomCursors )
    {
        wxStockCursor stock = GetStockCursor( aCursorType );

        if( stock == wxCURSOR_MAX )
            stock = wxCURSOR_ARROW;

        return WX_CURSOR_TYPE( stock );
    }

    wxStockCursor stock = GetStockCursor( aCursorType );

    if( stock != wxCURSOR_MAX )
        return WX_CURSOR_TYPE( stock );

#if wxCHECK_VERSION( 3, 3, 0 )
    // For wx 3.3+, return the pre-built cursor bundle (aHiDPI is ignored as bundles contain both)
    return store.storeGetBundle( aCursorType );
#else
    return store.storeGetCursor( aCursorType, aHiDPI );
#endif
}

/* static */
wxStockCursor CURSOR_STORE::GetStockCursor( KICURSOR aCursorType )
{
    wxStockCursor stockCursor;
    switch( aCursorType )
    {
    case KICURSOR::MOVING:
        stockCursor = wxCURSOR_SIZING;
        break;
    case KICURSOR::BULLSEYE:
        stockCursor = wxCURSOR_BULLSEYE;
        break;
    case KICURSOR::HAND:
        stockCursor = wxCURSOR_HAND;
        break;
    case KICURSOR::ARROW:
        stockCursor = wxCURSOR_ARROW;
        break;
    default:
        stockCursor = wxCURSOR_MAX;
        break;
    }

    if( !KIPLATFORM::UI::IsStockCursorOk( stockCursor ) )
        stockCursor = wxCURSOR_MAX;

    return stockCursor;
}
