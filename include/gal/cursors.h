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

#ifndef CURSOR_STORE__H
#define CURSOR_STORE__H

#include <wx/cursor.h>
#include <gal/kicursor.h>

#include <map>
#include <vector>

/**
 * Represents either a wxCursorBundle for wx 3.3+ or a wxCursor for older versions.
 * This allows consistent cursor handling across different wxWidgets versions.
 */
#if wxCHECK_VERSION( 3, 3, 0 )
typedef wxCursorBundle WX_CURSOR_TYPE;
#else
typedef wxCursor WX_CURSOR_TYPE;
#endif



/**
 * Simple class to construct and store cursors against unique ID keys.
 *
 * This can be used to lazily construct cursors as needed for specific
 * applications.
 */
class CURSOR_STORE
{
public:
    /**
     * Get a cursor bundle (wx 3.3+) or appropriate cursor (older versions)
     * @param  aCursorType the cursor type to get
     * @param  aHiDPI whether to prefer HiDPI version for older wx versions
     * @return             the cursor bundle or cursor
     */
    static const WX_CURSOR_TYPE GetCursor( KICURSOR aCursorType, bool aHiDPI = false );

    /**
     * Get stock cursor type for the given cursor
     * @param  aCursorType the cursor type
     * @return             stock cursor type, or wxCURSOR_MAX if not a stock cursor
     */
    static wxStockCursor GetStockCursor( KICURSOR aCursorType );

private:
    /**
     * Construct a store with cursors for all defined types.
     * The store will automatically contain both standard and HiDPI cursors.
     */
    CURSOR_STORE();

#if wxCHECK_VERSION( 3, 3, 0 )
    /**
     * Get a cursor bundle by its ID (wx 3.3+ only)
     * @param  aIdKey the ID key to look up
     * @return        the cursor bundle, if found, else an invalid bundle
     */
    const wxCursorBundle& storeGetBundle( KICURSOR aIdKey ) const;
#else
    /**
     * Get a cursor by its ID, automatically selecting the appropriate resolution
     * @param  aIdKey the ID key to look up
     * @param  aHiDPI whether to prefer HiDPI version if available
     * @return        the cursor, if found, else wxNullCursor
     */
    const wxCursor& storeGetCursor( KICURSOR aIdKey, bool aHiDPI = false ) const;
#endif

#if wxCHECK_VERSION( 3, 3, 0 )
    ///< Internal store of cursor bundles for wx 3.3+
    std::map<KICURSOR, wxCursorBundle> m_bundleMap;
#else
    std::map<KICURSOR, wxCursor> m_standardCursorMap;
    std::map<KICURSOR, wxCursor> m_hidpiCursorMap;
#endif

};

#endif // CURSOR_STORE__H
