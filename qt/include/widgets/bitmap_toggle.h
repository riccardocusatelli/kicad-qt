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
 * @file bitmap_toggle.h
 * BITMAP_TOGGLE of the Qt build: a check box shown as two icons.
 */

#ifndef _BITMAP_TOGGLE_H
#define _BITMAP_TOGGLE_H

#include <functional>

#include <QElapsedTimer>
#include <QIcon>
#include <QLabel>

#include <kiqt.h>


/**
 * A checkbox control except with custom bitmaps for the checked and unchecked states.
 *
 * A click toggles the state: the toggle callback is called (from the event loop, as the
 * TOGGLE_CHANGED event of the wxWidgets build) with the new state.
 */
class KIQT_API BITMAP_TOGGLE : public QLabel
{
public:
    BITMAP_TOGGLE( QWidget* aParent, int aId, const QIcon& aCheckedBitmap,
                   const QIcon& aUncheckedBitmap, bool aChecked = false );

    /// Set the checkbox state
    void SetValue( bool aValue );

    /// Read the checkbox state
    bool GetValue() const { return m_checked; }

    /// The id given at the creation (the layer of the appearance panels).
    int GetId() const { return m_id; }

    void SetWindowID( int aId ) { m_id = aId; }

    /// The function called when the user toggles the state.
    void SetToggleCallback( std::function<void( bool aChecked )> aCallback )
    {
        m_toggleCallback = std::move( aCallback );
    }

protected:
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;

private:
    void showBitmap();

    int           m_id;
    bool          m_checked;

    QIcon         m_unchecked_bitmap;
    QIcon         m_checked_bitmap;

    QElapsedTimer m_debounce;            ///< Time since the last toggle, for debouncing.

    std::function<void( bool )> m_toggleCallback;
};

#endif
