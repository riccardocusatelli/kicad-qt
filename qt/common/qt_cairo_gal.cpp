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

#include "qt_cairo_gal.h"

#include <QPainter>
#include <QPen>

using namespace KIGFX;


QT_CAIRO_GAL::QT_CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions,
                            std::function<void()> aPresent ) :
        CAIRO_RASTER_GAL( aDisplayOptions ),
        m_present( std::move( aPresent ) )
{
}


void QT_CAIRO_GAL::present()
{
    if( m_present )
        m_present();
}


void QT_CAIRO_GAL::PaintCursor( QPainter& aPainter, double aRatio ) const
{
    if( !IsCursorEnabled() )
        return;

    const VECTOR2D p = ToScreen( m_cursorPosition ) / aRatio;
    const VECTOR2D size = VECTOR2D( m_screenSize ) / aRatio;
    const COLOR4D  cColor = getCursorColor();

    QPen pen( QColor::fromRgbF( cColor.r * cColor.a, cColor.g * cColor.a, cColor.b * cColor.a ) );
    pen.setCosmetic( true );
    aPainter.setPen( pen );

    if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_CROSS )
    {
        aPainter.drawLine( QPointF( 0, p.y ), QPointF( size.x, p.y ) );
        aPainter.drawLine( QPointF( p.x, 0 ), QPointF( p.x, size.y ) );
    }
    else if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL )
    {
        // Oversized but that's ok
        double diagonalSize = size.x + size.y;
        aPainter.drawLine( QPointF( p.x - diagonalSize, p.y - diagonalSize ),
                           QPointF( p.x + diagonalSize, p.y + diagonalSize ) );
        aPainter.drawLine( QPointF( p.x - diagonalSize, p.y + diagonalSize ),
                           QPointF( p.x + diagonalSize, p.y - diagonalSize ) );
    }
    else
    {
        const double cursorSize = 80;
        aPainter.drawLine( QPointF( p.x - cursorSize / 2, p.y ), QPointF( p.x + cursorSize / 2, p.y ) );
        aPainter.drawLine( QPointF( p.x, p.y - cursorSize / 2 ), QPointF( p.x, p.y + cursorSize / 2 ) );
    }
}
