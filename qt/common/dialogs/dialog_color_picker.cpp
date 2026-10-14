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
 * @file dialog_color_picker.cpp
 * DIALOG_COLOR_PICKER of the Qt build (see common/dialogs/dialog_color_picker.cpp).
 */

#include <dialogs/dialog_color_picker.h>

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QSignalBlocker>

#include <kiface_base.h>
#include <settings/app_settings.h>
#include <widgets/color_swatch.h>
#include <core/ki_assert.h>
#include <qt_ui.h>
#include <ui_hooks.h>

#define ALPHA_MAX 100   // the max value returned by the alpha (opacity) slider
#define SLOPE_AXIS ( bmsize.height() / 5.28 ) // was 50 at 264 size

// The ids of the predefined colors
#define ID_COLOR_BLACK 2000 // colors_id = ID_COLOR_BLACK a ID_COLOR_BLACK + NBCOLORS-1

using KIGFX::COLOR4D;


DIALOG_COLOR_PICKER::DIALOG_COLOR_PICKER( wxWidgetArgCompat aParent, const COLOR4D& aCurrentColor,
                                          bool aAllowOpacityControl,
                                          CUSTOM_COLORS_LIST* aUserColors,
                                          const COLOR4D& aDefaultColor ) :
        DIALOG_COLOR_PICKER_BASE( aParent.get() )
{
    m_allowMouseEvents = false;
    m_allowOpacityCtrl = aAllowOpacityControl;
    m_previousColor4D = aCurrentColor;
    m_newColor4D = aCurrentColor;
    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    m_selectedCursor = nullptr;
    m_defaultColor = aDefaultColor;

    updateHandleSize();

    m_OldColorRect->setMinimumSize( 20, 20 );
    m_NewColorRect->setMinimumSize( 20, 20 );

    m_colorValue->setMinimumWidth( m_colorValue->fontMetrics().horizontalAdvance(
                                           QStringLiteral( "@{color(DEVICE_BACKGROUND)}" ) ) + 8 );

    if( !m_allowOpacityCtrl )
    {
        m_opacityLabel->hide();
        m_sliderTransparency->hide();

        if( aCurrentColor != COLOR4D::UNSPECIFIED )
        {
            m_previousColor4D.a = 1.0;
            m_newColor4D.a      = 1.0;
        }
    }

    // UNSPECIFIED is ( 0, 0, 0, 0 ) but that is unfriendly for editing because you have to notice
    // first that the value slider is all the way down before you get any color
    if( aCurrentColor == COLOR4D::UNSPECIFIED )
        m_val = 1.0;

    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
    KI_ASSERT( cfg );

    if( cfg )
        m_notebook->setCurrentIndex( cfg->m_ColorPicker.default_tab );

    // The palettes are drawn as bitmaps: a press selects a cursor, a move with the button
    // pressed drags it
    KIQT::FilterEvents( m_RgbBitmap,
                        [this]( QEvent* aEvent )
                        {
                            if( aEvent->type() == QEvent::MouseButtonPress )
                                rgbMouseClick( static_cast<QMouseEvent*>( aEvent ) );
                            else if( aEvent->type() == QEvent::MouseMove )
                                rgbMouseDrag( static_cast<QMouseEvent*>( aEvent ) );

                            return false;
                        } );

    KIQT::FilterEvents( m_HsvBitmap,
                        [this]( QEvent* aEvent )
                        {
                            if( aEvent->type() == QEvent::MouseButtonPress )
                                hsvMouseClick( static_cast<QMouseEvent*>( aEvent ) );
                            else if( aEvent->type() == QEvent::MouseMove )
                                hsvMouseDrag( static_cast<QMouseEvent*>( aEvent ) );

                            return false;
                        } );

    // Build the defined colors panel:
    initDefinedColors( aUserColors );

    /**
     * There are two types of color settings: theme colors and local overrides.
     * Theme colors have a default value, and the Reset to Default button reverts to it.
     * Local override colors have a default of UNSPECIFIED, which means "use the theme color".
     * The underlying action is the same, but we change the label here because the action from
     * the point of view of the user is slightly different.
     */
    if( aDefaultColor == COLOR4D::UNSPECIFIED )
        m_resetToDefault->setText( ToQString( _( "Clear Color" ) ) );

    SetupStandardButtons();
}


DIALOG_COLOR_PICKER::~DIALOG_COLOR_PICKER()
{
    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
    KI_ASSERT( cfg );

    if( cfg )
        cfg->m_ColorPicker.default_tab = m_notebook->currentIndex();
}


KIGFX::COLOR4D DIALOG_COLOR_PICKER::GetColor()
{
    return KIGFX::COLOR4D( ToWxString( m_colorValue->text() ) );
}


QPoint DIALOG_COLOR_PICKER::toPhys( const QPoint& aPoint ) const
{
    const double scale = devicePixelRatioF();

    return QPoint( (int) std::lround( aPoint.x() * scale ), (int) std::lround( aPoint.y() * scale ) );
}


void DIALOG_COLOR_PICKER::updatePreview( QLabel* aStaticBitmap, COLOR4D& aColor4D )
{
    QSize  size = aStaticBitmap->size().expandedTo( aStaticBitmap->minimumSize() );
    wxSize checkerboardSize = COLOR_SWATCH::DialogUnitsToPixels( this, CHECKERBOARD_SIZE_DU );
    QColor background = aStaticBitmap->parentWidget()->palette().color( QPalette::Window );

    aStaticBitmap->setPixmap( COLOR_SWATCH::MakeBitmap( aColor4D, COLOR4D::WHITE,
                                                        wxSize( size.width(), size.height() ),
                                                        checkerboardSize, FromQColor( background ),
                                                        devicePixelRatioF() ) );
}


bool DIALOG_COLOR_PICKER::TransferDataToWindow()
{
    SetEditVals( INIT, false );

    // The spin controls as large as 3 digits
    for( QSpinBox* spin : { m_spinCtrlGreen, m_spinCtrlBlue, m_spinCtrlRed, m_spinCtrlHue,
                            m_spinCtrlSaturation } )
    {
        spin->setMinimumWidth( spin->minimumSizeHint().width() );
    }

    finishDialogSettings();

    // Draw all bitmaps, with colors according to the color 4D
    updatePreview( m_OldColorRect, m_previousColor4D );
    drawAll();

    return true;
}


void DIALOG_COLOR_PICKER::initDefinedColors( CUSTOM_COLORS_LIST* aPredefinedColors )
{
    // Colors are built from the colorRefs() table (size NBCOLORS).
    // The look is better when colorRefs() order is displayed in a grid matrix
    // of 7 row and 5 columns, first filling a row, and after the next column.
    // But the grid used here must be filled by columns, then next row
    // the best interval colorRefs() from a matrix row to the next row is 6
    // So when have to reorder the index used to explore colorRefs()
    int grid_col = 0;
    int grid_row = 0;
    int table_row_count = 7;

    // The grid of the defined colors: a swatch and its name by cell, 5 cells by row
    const int columns = m_fgridColor->columnCount() > 1 ? m_fgridColor->columnCount() / 2 : 5;
    int       cell = 0;

    wxSize  swatchSize = COLOR_SWATCH::DialogUnitsToPixels( this, SWATCH_SIZE_LARGE_DU );
    wxSize  checkerboardSize = COLOR_SWATCH::DialogUnitsToPixels( this, CHECKERBOARD_SIZE_DU );
    COLOR4D checkboardBackground = FromQColor( m_panelDefinedColors->palette().color( QPalette::Window ) );

    auto addSwatch =
            [&]( int aId, COLOR4D aColor, const wxString& aColorName )
            {
                QLabel* swatch = new QLabel( m_panelDefinedColors );
                swatch->setPixmap( COLOR_SWATCH::MakeBitmap( aColor, COLOR4D::WHITE, swatchSize,
                                                             checkerboardSize, checkboardBackground,
                                                             devicePixelRatioF() ) );

                QLabel* label = new QLabel( ToQString( aColorName ), m_panelDefinedColors );

                int row = cell / columns;
                int col = ( cell % columns ) * 2;
                cell++;

                m_fgridColor->addWidget( swatch, row, col, Qt::AlignVCenter );
                m_fgridColor->addWidget( label, row, col + 1, Qt::AlignVCenter );

                m_colorSwatches.push_back( swatch );

                // A click chooses the color, a double click closes the dialog with it
                KIQT::FilterEvents( swatch,
                                    [this, aId]( QEvent* aEvent )
                                    {
                                        if( aEvent->type() == QEvent::MouseButtonPress )
                                        {
                                            buttColorClick( aId );
                                        }
                                        else if( aEvent->type() == QEvent::MouseButtonDblClick )
                                        {
                                            EndModal( wxID_OK );
                                            return true;
                                        }

                                        return false;
                                    } );
            };

    // If no predefined list is given, build the default predefined colors:
    if( aPredefinedColors )
    {
        for( unsigned jj = 0; jj < aPredefinedColors->size() && jj < NBCOLORS; ++jj )
        {
            CUSTOM_COLOR_ITEM* item = & *aPredefinedColors->begin() + jj;
            int                butt_ID = ID_COLOR_BLACK + jj;

            addSwatch( butt_ID, item->m_Color, item->m_ColorName );
            m_Color4DList.push_back( item->m_Color );
        }
    }
    else
    {
        m_Color4DList.assign( NBCOLORS, COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );

        for( int jj = 0; jj < NBCOLORS; ++jj, grid_col++ )
        {
            if( grid_col * table_row_count >= NBCOLORS )
            {
                // the current grid row is filled, and we must fill the next grid row
                grid_col = 0;
                grid_row++;
            }

            int     ii = grid_row + ( grid_col * table_row_count ); // The index in colorRefs()
            int     butt_ID = ID_COLOR_BLACK + ii;
            COLOR4D buttcolor = COLOR4D( colorRefs()[ii].m_Numcolor );

            addSwatch( butt_ID, buttcolor, wxGetTranslation( colorRefs()[ii].m_ColorName ) );
            m_Color4DList[ butt_ID - ID_COLOR_BLACK ] = buttcolor;
        }
    }
}


/// Set a pixel of a palette, if it is in the image.
static void setRGB( QImage& aImage, int aX, int aY, double aRed, double aGreen, double aBlue )
{
    if( aX >= 0 && aY >= 0 && aX < aImage.width() && aY < aImage.height() )
        aImage.setPixel( aX, aY, qRgb( (int) aRed, (int) aGreen, (int) aBlue ) );
}


void DIALOG_COLOR_PICKER::createRGBBitmap()
{
    QPoint phys = toPhys( QPoint( m_RgbBitmap->width(), m_RgbBitmap->height() ) );
    QSize  bmsize( phys.x(), phys.y() );
    int    half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    // We use here a Y axis from bottom to top and origin to center, So we need to map
    // coordinated to write pixel in an image.
#undef MAPX
#undef MAPY
#define MAPX( xx ) bmsize.width() / 2 + ( xx )
#define MAPY( yy ) bmsize.height() / 2 - ( yy )

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    COLOR4D color;

    // Red blue area in X Z 3d axis
    double inc = 255.0 / half_size;
    double slope = SLOPE_AXIS/half_size;
    color.g = 0.0;

    // a temporary buffer to build the color map, with the background of the window
    QImage img( bmsize, QImage::Format_RGB32 );
    img.fill( palette().color( QPalette::Window ) );

    // Build the palette
    for( int xx = 0; xx < half_size; xx++ ) // blue axis
    {
        color.b = inc * xx;

        for( int yy = 0; yy < half_size; yy++ )  // Red axis
        {
            color.r = inc * yy;
            setRGB( img, MAPX( xx ), MAPY( yy - (slope*xx) ), color.r, color.g, color.b );
        }
    }

    // Red green area in y Z 3d axis
    color.b = 0.0;

    for( int xx = 0; xx < half_size; xx++ )     // green axis
    {
        color.g = inc * xx;

        for( int yy = 0; yy < half_size; yy++ ) // Red axis
        {
            color.r = inc * yy;
            setRGB( img, MAPX( -xx ), MAPY( yy - (slope*xx) ), color.r, color.g, color.b );
        }
    }

    // Blue green area in x y 3d axis
    color.r = 0.0;

    for( int xx = 0; xx < half_size; xx++ )     // green axis
    {
        color.g = inc * xx;

        for( int yy = 0; yy < half_size; yy++ ) // blue axis
        {
            color.b = inc * yy;

            // Mapping the xx, yy color axis to draw coordinates is more tricky than previously
            // in DC coordinates:
            // the blue axis is the (0, 0) to half_size, (-yy - SLOPE_AXIS)
            // the green axis is the (0, 0) to - half_size, (-yy - SLOPE_AXIS)
            int drawX = -xx + yy;
            int drawY = - std::min( xx,yy ) * 0.9;
            setRGB( img, MAPX( drawX ), MAPY( drawY - std::abs( slope*drawX ) ),
                    color.r, color.g, color.b );
        }
    }

    m_bitmapRGB = img;
}


void DIALOG_COLOR_PICKER::createHSVBitmap()
{
    QPoint phys = toPhys( QPoint( m_HsvBitmap->width(), m_HsvBitmap->height() ) );
    QSize  bmsize( phys.x(), phys.y() );
    int    half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    // a temporary buffer to build the color map, with the background of the window
    QImage img( bmsize, QImage::Format_RGB32 );
    img.fill( palette().color( QPalette::Window ) );

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    double  hue, sat;
    COLOR4D color;
    int     sq_radius = half_size*half_size;

    // Build the palette
    for( int xx = -half_size; xx < half_size; xx++ )
    {
        for( int yy = -half_size; yy < half_size; yy++ )
        {
            sat = double(xx*xx + yy*yy) / sq_radius;

            // sat is <= 1.0
            // any value > 1.0 is not a valid HSB color:
            if( sat > 1.0 )
                continue;

            // sat is the distance from center
            sat = sqrt( sat );
            hue = atan2( (double)yy, (double)xx ) * 180 / M_PI;

            if( hue < 0.0 )
                hue += 360.0;

            color.FromHSV( hue, sat, 1.0 );

            setRGB( img, MAPX( xx ), MAPY( yy ), color.r * 255, color.g * 255, color.b * 255 );
        }
    }

    m_bitmapHSV = img;
}


void DIALOG_COLOR_PICKER::drawRGBPalette()
{
    QPoint phys = toPhys( QPoint( m_RgbBitmap->width(), m_RgbBitmap->height() ) );

    if( m_bitmapRGB.isNull() || m_bitmapRGB.size() != QSize( phys.x(), phys.y() ) )
        createRGBBitmap();

    QSize bmsize = m_bitmapRGB.size();
    int   half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    QImage newBm( m_bitmapRGB );

    {
        QPainter bitmapDC( &newBm );

        // Use Y axis from bottom to top and origin to center
        bitmapDC.translate( half_size, half_size );
        bitmapDC.scale( 1, -1 );

        // Reserve room to draw cursors inside the bitmap
        half_size -= m_cursorsSize / 2;

        // Draw the 3 RGB cursors, using white color to make them always visible:
        bitmapDC.setPen( QPen( QColor( 255, 255, 255 ), 2 ) );  // use 2 pixels for pen size
        bitmapDC.setBrush( Qt::NoBrush );
        int half_csize = m_cursorsSize / 2;

        double slope = SLOPE_AXIS / half_size;

        // Red axis cursor (Z 3Daxis):
        m_cursorBitmapRed.setX( 0 );
        m_cursorBitmapRed.setY( m_newColor4D.r * half_size );
        bitmapDC.drawRect( m_cursorBitmapRed.x() - half_csize, m_cursorBitmapRed.y() - half_csize,
                           m_cursorsSize, m_cursorsSize );

        // Blue axis cursor (X 3Daxis):
        m_cursorBitmapBlue.setX( m_newColor4D.b * half_size );
        m_cursorBitmapBlue.setY( - slope*m_cursorBitmapBlue.x() );
        bitmapDC.drawRect( m_cursorBitmapBlue.x() - half_csize, m_cursorBitmapBlue.y() - half_csize,
                           m_cursorsSize, m_cursorsSize );

        // Green axis cursor (Y 3Daxis):
        m_cursorBitmapGreen.setX( m_newColor4D.g * half_size );
        m_cursorBitmapGreen.setY( - slope * m_cursorBitmapGreen.x() );
        m_cursorBitmapGreen.setX( -m_cursorBitmapGreen.x() );

        bitmapDC.drawRect( m_cursorBitmapGreen.x() - half_csize,
                           m_cursorBitmapGreen.y() - half_csize,
                           m_cursorsSize, m_cursorsSize );

        // Draw the 3 RGB axis:
        half_size += half_size/5;
        bitmapDC.drawLine( 0, 0, 0, half_size );                         // Red axis (Z 3D axis)
        bitmapDC.drawLine( 0, 0, half_size, - half_size*slope );         // Blue axis (X 3D axis)
        bitmapDC.drawLine( 0, 0, -half_size, - half_size*slope );        // green axis (Y 3D axis)
    }

    newBm.setDevicePixelRatio( devicePixelRatioF() );
    m_RgbBitmap->setPixmap( QPixmap::fromImage( newBm ) );
}


void DIALOG_COLOR_PICKER::drawHSVPalette()
{
    QPoint phys = toPhys( QPoint( m_HsvBitmap->width(), m_HsvBitmap->height() ) );

    if( m_bitmapHSV.isNull() || m_bitmapHSV.size() != QSize( phys.x(), phys.y() ) )
        createHSVBitmap();

    QSize bmsize = m_bitmapHSV.size();
    int   half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    QImage newBm( m_bitmapHSV );

    {
        QPainter bitmapDC( &newBm );

        // Use Y axis from bottom to top and origin to center
        bitmapDC.translate( half_size, half_size );
        bitmapDC.scale( 1, -1 );

        // Reserve room to draw cursors inside the bitmap
        half_size -= m_cursorsSize / 2;

        // Draw the HSB cursor:
        m_cursorBitmapHSV.setX( cos( m_hue * M_PI / 180.0 ) * half_size * m_sat );
        m_cursorBitmapHSV.setY( sin( m_hue * M_PI / 180.0 ) * half_size * m_sat );

        bitmapDC.setPen( QPen( QColor( 0, 0, 0 ), 2 ) );     // Use 2 pixels as pensize
        bitmapDC.setBrush( Qt::NoBrush );

        bitmapDC.drawRect( m_cursorBitmapHSV.x() - ( m_cursorsSize / 2 ),
                           m_cursorBitmapHSV.y() - ( m_cursorsSize / 2 ),
                           m_cursorsSize, m_cursorsSize );
    }

    newBm.setDevicePixelRatio( devicePixelRatioF() );
    m_HsvBitmap->setPixmap( QPixmap::fromImage( newBm ) );
}


void DIALOG_COLOR_PICKER::SetEditVals( CHANGED_COLOR aChanged, bool aCheckTransparency )
{
    if( aCheckTransparency )
    {
        // If they've changed the color, they probably don't want it to remain 100% transparent,
        // and it looks like a bug when changing the color has no effect.
        if( m_newColor4D.a == 0.0 )
            m_newColor4D.a = 1.0;
    }

    // The values are shown without the change events of the controls (as SetValue() of
    // wxWidgets)
    QSignalBlocker blockAlpha( m_sliderTransparency );
    QSignalBlocker blockRed( m_spinCtrlRed );
    QSignalBlocker blockGreen( m_spinCtrlGreen );
    QSignalBlocker blockBlue( m_spinCtrlBlue );
    QSignalBlocker blockHue( m_spinCtrlHue );
    QSignalBlocker blockSat( m_spinCtrlSaturation );
    QSignalBlocker blockVal( m_sliderBrightness );
    QSignalBlocker blockText( m_colorValue );

    m_sliderTransparency->setValue( normalizeToInt( m_newColor4D.a, ALPHA_MAX ) );

    if( aChanged == RED_CHANGED || aChanged == GREEN_CHANGED || aChanged == BLUE_CHANGED )
        m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );

    if( aChanged != RED_CHANGED )
        m_spinCtrlRed->setValue( normalizeToInt( m_newColor4D.r ) );

    if( aChanged != GREEN_CHANGED )
        m_spinCtrlGreen->setValue( normalizeToInt( m_newColor4D.g ) );

    if( aChanged != BLUE_CHANGED )
        m_spinCtrlBlue->setValue( normalizeToInt( m_newColor4D.b  ) );

    if( aChanged != HUE_CHANGED )
        m_spinCtrlHue->setValue( (int)m_hue );

    if( aChanged != SAT_CHANGED )
        m_spinCtrlSaturation->setValue( m_sat * 255 );

    if( aChanged != VAL_CHANGED )
        m_sliderBrightness->setValue(normalizeToInt( m_val ) );

    if( aChanged == INIT )
    {
        if( m_newColor4D.m_text )
            m_colorValue->setText( ToQString( *m_newColor4D.m_text ) );
        else
            m_colorValue->setText( ToQString( m_newColor4D.ToHexString() ) );
    }
    else if( aChanged != HEX_CHANGED )
    {
        m_newColor4D.m_text = nullptr;
        m_colorValue->setText( ToQString( m_newColor4D.ToHexString() ) );
    }
}


void DIALOG_COLOR_PICKER::updateHandleSize()
{
    // Size of square cursors drawn on color bitmaps
    m_cursorsSize = (int) std::lround( 8 * devicePixelRatioF() );
}


void DIALOG_COLOR_PICKER::drawAll()
{
    updateHandleSize();
    updatePreview( m_NewColorRect, m_newColor4D );
    drawHSVPalette();
    drawRGBPalette();
}


void DIALOG_COLOR_PICKER::buttColorClick( int aId )
{
    COLOR4D color( m_Color4DList[aId - ID_COLOR_BLACK] );
    m_newColor4D.r = color.r;
    m_newColor4D.g = color.g;
    m_newColor4D.b = color.b;
    m_newColor4D.a = color.a;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED, false );

    drawAll();
}


void DIALOG_COLOR_PICKER::rgbMouseClick( QMouseEvent* aEvent )
{
    m_allowMouseEvents = true;

    // The cursor position is relative to the RGB bitmap center
    QPoint mousePos = toPhys( aEvent->position().toPoint() );
    QSize  bmsize = m_bitmapRGB.size();
    int    half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    mousePos.rx() -= half_size;
    mousePos.ry() -= half_size;
    mousePos.setY( -mousePos.y() );       // Use the bottom to top vertical axis

    for( QPoint* cursor : { &m_cursorBitmapRed, &m_cursorBitmapGreen, &m_cursorBitmapBlue } )
    {
        QPoint dist = *cursor - mousePos;

        if( std::abs( dist.x() ) <= m_cursorsSize / 2 && std::abs( dist.y() ) <= m_cursorsSize / 2 )
        {
            m_selectedCursor = cursor;
            return;
        }
    }

    m_selectedCursor = nullptr;
}


void DIALOG_COLOR_PICKER::rgbMouseDrag( QMouseEvent* aEvent )
{
    if( !( aEvent->buttons() & Qt::LeftButton ) || !m_allowMouseEvents )
    {
        m_selectedCursor = nullptr;
        return;
    }

    if( m_selectedCursor != &m_cursorBitmapRed
     && m_selectedCursor != &m_cursorBitmapGreen
     && m_selectedCursor != &m_cursorBitmapBlue )
    {
        return;
    }

    // Adjust the HSV cursor position to follow the mouse cursor
    // The cursor position is relative to the RGB bitmap center
    QPoint mousePos = toPhys( aEvent->position().toPoint() );
    QSize  bmsize = m_bitmapRGB.size();
    int    half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    mousePos.rx() -= half_size;
    mousePos.ry() -= half_size;
    mousePos.setY( -mousePos.y() );     // Use the bottom to top vertical axis

    half_size -= m_cursorsSize / 2;     // the actual half_size of the palette area

    // Change colors according to the selected cursor:
    if( m_selectedCursor == &m_cursorBitmapRed )
    {
        if( mousePos.y() >= 0 && mousePos.y() <= half_size )
            m_newColor4D.r = (double)mousePos.y() / half_size;
        else
            return;
    }

    if( m_selectedCursor == &m_cursorBitmapGreen )
    {
        mousePos.setX( -mousePos.x() );

        if( mousePos.x() >= 0 && mousePos.x() <= half_size )
            m_newColor4D.g = (double)mousePos.x() / half_size;
        else
            return;
    }

    if( m_selectedCursor == &m_cursorBitmapBlue )
    {
        if( mousePos.x() >= 0 && mousePos.x() <= half_size )
            m_newColor4D.b = (double)mousePos.x() / half_size;
        else
            return;
    }

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::hsvMouseClick( QMouseEvent* aEvent )
{
    m_allowMouseEvents = true;

    if( setHSvaluesFromCursor( aEvent->position().toPoint() ) )
        drawAll();
}


void DIALOG_COLOR_PICKER::hsvMouseDrag( QMouseEvent* aEvent )
{
    if( !( aEvent->buttons() & Qt::LeftButton ) || !m_allowMouseEvents )
        return;

    if( setHSvaluesFromCursor( aEvent->position().toPoint() ) )
        drawAll();
}


void DIALOG_COLOR_PICKER::onSize( QWidget* aSource )
{
    drawAll();
}


void DIALOG_COLOR_PICKER::OnColorValueText( QWidget* aSource )
{
    if( m_newColor4D.SetFromHexString( ToWxString( m_colorValue->text() ) ) )
        m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );

    SetEditVals( HEX_CHANGED, false );
    drawAll();
}


bool DIALOG_COLOR_PICKER::setHSvaluesFromCursor( const QPoint& aMouseCursor )
{
    QPoint mousePos = toPhys( aMouseCursor );
    QSize  bmsize = m_bitmapHSV.size();
    int    half_size = std::min( bmsize.width(), bmsize.height() ) / 2;

    // Make the cursor position relative to the HSV bitmap center
    mousePos.rx() -= half_size;
    mousePos.ry() -= half_size;
    mousePos.setY( -mousePos.y() );   // Use the bottom to top vertical axis

    // The HS cursor position is restricted to a circle of radius half_size
    double dist_from_centre = hypot( (double)mousePos.x(), (double)mousePos.y() );

    if( dist_from_centre > half_size )
    {
        // Saturation cannot be calculated:
        return false;
    }

    m_cursorBitmapHSV = mousePos;

    // Set saturation and hue from new cursor position:
    half_size -= m_cursorsSize / 2;       // the actual half_size of the palette area
    m_sat = dist_from_centre / half_size;

    if( m_sat > 1.0 )
        m_sat = 1.0;

    m_hue = atan2( mousePos.y(), mousePos.x() ) / M_PI * 180.0;

    if( m_hue < 0 )
        m_hue += 360.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );
    SetEditVals( ALL_CHANGED, true );

    return true;
}


void DIALOG_COLOR_PICKER::OnChangeAlpha( QWidget* aSource )
{
    double alpha = (double)m_sliderTransparency->value() / ALPHA_MAX;
    m_newColor4D.a = alpha;
    updatePreview( m_NewColorRect, m_newColor4D );
    SetEditVals( ALPHA_CHANGED, false );
}


void DIALOG_COLOR_PICKER::OnChangeEditRed( QWidget* aSource )
{
    double val = (double)m_spinCtrlRed->value() / 255.0;
    m_newColor4D.r = val;
    SetEditVals( RED_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditGreen( QWidget* aSource )
{
    double val = (double)m_spinCtrlGreen->value() / 255.0;
    m_newColor4D.g = val;
    SetEditVals( GREEN_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditBlue( QWidget* aSource )
{
    double val = (double)m_spinCtrlBlue->value() / 255.0;
    m_newColor4D.b = val;
    SetEditVals( BLUE_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditHue( QWidget* aSource )
{
    m_hue = (double)m_spinCtrlHue->value();

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( HUE_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditSat( QWidget* aSource )
{
    m_sat = (double)m_spinCtrlSaturation->value() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( SAT_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeBrightness( QWidget* aSource )
{
    m_val = (double)m_sliderBrightness->value() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( VAL_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnResetButton( QWidget* aSource )
{
    m_newColor4D.r = m_defaultColor.r;
    m_newColor4D.g = m_defaultColor.g;
    m_newColor4D.b = m_defaultColor.b;
    m_newColor4D.a = m_defaultColor.a;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED, false );

    drawAll();
}
