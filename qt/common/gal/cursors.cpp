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


#include <gal/cursors.h>

#include <QPixmap>

#include <gal/cursor_defs.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <core/ki_assert.h>


/// A cursor from its image; the image of a HiDPI cursor is twice the size of the cursor.
static QCursor makeCursor( const CURSOR_DEF& aDef, int aScale )
{
    KI_CHECK( aDef.m_xpm, QCursor() );

    QPixmap pixmap( aDef.m_xpm );
    pixmap.setDevicePixelRatio( aScale );

    return QCursor( pixmap, aDef.m_hotspot.x / aScale, aDef.m_hotspot.y / aScale );
}


CURSOR_STORE::CURSOR_STORE()
{
    for( const auto& [cursorId, defs] : CursorDefinitions() )
    {
        KI_CHECK2( !defs.empty(), continue );

        // Standard cursor (first definition)
        m_standardCursorMap[cursorId] = makeCursor( defs[0], 1 );

        // HiDPI cursor (second definition if available, otherwise fallback to standard)
        if( defs.size() > 1 )
            m_hidpiCursorMap[cursorId] = makeCursor( defs[1], 2 );
        else
            m_hidpiCursorMap[cursorId] = m_standardCursorMap[cursorId];
    }
}


QCursor CURSOR_STORE::storeGetCursor( KICURSOR aIdKey, bool aHiDPI ) const
{
    const auto& store = aHiDPI ? m_hidpiCursorMap : m_standardCursorMap;
    const auto  find_iter = store.find( aIdKey );

    if( find_iter != store.end() )
        return find_iter->second;

    KI_ASSERT_MSG( false, wxString::Format( "Could not find cursor with ID %d", static_cast<int>( aIdKey ) ) );

    return QCursor( Qt::ArrowCursor );
}


/* static */
QCursor CURSOR_STORE::GetCursor( KICURSOR aCursorType, bool aHiDPI )
{
    // The cursors are made when the first one is needed (a QApplication must exist)
    static CURSOR_STORE store;

    bool useCustomCursors = true;

    if( COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings() )
        useCustomCursors = commonSettings->m_Appearance.use_custom_cursors;

    Qt::CursorShape stock = Qt::ArrowCursor;
    bool            hasStock = GetStockCursor( aCursorType, stock );

    if( !useCustomCursors || hasStock )
        return QCursor( stock );

    return store.storeGetCursor( aCursorType, aHiDPI );
}


/* static */
bool CURSOR_STORE::GetStockCursor( KICURSOR aCursorType, Qt::CursorShape& aShape )
{
    switch( aCursorType )
    {
    case KICURSOR::MOVING:   aShape = Qt::SizeAllCursor;      return true;
    case KICURSOR::BULLSEYE: aShape = Qt::CrossCursor;        return true;
    case KICURSOR::HAND:     aShape = Qt::PointingHandCursor; return true;
    case KICURSOR::ARROW:    aShape = Qt::ArrowCursor;        return true;
    default:                 aShape = Qt::ArrowCursor;        return false;
    }
}
