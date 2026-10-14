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
 * @file kiwx_gdi.cpp
 * The graphics of the wxWidgets compatibility layer of the Qt build.
 */

#include <wx/kiwx_gdi.h>

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionFocusRect>
#include <QWidget>

#include <wx/defs.h>

#include <gal/color4d.h>
#include <qt_strings.h>


// Colours ----------------------------------------------------------------------------------

wxColour::wxColour( const wxString& aName ) :
        m_ok( false )
{
    Set( aName );
}


bool wxColour::Set( const wxString& aName )
{
    QString name = ToQString( aName ).trimmed();

    // "rgb(r, g, b)" and "rgba(r, g, b, a)" (the CSS syntax of wxWidgets)
    if( name.startsWith( QStringLiteral( "rgb" ), Qt::CaseInsensitive ) )
    {
        int open = name.indexOf( QLatin1Char( '(' ) );
        int close = name.lastIndexOf( QLatin1Char( ')' ) );

        if( open < 0 || close < open )
            return false;

        QStringList parts = name.mid( open + 1, close - open - 1 ).split( QLatin1Char( ',' ) );

        if( parts.size() < 3 )
            return false;

        int    r = parts[0].trimmed().toInt();
        int    g = parts[1].trimmed().toInt();
        int    b = parts[2].trimmed().toInt();
        double a = parts.size() > 3 ? parts[3].trimmed().toDouble() : 1.0;

        m_color = QColor( r, g, b, int( a * 255 + 0.5 ) );
        m_ok = true;
        return true;
    }

    QColor color = QColor::fromString( name );

    if( !color.isValid() )
        return false;

    m_color = color;
    m_ok = true;
    return true;
}


wxString wxColour::GetAsString( long aFlags ) const
{
    if( aFlags & wxC2S_HTML_SYNTAX )
        return ToWxString( m_color.name( QColor::HexRgb ).toUpper() );

    if( m_color.alpha() < 255 )
    {
        return wxString::Format( wxT( "rgba(%d, %d, %d, %.3f)" ), m_color.red(), m_color.green(),
                                 m_color.blue(), m_color.alphaF() );
    }

    return wxString::Format( wxT( "rgb(%d, %d, %d)" ), m_color.red(), m_color.green(),
                             m_color.blue() );
}


wxColour wxColour::ChangeLightness( int aAlpha ) const
{
    // As wxWidgets: 0 is black, 100 is the colour, 200 is white
    auto blend = [aAlpha]( int aChannel )
                 {
                     double c = aAlpha < 100 ? aChannel * aAlpha / 100.0
                                             : aChannel + ( 255 - aChannel ) * ( aAlpha - 100 ) / 100.0;
                     return static_cast<unsigned char>( std::clamp( c, 0.0, 255.0 ) );
                 };

    return wxColour( blend( m_color.red() ), blend( m_color.green() ), blend( m_color.blue() ),
                     m_color.alpha() );
}


const wxColour wxNullColour;

static const wxColour s_black( 0, 0, 0 );
static const wxColour s_white( 255, 255, 255 );
static const wxColour s_red( 255, 0, 0 );
static const wxColour s_blue( 0, 0, 255 );
static const wxColour s_green( 0, 255, 0 );
static const wxColour s_lightGrey( 211, 211, 211 );

const wxColour* wxBLACK = &s_black;
const wxColour* wxWHITE = &s_white;
const wxColour* wxRED = &s_red;
const wxColour* wxBLUE = &s_blue;
const wxColour* wxGREEN = &s_green;
const wxColour* wxLIGHT_GREY = &s_lightGrey;


// The conversions of KIGFX::COLOR4D (color4d_wx.cpp with wxWidgets)
KIGFX::COLOR4D::COLOR4D( const wxColour& aColor )
{
    r = aColor.Red() / 255.0;
    g = aColor.Green() / 255.0;
    b = aColor.Blue() / 255.0;
    a = aColor.Alpha() / 255.0;
}


wxColour KIGFX::COLOR4D::ToColour() const
{
    return wxColour( Red8(), Green8(), Blue8(), static_cast<unsigned char>( a * 255 + 0.5 ) );
}


// Pens and brushes -------------------------------------------------------------------------

static const wxPen   s_transparentPen( wxColour( 0, 0, 0 ), 1, wxPENSTYLE_TRANSPARENT );
static const wxPen   s_blackPen( wxColour( 0, 0, 0 ) );
static const wxPen   s_whitePen( wxColour( 255, 255, 255 ) );
static const wxBrush s_transparentBrush( wxColour( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
static const wxBrush s_blackBrush( wxColour( 0, 0, 0 ) );
static const wxBrush s_whiteBrush( wxColour( 255, 255, 255 ) );

const wxPen*   wxTRANSPARENT_PEN = &s_transparentPen;
const wxPen*   wxBLACK_PEN = &s_blackPen;
const wxPen*   wxWHITE_PEN = &s_whitePen;
const wxBrush* wxTRANSPARENT_BRUSH = &s_transparentBrush;
const wxBrush* wxBLACK_BRUSH = &s_blackBrush;
const wxBrush* wxWHITE_BRUSH = &s_whiteBrush;
const wxPen    wxNullPen;
const wxBrush  wxNullBrush;


// Fonts ------------------------------------------------------------------------------------

wxFont::wxFont( int aPointSize, wxFontFamily aFamily, wxFontStyle aStyle, wxFontWeight aWeight,
                bool aUnderline, const wxString& aFaceName ) :
        QFont( QApplication::font() )
{
    if( aPointSize > 0 )
        setPointSize( aPointSize );

    if( aFamily == wxFONTFAMILY_TELETYPE || aFamily == wxFONTFAMILY_MODERN )
        setStyleHint( QFont::Monospace );

    setItalic( aStyle != wxFONTSTYLE_NORMAL );
    setBold( aWeight == wxFONTWEIGHT_BOLD );
    setUnderline( aUnderline );

    if( !aFaceName.IsEmpty() )
        setFamily( ToQString( aFaceName ) );
}


wxString wxFont::ToWxString( const QString& aText )
{
    return ::ToWxString( aText );
}


bool wxFont::SetFaceName( const wxString& aFaceName )
{
    setFamily( ToQString( aFaceName ) );
    return true;
}


wxFont wxFont::Bold() const
{
    wxFont font( *this );
    font.setBold( true );
    return font;
}


wxFont wxFont::Italic() const
{
    wxFont font( *this );
    font.setItalic( true );
    return font;
}


wxFont wxFont::Scaled( float aScale ) const
{
    wxFont font( *this );

    if( pointSizeF() > 0 )
        font.setPointSizeF( pointSizeF() * aScale );
    else if( pixelSize() > 0 )
        font.setPixelSize( int( pixelSize() * aScale + 0.5 ) );

    return font;
}


// The steps of the sizes of wxWidgets
wxFont wxFont::Smaller() const
{
    return Scaled( 1.0f / 1.2f );
}


wxFont wxFont::Larger() const
{
    return Scaled( 1.2f );
}


wxFont& wxFont::MakeSmaller()
{
    *this = Smaller();
    return *this;
}


wxFont& wxFont::MakeLarger()
{
    *this = Larger();
    return *this;
}


const wxFont wxNullFont;


// Bitmaps ----------------------------------------------------------------------------------

wxBitmap::wxBitmap( int aWidth, int aHeight, int aDepth ) :
        QPixmap( std::max( aWidth, 0 ), std::max( aHeight, 0 ) )
{
    fill( Qt::transparent );
}


wxSize wxBitmap::GetLogicalSize() const
{
    return wxSize( int( GetLogicalWidth() + 0.5 ), int( GetLogicalHeight() + 0.5 ) );
}


const wxBitmap& wxNullBitmapCompat()
{
    // Made on the first use: a pixmap needs the application of Qt
    static const wxBitmap s_nullBitmap;
    return s_nullBitmap;
}


wxSize wxBitmapBundle::GetDefaultSize() const
{
    QList<QSize> sizes = availableSizes();

    if( sizes.isEmpty() )
        return wxSize( 16, 16 );

    // The smallest image is the one at the scale 1
    QSize smallest = sizes.first();

    for( const QSize& size : sizes )
    {
        if( size.width() < smallest.width() )
            smallest = size;
    }

    return wxSize( smallest.width(), smallest.height() );
}


wxSize wxBitmapBundle::GetPreferredBitmapSizeAtScale( double aScale ) const
{
    wxSize size = GetDefaultSize();
    return wxSize( int( size.x * aScale + 0.5 ), int( size.y * aScale + 0.5 ) );
}


wxSize wxBitmapBundle::GetPreferredBitmapSizeFor( const QWidget* aWindow ) const
{
    return GetPreferredBitmapSizeAtScale( aWindow ? aWindow->devicePixelRatioF() : 1.0 );
}


wxBitmap wxBitmapBundle::GetBitmap( const wxSize& aSize ) const
{
    wxSize size = aSize.x > 0 && aSize.y > 0 ? aSize : GetDefaultSize();
    return wxBitmap( pixmap( QSize( size.x, size.y ) ) );
}


wxBitmap wxBitmapBundle::GetBitmapFor( const QWidget* aWindow ) const
{
    // A bitmap of the logical size of the icon, at the scale of the window
    wxSize size = GetDefaultSize();
    double scale = aWindow ? aWindow->devicePixelRatioF() : 1.0;
    return wxBitmap( pixmap( QSize( size.x, size.y ), scale ) );
}


// The colours of the system ----------------------------------------------------------------

int wxSystemSettings::GetMetric( wxSystemMetric aIndex, const QWidget* aWindow )
{
    QStyle* style = aWindow ? aWindow->style() : QApplication::style();

    switch( aIndex )
    {
    case wxSYS_HSCROLL_Y:
    case wxSYS_VSCROLL_X:       return style->pixelMetric( QStyle::PM_ScrollBarExtent, nullptr, aWindow );
    case wxSYS_HSCROLL_ARROW_X:
    case wxSYS_VSCROLL_ARROW_Y: return style->pixelMetric( QStyle::PM_ScrollBarSliderMin, nullptr, aWindow );
    case wxSYS_BORDER_X:
    case wxSYS_BORDER_Y:        return 1;
    case wxSYS_EDGE_X:
    case wxSYS_EDGE_Y:          return style->pixelMetric( QStyle::PM_DefaultFrameWidth, nullptr, aWindow );
    case wxSYS_ICON_X:
    case wxSYS_ICON_Y:          return style->pixelMetric( QStyle::PM_LargeIconSize, nullptr, aWindow );
    case wxSYS_SMALLICON_X:
    case wxSYS_SMALLICON_Y:     return style->pixelMetric( QStyle::PM_SmallIconSize, nullptr, aWindow );
    case wxSYS_DRAG_X:
    case wxSYS_DRAG_Y:          return QApplication::startDragDistance();
    case wxSYS_DCLICK_MSEC:     return QApplication::doubleClickInterval();
    case wxSYS_CARET_ON_MSEC:
    case wxSYS_CARET_OFF_MSEC:  return QApplication::cursorFlashTime() / 2;
    default:                    return -1;
    }
}


wxColour wxSystemSettings::GetColour( wxSystemColour aIndex )
{
    QPalette palette = QApplication::palette();

    switch( aIndex )
    {
    case wxSYS_COLOUR_WINDOW:
    case wxSYS_COLOUR_LISTBOX:
        return palette.color( QPalette::Base );

    case wxSYS_COLOUR_WINDOWTEXT:
    case wxSYS_COLOUR_LISTBOXTEXT:
    case wxSYS_COLOUR_MENUTEXT:
    case wxSYS_COLOUR_CAPTIONTEXT:
        return palette.color( QPalette::Text );

    case wxSYS_COLOUR_BTNTEXT:
        return palette.color( QPalette::ButtonText );

    case wxSYS_COLOUR_HIGHLIGHT:
    case wxSYS_COLOUR_MENUHILIGHT:
        return palette.color( QPalette::Highlight );

    case wxSYS_COLOUR_HIGHLIGHTTEXT:
    case wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT:
        return palette.color( QPalette::HighlightedText );

    case wxSYS_COLOUR_GRAYTEXT:
        return palette.color( QPalette::Disabled, QPalette::Text );

    case wxSYS_COLOUR_HOTLIGHT:
        return palette.color( QPalette::Link );

    case wxSYS_COLOUR_INFOBK:
        return palette.color( QPalette::ToolTipBase );

    case wxSYS_COLOUR_INFOTEXT:
        return palette.color( QPalette::ToolTipText );

    case wxSYS_COLOUR_3DLIGHT:
    case wxSYS_COLOUR_BTNHIGHLIGHT:
        return palette.color( QPalette::Light );

    case wxSYS_COLOUR_3DDKSHADOW:
        return palette.color( QPalette::Shadow );

    case wxSYS_COLOUR_BTNSHADOW:
    case wxSYS_COLOUR_ACTIVEBORDER:
    case wxSYS_COLOUR_INACTIVEBORDER:
        return palette.color( QPalette::Mid );

    case wxSYS_COLOUR_BTNFACE:
    case wxSYS_COLOUR_FRAMEBK:
    case wxSYS_COLOUR_MENU:
    case wxSYS_COLOUR_MENUBAR:
    case wxSYS_COLOUR_APPWORKSPACE:
    default:
        return palette.color( QPalette::Window );
    }
}


// The device context -----------------------------------------------------------------------

wxDC::wxDC( QPainter* aPainter, QWidget* aWindow ) :
        m_painter( aPainter ),
        m_window( aWindow ),
        m_font( aWindow ? aWindow->font() : QApplication::font() ),
        m_textForeground( aWindow ? aWindow->palette().color( QPalette::Text )
                                  : QApplication::palette().color( QPalette::Text ) )
{
}


void wxDC::SetPen( const wxPen& aPen )
{
    m_pen = aPen;

    if( !aPen.IsOk() || aPen.GetStyle() == wxPENSTYLE_TRANSPARENT )
    {
        m_painter->setPen( Qt::NoPen );
        return;
    }

    QPen pen( QColor( aPen.GetColour() ) );
    pen.setWidth( aPen.GetWidth() );

    switch( aPen.GetStyle() )
    {
    case wxPENSTYLE_DOT:        pen.setStyle( Qt::DotLine );     break;
    case wxPENSTYLE_LONG_DASH:
    case wxPENSTYLE_SHORT_DASH: pen.setStyle( Qt::DashLine );    break;
    case wxPENSTYLE_DOT_DASH:   pen.setStyle( Qt::DashDotLine ); break;
    default:                    pen.setStyle( Qt::SolidLine );   break;
    }

    m_painter->setPen( pen );
}


void wxDC::SetBrush( const wxBrush& aBrush )
{
    m_brush = aBrush;

    if( !aBrush.IsOk() || aBrush.IsTransparent() )
        m_painter->setBrush( Qt::NoBrush );
    else
        m_painter->setBrush( QColor( aBrush.GetColour() ) );
}


void wxDC::SetFont( const wxFont& aFont )
{
    m_font = aFont;
    m_painter->setFont( aFont );
}


void wxDC::DrawRectangle( const wxRect& aRect )
{
    DrawRectangle( aRect.x, aRect.y, aRect.width, aRect.height );
}


void wxDC::DrawRectangle( int aX, int aY, int aWidth, int aHeight )
{
    // A wxDC fills the rectangle and draws its outline inside it
    QRect rect( aX, aY, aWidth, aHeight );

    if( m_painter->pen().style() != Qt::NoPen )
        rect.adjust( 0, 0, -1, -1 );

    m_painter->drawRect( rect );
}


void wxDC::DrawRoundedRectangle( const wxRect& aRect, double aRadius )
{
    m_painter->drawRoundedRect( QRect( aRect.x, aRect.y, aRect.width, aRect.height ), aRadius,
                                aRadius );
}


void wxDC::DrawLine( int aX1, int aY1, int aX2, int aY2 )
{
    m_painter->drawLine( aX1, aY1, aX2, aY2 );
}


void wxDC::DrawCircle( int aX, int aY, int aRadius )
{
    m_painter->drawEllipse( QPoint( aX, aY ), aRadius, aRadius );
}


void wxDC::DrawText( const wxString& aText, int aX, int aY )
{
    m_painter->save();
    m_painter->setFont( m_font );
    m_painter->setPen( QColor( m_textForeground ) );

    // The position is the top left corner of the text
    QFontMetrics metrics( m_font );
    int          y = aY + metrics.ascent();

    for( const QString& line : ToQString( aText ).split( QLatin1Char( '\n' ) ) )
    {
        m_painter->drawText( aX, y, line );
        y += metrics.lineSpacing();
    }

    m_painter->restore();
}


void wxDC::DrawLabel( const wxString& aText, const wxRect& aRect, int aAlignment )
{
    int flags = 0;

    if( aAlignment & wxALIGN_RIGHT )
        flags |= Qt::AlignRight;
    else if( aAlignment & wxALIGN_CENTER_HORIZONTAL )
        flags |= Qt::AlignHCenter;
    else
        flags |= Qt::AlignLeft;

    if( aAlignment & wxALIGN_BOTTOM )
        flags |= Qt::AlignBottom;
    else if( aAlignment & wxALIGN_CENTER_VERTICAL )
        flags |= Qt::AlignVCenter;
    else
        flags |= Qt::AlignTop;

    m_painter->save();
    m_painter->setFont( m_font );
    m_painter->setPen( QColor( m_textForeground ) );
    m_painter->drawText( QRect( aRect.x, aRect.y, aRect.width, aRect.height ), flags,
                         ToQString( aText ) );
    m_painter->restore();
}


void wxDC::DrawBitmap( const wxBitmap& aBitmap, int aX, int aY, bool aUseMask )
{
    m_painter->drawPixmap( aX, aY, aBitmap );
}


void wxDC::Clear()
{
    if( m_background.IsOk() )
        m_painter->fillRect( m_painter->window(), QColor( m_background.GetColour() ) );
}


void wxDC::GetTextExtent( const wxString& aText, int* aWidth, int* aHeight ) const
{
    wxSize size = GetTextExtent( aText );

    if( aWidth )
        *aWidth = size.x;

    if( aHeight )
        *aHeight = size.y;
}


wxSize wxDC::GetTextExtent( const wxString& aText ) const
{
    QFontMetrics metrics( m_font );
    QSize        size = metrics.size( Qt::TextSingleLine, ToQString( aText ) );
    return wxSize( size.width(), size.height() );
}


wxSize wxDC::GetMultiLineTextExtent( const wxString& aText ) const
{
    QFontMetrics metrics( m_font );
    QSize        size = metrics.size( 0, ToQString( aText ) );
    return wxSize( size.width(), size.height() );
}


void wxDC::SetClippingRegion( const wxRect& aRect )
{
    SetClippingRegion( aRect.x, aRect.y, aRect.width, aRect.height );
}


void wxDC::SetClippingRegion( int aX, int aY, int aWidth, int aHeight )
{
    m_painter->setClipRect( aX, aY, aWidth, aHeight, Qt::IntersectClip );
}


void wxDC::DestroyClippingRegion()
{
    m_painter->setClipping( false );
}


wxSize wxDC::GetSize() const
{
    QRect window = m_painter->window();
    return wxSize( window.width(), window.height() );
}


double wxDC::GetContentScaleFactor() const
{
    return m_window ? m_window->devicePixelRatioF() : qApp->devicePixelRatio();
}


// The controls in the style of the system ---------------------------------------------------

wxRendererNative& wxRendererNative::Get()
{
    static wxRendererNative s_renderer;
    return s_renderer;
}


static QStyle* styleOf( QWidget* aWindow )
{
    return aWindow ? aWindow->style() : QApplication::style();
}


static void initStyleOption( QStyleOption& aOption, QWidget* aWindow, const wxRect& aRect,
                             int aFlags )
{
    if( aWindow )
        aOption.initFrom( aWindow );

    aOption.rect = QRect( aRect.x, aRect.y, aRect.width, aRect.height );
    aOption.state = QStyle::State_None;

    if( !( aFlags & wxCONTROL_DISABLED ) )
        aOption.state |= QStyle::State_Enabled;

    if( aFlags & wxCONTROL_FOCUSED )
        aOption.state |= QStyle::State_HasFocus;

    if( aFlags & wxCONTROL_PRESSED )
        aOption.state |= QStyle::State_Sunken;
    else
        aOption.state |= QStyle::State_Raised;

    if( aFlags & wxCONTROL_CURRENT )
        aOption.state |= QStyle::State_MouseOver;
}


void wxRendererNative::DrawCheckBox( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags )
{
    QStyleOptionButton option;
    initStyleOption( option, aWindow, aRect, aFlags );

    if( aFlags & wxCONTROL_CHECKED )
        option.state |= QStyle::State_On;
    else if( aFlags & wxCONTROL_UNDETERMINED )
        option.state |= QStyle::State_NoChange;
    else
        option.state |= QStyle::State_Off;

    styleOf( aWindow )->drawPrimitive( QStyle::PE_IndicatorCheckBox, &option, aDC.GetPainter(),
                                       aWindow );
}


void wxRendererNative::DrawPushButton( QWidget* aWindow, wxDC& aDC, const wxRect& aRect,
                                       int aFlags )
{
    QStyleOptionButton option;
    initStyleOption( option, aWindow, aRect, aFlags );
    styleOf( aWindow )->drawControl( QStyle::CE_PushButtonBevel, &option, aDC.GetPainter(),
                                     aWindow );
}


void wxRendererNative::DrawComboBoxDropButton( QWidget* aWindow, wxDC& aDC, const wxRect& aRect,
                                               int aFlags )
{
    QStyleOption option;
    initStyleOption( option, aWindow, aRect, aFlags );
    styleOf( aWindow )->drawPrimitive( QStyle::PE_IndicatorArrowDown, &option, aDC.GetPainter(),
                                       aWindow );
}


void wxRendererNative::DrawFocusRect( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags )
{
    QStyleOptionFocusRect option;
    initStyleOption( option, aWindow, aRect, aFlags );
    styleOf( aWindow )->drawPrimitive( QStyle::PE_FrameFocusRect, &option, aDC.GetPainter(),
                                       aWindow );
}


wxSize wxRendererNative::GetCheckBoxSize( QWidget* aWindow, int aFlags )
{
    QStyle* style = styleOf( aWindow );
    return wxSize( style->pixelMetric( QStyle::PM_IndicatorWidth, nullptr, aWindow ),
                   style->pixelMetric( QStyle::PM_IndicatorHeight, nullptr, aWindow ) );
}
