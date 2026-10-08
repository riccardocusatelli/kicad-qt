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


#include <widgets/bitmap_toggle.h>

#include <QMouseEvent>


BITMAP_TOGGLE::BITMAP_TOGGLE( QWidget* aParent, int aId, const QIcon& aCheckedBitmap,
                              const QIcon& aUncheckedBitmap, bool aChecked ) :
        QLabel( aParent ),
        m_id( aId ),
        m_checked( aChecked ),
        m_unchecked_bitmap( aUncheckedBitmap ),
        m_checked_bitmap( aCheckedBitmap )
{
    showBitmap();
}


void BITMAP_TOGGLE::showBitmap()
{
    const QIcon& icon = m_checked ? m_checked_bitmap : m_unchecked_bitmap;
    QList<QSize> sizes = icon.availableSizes();
    QSize        size = sizes.isEmpty() ? QSize( 16, 16 ) : sizes.first();

    setPixmap( icon.pixmap( size, devicePixelRatioF() ) );
}


void BITMAP_TOGGLE::SetValue( bool aValue )
{
    m_checked = aValue;
    showBitmap();
}


void BITMAP_TOGGLE::mouseReleaseEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() != Qt::LeftButton )
    {
        QLabel::mouseReleaseEvent( aEvent );
        return;
    }

    if( m_debounce.isValid() && m_debounce.elapsed() < 200 )
        return;

    m_debounce.start();

    SetValue( !GetValue() );

    // From the event loop, as the event of the wxWidgets build
    bool checked = m_checked;

    QMetaObject::invokeMethod( this,
                               [this, checked]()
                               {
                                   if( m_toggleCallback )
                                       m_toggleCallback( checked );
                               },
                               Qt::QueuedConnection );
}
