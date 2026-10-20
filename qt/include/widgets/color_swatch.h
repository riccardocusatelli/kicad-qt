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
 * @file color_swatch.h
 * COLOR_SWATCH of the Qt build: a small widget showing a color, which opens the color picker
 * of KiCad (DIALOG_COLOR_PICKER).
 */

#ifndef COLOR_SWATCH__H_
#define COLOR_SWATCH__H_

#include <functional>
#include <vector>

#include <QLabel>

#include <wx/kiwx_gdi.h>
#include <wx/kiwx_window.h>
#include <QPixmap>

#include <kiqt.h>
#include <gal/color4d.h>
#include <custom_color_item.h>
#include <wx/gdicmn.h>


enum SWATCH_SIZE
{
    SWATCH_SMALL,
    SWATCH_MEDIUM,
    SWATCH_LARGE,
    SWATCH_EXPAND
};


/// The sizes of the swatches, in dialog units (as the wxWidgets build: a dialog unit is a
/// quarter of the average width of a character, an eighth of its height).
const static wxSize SWATCH_SIZE_SMALL_DU( 8, 6 );
const static wxSize SWATCH_SIZE_MEDIUM_DU( 24, 10 );
const static wxSize SWATCH_SIZE_LARGE_DU( 24, 16 );
const static wxSize CHECKERBOARD_SIZE_DU( 3, 3 );


/**
 * A simple color swatch of the kind used to set layer colors.
 *
 * A double click (or a single click in a dialog) opens the color picker; the changed callback
 * is called when the user chose a color (from the event loop, as the COLOR_SWATCH_CHANGED event
 * of the wxWidgets build).
 */
class COLOR_SWATCH : public QLabel
{
public:
    /**
     * Construct a COLOR_SWATCH
     *
     * @param aParent parent window
     * @param aColor initial swatch color
     * @param aID id to use when sending swatch events
     * @param aBackground the background color
     * @param aDefault the default color
     * @param aSwatchSize the size of the swatch
     * @param aTriggerWithSingleClick a single click opens the color picker
     */
    COLOR_SWATCH( wxWidgetArgCompat aParent, const KIGFX::COLOR4D& aColor, int aID,
                  const KIGFX::COLOR4D& aBackground, const KIGFX::COLOR4D& aDefault,
                  SWATCH_SIZE aSwatchSize, bool aTriggerWithSingleClick = false );

    /**
     * Constructor for wxFormBuilder
     */
    COLOR_SWATCH( wxWidgetArgCompat aParent );

    COLOR_SWATCH( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos = wxDefaultPosition,
                  const wxSize& aSize = wxDefaultSize, long aStyle = 0 ) :
            COLOR_SWATCH( aParent )
    {
        SetWindowID( aId );
    }

    /**
     * Set the current swatch color directly.
     */
    void SetSwatchColor( const KIGFX::COLOR4D& aColor, bool aSendEvent );

    /**
     * Sets the color that will be chosen with the "Reset to Default" button in the chooser
     */
    void SetDefaultColor( const KIGFX::COLOR4D& aColor );

    /**
     * Set the swatch background color.
     */
    void SetSwatchBackground( const KIGFX::COLOR4D& aBackground );

    /**
     * @return the current swatch color
     */
    KIGFX::COLOR4D GetSwatchColor() const;

    /// The id given at the creation (the layer of the appearance panels).
    int GetId() const { return m_id; }

    void SetWindowID( int aId ) { m_id = aId; }

    /**
     * Prompt for a new colour, using the colour picker dialog.
     *
     * A colour change event will be sent if it's set.
     */
    void GetNewSwatchColor();

    void SetReadOnly( bool aReadOnly = true ) { m_readOnly = aReadOnly; }
    bool IsReadOnly() const { return m_readOnly; }

    void SetSupportsOpacity( bool aSupportsOpacity ) { m_supportsOpacity = aSupportsOpacity; }

    /// Register a handler for when the user tries to interact with a read-only swatch.
    void SetReadOnlyCallback( std::function<void()> aCallback ) { m_readOnlyCallback = aCallback; }

    /// Register a handler for when the color is changed (by the user or SetSwatchColor()).
    void SetChangedCallback( std::function<void()> aCallback ) { m_changedCallback = aCallback; }

    /// Respond to a change in the OS's DarkMode setting.
    void OnDarkModeToggle();

    /**
     * The image of a swatch: the color over a checkerboard (the transparency).
     *
     * @param aSize and @a aCheckerboardSize are in logical pixels; the pixmap is drawn for the
     *        device pixel ratio @a aScale.
     */
    static QPixmap MakeBitmap( const KIGFX::COLOR4D& aColor, const KIGFX::COLOR4D& aBackground,
                               const wxSize& aSize, const wxSize& aCheckerboardSize,
                               const KIGFX::COLOR4D& aCheckerboardBackground, double aScale,
                               const std::vector<int>& aMargins = { 0, 0, 0, 0 } );

    /// The image of a swatch at the scale 1 (the API of the wxWidgets build).
    static wxBitmap MakeBitmap( const KIGFX::COLOR4D& aColor, const KIGFX::COLOR4D& aBackground,
                                const wxSize& aSize, const wxSize& aCheckerboardSize,
                                const KIGFX::COLOR4D& aCheckerboardBackground,
                                const std::vector<int>& aMargins = { 0, 0, 0, 0 } )
    {
        return wxBitmap( MakeBitmap( aColor, aBackground, aSize, aCheckerboardSize,
                                     aCheckerboardBackground, 1.0, aMargins ) );
    }

    /// The size in pixels of a size in dialog units, for the font of @a aWidget.
    static wxSize DialogUnitsToPixels( const QWidget* aWidget, const wxSize& aSize );

protected:
    void mousePressEvent( QMouseEvent* aEvent ) override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;
    void mouseDoubleClickEvent( QMouseEvent* aEvent ) override;

private:
    void init( bool aTriggerWithSingleClick );

    void showBitmap();

    /// Call the changed callback, from the event loop.
    void sendChanged();

    int                   m_id;
    KIGFX::COLOR4D        m_color;
    KIGFX::COLOR4D        m_background;
    KIGFX::COLOR4D        m_default;
    CUSTOM_COLORS_LIST*   m_userColors;

    wxSize                m_size;
    wxSize                m_checkerboardSize;
    KIGFX::COLOR4D        m_checkerboardBg;

    /// The color picker opens on a single click (in a dialog), or a double click
    bool                  m_singleClick;

    /// A read-only swatch won't show the color chooser dialog but otherwise works normally
    bool                  m_readOnly;
    std::function<void()> m_readOnlyCallback;
    std::function<void()> m_changedCallback;

    /// If opacity is not supported the color chooser dialog will be displayed without it
    bool                  m_supportsOpacity;
};


#endif
