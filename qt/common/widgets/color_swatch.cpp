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
 * @file color_swatch.cpp
 * COLOR_SWATCH of the Qt build (see common/widgets/color_swatch.cpp).
 */

#include <widgets/color_swatch.h>

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

#include <dialog_shim.h>
#include <dialogs/dialog_color_picker.h>
#include <core/ki_assert.h>
#include <qt_ui.h>

using KIGFX::COLOR4D;


wxSize COLOR_SWATCH::DialogUnitsToPixels( const QWidget* aWidget, const wxSize& aSize )
{
    return KIQT::DialogUnitsToPixels( aWidget, aSize );
}


QPixmap COLOR_SWATCH::MakeBitmap( const COLOR4D& aColor, const COLOR4D& aBackground,
                                  const wxSize& aSize, const wxSize& aCheckerboardSize,
                                  const COLOR4D& aCheckerboardBackground, double aScale,
                                  const std::vector<int>& aMargins )
{
    QPixmap pixmap( std::max( 1, (int) std::lround( aSize.x * aScale ) ),
                    std::max( 1, (int) std::lround( aSize.y * aScale ) ) );
    pixmap.setDevicePixelRatio( aScale );

    QPainter painter( &pixmap );
    painter.setPen( Qt::NoPen );

    COLOR4D fg = aColor.m_text ? COLOR4D::UNSPECIFIED : aColor;

    // Draw a checkerboard
    COLOR4D white;
    COLOR4D black;
    bool    rowCycle;

    if( aColor.m_text || aColor == COLOR4D::UNSPECIFIED )
    {
        if( aCheckerboardBackground.GetBrightness() > 0.4 )
        {
            white = COLOR4D::WHITE;
            black = white.Darkened( 0.15 );
            rowCycle = true;
        }
        else
        {
            black = COLOR4D::BLACK;
            white = black.Brightened( 0.15 );
            rowCycle = false;
        }
    }
    else
    {
        if( aBackground.GetBrightness() > 0.4 )
        {
            white = aBackground;
            black = white.Darkened( 0.15 );
            rowCycle = true;
        }
        else
        {
            black = COLOR4D::BLACK;
            white = black.Brightened( 0.15 );
            rowCycle = false;
        }
    }

    const int cellX = std::max( 1, aCheckerboardSize.x );
    const int cellY = std::max( 1, aCheckerboardSize.y );

    for( int x = 0; x < aSize.x; x += cellX )
    {
        bool colCycle = rowCycle;

        for( int y = 0; y < aSize.y; y += cellY )
        {
            COLOR4D bg = colCycle ? black : white;

            // Blend fg bg with the checkerboard
            painter.setBrush( ToQColor( fg.Mix( bg, fg.a ).WithAlpha( 1.0 ) ) );
            painter.drawRect( x, y, cellX, cellY );

            colCycle = !colCycle;
        }

        rowCycle = !rowCycle;
    }

    painter.setBrush( Qt::white );

    if( aMargins[0] )
        painter.drawRect( 0, 0, aMargins[0], aSize.y );

    if( aMargins[1] )
        painter.drawRect( 0, 0, aSize.x, aMargins[1] );

    if( aMargins[2] )
        painter.drawRect( aSize.x - aMargins[2], 0, aMargins[2], aSize.y );

    if( aMargins[3] )
        painter.drawRect( 0, aSize.y - aMargins[3], aSize.x, aMargins[3] );

    return pixmap;
}


COLOR_SWATCH::COLOR_SWATCH( wxWidgetArgCompat aParent, const COLOR4D& aColor, int aID,
                            const COLOR4D& aBackground, const COLOR4D& aDefault,
                            SWATCH_SIZE aSwatchSize, bool aTriggerWithSingleClick ) :
        QLabel( aParent.get() ),
        m_id( aID ),
        m_color( aColor ),
        m_background( aBackground ),
        m_default( aDefault ),
        m_userColors( nullptr ),
        m_readOnly( false ),
        m_supportsOpacity( true )
{
    KI_ASSERT_MSG( aSwatchSize != SWATCH_EXPAND,
                   wxS( "SWATCH_EXPAND not supported in COLOR_SWATCH" ) );

    switch( aSwatchSize )
    {
    case SWATCH_MEDIUM: m_size = DialogUnitsToPixels( this, SWATCH_SIZE_MEDIUM_DU ); break;
    case SWATCH_SMALL:  m_size = DialogUnitsToPixels( this, SWATCH_SIZE_SMALL_DU );  break;
    case SWATCH_LARGE:  m_size = DialogUnitsToPixels( this, SWATCH_SIZE_LARGE_DU );  break;
    case SWATCH_EXPAND: m_size = DialogUnitsToPixels( this, SWATCH_SIZE_LARGE_DU );  break;
    }

    init( aTriggerWithSingleClick );
}


COLOR_SWATCH::COLOR_SWATCH( wxWidgetArgCompat aParent ) :
        QLabel( aParent.get() ),
        m_id( -1 ),
        m_userColors( nullptr ),
        m_readOnly( false ),
        m_supportsOpacity( true )
{
    m_size = DialogUnitsToPixels( this, SWATCH_SIZE_MEDIUM_DU );
    init( false );
}


void COLOR_SWATCH::init( bool aTriggerWithSingleClick )
{
    m_checkerboardSize = DialogUnitsToPixels( this, CHECKERBOARD_SIZE_DU );
    m_checkerboardBg = FromQColor( parentWidget() ? parentWidget()->palette().color( QPalette::Window )
                                                  : palette().color( QPalette::Window ) );

    // In a dialog, a click opens the color picker
    m_singleClick = aTriggerWithSingleClick
                    || dynamic_cast<DIALOG_SHIM*>( parentWidget() ? parentWidget()->window() : nullptr );

    setFixedSize( m_size.x, m_size.y );
    showBitmap();
}


void COLOR_SWATCH::showBitmap()
{
    setPixmap( MakeBitmap( m_color, m_background, m_size, m_checkerboardSize, m_checkerboardBg,
                           devicePixelRatioF() ) );
}


void COLOR_SWATCH::sendChanged()
{
    QMetaObject::invokeMethod( this,
                               [this]()
                               {
                                   if( m_changedCallback )
                                       m_changedCallback();
                               },
                               Qt::QueuedConnection );
}


void COLOR_SWATCH::mousePressEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::MiddleButton
            || ( aEvent->button() == Qt::LeftButton && m_singleClick ) )
    {
        GetNewSwatchColor();
        return;
    }

    // The other clicks go to the parent (the context menus of the panels)
    QLabel::mousePressEvent( aEvent );
}


void COLOR_SWATCH::mouseReleaseEvent( QMouseEvent* aEvent )
{
    QLabel::mouseReleaseEvent( aEvent );
}


void COLOR_SWATCH::mouseDoubleClickEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::LeftButton && !m_singleClick )
    {
        GetNewSwatchColor();
        return;
    }

    QLabel::mouseDoubleClickEvent( aEvent );
}


void COLOR_SWATCH::SetSwatchColor( const COLOR4D& aColor, bool aSendEvent )
{
    m_color = aColor;

    showBitmap();

    if( aSendEvent )
        sendChanged();
}


void COLOR_SWATCH::SetDefaultColor( const COLOR4D& aColor )
{
    m_default = aColor;
}


void COLOR_SWATCH::SetSwatchBackground( const COLOR4D& aBackground )
{
    m_background = aBackground;

    showBitmap();
}


COLOR4D COLOR_SWATCH::GetSwatchColor() const
{
    return m_color;
}


void COLOR_SWATCH::GetNewSwatchColor()
{
    if( m_readOnly )
    {
        if( m_readOnlyCallback )
            m_readOnlyCallback();

        return;
    }

    DIALOG_COLOR_PICKER dialog( window(), m_color, m_supportsOpacity, m_userColors, m_default );

    // ShowModal()'s event pump can let our owning panel rebuild or destroy us; guard `this`.
    QPointer<COLOR_SWATCH> self( this );
    int                    result = dialog.ShowModal();

    if( !self )
        return;

    if( result == wxID_OK )
    {
        COLOR4D newColor = dialog.GetColor();

        m_color = newColor;
        showBitmap();
        sendChanged();
    }
}


void COLOR_SWATCH::OnDarkModeToggle()
{
    QWidget* parent = parentWidget();
    m_checkerboardBg = FromQColor( ( parent ? parent : this )->palette().color( QPalette::Window ) );

    showBitmap();
}
