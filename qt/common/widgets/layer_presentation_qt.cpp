/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file layer_presentation_qt.cpp
 * The drawing of the layer swatches with Qt (see common/widgets/layer_presentation.cpp).
 */

#include <widgets/layer_presentation.h>

#include <algorithm>
#include <cmath>

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

#include <gal/color4d.h>
#include <toolkit/bitmaps.h>
#include <qt_ui.h>

using namespace KIGFX;


void LAYER_PRESENTATION::DrawColorSwatch( QPixmap& aLayerbmp, const COLOR4D& aBackground,
                                          const COLOR4D& aColor )
{
    QPainter painter( &aLayerbmp );
    QRect    rect( 0, 0, aLayerbmp.width(), aLayerbmp.height() );

    if( aBackground != COLOR4D::UNSPECIFIED )
        painter.fillRect( rect, ToQColor( aBackground.WithAlpha( 1.0 ) ) );

    // The color of the layer, over the background, framed by a black line
    painter.fillRect( rect, ToQColor( aColor ) );

    painter.setBrush( Qt::NoBrush );
    painter.setPen( QPen( Qt::black, 1 ) );
    painter.drawRect( rect.adjusted( 0, 0, -1, -1 ) );
}


void LAYER_PRESENTATION::DrawColorSwatch( QPixmap& aLayerbmp, int aLayer ) const
{
    const COLOR4D bgColor = getLayerColor( LAYER_PCB_BACKGROUND );
    const COLOR4D color = getLayerColor( aLayer );

    DrawColorSwatch( aLayerbmp, bgColor, color );
}


// Helper function to create a single bitmap at a specific size using vector graphics
static QPixmap createLayerPairBitmapAtSize( const COLOR4D& aTopColor, const COLOR4D& aBottomColor,
                                            int aSize )
{
    QPixmap bitmap( aSize, aSize );
    bitmap.fill( Qt::transparent );

    QPainter painter( &bitmap );
    painter.setRenderHint( QPainter::Antialiasing );

    int sepTopX = aSize - aSize / 3;
    int sepTopY = -1;
    int sepBotX = aSize / 3 - 1;
    int sepBotY = aSize;

    // Draw left quadrilateral (top layer)
    QPainterPath topPath;
    topPath.moveTo( 0, 0 );
    topPath.lineTo( sepTopX, 0 );
    topPath.lineTo( sepBotX, aSize );
    topPath.lineTo( 0, aSize );
    topPath.closeSubpath();

    painter.setPen( Qt::NoPen );
    painter.setBrush( ToQColor( aTopColor.WithAlpha( 1.0 ) ) );
    painter.drawPath( topPath );

    // Draw right quadrilateral (bottom layer)
    QPainterPath bottomPath;
    bottomPath.moveTo( sepTopX, 0 );
    bottomPath.lineTo( aSize, 0 );
    bottomPath.lineTo( aSize, aSize );
    bottomPath.lineTo( sepBotX, aSize );
    bottomPath.closeSubpath();

    painter.setBrush( ToQColor( aBottomColor.WithAlpha( 1.0 ) ) );
    painter.drawPath( bottomPath );

    int lineScale = std::max( 1, (int) std::lround( aSize / 24.0 ) );

    // Draw separator line with white outline and black center
    painter.setPen( QPen( Qt::white, 3 * lineScale ) );
    painter.drawLine( sepTopX, sepTopY, sepBotX, sepBotY );

    painter.setPen( QPen( Qt::black, 1 * lineScale ) );
    painter.drawLine( sepTopX, sepTopY, sepBotX, sepBotY );

    return bitmap;
}


TOOLKIT_BITMAP_BUNDLE LAYER_PRESENTATION::CreateLayerPairIcon( const COLOR4D& aTopColor,
                                                               const COLOR4D& aBottomColor, int aDefSize )
{
    QIcon icon;

    for( double scale : { 1.0, 1.3334, 1.5, 2.0, 2.6667, 3.0 } )
    {
        icon.addPixmap( createLayerPairBitmapAtSize( aTopColor, aBottomColor,
                                                     (int) ( aDefSize * scale ) ) );
    }

    return icon;
}


TOOLKIT_BITMAP_BUNDLE LAYER_PRESENTATION::CreateLayerPairIcon( int aLeftLayer, int aRightLayer,
                                                               int aDefSize ) const
{
    const COLOR4D topColor = getLayerColor( aLeftLayer );
    const COLOR4D bottomColor = getLayerColor( aRightLayer );

    return CreateLayerPairIcon( topColor, bottomColor, aDefSize );
}
