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
 * @file kiwx_gdi.h
 * The graphics of the wxWidgets compatibility layer of the Qt build (wxcompat): the colours,
 * pens, brushes, fonts, bitmaps and the device context (wxDC, on a QPainter) that the code of
 * the dialogs and of the cells of the grids uses.
 *
 * wxFont, wxBitmap and wxBitmapBundle are QFont, QPixmap and QIcon with the API of wxWidgets:
 * they convert to and from the classes of Qt.
 */

#ifndef KIWX_GDI_H
#define KIWX_GDI_H

#include <kiqt.h>

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QPixmap>

#include <wx/gdicmn.h>
#include <wx/string.h>

class QPainter;
class QWidget;


/// The colours of the system (wxSystemSettings::GetColour()).
enum wxSystemColour
{
    wxSYS_COLOUR_SCROLLBAR,
    wxSYS_COLOUR_DESKTOP,
    wxSYS_COLOUR_ACTIVECAPTION,
    wxSYS_COLOUR_INACTIVECAPTION,
    wxSYS_COLOUR_MENU,
    wxSYS_COLOUR_WINDOW,
    wxSYS_COLOUR_WINDOWFRAME,
    wxSYS_COLOUR_MENUTEXT,
    wxSYS_COLOUR_WINDOWTEXT,
    wxSYS_COLOUR_CAPTIONTEXT,
    wxSYS_COLOUR_ACTIVEBORDER,
    wxSYS_COLOUR_INACTIVEBORDER,
    wxSYS_COLOUR_APPWORKSPACE,
    wxSYS_COLOUR_HIGHLIGHT,
    wxSYS_COLOUR_HIGHLIGHTTEXT,
    wxSYS_COLOUR_BTNFACE,
    wxSYS_COLOUR_BTNSHADOW,
    wxSYS_COLOUR_GRAYTEXT,
    wxSYS_COLOUR_BTNTEXT,
    wxSYS_COLOUR_INACTIVECAPTIONTEXT,
    wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DDKSHADOW,
    wxSYS_COLOUR_3DLIGHT,
    wxSYS_COLOUR_INFOTEXT,
    wxSYS_COLOUR_INFOBK,
    wxSYS_COLOUR_LISTBOX,
    wxSYS_COLOUR_HOTLIGHT,
    wxSYS_COLOUR_GRADIENTACTIVECAPTION,
    wxSYS_COLOUR_GRADIENTINACTIVECAPTION,
    wxSYS_COLOUR_MENUHILIGHT,
    wxSYS_COLOUR_MENUBAR,
    wxSYS_COLOUR_LISTBOXTEXT,
    wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT,
    wxSYS_COLOUR_FRAMEBK,

    wxSYS_COLOUR_BACKGROUND = wxSYS_COLOUR_DESKTOP,
    wxSYS_COLOUR_3DFACE = wxSYS_COLOUR_BTNFACE,
    wxSYS_COLOUR_3DSHADOW = wxSYS_COLOUR_BTNSHADOW,
    wxSYS_COLOUR_BTNHILIGHT = wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DHIGHLIGHT = wxSYS_COLOUR_BTNHIGHLIGHT,
    wxSYS_COLOUR_3DHILIGHT = wxSYS_COLOUR_BTNHIGHLIGHT
};


enum wxPenStyle
{
    wxPENSTYLE_INVALID = -1,
    wxPENSTYLE_SOLID = 100,
    wxPENSTYLE_DOT,
    wxPENSTYLE_LONG_DASH,
    wxPENSTYLE_SHORT_DASH,
    wxPENSTYLE_DOT_DASH,
    wxPENSTYLE_USER_DASH,
    wxPENSTYLE_TRANSPARENT = 106
};

enum wxBrushStyle
{
    wxBRUSHSTYLE_INVALID = -1,
    wxBRUSHSTYLE_SOLID = wxPENSTYLE_SOLID,
    wxBRUSHSTYLE_TRANSPARENT = wxPENSTYLE_TRANSPARENT
};

enum wxFontFamily
{
    wxFONTFAMILY_DEFAULT = 70,
    wxFONTFAMILY_DECORATIVE,
    wxFONTFAMILY_ROMAN,
    wxFONTFAMILY_SCRIPT,
    wxFONTFAMILY_SWISS,
    wxFONTFAMILY_MODERN,
    wxFONTFAMILY_TELETYPE
};

enum wxFontStyle
{
    wxFONTSTYLE_NORMAL = 90,
    wxFONTSTYLE_ITALIC = 93,
    wxFONTSTYLE_SLANT = 94
};

enum wxFontWeight
{
    wxFONTWEIGHT_NORMAL = 90,
    wxFONTWEIGHT_LIGHT = 91,
    wxFONTWEIGHT_BOLD = 92
};

/// The states of the controls drawn by wxRendererNative.
enum
{
    wxCONTROL_NONE = 0x00000000,
    wxCONTROL_DISABLED = 0x00000001,
    wxCONTROL_FOCUSED = 0x00000002,
    wxCONTROL_PRESSED = 0x00000004,
    wxCONTROL_SPECIAL = 0x00000008,
    wxCONTROL_ISDEFAULT = wxCONTROL_SPECIAL,
    wxCONTROL_ISSUBMENU = wxCONTROL_SPECIAL,
    wxCONTROL_EXPANDED = wxCONTROL_SPECIAL,
    wxCONTROL_SIZEGRIP = wxCONTROL_SPECIAL,
    wxCONTROL_FLAT = wxCONTROL_SPECIAL,
    wxCONTROL_CELL = wxCONTROL_SPECIAL,
    wxCONTROL_CURRENT = 0x00000010,
    wxCONTROL_SELECTED = 0x00000020,
    wxCONTROL_CHECKED = 0x00000040,
    wxCONTROL_CHECKABLE = 0x00000080,
    wxCONTROL_UNDETERMINED = wxCONTROL_CHECKABLE
};


class KIQT_API wxColour
{
public:
    wxColour() :
            m_color(),
            m_ok( false )
    {
    }

    wxColour( unsigned char aRed, unsigned char aGreen, unsigned char aBlue,
              unsigned char aAlpha = 255 ) :
            m_color( aRed, aGreen, aBlue, aAlpha ),
            m_ok( true )
    {
    }

    /// A colour from its name or its HTML code ("#RRGGBB").
    wxColour( const wxString& aName );
    wxColour( const char* aName ) : wxColour( wxString( aName ) ) {}

    wxColour( const QColor& aColor ) :
            m_color( aColor ),
            m_ok( aColor.isValid() )
    {
    }

    operator QColor() const { return m_color; }

    bool IsOk() const { return m_ok; }

    unsigned char Red() const { return m_color.red(); }
    unsigned char Green() const { return m_color.green(); }
    unsigned char Blue() const { return m_color.blue(); }
    unsigned char Alpha() const { return m_color.alpha(); }

    void Set( unsigned char aRed, unsigned char aGreen, unsigned char aBlue,
              unsigned char aAlpha = 255 )
    {
        m_color.setRgb( aRed, aGreen, aBlue, aAlpha );
        m_ok = true;
    }

    bool Set( const wxString& aName );

    /// The colour as "rgb(r, g, b)" or "#RRGGBB" (wxC2S_CSS_SYNTAX, wxC2S_HTML_SYNTAX).
    wxString GetAsString( long aFlags = 0 ) const;

    /// A lighter (> 100) or darker (< 100) colour.
    wxColour ChangeLightness( int aAlpha ) const;

    bool operator==( const wxColour& aOther ) const
    {
        return m_ok == aOther.m_ok && ( !m_ok || m_color == aOther.m_color );
    }

    bool operator!=( const wxColour& aOther ) const { return !( *this == aOther ); }

private:
    QColor m_color;
    bool   m_ok;
};

using wxColor = wxColour;

#define wxC2S_NAME          1
#define wxC2S_CSS_SYNTAX    2
#define wxC2S_HTML_SYNTAX   4

KIQT_API extern const wxColour wxNullColour;
KIQT_API extern const wxColour* wxBLACK;
KIQT_API extern const wxColour* wxWHITE;
KIQT_API extern const wxColour* wxRED;
KIQT_API extern const wxColour* wxBLUE;
KIQT_API extern const wxColour* wxGREEN;
KIQT_API extern const wxColour* wxLIGHT_GREY;


class KIQT_API wxPen
{
public:
    wxPen() :
            m_colour(),
            m_width( 1 ),
            m_style( wxPENSTYLE_INVALID )
    {
    }

    wxPen( const wxColour& aColour, int aWidth = 1, wxPenStyle aStyle = wxPENSTYLE_SOLID ) :
            m_colour( aColour ),
            m_width( aWidth ),
            m_style( aStyle )
    {
    }

    bool IsOk() const { return m_style != wxPENSTYLE_INVALID; }

    const wxColour& GetColour() const { return m_colour; }
    void SetColour( const wxColour& aColour ) { m_colour = aColour; }
    int GetWidth() const { return m_width; }
    void SetWidth( int aWidth ) { m_width = aWidth; }
    wxPenStyle GetStyle() const { return m_style; }
    void SetStyle( wxPenStyle aStyle ) { m_style = aStyle; }

private:
    wxColour   m_colour;
    int        m_width;
    wxPenStyle m_style;
};


class KIQT_API wxBrush
{
public:
    wxBrush() :
            m_colour(),
            m_style( wxBRUSHSTYLE_INVALID )
    {
    }

    wxBrush( const wxColour& aColour, wxBrushStyle aStyle = wxBRUSHSTYLE_SOLID ) :
            m_colour( aColour ),
            m_style( aStyle )
    {
    }

    bool IsOk() const { return m_style != wxBRUSHSTYLE_INVALID; }

    const wxColour& GetColour() const { return m_colour; }
    void SetColour( const wxColour& aColour ) { m_colour = aColour; }
    wxBrushStyle GetStyle() const { return m_style; }
    void SetStyle( wxBrushStyle aStyle ) { m_style = aStyle; }
    bool IsTransparent() const { return m_style == wxBRUSHSTYLE_TRANSPARENT; }

private:
    wxColour     m_colour;
    wxBrushStyle m_style;
};

KIQT_API extern const wxPen* wxTRANSPARENT_PEN;
KIQT_API extern const wxPen* wxBLACK_PEN;
KIQT_API extern const wxPen* wxWHITE_PEN;
KIQT_API extern const wxBrush* wxTRANSPARENT_BRUSH;
KIQT_API extern const wxBrush* wxBLACK_BRUSH;
KIQT_API extern const wxBrush* wxWHITE_BRUSH;
KIQT_API extern const wxPen wxNullPen;
KIQT_API extern const wxBrush wxNullBrush;


/// A font: a QFont with the API of wxWidgets.
class KIQT_API wxFont : public QFont
{
public:
    wxFont() = default;
    wxFont( const QFont& aFont ) : QFont( aFont ) {}

    wxFont( int aPointSize, wxFontFamily aFamily, wxFontStyle aStyle, wxFontWeight aWeight,
            bool aUnderline = false, const wxString& aFaceName = wxEmptyString );

    bool IsOk() const { return true; }

    int GetPointSize() const { return pointSize(); }
    void SetPointSize( int aSize ) { setPointSize( aSize ); }
    double GetFractionalPointSize() const { return pointSizeF(); }
    void SetFractionalPointSize( double aSize ) { setPointSizeF( aSize ); }

    wxFontWeight GetWeight() const { return bold() ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL; }
    void SetWeight( wxFontWeight aWeight ) { setBold( aWeight == wxFONTWEIGHT_BOLD ); }
    wxFontStyle GetStyle() const { return italic() ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL; }
    void SetStyle( wxFontStyle aStyle ) { setItalic( aStyle != wxFONTSTYLE_NORMAL ); }
    bool GetUnderlined() const { return underline(); }
    void SetUnderlined( bool aUnderlined ) { setUnderline( aUnderlined ); }

    wxString GetFaceName() const { return ToWxString( family() ); }
    bool SetFaceName( const wxString& aFaceName );

    wxFont Bold() const;
    wxFont Italic() const;
    wxFont Smaller() const;
    wxFont Larger() const;
    wxFont Scaled( float aScale ) const;
    wxFont& MakeBold() { setBold( true ); return *this; }
    wxFont& MakeItalic() { setItalic( true ); return *this; }
    wxFont& MakeSmaller();
    wxFont& MakeLarger();

private:
    static wxString ToWxString( const QString& aText );
};

KIQT_API extern const wxFont wxNullFont;


#define wxIMAGE_ALPHA_TRANSPARENT 0
#define wxIMAGE_ALPHA_THRESHOLD 0x80
#define wxIMAGE_ALPHA_OPAQUE 0xff

enum wxImageResizeQuality
{
    wxIMAGE_QUALITY_NEAREST = 0,
    wxIMAGE_QUALITY_BILINEAR,
    wxIMAGE_QUALITY_BICUBIC,
    wxIMAGE_QUALITY_BOX_AVERAGE,
    wxIMAGE_QUALITY_NORMAL = wxIMAGE_QUALITY_NEAREST,
    wxIMAGE_QUALITY_HIGH = 4
};


/// An image (wxImage): a QImage with the API of wxWidgets, RGB with an optional alpha.
class KIQT_API wxImage : public QImage
{
public:
    wxImage() = default;
    wxImage( const QImage& aImage ) : QImage( aImage ) {}
    wxImage( int aWidth, int aHeight, bool aClear = true );
    wxImage( const wxSize& aSize, bool aClear = true ) : wxImage( aSize.x, aSize.y, aClear ) {}

    bool IsOk() const { return !isNull(); }
    int GetWidth() const { return width(); }
    int GetHeight() const { return height(); }
    wxSize GetSize() const { return wxSize( width(), height() ); }

    bool HasAlpha() const { return hasAlphaChannel(); }

    /// Add an alpha channel (opaque).
    void InitAlpha();

    void SetRGB( int aX, int aY, unsigned char aRed, unsigned char aGreen, unsigned char aBlue );
    void SetAlpha( int aX, int aY, unsigned char aAlpha );
    unsigned char GetRed( int aX, int aY ) const { return qRed( pixel( aX, aY ) ); }
    unsigned char GetGreen( int aX, int aY ) const { return qGreen( pixel( aX, aY ) ); }
    unsigned char GetBlue( int aX, int aY ) const { return qBlue( pixel( aX, aY ) ); }
    unsigned char GetAlpha( int aX, int aY ) const { return qAlpha( pixel( aX, aY ) ); }

    wxImage Rotate90( bool aClockwise = true ) const;
    wxImage Rotate180() const;
    wxImage Mirror( bool aHorizontally = true ) const;
    wxImage Scale( int aWidth, int aHeight,
                   wxImageResizeQuality aQuality = wxIMAGE_QUALITY_NORMAL ) const;
    wxImage& Rescale( int aWidth, int aHeight,
                      wxImageResizeQuality aQuality = wxIMAGE_QUALITY_NORMAL );
    wxImage Copy() const { return wxImage( copy() ); }
    wxImage GetSubImage( const wxRect& aRect ) const;
    wxImage ConvertToGreyscale() const;
    wxImage ConvertToDisabled( unsigned char aBrightness = 255 ) const;
    void Clear( unsigned char aValue = 0 );

    bool LoadFile( const wxString& aName, int aType = -1 );
    bool SaveFile( const wxString& aName, int aType = -1 ) const;
};


/// A bitmap: a QPixmap with the API of wxWidgets.
class KIQT_API wxBitmap : public QPixmap
{
public:
    wxBitmap() = default;
    wxBitmap( const QPixmap& aPixmap ) : QPixmap( aPixmap ) {}
    wxBitmap( const wxImage& aImage, int aDepth = -1, double aScale = 1.0 );
    wxBitmap( int aWidth, int aHeight, int aDepth = -1 );
    wxBitmap( const wxSize& aSize, int aDepth = -1 ) : wxBitmap( aSize.x, aSize.y, aDepth ) {}

    bool IsOk() const { return !isNull(); }

    int GetWidth() const { return width(); }
    int GetHeight() const { return height(); }
    wxSize GetSize() const { return wxSize( width(), height() ); }

    /// The size in logical pixels (the size of the bitmap divided by its scale).
    double GetLogicalWidth() const { return width() / devicePixelRatio(); }
    double GetLogicalHeight() const { return height() / devicePixelRatio(); }
    wxSize GetLogicalSize() const;
    wxSize GetDIPSize() const { return GetLogicalSize(); }

    double GetScaleFactor() const { return devicePixelRatio(); }
    void SetScaleFactor( double aScale ) { setDevicePixelRatio( aScale ); }

    wxImage ConvertToImage() const { return wxImage( toImage() ); }
    bool HasAlpha() const { return hasAlphaChannel(); }
};

/// The null bitmap (wxNullBitmap), made on its first use: a pixmap needs the application of Qt.
KIQT_API const wxBitmap& wxNullBitmapCompat();
#define wxNullBitmap ( wxNullBitmapCompat() )


/// The images of an icon at the scales of the screens: a QIcon with the API of wxWidgets.
class KIQT_API wxBitmapBundle : public QIcon
{
public:
    wxBitmapBundle() = default;
    wxBitmapBundle( const QIcon& aIcon ) : QIcon( aIcon ) {}
    wxBitmapBundle( const QPixmap& aBitmap ) : QIcon( aBitmap ) {}

    bool IsOk() const { return !isNull(); }

    /// The size of the icon at the scale 1.
    wxSize GetDefaultSize() const;

    /// The size of the image best for a scale.
    wxSize GetPreferredBitmapSizeAtScale( double aScale ) const;
    wxSize GetPreferredBitmapSizeFor( const QWidget* aWindow ) const;

    /// The size of the image best for a window, in logical pixels.
    wxSize GetPreferredLogicalSizeFor( const QWidget* aWindow ) const { return GetDefaultSize(); }

    /// The image of the icon at a size (in physical pixels).
    wxBitmap GetBitmap( const wxSize& aSize ) const;
    wxBitmap GetBitmapFor( const QWidget* aWindow ) const;

    static wxBitmapBundle FromBitmap( const wxBitmap& aBitmap ) { return wxBitmapBundle( aBitmap ); }
};


using wxIcon = wxBitmap;

using wxArtID = wxString;
using wxArtClient = wxString;

#define wxART_TOOLBAR      wxString( "wxART_TOOLBAR_C" )
#define wxART_MENU         wxString( "wxART_MENU_C" )
#define wxART_FRAME_ICON   wxString( "wxART_FRAME_ICON_C" )
#define wxART_CMN_DIALOG   wxString( "wxART_CMN_DIALOG_C" )
#define wxART_HELP_BROWSER wxString( "wxART_HELP_BROWSER_C" )
#define wxART_MESSAGE_BOX  wxString( "wxART_MESSAGE_BOX_C" )
#define wxART_BUTTON       wxString( "wxART_BUTTON_C" )
#define wxART_LIST         wxString( "wxART_LIST_C" )
#define wxART_OTHER        wxString( "wxART_OTHER_C" )

#define wxART_ERROR        wxString( "wxART_ERROR" )
#define wxART_QUESTION     wxString( "wxART_QUESTION" )
#define wxART_WARNING      wxString( "wxART_WARNING" )
#define wxART_INFORMATION  wxString( "wxART_INFORMATION" )
#define wxART_HELP         wxString( "wxART_HELP" )
#define wxART_TIP          wxString( "wxART_TIP" )
#define wxART_GO_BACK      wxString( "wxART_GO_BACK" )
#define wxART_GO_FORWARD   wxString( "wxART_GO_FORWARD" )
#define wxART_GO_UP        wxString( "wxART_GO_UP" )
#define wxART_GO_DOWN      wxString( "wxART_GO_DOWN" )
#define wxART_GO_HOME      wxString( "wxART_GO_HOME" )
#define wxART_FILE_OPEN    wxString( "wxART_FILE_OPEN" )
#define wxART_FILE_SAVE    wxString( "wxART_FILE_SAVE" )
#define wxART_FOLDER       wxString( "wxART_FOLDER" )
#define wxART_FOLDER_OPEN  wxString( "wxART_FOLDER_OPEN" )
#define wxART_NORMAL_FILE  wxString( "wxART_NORMAL_FILE" )
#define wxART_NEW          wxString( "wxART_NEW" )
#define wxART_DELETE       wxString( "wxART_DELETE" )
#define wxART_CLOSE        wxString( "wxART_CLOSE" )
#define wxART_CROSS_MARK   wxString( "wxART_CROSS_MARK" )
#define wxART_TICK_MARK    wxString( "wxART_TICK_MARK" )
#define wxART_FIND         wxString( "wxART_FIND" )
#define wxART_COPY         wxString( "wxART_COPY" )
#define wxART_PASTE        wxString( "wxART_PASTE" )
#define wxART_UNDO         wxString( "wxART_UNDO" )
#define wxART_REDO         wxString( "wxART_REDO" )
#define wxART_PLUS         wxString( "wxART_PLUS" )
#define wxART_MINUS        wxString( "wxART_MINUS" )


/// The standard icons of the system (wxArtProvider): the ones of the style of Qt.
class KIQT_API wxArtProvider
{
public:
    static wxBitmap GetBitmap( const wxArtID& aId, const wxArtClient& aClient = wxART_OTHER,
                               const wxSize& aSize = wxDefaultSize );
    static wxBitmapBundle GetBitmapBundle( const wxArtID& aId,
                                           const wxArtClient& aClient = wxART_OTHER,
                                           const wxSize& aSize = wxDefaultSize );
    static wxIcon GetIcon( const wxArtID& aId, const wxArtClient& aClient = wxART_OTHER,
                           const wxSize& aSize = wxDefaultSize );

    /// The icon of a message box style (wxICON_ERROR...).
    static wxArtID GetMessageBoxIconId( int aFlags );

    /// The size of the icons of a client (in logical pixels).
    static wxSize GetSizeHint( const wxArtClient& aClient, QWidget* aWindow = nullptr );
    static wxSize GetNativeSizeHint( const wxArtClient& aClient, QWidget* aWindow = nullptr )
    {
        return GetSizeHint( aClient, aWindow );
    }
};


/// The colours of the system.
/// The metrics of the system (wxSystemMetric of wxWidgets).
enum wxSystemMetric
{
    wxSYS_BORDER_X = 1,
    wxSYS_BORDER_Y,
    wxSYS_EDGE_X,
    wxSYS_EDGE_Y,
    wxSYS_ICON_X,
    wxSYS_ICON_Y,
    wxSYS_SMALLICON_X,
    wxSYS_SMALLICON_Y,
    wxSYS_HSCROLL_Y,
    wxSYS_VSCROLL_X,
    wxSYS_HSCROLL_ARROW_X,
    wxSYS_VSCROLL_ARROW_Y,
    wxSYS_SCREEN_X,
    wxSYS_SCREEN_Y,
    wxSYS_DCLICK_X,
    wxSYS_DCLICK_Y,
    wxSYS_DRAG_X,
    wxSYS_DRAG_Y,
    wxSYS_CARET_ON_MSEC,
    wxSYS_CARET_OFF_MSEC,
    wxSYS_DCLICK_MSEC
};


class KIQT_API wxSystemSettings
{
public:
    static wxColour GetColour( wxSystemColour aIndex );

    /// A metric of the style of Qt (in logical pixels), -1 if unknown.
    static int GetMetric( wxSystemMetric aIndex, const QWidget* aWindow = nullptr );
};


/**
 * The device context of the drawings of the cells of the grids and of the custom controls
 * (wxDC): a QPainter.
 */
class KIQT_API wxDC
{
public:
    explicit wxDC( QPainter* aPainter, QWidget* aWindow = nullptr );
    virtual ~wxDC() = default;

    QPainter* GetPainter() const { return m_painter; }
    QWidget* GetWindow() const { return m_window; }

    void SetPen( const wxPen& aPen );
    void SetBrush( const wxBrush& aBrush );
    const wxPen& GetPen() const { return m_pen; }
    const wxBrush& GetBrush() const { return m_brush; }
    void SetFont( const wxFont& aFont );
    const wxFont& GetFont() const { return m_font; }
    void SetTextForeground( const wxColour& aColour ) { m_textForeground = aColour; }
    void SetTextBackground( const wxColour& aColour ) { m_textBackground = aColour; }
    const wxColour& GetTextForeground() const { return m_textForeground; }
    void SetBackgroundMode( int aMode ) {}
    void SetBackground( const wxBrush& aBrush ) { m_background = aBrush; }

    void DrawRectangle( const wxRect& aRect );
    void DrawRectangle( int aX, int aY, int aWidth, int aHeight );
    void DrawRoundedRectangle( const wxRect& aRect, double aRadius );
    void DrawLine( int aX1, int aY1, int aX2, int aY2 );
    void DrawLine( const wxPoint& aP1, const wxPoint& aP2 ) { DrawLine( aP1.x, aP1.y, aP2.x, aP2.y ); }
    void DrawCircle( int aX, int aY, int aRadius );
    void DrawText( const wxString& aText, int aX, int aY );
    void DrawText( const wxString& aText, const wxPoint& aPos ) { DrawText( aText, aPos.x, aPos.y ); }

    /// Draw a text in a rectangle with an alignment (wxALIGN_...).
    void DrawLabel( const wxString& aText, const wxRect& aRect, int aAlignment = 0 );

    void DrawBitmap( const wxBitmap& aBitmap, int aX, int aY, bool aUseMask = false );
    void DrawBitmap( const wxBitmap& aBitmap, const wxPoint& aPos, bool aUseMask = false )
    {
        DrawBitmap( aBitmap, aPos.x, aPos.y, aUseMask );
    }

    void Clear();

    void GetTextExtent( const wxString& aText, int* aWidth, int* aHeight ) const;
    wxSize GetTextExtent( const wxString& aText ) const;
    wxSize GetMultiLineTextExtent( const wxString& aText ) const;

    void SetClippingRegion( const wxRect& aRect );
    void SetClippingRegion( int aX, int aY, int aWidth, int aHeight );
    void DestroyClippingRegion();

    wxSize GetSize() const;

    /// Sizes in the logical pixels of Qt: already independent of the DPI.
    int FromDIP( int aValue ) const { return aValue; }
    wxSize FromDIP( const wxSize& aSize ) const { return aSize; }
    int ToDIP( int aValue ) const { return aValue; }
    double GetContentScaleFactor() const;

private:
    QPainter* m_painter;
    QWidget*  m_window;
    wxPen     m_pen;
    wxBrush   m_brush;
    wxFont    m_font;
    wxColour  m_textForeground;
    wxColour  m_textBackground;
    wxBrush   m_background;
};

using wxGCDC = wxDC;
using wxPaintDC = wxDC;
using wxClientDC = wxDC;


/// The pen of a DC while the changer lives (wxDCPenChanger).
class wxDCPenChanger
{
public:
    wxDCPenChanger( wxDC& aDC, const wxPen& aPen ) : m_dc( aDC ), m_old( aDC.GetPen() )
    {
        aDC.SetPen( aPen );
    }

    ~wxDCPenChanger() { m_dc.SetPen( m_old ); }

private:
    wxDC& m_dc;
    wxPen m_old;
};


/// The brush of a DC while the changer lives (wxDCBrushChanger).
class wxDCBrushChanger
{
public:
    wxDCBrushChanger( wxDC& aDC, const wxBrush& aBrush ) : m_dc( aDC ), m_old( aDC.GetBrush() )
    {
        aDC.SetBrush( aBrush );
    }

    ~wxDCBrushChanger() { m_dc.SetBrush( m_old ); }

private:
    wxDC&   m_dc;
    wxBrush m_old;
};


/**
 * The drawing of the controls in the style of the system (wxRendererNative): the check boxes
 * and buttons of the cells of the grids.
 */
class KIQT_API wxRendererNative
{
public:
    static wxRendererNative& Get();
    static wxRendererNative& GetDefault() { return Get(); }

    void DrawCheckBox( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags = 0 );
    void DrawPushButton( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags = 0 );
    void DrawComboBoxDropButton( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags = 0 );
    void DrawFocusRect( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags = 0 );
    wxSize GetCheckBoxSize( QWidget* aWindow, int aFlags = 0 );
};

#endif // KIWX_GDI_H
