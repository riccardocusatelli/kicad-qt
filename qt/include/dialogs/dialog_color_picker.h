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
 * @file dialog_color_picker.h
 * The color picker of KiCad, of the Qt build: the layout of dialog_color_picker_base.fbp,
 * with the widgets of Qt (see include/dialogs/dialog_color_picker.h).
 */

#ifndef DIALOG_COLOR_PICKER_H
#define DIALOG_COLOR_PICKER_H

#include <vector>

#include <QImage>
#include <QPoint>

#include <gal/color4d.h>
#include <custom_color_item.h>
#include <dialogs/dialog_color_picker_base.h>

class QMouseEvent;


/**
 * A class to handle a custom color (predefined color) for the color picker dialog.
 */
enum CHANGED_COLOR
{
    ALL_CHANGED,
    RED_CHANGED,
    BLUE_CHANGED,
    GREEN_CHANGED,
    ALPHA_CHANGED,
    HUE_CHANGED,
    SAT_CHANGED,
    VAL_CHANGED,
    HEX_CHANGED,
    INIT
};


class DIALOG_COLOR_PICKER : public DIALOG_COLOR_PICKER_BASE
{
public:
    /**
     * Dialog constructor
     *
     * @param aParent is the caller
     * @param aCurrentColor is the current color, used to show it in dialog
     * @param aAllowOpacityControl true to allow opacity (alpha channel) setting
     *        false to not show this setting (opacity = 1.0 always)
     * @param aUserColors: if not null is a list of defined colors replacing the dialog
     *        predefined colors
     * @param aDefaultColor: if not null the color of the "Reset to Default" button
     */
    DIALOG_COLOR_PICKER( QWidget* aParent, const KIGFX::COLOR4D& aCurrentColor,
                         bool aAllowOpacityControl, CUSTOM_COLORS_LIST* aUserColors = nullptr,
                         const KIGFX::COLOR4D& aDefaultColor = KIGFX::COLOR4D::UNSPECIFIED );
    ~DIALOG_COLOR_PICKER() override;

    KIGFX::COLOR4D GetColor();

private:
    /* When the dialog is created, the mouse cursor can be on the RGB or HSV palette selector
     * Because this dialog is created by clicking double clicking on a widget, the left mouse
     * button is down, thus creating a not wanted mouse event inside this dialog
     * m_allowMouseEvents is first set to false, and then set to true on the first left mouse
     * clicking inside a palette
     */
    bool m_allowMouseEvents;
    bool m_allowOpacityCtrl;            ///< true to show the widget,
                                        ///< false to keep alpha channel = 1.0
    KIGFX::COLOR4D m_previousColor4D;   ///< the initial color4d
    KIGFX::COLOR4D m_newColor4D;        ///< the current color4d
    KIGFX::COLOR4D m_defaultColor;      ///< The default color4d

    /// the list of color4d ordered by button ID, for predefined colors
    std::vector<KIGFX::COLOR4D> m_Color4DList;
    int m_cursorsSize;

    QPoint m_cursorBitmapRed;           ///< the red cursor on the RGB bitmap palette.
    QPoint m_cursorBitmapGreen;         ///< the green cursor on the RGB bitmap palette.
    QPoint m_cursorBitmapBlue;          ///< the blue cursor on the RGB bitmap palette.
    QPoint m_cursorBitmapHSV;           ///< the cursor on the HSV bitmap palette.
    QPoint* m_selectedCursor;           ///< the ref cursor to the selected cursor, if any, or null.

    double m_hue;                       ///< the current hue, in degrees (0 ... 360)
    double m_sat;                       ///< the current saturation (0 ... 1.0)
    double m_val;                       ///< the current value (0 ... 1.0)

    QImage m_bitmapRGB;                 ///< the basic RGB palette (physical pixels)
    QImage m_bitmapHSV;                 ///< the basic HUV palette (physical pixels)

    std::vector<QLabel*> m_colorSwatches;    ///< list of defined colors buttons

    void SetEditVals( CHANGED_COLOR aChanged, bool aCheckTransparency );
    void drawAll();

    void createHSVBitmap();             ///< generate the bitmap that shows the HSV color circle
    void drawHSVPalette();              ///< draws the HSV color circle
    void createRGBBitmap();             ///< generate the bitmap that shows the RVB color space
    void drawRGBPalette();              ///< draws the RVB color space

    ///< repaint a static bitmap with the aColor4D color
    void updatePreview( QLabel* aStaticBitmap, KIGFX::COLOR4D& aColor4D );

    ///< Event handler from the slider: brightness (value) control
    void OnChangeBrightness( QWidget* aSource ) override;

    ///< Event handler from the slider: alpha (transparency) control
    void OnChangeAlpha( QWidget* aSource ) override;

    ///< Event handlers from the spin controls
    void OnChangeEditRed( QWidget* aSource ) override;
    void OnChangeEditGreen( QWidget* aSource ) override;
    void OnChangeEditBlue( QWidget* aSource ) override;
    void OnChangeEditHue( QWidget* aSource ) override;
    void OnChangeEditSat( QWidget* aSource ) override;

    ///< mouse handlers, when clicking on a palette bitmap
    void rgbMouseClick( QMouseEvent* aEvent );
    void rgbMouseDrag( QMouseEvent* aEvent );
    void hsvMouseClick( QMouseEvent* aEvent );
    void hsvMouseDrag( QMouseEvent* aEvent );

    void onSize( QWidget* aSource ) override;

    void OnColorValueText( QWidget* aSource ) override;

    ///< Event handler for the reset button press
    void OnResetButton( QWidget* aSource ) override;

    bool setHSvaluesFromCursor( const QPoint& aMouseCursor );

    ///< Event handler for defined color buttons
    void buttColorClick( int aId );

    ///< called when creating the dialog
    bool TransferDataToWindow() override;

    void initDefinedColors( CUSTOM_COLORS_LIST* aPredefinedColors );

    void updateHandleSize();

    /// The physical pixels of a position (or size) in logical pixels.
    QPoint toPhys( const QPoint& aPoint ) const;

    // convert double value 0 ... 1 to int 0 ... aValMax
    int normalizeToInt( double aValue, int aValMax = 255 )
    {
        return ( aValue * aValMax ) + 0.5;
    }
};

#endif  // #define DIALOG_COLOR_PICKER_H
