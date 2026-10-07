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
 * @file cursors.h
 * CURSOR_STORE of the Qt build: the cursors of KiCad as QCursors.
 */

#ifndef CURSOR_STORE__H
#define CURSOR_STORE__H

#include <map>

#include <kiqt.h>
#include <QCursor>

#include <gal/kicursor.h>


/**
 * Simple class to construct and store cursors against unique ID keys.
 */
class KIQT_API CURSOR_STORE
{
public:
    /**
     * Get a cursor of KiCad (or a cursor of the system when it has it).
     *
     * @param aCursorType is the cursor
     * @param aHiDPI gives the cursor for the screens of high resolution
     */
    static QCursor GetCursor( KICURSOR aCursorType, bool aHiDPI = false );

    /**
     * The cursor of the system for a cursor of KiCad, if it has one.
     *
     * @return false if the cursor is drawn by KiCad.
     */
    static bool GetStockCursor( KICURSOR aCursorType, Qt::CursorShape& aShape );

private:
    CURSOR_STORE();

    QCursor storeGetCursor( KICURSOR aIdKey, bool aHiDPI ) const;

    std::map<KICURSOR, QCursor> m_standardCursorMap;
    std::map<KICURSOR, QCursor> m_hidpiCursorMap;
};

#endif // CURSOR_STORE__H
