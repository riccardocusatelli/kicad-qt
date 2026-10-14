/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file bitmap_button.cpp
 * BITMAP_BUTTON of the Qt build (see common/widgets/bitmap_button.cpp).
 */

#include <widgets/bitmap_button.h>

#include <algorithm>

#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

#include <kiplatform/ui.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <ui_hooks.h>


BITMAP_BUTTON::BITMAP_BUTTON( wxWidgetArgCompat aParent, wxWindowID aId, const wxPoint& aPos,
                              const wxSize& aSize ) :
        wxWindowCompat<QAbstractButton>( aParent.get() ),
        m_isRadioButton( false ),
        m_isSeparator( false ),
        m_showBadge( false ),
        m_badgeColor( 210, 0, 0 ), // dark red
        m_badgeTextColor( Qt::white ),
        m_padding( 0 ),
        m_isToolbarButton( false ),
        m_acceptDraggedInClicks( false ),
        m_centerBitmap( true )
{
    if( aId != wxID_ANY )
        KIQT::SetWindowId( this, aId );

    if( aSize.x > 0 && aSize.y > 0 )
        resize( aSize.x, aSize.y );

    // The hover state is drawn as the buttons of the toolbars
    setAttribute( Qt::WA_Hover );
    setFocusPolicy( Qt::StrongFocus );
}


BITMAP_BUTTON::~BITMAP_BUTTON()
{
}


QSize BITMAP_BUTTON::sizeHint() const
{
    if( m_isSeparator )
        return QSize( m_bitmapSize.width() + m_padding * 2, QPushButton().sizeHint().height() );

    return m_bitmapSize + QSize( m_padding * 2, m_padding * 2 );
}


void BITMAP_BUTTON::SetPadding( int aPadding )
{
    m_padding = aPadding;
    updateGeometry();
}


void BITMAP_BUTTON::SetBitmap( const QIcon& aBmp )
{
    m_normalBitmap = aBmp;

    // The default size is the smallest one of the bundle
    QList<QSize> sizes = aBmp.availableSizes();
    m_bitmapSize = sizes.isEmpty() ? QSize() : sizes.first();

    for( const QSize& size : sizes )
    {
        if( size.height() < m_bitmapSize.height() )
            m_bitmapSize = size;
    }

    updateGeometry();
    update();
}


void BITMAP_BUTTON::SetDisabledBitmap( const QIcon& aBmp )
{
    m_disabledBitmap = aBmp;
    update();
}


void BITMAP_BUTTON::SetBadgeText( const wxString& aText )
{
    m_badgeText = ToQString( aText );
    update();
}


bool BITMAP_BUTTON::Enable( bool aEnable )
{
    // If the requested state is already the current state, don't do anything
    if( aEnable == isEnabled() )
        return false;

    setEnabled( aEnable );
    update();
    return true;
}


void BITMAP_BUTTON::SetIsCheckButton()
{
    setCheckable( true );
}


void BITMAP_BUTTON::SetIsRadioButton()
{
    setCheckable( true );
    m_isRadioButton = true;
}


void BITMAP_BUTTON::SetIsSeparator()
{
    m_isSeparator = true;
    setEnabled( false );
    updateGeometry();
}


void BITMAP_BUTTON::Check( bool aCheck )
{
    KI_ASSERT_MSG( isCheckable(), wxS( "Button is not a checkButton." ) );

    setChecked( aCheck );
}


bool BITMAP_BUTTON::IsChecked() const
{
    KI_ASSERT_MSG( isCheckable(), wxS( "Button is not a checkButton." ) );

    return isChecked();
}


void BITMAP_BUTTON::nextCheckState()
{
    // A radio button stays checked when it is clicked again
    if( m_isRadioButton && isChecked() )
        return;

    QAbstractButton::nextCheckState();
}


void BITMAP_BUTTON::mouseReleaseEvent( QMouseEvent* aEvent )
{
    // Accept clicks that came without prior mouse-down if configured
    if( m_acceptDraggedInClicks && !isDown() && aEvent->button() == Qt::LeftButton
            && isEnabled() && rect().contains( aEvent->position().toPoint() ) )
    {
        click();
        return;
    }

    QAbstractButton::mouseReleaseEvent( aEvent );
}


void BITMAP_BUTTON::paintEvent( QPaintEvent* aEvent )
{
    bool     darkMode = KIPLATFORM::UI::IsDarkTheme();
    QColor   highlightColor = palette().color( QPalette::Highlight );
    QRect    rect( QPoint( 0, 0 ), size() );
    QPainter painter( this );

    if( m_isSeparator )
    {
        painter.setPen( palette().color( QPalette::Disabled, QPalette::WindowText ) );
        painter.drawLine( width() / 2, 0, width() / 2, height() );
        return;
    }

    // As wxColour::ChangeLightness(): 100 is the color, below is darker, above is lighter
    auto lightness =
            [&]( int aLightness )
            {
                return aLightness >= 100 ? highlightColor.lighter( 100 + ( aLightness - 100 ) * 2 )
                                         : highlightColor.darker( 100 + ( 100 - aLightness ) * 2 );
            };

    // This drawing is done so the button looks the same as a toolbar button
    if( isEnabled() )
    {
        bool hovered = underMouse() || hasFocus();

        if( isDown() )
        {
            painter.setPen( highlightColor );
            painter.setBrush( lightness( darkMode ? 20 : 150 ) );
            painter.drawRect( rect.adjusted( 0, 0, -1, -1 ) );
        }
        else if( hovered )
        {
            painter.setPen( highlightColor );

            // Checked items need a lighter hover rectangle
            if( isChecked() )
                painter.setBrush( lightness( darkMode ? 50 : 180 ) );
            else
                painter.setBrush( lightness( darkMode ? 40 : 170 ) );

            painter.drawRect( rect.adjusted( 0, 0, -1, -1 ) );
        }
        else if( isChecked() )
        {
            painter.setPen( highlightColor );
            painter.setBrush( lightness( darkMode ? 40 : 170 ) );
            painter.drawRect( rect.adjusted( 0, 0, -1, -1 ) );
        }
    }

    bool         useDisabled = !isEnabled() && !m_disabledBitmap.isNull();
    const QIcon& bmp = useDisabled ? m_disabledBitmap : m_normalBitmap;
    QPoint       drawBmpPos( m_padding, m_padding );

    if( m_centerBitmap )
    {
        drawBmpPos.setX( ( rect.width() - m_bitmapSize.width() ) / 2 );
        drawBmpPos.setY( ( rect.height() - m_bitmapSize.height() ) / 2 );
    }

    // Draw the bitmap with the upper-left corner offset by the padding
    if( !bmp.isNull() )
    {
        QIcon::Mode mode = ( isEnabled() || useDisabled ) ? QIcon::Normal : QIcon::Disabled;
        bmp.paint( &painter, QRect( drawBmpPos, m_bitmapSize ), Qt::AlignCenter, mode );
    }

    // Draw the badge
    if( m_showBadge && !m_badgeText.isEmpty() )
    {
        QFont badgeFont = font();
        badgeFont.setPointSizeF( badgeFont.pointSizeF() / 1.2 );   // as wxFont::Smaller()
        badgeFont.setBold( true );
        painter.setFont( badgeFont );

        QSize textPadding( 2, 1 );

        if( m_padding )
            textPadding *= 2;

        QSize boxSize = QFontMetrics( badgeFont ).size( Qt::TextSingleLine, m_badgeText )
                        + textPadding;
        QSize boxOffset = boxSize;

        if( m_padding != 0 )
            boxOffset += QSize( m_padding / 3, m_padding / 3 );

        QPoint corner( rect.right() + 1 - boxOffset.width(), rect.bottom() + 1 - boxOffset.height() );
        QRect  box( corner, boxSize );
        qreal  radius = 0.25 * std::min( box.width(), box.height() );

        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( m_badgeColor );
        painter.setBrush( m_badgeColor );
        painter.drawRoundedRect( box, radius, radius );

        painter.setPen( m_badgeTextColor );
        painter.drawText( box, Qt::AlignCenter, m_badgeText );
    }
}
