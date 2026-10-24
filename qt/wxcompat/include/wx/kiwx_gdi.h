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

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QTransform>

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
    /// A component blended over another one with an opacity.
    static unsigned char AlphaBlend( unsigned char aForeground, unsigned char aBackground,
                                     double aAlpha )
    {
        double result = aForeground * aAlpha + aBackground * ( 1.0 - aAlpha );
        return static_cast<unsigned char>( std::clamp( result + 0.5, 0.0, 255.0 ) );
    }

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
KIQT_API extern const wxColour* wxYELLOW;
KIQT_API extern const wxColour* wxCYAN;
KIQT_API extern const wxColour* wxGREY;


/// A length of the dashes of a pen (wxPen::SetDashes()).
typedef signed char wxDash;


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

    /// The dashes of a wxPENSTYLE_USER_DASH pen: the lengths of the dashes and of the spaces,
    /// in widths of the pen.
    void SetDashes( int aCount, const wxDash* aDashes )
    {
        m_dashes.assign( aDashes, aDashes + std::max( aCount, 0 ) );
    }

    int GetDashCount() const { return int( m_dashes.size() ); }
    const std::vector<wxDash>& GetDashesCompat() const { return m_dashes; }

private:
    wxColour            m_colour;
    int                 m_width;
    wxPenStyle          m_style;
    std::vector<wxDash> m_dashes;
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
/// The description of a font (wxFontInfo), given to the constructor of wxFont.
class wxFontInfo
{
public:
    wxFontInfo() = default;
    explicit wxFontInfo( double aPointSize ) : m_pointSize( aPointSize ) {}

    wxFontInfo& Family( wxFontFamily aFamily ) { m_family = aFamily; return *this; }
    wxFontInfo& FaceName( const wxString& aFaceName ) { m_faceName = aFaceName; return *this; }
    wxFontInfo& Bold( bool aBold = true ) { m_bold = aBold; return *this; }
    wxFontInfo& Italic( bool aItalic = true ) { m_italic = aItalic; return *this; }
    wxFontInfo& Underlined( bool aUnderlined = true ) { m_underlined = aUnderlined; return *this; }

    double          GetPointSize() const { return m_pointSize; }
    wxFontFamily    GetFamily() const { return m_family; }
    const wxString& GetFaceName() const { return m_faceName; }
    bool            IsBold() const { return m_bold; }
    bool            IsItalic() const { return m_italic; }
    bool            IsUnderlined() const { return m_underlined; }

private:
    double       m_pointSize = -1;
    wxFontFamily m_family = wxFONTFAMILY_DEFAULT;
    wxString     m_faceName;
    bool         m_bold = false;
    bool         m_italic = false;
    bool         m_underlined = false;
};


class KIQT_API wxFont : public QFont
{
public:
    wxFont() = default;
    wxFont( const QFont& aFont ) : QFont( aFont ) {}
    wxFont( const wxFontInfo& aInfo );

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

/// How wxImage::Paste() pastes the alpha channel of an image.
enum wxImageAlphaBlendMode
{
    wxIMAGE_ALPHA_BLEND_OVER = 0,   ///< Replace the pixels
    wxIMAGE_ALPHA_BLEND_COMPOSE = 1 ///< Compose the pixels on the image
};

enum wxImageResizeQuality
{
    wxIMAGE_QUALITY_NEAREST = 0,
    wxIMAGE_QUALITY_BILINEAR,
    wxIMAGE_QUALITY_BICUBIC,
    wxIMAGE_QUALITY_BOX_AVERAGE,
    wxIMAGE_QUALITY_NORMAL = wxIMAGE_QUALITY_NEAREST,
    wxIMAGE_QUALITY_HIGH = 4
};


/// The formats of the image files (wxBitmapType): Qt finds the format from the file.
enum wxBitmapType
{
    wxBITMAP_TYPE_INVALID = 0,
    wxBITMAP_TYPE_BMP = 1,
    wxBITMAP_TYPE_ICO = 3,
    wxBITMAP_TYPE_CUR = 5,
    wxBITMAP_TYPE_XBM = 7,
    wxBITMAP_TYPE_XPM = 9,
    wxBITMAP_TYPE_TIFF = 11,
    wxBITMAP_TYPE_TIF = wxBITMAP_TYPE_TIFF,
    wxBITMAP_TYPE_GIF = 13,
    wxBITMAP_TYPE_PNG = 15,
    wxBITMAP_TYPE_JPEG = 17,
    wxBITMAP_TYPE_PNM = 19,
    wxBITMAP_TYPE_PCX = 21,
    wxBITMAP_TYPE_ICON = 25,
    wxBITMAP_TYPE_TGA = 29,
    wxBITMAP_TYPE_ANY = 50
};


class wxInputStream;
class wxOutputStream;

/// The options of the image files (kept by the image, the encoders of Qt choose).
#define wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL        wxString( wxS( "PngZL" ) )
#define wxIMAGE_OPTION_PNG_COMPRESSION_MEM_LEVEL    wxString( wxS( "PngZM" ) )
#define wxIMAGE_OPTION_PNG_COMPRESSION_STRATEGY     wxString( wxS( "PngZS" ) )
#define wxIMAGE_OPTION_PNG_COMPRESSION_BUFFER_SIZE  wxString( wxS( "PngZB" ) )
#define wxIMAGE_OPTION_PNG_FILTER                   wxString( wxS( "PngF" ) )
#define wxIMAGE_OPTION_QUALITY                      wxString( wxS( "quality" ) )
#define wxIMAGE_OPTION_RESOLUTION                   wxString( wxS( "Resolution" ) )


/// An image (wxImage): a QImage with the API of wxWidgets, RGB with an optional alpha.
class KIQT_API wxImage : public QImage
{
public:
    wxImage() = default;
    wxImage( const QImage& aImage ) : QImage( aImage ) {}
    wxImage( int aWidth, int aHeight, bool aClear = true );
    wxImage( const wxSize& aSize, bool aClear = true ) : wxImage( aSize.x, aSize.y, aClear ) {}

    explicit wxImage( const wxString& aName, wxBitmapType aType = wxBITMAP_TYPE_ANY,
                      int aIndex = -1 )
    {
        LoadFile( aName, aType );
    }

    wxImage( wxInputStream& aStream, wxBitmapType aType = wxBITMAP_TYPE_ANY, int aIndex = -1 )
    {
        LoadFile( aStream, aType );
    }

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

    /// Set the colour of the pixels of a rectangle.
    void SetRGB( const wxRect& aRect, unsigned char aRed, unsigned char aGreen,
                 unsigned char aBlue );

    /// Paste an image at a position.
    void Paste( const wxImage& aImage, int aX, int aY,
                wxImageAlphaBlendMode aAlphaBlend = wxIMAGE_ALPHA_BLEND_OVER );
    wxImage ConvertToDisabled( unsigned char aBrightness = 255 ) const;
    void Clear( unsigned char aValue = 0 );

    bool LoadFile( const wxString& aName, int aType = -1 );
    bool SaveFile( const wxString& aName, int aType = -1 ) const;

    /// The image files in streams (the format of @a aType, PNG if it's not known).
    bool LoadFile( wxInputStream& aStream, wxBitmapType aType = wxBITMAP_TYPE_ANY );
    bool SaveFile( wxOutputStream& aStream, wxBitmapType aType ) const;

    void SetOption( const wxString& aName, const wxString& aValue ) { m_options[aName] = aValue; }
    void SetOption( const wxString& aName, int aValue );
    wxString GetOption( const wxString& aName ) const;
    int GetOptionInt( const wxString& aName ) const;
    bool HasOption( const wxString& aName ) const { return m_options.count( aName ) > 0; }

private:
    std::map<wxString, wxString> m_options;
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

    /// The image of a bitmap (wxIcon::CopyFromBitmap(): an icon is a bitmap here).
    bool CopyFromBitmap( const wxBitmap& aBitmap )
    {
        *this = aBitmap;
        return true;
    }

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

    bool LoadFile( const wxString& aName, wxBitmapType aType = wxBITMAP_TYPE_ANY );
    bool SaveFile( const wxString& aName, wxBitmapType aType ) const;
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

    /// The images of the icon at two scales.
    static wxBitmapBundle FromBitmaps( const wxBitmap& aBitmap1, const wxBitmap& aBitmap2 )
    {
        wxBitmapBundle bundle( aBitmap1 );
        bundle.addPixmap( aBitmap2 );
        return bundle;
    }
};


/// The changes of the bitmaps (wxBitmapHelpers).
namespace wxBitmapHelpers
{
/// Scale a bitmap to a size (in pixels).
inline void Rescale( wxBitmap& aBitmap, const wxSize& aSize )
{
    aBitmap = wxBitmap( aBitmap.scaled( aSize.x, aSize.y, Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation ) );
}
}


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


/// The fonts of the system (wxSystemFont of wxWidgets).
enum wxSystemFont
{
    wxSYS_OEM_FIXED_FONT = 10,
    wxSYS_ANSI_FIXED_FONT,
    wxSYS_ANSI_VAR_FONT,
    wxSYS_SYSTEM_FONT,
    wxSYS_DEVICE_DEFAULT_FONT,
    wxSYS_DEFAULT_GUI_FONT = 17
};


class KIQT_API wxSystemSettings
{
public:
    static wxColour GetColour( wxSystemColour aIndex );

    /// The font of the application, or its fixed pitch font.
    static wxFont GetFont( wxSystemFont aIndex );

    /// A metric of the style of Qt (in logical pixels), -1 if unknown.
    static int GetMetric( wxSystemMetric aIndex, const QWidget* aWindow = nullptr );
};


/// The raster operations of the copies between device contexts (wxRasterOperationMode).
enum wxRasterOperationMode
{
    wxCLEAR,
    wxXOR,
    wxINVERT,
    wxOR_REVERSE,
    wxAND_REVERSE,
    wxCOPY,
    wxAND,
    wxAND_INVERT,
    wxNO_OP,
    wxNOR,
    wxEQUIV,
    wxSRC_INVERT,
    wxOR_INVERT,
    wxNAND,
    wxOR,
    wxSET
};


/// The fill of the polygons (wxDC::DrawPolygon()).
enum wxPolygonFillMode
{
    wxODDEVEN_RULE = 1,
    wxWINDING_RULE
};


/// An affine transformation of the coordinates (wxAffineMatrix2D): a QTransform.
class wxAffineMatrix2D
{
public:
    wxAffineMatrix2D() = default;
    explicit wxAffineMatrix2D( const QTransform& aTransform ) : m_transform( aTransform ) {}

    void Translate( double aDx, double aDy ) { m_transform = QTransform::fromTranslate( aDx, aDy ) * m_transform; }
    void Scale( double aX, double aY ) { m_transform = QTransform::fromScale( aX, aY ) * m_transform; }
    void Rotate( double aRadians ) { m_transform = QTransform().rotateRadians( aRadians ) * m_transform; }
    void Mirror( int aDirection = wxHORIZONTAL )
    {
        Scale( ( aDirection & wxHORIZONTAL ) ? -1 : 1, ( aDirection & wxVERTICAL ) ? -1 : 1 );
    }

    bool Invert()
    {
        bool invertible = false;
        QTransform inverted = m_transform.inverted( &invertible );

        if( invertible )
            m_transform = inverted;

        return invertible;
    }

    bool IsIdentity() const { return m_transform.isIdentity(); }

    const QTransform& GetTransformCompat() const { return m_transform; }

private:
    QTransform m_transform;
};


/**
 * The device context of the drawings of the cells of the grids and of the custom controls
 * (wxDC): a QPainter.
 *
 * The logical coordinates are mapped to the coordinates of the device (the ones of the painter
 * when the DC was given it) as wxWidgets: (logical - logical origin) * user scale * axis sign
 * + device origin.
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
    void DrawEllipse( int aX, int aY, int aWidth, int aHeight );
    void DrawEllipse( const wxRect& aRect ) { DrawEllipse( aRect.x, aRect.y, aRect.width, aRect.height ); }

    /**
     * Draw an arc from @a aX1, @a aY1 to @a aX2, @a aY2, counterclockwise on the device, around
     * the center @a aXc, @a aYc: a pie filled with the brush.  The same start and end draw a
     * circle.
     */
    void DrawArc( int aX1, int aY1, int aX2, int aY2, int aXc, int aYc );
    void DrawArc( const wxPoint& aStart, const wxPoint& aEnd, const wxPoint& aCenter )
    {
        DrawArc( aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y );
    }

    void DrawPolygon( int aCount, const wxPoint aPoints[], wxCoord aXOffset = 0,
                      wxCoord aYOffset = 0, wxPolygonFillMode aFillStyle = wxODDEVEN_RULE );
    void DrawLines( int aCount, const wxPoint aPoints[], wxCoord aXOffset = 0,
                    wxCoord aYOffset = 0 );
    void DrawPoint( int aX, int aY );
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
    void GetTextExtent( const wxString& aText, int* aWidth, int* aHeight, int* aDescent,
                        int* aExternalLeading = nullptr, const wxFont* aFont = nullptr ) const;
    wxSize GetTextExtent( const wxString& aText ) const;
    wxSize GetMultiLineTextExtent( const wxString& aText ) const;

    void SetClippingRegion( const wxRect& aRect );
    void SetClippingRegion( int aX, int aY, int aWidth, int aHeight );

    void SetClippingRegion( const wxPoint& aPos, const wxSize& aSize )
    {
        SetClippingRegion( aPos.x, aPos.y, aSize.x, aSize.y );
    }
    void DestroyClippingRegion();

    /// The scale of the logical coordinates (on the transformation of the painter when the DC
    /// was given it).
    void SetUserScale( double aX, double aY );
    void GetUserScale( double* aX, double* aY ) const;

    /// The origin of the logical coordinates, and the device coordinates of that origin.
    void SetLogicalOrigin( wxCoord aX, wxCoord aY );
    void GetLogicalOrigin( wxCoord* aX, wxCoord* aY ) const;
    wxPoint GetLogicalOrigin() const { return m_logicalOrigin; }
    void SetDeviceOrigin( wxCoord aX, wxCoord aY );
    void GetDeviceOrigin( wxCoord* aX, wxCoord* aY ) const;
    wxPoint GetDeviceOrigin() const { return m_deviceOrigin; }

    /// The directions of the axes: the x axis from the left to the right, the y axis from the
    /// bottom to the top (or else the opposite).
    void SetAxisOrientation( bool aXLeftRight, bool aYBottomUp );

    wxCoord DeviceToLogicalX( wxCoord aX ) const;
    wxCoord DeviceToLogicalY( wxCoord aY ) const;
    wxCoord DeviceToLogicalXRel( wxCoord aX ) const;
    wxCoord DeviceToLogicalYRel( wxCoord aY ) const;
    wxCoord LogicalToDeviceX( wxCoord aX ) const;
    wxCoord LogicalToDeviceY( wxCoord aY ) const;
    wxCoord LogicalToDeviceXRel( wxCoord aX ) const;
    wxCoord LogicalToDeviceYRel( wxCoord aY ) const;

    /// No matrix of transformation: the users of the DC use the scale and the origins.
    bool CanUseTransformMatrix() const { return false; }
    wxAffineMatrix2D GetTransformMatrix() const { return wxAffineMatrix2D(); }
    bool SetTransformMatrix( const wxAffineMatrix2D& aMatrix ) { return false; }
    void ResetTransformMatrix() {}

    /// Copy a part of a memory DC (the only DCs that keep their drawing).
    bool Blit( wxCoord aXDest, wxCoord aYDest, wxCoord aWidth, wxCoord aHeight, wxDC* aSource,
               wxCoord aXSrc, wxCoord aYSrc, wxRasterOperationMode aMode = wxCOPY,
               bool aUseMask = false, wxCoord aXSrcMask = -1, wxCoord aYSrcMask = -1 );

    bool Blit( const wxPoint& aDest, const wxSize& aSize, wxDC* aSource, const wxPoint& aSrc,
               wxRasterOperationMode aMode = wxCOPY, bool aUseMask = false,
               const wxPoint& aSrcMask = wxDefaultPosition )
    {
        return Blit( aDest.x, aDest.y, aSize.x, aSize.y, aSource, aSrc.x, aSrc.y, aMode, aUseMask,
                     aSrcMask.x, aSrcMask.y );
    }

    /// The image a DC draws on, if it keeps it (wxMemoryDC).
    virtual const QImage* GetImageCompat() const { return nullptr; }

    wxSize GetSize() const;

    /// The resolution of the device, in dots per inch.
    wxSize GetPPI() const;

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

    /// Map the logical coordinates to the ones of the device on the painter.
    void applyTransformCompat();

    bool       m_userScaled = false;    ///< The transformation of the painter is the DC's one
    double     m_userScaleX = 1.0;
    double     m_userScaleY = 1.0;
    int        m_signX = 1;
    int        m_signY = 1;
    wxPoint    m_logicalOrigin;
    wxPoint    m_deviceOrigin;
    QTransform m_baseTransform;

protected:
    void SetPainterCompat( QPainter* aPainter );
};

/// A DC drawing with a graphic context (wxGCDC): the DCs of the layer all draw with a QPainter.
class KIQT_API wxGCDC : public wxDC
{
public:
    using wxDC::wxDC;
};

class wxWindow;


/**
 * The device context of a window (wxPaintDC, wxClientDC): a painter on the window while it
 * is painted (in a handler of wxEVT_PAINT), or else on an image of its size (to measure texts:
 * the windows of Qt are painted only when they are painted).
 */
class KIQT_API wxPaintDC : public wxDC
{
public:
    explicit wxPaintDC( const wxWindow* aWindow );
    ~wxPaintDC() override;

    // Owns objects through unique_ptr; MSVC emits the copy of a dllexport class eagerly.
    wxPaintDC( const wxPaintDC& ) = delete;
    wxPaintDC& operator=( const wxPaintDC& ) = delete;

private:
    void init( QWidget* aWindow );

    std::unique_ptr<QPainter> m_ownPainter;
    std::unique_ptr<QImage>   m_image;
};

using wxClientDC = wxPaintDC;
using wxWindowDC = wxPaintDC;
using wxBufferedPaintDC = wxPaintDC;
using wxAutoBufferedPaintDC = wxPaintDC;

/// The widgets painted now (their wxPaintDC paint on them): the paint events of the layer.
KIQT_API void wxSetPaintingCompat( QWidget* aWidget, bool aPainting );
KIQT_API bool wxIsPaintingCompat( const QWidget* aWidget );


/// A device context drawing on a bitmap (wxMemoryDC).
class KIQT_API wxMemoryDC : public wxDC
{
public:
    wxMemoryDC();
    explicit wxMemoryDC( wxBitmap& aBitmap );
    ~wxMemoryDC() override;

    // Owns objects through unique_ptr; MSVC emits the copy of a dllexport class eagerly.
    wxMemoryDC( const wxMemoryDC& ) = delete;
    wxMemoryDC& operator=( const wxMemoryDC& ) = delete;

    /// Draw on @a aBitmap: the drawing is in the bitmap when another one is selected (or
    /// wxNullBitmap), or when the DC is deleted.
    void SelectObject( wxBitmap& aBitmap );

    /// Draw on a copy of a constant bitmap (wxNullBitmap: on nothing).
    void SelectObject( const wxBitmap& aBitmap ) { SelectObjectAsSource( aBitmap ); }
    void SelectObjectAsSource( const wxBitmap& aBitmap );

    const QImage* GetImageCompat() const override { return &m_image; }

private:
    void release();

    std::unique_ptr<QPainter> m_ownPainter;
    QImage                    m_image;
    wxBitmap*                 m_bitmap = nullptr;
};


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
    void DrawDropArrow( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags = 0 );
    wxSize GetCheckBoxSize( QWidget* aWindow, int aFlags = 0 );
};

#endif // KIWX_GDI_H
