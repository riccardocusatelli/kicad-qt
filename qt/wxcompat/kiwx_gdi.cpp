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

#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QBuffer>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionFocusRect>
#include <QWidget>

#include <wx/stream.h>

#include <wx/defs.h>
#include <wx/kiwx_window.h>

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
static const wxColour s_yellow( 255, 255, 0 );
static const wxColour s_cyan( 0, 255, 255 );
static const wxColour s_grey( 128, 128, 128 );

const wxColour* wxBLACK = &s_black;
const wxColour* wxWHITE = &s_white;
const wxColour* wxRED = &s_red;
const wxColour* wxBLUE = &s_blue;
const wxColour* wxGREEN = &s_green;
const wxColour* wxLIGHT_GREY = &s_lightGrey;
const wxColour* wxYELLOW = &s_yellow;
const wxColour* wxCYAN = &s_cyan;
const wxColour* wxGREY = &s_grey;


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
static const wxPen   s_greyPen( wxColour( 128, 128, 128 ) );
static const wxPen   s_mediumGreyPen( wxColour( 100, 100, 100 ) );
static const wxPen   s_lightGreyPen( wxColour( 192, 192, 192 ) );
static const wxBrush s_transparentBrush( wxColour( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
static const wxBrush s_blackBrush( wxColour( 0, 0, 0 ) );
static const wxBrush s_whiteBrush( wxColour( 255, 255, 255 ) );

const wxPen*   wxTRANSPARENT_PEN = &s_transparentPen;
const wxPen*   wxBLACK_PEN = &s_blackPen;
const wxPen*   wxWHITE_PEN = &s_whitePen;
const wxPen*   wxGREY_PEN = &s_greyPen;
const wxPen*   wxMEDIUM_GREY_PEN = &s_mediumGreyPen;
const wxPen*   wxLIGHT_GREY_PEN = &s_lightGreyPen;
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


wxFont::wxFont( const wxFontInfo& aInfo ) :
        wxFont( 0, aInfo.GetFamily(), aInfo.IsItalic() ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
                aInfo.IsBold() ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL, aInfo.IsUnderlined(),
                aInfo.GetFaceName() )
{
    if( aInfo.GetPointSize() > 0 )
        setPointSizeF( aInfo.GetPointSize() );
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


// The stock fonts of wxWidgets: the font of the application, in sizes and styles as wxStockGDI

const wxFont* wxNormalFontCompat()
{
    static const wxFont font( QApplication::font() );
    return &font;
}


const wxFont* wxSmallFontCompat()
{
    static const wxFont font = []()
                               {
                                   wxFont small( QApplication::font() );
                                   small.SetPointSize( std::max( small.GetPointSize() - 2, 1 ) );
                                   return small;
                               }();
    return &font;
}


const wxFont* wxSwissFontCompat()
{
    static const wxFont font = []()
                               {
                                   wxFont swiss( QApplication::font() );
                                   swiss.setStyleHint( QFont::SansSerif );
                                   return swiss;
                               }();
    return &font;
}


const wxFont* wxItalicFontCompat()
{
    static const wxFont font = []()
                               {
                                   wxFont italic( QApplication::font() );
                                   italic.SetStyle( wxFONTSTYLE_ITALIC );
                                   return italic;
                               }();
    return &font;
}


// Bitmaps ----------------------------------------------------------------------------------

// Images -----------------------------------------------------------------------------------

wxImage::wxImage( int aWidth, int aHeight, bool aClear ) :
        QImage( std::max( aWidth, 0 ), std::max( aHeight, 0 ), QImage::Format_RGB32 )
{
    // A new image of wxWidgets is black
    fill( Qt::black );
}


void wxImage::InitAlpha()
{
    if( !hasAlphaChannel() )
        *this = wxImage( convertToFormat( QImage::Format_ARGB32 ) );
}


void wxImage::SetAlpha( unsigned char* aAlpha, bool aStaticData )
{
    InitAlpha();

    if( !aAlpha )
        return;

    for( int y = 0; y < height(); ++y )
    {
        for( int x = 0; x < width(); ++x )
            SetAlpha( x, y, aAlpha[y * width() + x] );
    }

    if( !aStaticData )
        free( aAlpha );
}


void wxImage::SetRGB( int aX, int aY, unsigned char aRed, unsigned char aGreen,
                      unsigned char aBlue )
{
    if( !valid( aX, aY ) )
        return;

    setPixel( aX, aY, qRgba( aRed, aGreen, aBlue, qAlpha( pixel( aX, aY ) ) ) );
}


void wxImage::SetAlpha( int aX, int aY, unsigned char aAlpha )
{
    InitAlpha();

    if( !valid( aX, aY ) )
        return;

    QRgb rgb = pixel( aX, aY );
    setPixel( aX, aY, qRgba( qRed( rgb ), qGreen( rgb ), qBlue( rgb ), aAlpha ) );
}


wxImage wxImage::Rotate90( bool aClockwise ) const
{
    return wxImage( transformed( QTransform().rotate( aClockwise ? 90 : -90 ) ) );
}


wxImage wxImage::Rotate180() const
{
    return wxImage( transformed( QTransform().rotate( 180 ) ) );
}


wxImage wxImage::Mirror( bool aHorizontally ) const
{
    return wxImage( mirrored( aHorizontally, !aHorizontally ) );
}


wxImage wxImage::Scale( int aWidth, int aHeight, wxImageResizeQuality aQuality ) const
{
    return wxImage( scaled( aWidth, aHeight, Qt::IgnoreAspectRatio,
                            aQuality == wxIMAGE_QUALITY_NEAREST ? Qt::FastTransformation
                                                                : Qt::SmoothTransformation ) );
}


wxImage& wxImage::Rescale( int aWidth, int aHeight, wxImageResizeQuality aQuality )
{
    *this = Scale( aWidth, aHeight, aQuality );
    return *this;
}


wxImage wxImage::GetSubImage( const wxRect& aRect ) const
{
    return wxImage( copy( aRect.x, aRect.y, aRect.width, aRect.height ) );
}


void wxImage::SetRGB( const wxRect& aRect, unsigned char aRed, unsigned char aGreen,
                      unsigned char aBlue )
{
    QRect rect = QRect( aRect.x, aRect.y, aRect.width, aRect.height ).intersected( this->rect() );

    for( int y = rect.top(); y <= rect.bottom(); ++y )
    {
        for( int x = rect.left(); x <= rect.right(); ++x )
            SetRGB( x, y, aRed, aGreen, aBlue );
    }
}


void wxImage::Paste( const wxImage& aImage, int aX, int aY, wxImageAlphaBlendMode aAlphaBlend )
{
    if( format() != QImage::Format_ARGB32 && format() != QImage::Format_ARGB32_Premultiplied )
        *this = wxImage( convertToFormat( QImage::Format_ARGB32 ) );

    QPainter painter( this );

    // Over: the pixels of the image replace the ones under them, with their alpha
    painter.setCompositionMode( aAlphaBlend == wxIMAGE_ALPHA_BLEND_COMPOSE
                                        ? QPainter::CompositionMode_SourceOver
                                        : QPainter::CompositionMode_Source );
    painter.drawImage( aX, aY, aImage );
}


wxImage wxImage::ConvertToGreyscale() const
{
    wxImage grey( convertToFormat( QImage::Format_ARGB32 ) );

    for( int y = 0; y < grey.height(); ++y )
    {
        for( int x = 0; x < grey.width(); ++x )
        {
            QRgb rgb = grey.pixel( x, y );
            int  value = qGray( rgb );
            grey.setPixel( x, y, qRgba( value, value, value, qAlpha( rgb ) ) );
        }
    }

    return grey;
}


wxImage wxImage::ConvertToDisabled( unsigned char aBrightness ) const
{
    wxImage disabled = ConvertToGreyscale();

    for( int y = 0; y < disabled.height(); ++y )
    {
        for( int x = 0; x < disabled.width(); ++x )
        {
            QRgb rgb = disabled.pixel( x, y );
            int  value = ( qRed( rgb ) + aBrightness ) / 2;
            disabled.setPixel( x, y, qRgba( value, value, value, qAlpha( rgb ) ) );
        }
    }

    return disabled;
}


void wxImage::Clear( unsigned char aValue )
{
    fill( qRgba( aValue, aValue, aValue, hasAlphaChannel() ? aValue : 255 ) );
}


bool wxImage::LoadFile( const wxString& aName, int aType )
{
    return load( ToQString( aName ) );
}


bool wxImage::SaveFile( const wxString& aName, int aType ) const
{
    return save( ToQString( aName ) );
}


/// The format of an image file for Qt.
static const char* imageFormat( wxBitmapType aType )
{
    switch( aType )
    {
    case wxBITMAP_TYPE_BMP:  return "BMP";
    case wxBITMAP_TYPE_JPEG: return "JPG";
    case wxBITMAP_TYPE_GIF:  return "GIF";
    case wxBITMAP_TYPE_TIFF: return "TIFF";
    case wxBITMAP_TYPE_XPM:  return "XPM";
    case wxBITMAP_TYPE_ICO:  return "ICO";
    default:                 return "PNG";
    }
}


bool wxImage::LoadFile( wxInputStream& aStream, wxBitmapType aType )
{
    QByteArray bytes;
    char       buffer[16384];

    while( aStream.CanRead() )
    {
        aStream.Read( buffer, sizeof( buffer ) );

        if( aStream.LastRead() == 0 )
            break;

        bytes.append( buffer, qsizetype( aStream.LastRead() ) );
    }

    // Qt finds the format from the data
    return loadFromData( bytes, aType == wxBITMAP_TYPE_ANY ? nullptr : imageFormat( aType ) );
}


bool wxImage::SaveFile( wxOutputStream& aStream, wxBitmapType aType ) const
{
    QByteArray bytes;
    QBuffer    buffer( &bytes );

    buffer.open( QIODevice::WriteOnly );

    if( !save( &buffer, imageFormat( aType ) ) )
        return false;

    aStream.Write( bytes.constData(), size_t( bytes.size() ) );
    return aStream.LastWrite() == size_t( bytes.size() );
}


void wxImage::SetOption( const wxString& aName, int aValue )
{
    m_options[aName] = wxString::Format( wxS( "%d" ), aValue );
}


wxString wxImage::GetOption( const wxString& aName ) const
{
    auto it = m_options.find( aName );
    return it == m_options.end() ? wxString() : it->second;
}


int wxImage::GetOptionInt( const wxString& aName ) const
{
    long value = 0;
    GetOption( aName ).ToLong( &value );
    return int( value );
}


wxBitmap::wxBitmap( const wxImage& aImage, int aDepth, double aScale ) :
        QPixmap( QPixmap::fromImage( aImage ) )
{
    setDevicePixelRatio( aScale );
}


wxBitmap::wxBitmap( int aWidth, int aHeight, int aDepth ) :
        QPixmap( std::max( aWidth, 0 ), std::max( aHeight, 0 ) )
{
    fill( Qt::transparent );
}


wxSize wxBitmap::GetLogicalSize() const
{
    return wxSize( int( GetLogicalWidth() + 0.5 ), int( GetLogicalHeight() + 0.5 ) );
}


bool wxBitmap::LoadFile( const wxString& aName, wxBitmapType aType )
{
    return load( ToQString( aName ) );
}


bool wxBitmap::SaveFile( const wxString& aName, wxBitmapType aType ) const
{
    return save( ToQString( aName ) );
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

// The standard icons ------------------------------------------------------------------------

static QStyle::StandardPixmap standardPixmap( const wxArtID& aId )
{
    static const std::map<wxString, QStyle::StandardPixmap> s_pixmaps = {
        { wxART_ERROR,       QStyle::SP_MessageBoxCritical },
        { wxART_QUESTION,    QStyle::SP_MessageBoxQuestion },
        { wxART_WARNING,     QStyle::SP_MessageBoxWarning },
        { wxART_INFORMATION, QStyle::SP_MessageBoxInformation },
        { wxART_HELP,        QStyle::SP_DialogHelpButton },
        { wxART_TIP,         QStyle::SP_MessageBoxInformation },
        { wxART_GO_BACK,     QStyle::SP_ArrowBack },
        { wxART_GO_FORWARD,  QStyle::SP_ArrowForward },
        { wxART_GO_UP,       QStyle::SP_ArrowUp },
        { wxART_GO_DOWN,     QStyle::SP_ArrowDown },
        { wxART_GO_HOME,     QStyle::SP_DirHomeIcon },
        { wxART_FILE_OPEN,   QStyle::SP_DialogOpenButton },
        { wxART_FILE_SAVE,   QStyle::SP_DialogSaveButton },
        { wxART_FOLDER,      QStyle::SP_DirClosedIcon },
        { wxART_FOLDER_OPEN, QStyle::SP_DirOpenIcon },
        { wxART_NORMAL_FILE, QStyle::SP_FileIcon },
        { wxART_NEW,         QStyle::SP_FileIcon },
        { wxART_DELETE,      QStyle::SP_TrashIcon },
        { wxART_CLOSE,       QStyle::SP_DialogCloseButton },
        { wxART_CROSS_MARK,  QStyle::SP_DialogCancelButton },
        { wxART_TICK_MARK,   QStyle::SP_DialogApplyButton },
        { wxART_FIND,        QStyle::SP_FileDialogContentsView },
        { wxART_UNDO,        QStyle::SP_ArrowBack },
        { wxART_REDO,        QStyle::SP_ArrowForward }
    };

    auto it = s_pixmaps.find( aId );
    return it != s_pixmaps.end() ? it->second : QStyle::SP_CustomBase;
}


wxBitmapBundle wxArtProvider::GetBitmapBundle( const wxArtID& aId, const wxArtClient& aClient,
                                               const wxSize& aSize )
{
    QStyle::StandardPixmap pixmap = standardPixmap( aId );

    if( pixmap == QStyle::SP_CustomBase )
        return wxBitmapBundle();

    return wxBitmapBundle( QApplication::style()->standardIcon( pixmap ) );
}


wxBitmap wxArtProvider::GetBitmap( const wxArtID& aId, const wxArtClient& aClient,
                                   const wxSize& aSize )
{
    wxBitmapBundle bundle = GetBitmapBundle( aId, aClient, aSize );

    if( !bundle.IsOk() )
        return wxBitmap();

    wxSize size = aSize.x > 0 && aSize.y > 0 ? aSize : GetSizeHint( aClient );
    return wxBitmap( bundle.pixmap( QSize( size.x, size.y ) ) );
}


wxIcon wxArtProvider::GetIcon( const wxArtID& aId, const wxArtClient& aClient, const wxSize& aSize )
{
    return GetBitmap( aId, aClient, aSize );
}


wxArtID wxArtProvider::GetMessageBoxIconId( int aFlags )
{
    switch( aFlags & wxICON_MASK )
    {
    case wxICON_ERROR:       return wxART_ERROR;
    case wxICON_WARNING:     return wxART_WARNING;
    case wxICON_QUESTION:    return wxART_QUESTION;
    case wxICON_INFORMATION: return wxART_INFORMATION;
    default:                 return wxArtID();
    }
}


wxSize wxArtProvider::GetSizeHint( const wxArtClient& aClient, QWidget* aWindow )
{
    QStyle*            style = aWindow ? aWindow->style() : QApplication::style();
    QStyle::PixelMetric metric = QStyle::PM_SmallIconSize;

    if( aClient == wxART_TOOLBAR )
        metric = QStyle::PM_ToolBarIconSize;
    else if( aClient == wxART_MESSAGE_BOX )
        metric = QStyle::PM_MessageBoxIconSize;
    else if( aClient == wxART_BUTTON )
        metric = QStyle::PM_ButtonIconSize;
    else if( aClient == wxART_CMN_DIALOG || aClient == wxART_FRAME_ICON )
        metric = QStyle::PM_LargeIconSize;

    int size = style->pixelMetric( metric, nullptr, aWindow );
    return wxSize( size, size );
}


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


wxFont wxSystemSettings::GetFont( wxSystemFont aIndex )
{
    switch( aIndex )
    {
    case wxSYS_OEM_FIXED_FONT:
    case wxSYS_ANSI_FIXED_FONT: return QFontDatabase::systemFont( QFontDatabase::FixedFont );
    default:                    return QApplication::font();
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

    // The ends and the joins of the pens of wxWidgets
    pen.setCapStyle( Qt::RoundCap );
    pen.setJoinStyle( Qt::RoundJoin );

    switch( aPen.GetStyle() )
    {
    case wxPENSTYLE_DOT:        pen.setStyle( Qt::DotLine );     break;
    case wxPENSTYLE_LONG_DASH:
    case wxPENSTYLE_SHORT_DASH: pen.setStyle( Qt::DashLine );    break;
    case wxPENSTYLE_DOT_DASH:   pen.setStyle( Qt::DashDotLine ); break;

    case wxPENSTYLE_USER_DASH:
    {
        // The dashes of Qt are in widths of the pen too, but need an even count
        QList<qreal> pattern;

        for( wxDash dash : aPen.GetDashesCompat() )
            pattern << std::max<qreal>( dash, 1 );

        if( pattern.size() % 2 )
            pattern << pattern.back();

        if( pattern.isEmpty() )
            pen.setStyle( Qt::SolidLine );
        else
            pen.setDashPattern( pattern );

        break;
    }

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


void wxDC::DrawEllipse( int aX, int aY, int aWidth, int aHeight )
{
    m_painter->drawEllipse( QRect( aX, aY, aWidth, aHeight ) );
}


void wxDC::DrawArc( int aX1, int aY1, int aX2, int aY2, int aXc, int aYc )
{
    // The arc goes counterclockwise on the device: on a mirrored device, it goes clockwise in
    // the logical coordinates (from the end to the start counterclockwise)
    if( m_painter->worldTransform().determinant() < 0 )
    {
        std::swap( aX1, aX2 );
        std::swap( aY1, aY2 );
    }

    double dx = aX1 - aXc;
    double dy = aY1 - aYc;
    double radius = std::hypot( dx, dy );

    // The angles of Qt are counterclockwise, the y axis going down
    double startAngle = std::atan2( -dy, dx ) * 180.0 / M_PI;
    double endAngle = std::atan2( -( aY2 - aYc ), aX2 - aXc ) * 180.0 / M_PI;
    double sweep = endAngle - startAngle;

    if( aX1 == aX2 && aY1 == aY2 )
        sweep = 360.0;
    else if( sweep <= 0.0 )
        sweep += 360.0;

    QRectF bounds( aXc - radius, aYc - radius, 2 * radius, 2 * radius );

    if( m_brush.IsOk() && !m_brush.IsTransparent() )
    {
        // A pie: the arc and its radii
        QPainterPath path( QPointF( aXc, aYc ) );
        path.arcTo( bounds, startAngle, sweep );
        path.closeSubpath();
        m_painter->drawPath( path );
    }
    else
    {
        QPainterPath path;
        path.arcMoveTo( bounds, startAngle );
        path.arcTo( bounds, startAngle, sweep );
        m_painter->drawPath( path );
    }
}


void wxDC::DrawPolygon( int aCount, const wxPoint aPoints[], wxCoord aXOffset, wxCoord aYOffset,
                        wxPolygonFillMode aFillStyle )
{
    QPolygon polygon;

    for( int i = 0; i < aCount; ++i )
        polygon << QPoint( aPoints[i].x + aXOffset, aPoints[i].y + aYOffset );

    m_painter->drawPolygon( polygon, aFillStyle == wxWINDING_RULE ? Qt::WindingFill
                                                                  : Qt::OddEvenFill );
}


void wxDC::DrawLines( int aCount, const wxPoint aPoints[], wxCoord aXOffset, wxCoord aYOffset )
{
    QPolygon polyline;

    for( int i = 0; i < aCount; ++i )
        polyline << QPoint( aPoints[i].x + aXOffset, aPoints[i].y + aYOffset );

    m_painter->drawPolyline( polyline );
}


void wxDC::DrawPoint( int aX, int aY )
{
    m_painter->drawPoint( aX, aY );
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
    // A transparent background leaves what is under the window (the background of its parent)
    if( m_background.IsOk() && !m_background.IsTransparent()
            && m_background.GetColour().Alpha() != 0 )
    {
        m_painter->fillRect( m_painter->window(), QColor( m_background.GetColour() ) );
    }
}


wxCoord wxDC::GetCharHeight() const
{
    return QFontMetrics( m_font ).height();
}


wxCoord wxDC::GetCharWidth() const
{
    return QFontMetrics( m_font ).averageCharWidth();
}


bool wxDC::GetPartialTextExtents( const wxString& aText, wxArrayInt& aWidths ) const
{
    QFontMetrics metrics( m_font );
    QString      text = ToQString( aText );

    aWidths.clear();

    for( qsizetype i = 1; i <= text.size(); ++i )
        aWidths.push_back( metrics.horizontalAdvance( text.left( i ) ) );

    return true;
}


void wxDC::GetTextExtent( const wxString& aText, int* aWidth, int* aHeight ) const
{
    wxSize size = GetTextExtent( aText );

    if( aWidth )
        *aWidth = size.x;

    if( aHeight )
        *aHeight = size.y;
}


void wxDC::GetTextExtent( const wxString& aText, int* aWidth, int* aHeight, int* aDescent,
                          int* aExternalLeading, const wxFont* aFont ) const
{
    QFontMetrics metrics( aFont ? *aFont : m_font );
    QSize        size = metrics.size( Qt::TextSingleLine, ToQString( aText ) );

    if( aWidth )
        *aWidth = size.width();

    if( aHeight )
        *aHeight = size.height();

    if( aDescent )
        *aDescent = metrics.descent();

    if( aExternalLeading )
        *aExternalLeading = metrics.leading();
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


/// The rounded coordinate of a value.
static wxCoord KiROUNDCompat( double aValue )
{
    return wxCoord( std::lround( aValue ) );
}


void wxDC::SetPainterCompat( QPainter* aPainter )
{
    m_painter = aPainter;

    // The transformation of the new painter, with the scale and the origins of the DC
    m_userScaled = false;

    if( m_painter && ( m_userScaleX != 1.0 || m_userScaleY != 1.0 || m_signX != 1 || m_signY != 1
                       || m_logicalOrigin != wxPoint() || m_deviceOrigin != wxPoint() ) )
    {
        applyTransformCompat();
    }
}


void wxDC::applyTransformCompat()
{
    // The transformation is on the one the painter had (the origin of a window, a cell...)
    if( !m_userScaled )
    {
        m_baseTransform = m_painter->worldTransform();
        m_userScaled = true;
    }

    double sx = m_userScaleX * m_signX;
    double sy = m_userScaleY * m_signY;

    // device = ( logical - logical origin ) * scale + device origin
    QTransform transform( sx, 0, 0, sy, m_deviceOrigin.x - m_logicalOrigin.x * sx,
                          m_deviceOrigin.y - m_logicalOrigin.y * sy );

    m_painter->setWorldTransform( transform * m_baseTransform );
}


void wxDC::SetUserScale( double aX, double aY )
{
    m_userScaleX = aX;
    m_userScaleY = aY;
    applyTransformCompat();
}


void wxDC::SetLogicalOrigin( wxCoord aX, wxCoord aY )
{
    m_logicalOrigin = wxPoint( aX, aY );
    applyTransformCompat();
}


void wxDC::GetLogicalOrigin( wxCoord* aX, wxCoord* aY ) const
{
    if( aX )
        *aX = m_logicalOrigin.x;

    if( aY )
        *aY = m_logicalOrigin.y;
}


void wxDC::SetDeviceOrigin( wxCoord aX, wxCoord aY )
{
    m_deviceOrigin = wxPoint( aX, aY );
    applyTransformCompat();
}


void wxDC::GetDeviceOrigin( wxCoord* aX, wxCoord* aY ) const
{
    if( aX )
        *aX = m_deviceOrigin.x;

    if( aY )
        *aY = m_deviceOrigin.y;
}


void wxDC::SetAxisOrientation( bool aXLeftRight, bool aYBottomUp )
{
    m_signX = aXLeftRight ? 1 : -1;
    m_signY = aYBottomUp ? -1 : 1;
    applyTransformCompat();
}


wxCoord wxDC::DeviceToLogicalX( wxCoord aX ) const
{
    return KiROUNDCompat( ( aX - m_deviceOrigin.x ) / ( m_userScaleX * m_signX ) ) + m_logicalOrigin.x;
}


wxCoord wxDC::DeviceToLogicalY( wxCoord aY ) const
{
    return KiROUNDCompat( ( aY - m_deviceOrigin.y ) / ( m_userScaleY * m_signY ) ) + m_logicalOrigin.y;
}


wxCoord wxDC::DeviceToLogicalXRel( wxCoord aX ) const
{
    return KiROUNDCompat( aX / m_userScaleX );
}


wxCoord wxDC::DeviceToLogicalYRel( wxCoord aY ) const
{
    return KiROUNDCompat( aY / m_userScaleY );
}


wxCoord wxDC::LogicalToDeviceX( wxCoord aX ) const
{
    return KiROUNDCompat( ( aX - m_logicalOrigin.x ) * m_userScaleX * m_signX ) + m_deviceOrigin.x;
}


wxCoord wxDC::LogicalToDeviceY( wxCoord aY ) const
{
    return KiROUNDCompat( ( aY - m_logicalOrigin.y ) * m_userScaleY * m_signY ) + m_deviceOrigin.y;
}


wxCoord wxDC::LogicalToDeviceXRel( wxCoord aX ) const
{
    return KiROUNDCompat( aX * m_userScaleX );
}


wxCoord wxDC::LogicalToDeviceYRel( wxCoord aY ) const
{
    return KiROUNDCompat( aY * m_userScaleY );
}


void wxDC::GetUserScale( double* aX, double* aY ) const
{
    if( aX )
        *aX = m_userScaleX;

    if( aY )
        *aY = m_userScaleY;
}


bool wxDC::Blit( wxCoord aXDest, wxCoord aYDest, wxCoord aWidth, wxCoord aHeight, wxDC* aSource,
                 wxCoord aXSrc, wxCoord aYSrc, wxRasterOperationMode aMode, bool aUseMask,
                 wxCoord aXSrcMask, wxCoord aYSrcMask )
{
    const QImage* image = aSource ? aSource->GetImageCompat() : nullptr;

    if( !image || !m_painter )
        return false;

    QPainter::CompositionMode oldMode = m_painter->compositionMode();

    switch( aMode )
    {
    case wxXOR: m_painter->setCompositionMode( QPainter::RasterOp_SourceXorDestination ); break;
    case wxAND: m_painter->setCompositionMode( QPainter::RasterOp_SourceAndDestination ); break;
    case wxOR:  m_painter->setCompositionMode( QPainter::RasterOp_SourceOrDestination ); break;
    default:
        // with the mask the transparent pixels are not copied
        m_painter->setCompositionMode( aUseMask ? QPainter::CompositionMode_SourceOver
                                                : QPainter::CompositionMode_Source );
        break;
    }

    m_painter->drawImage( QRect( aXDest, aYDest, aWidth, aHeight ), *image,
                          QRect( aXSrc, aYSrc, aWidth, aHeight ) );
    m_painter->setCompositionMode( oldMode );
    return true;
}


wxSize wxDC::GetPPI() const
{
    QPaintDevice* device = m_painter ? m_painter->device() : nullptr;

    if( !device )
        return wxSize( 96, 96 );

    return wxSize( device->logicalDpiX(), device->logicalDpiY() );
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


void wxRendererNative::DrawDropArrow( QWidget* aWindow, wxDC& aDC, const wxRect& aRect, int aFlags )
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


// The device contexts of the windows and of the bitmaps -------------------------------------

namespace
{
std::set<const QWidget*>& paintingWidgets()
{
    static std::set<const QWidget*> widgets;
    return widgets;
}


/// The widget painted for a window (the viewport of a scroll area).
QWidget* paintedWidget( QWidget* aWindow )
{
    if( QAbstractScrollArea* area = qobject_cast<QAbstractScrollArea*>( aWindow ) )
        return area->viewport();

    return aWindow;
}
} // namespace


void wxSetPaintingCompat( QWidget* aWidget, bool aPainting )
{
    if( aPainting )
        paintingWidgets().insert( aWidget );
    else
        paintingWidgets().erase( aWidget );
}


bool wxIsPaintingCompat( const QWidget* aWidget )
{
    return paintingWidgets().count( aWidget ) > 0;
}


wxPaintDC::wxPaintDC( const wxWindow* aWindow ) :
        wxDC( nullptr, aWindow ? aWindow->GetHandle() : nullptr )
{
    init( aWindow ? aWindow->GetHandle() : nullptr );
}


void wxPaintDC::init( QWidget* aWindow )
{
    QWidget* widget = aWindow ? paintedWidget( aWindow ) : nullptr;

    if( widget && wxIsPaintingCompat( widget ) )
    {
        m_ownPainter = std::make_unique<QPainter>( widget );
    }
    else
    {
        // Not painted now: an image of the size of the window, for the measures
        QSize size = widget ? widget->size().expandedTo( QSize( 1, 1 ) ) : QSize( 1, 1 );
        m_image = std::make_unique<QImage>( size, QImage::Format_ARGB32_Premultiplied );
        m_image->fill( Qt::transparent );
        m_ownPainter = std::make_unique<QPainter>( m_image.get() );
    }

    m_ownPainter->setRenderHint( QPainter::Antialiasing );
    SetPainterCompat( m_ownPainter.get() );
    SetFont( GetFont() );
}


wxPaintDC::~wxPaintDC()
{
    m_ownPainter.reset();
}


wxMemoryDC::wxMemoryDC() :
        wxDC( nullptr )
{
    SelectObjectAsSource( wxNullBitmap );
}


wxMemoryDC::wxMemoryDC( wxBitmap& aBitmap ) :
        wxDC( nullptr )
{
    SelectObject( aBitmap );
}


wxMemoryDC::~wxMemoryDC()
{
    release();
}


void wxMemoryDC::release()
{
    m_ownPainter.reset();

    if( m_bitmap )
        *m_bitmap = wxBitmap( QPixmap::fromImage( m_image ) );

    m_bitmap = nullptr;
}


wxBitmap& wxMemoryDC::GetSelectedBitmap()
{
    if( m_bitmap )
        return *m_bitmap;

    m_sourceCompat = wxBitmap( QPixmap::fromImage( m_image ) );
    return m_sourceCompat;
}


void wxMemoryDC::SelectObject( wxBitmap& aBitmap )
{
    SelectObjectAsSource( aBitmap );

    if( !aBitmap.isNull() )
        m_bitmap = &aBitmap;
}


void wxMemoryDC::SelectObjectAsSource( const wxBitmap& aBitmap )
{
    release();

    m_image = aBitmap.isNull() ? QImage( 1, 1, QImage::Format_ARGB32_Premultiplied )
                               : aBitmap.toImage().convertToFormat( QImage::Format_ARGB32_Premultiplied );

    if( aBitmap.isNull() )
        m_image.fill( Qt::transparent );

    m_ownPainter = std::make_unique<QPainter>( &m_image );
    m_ownPainter->setRenderHint( QPainter::Antialiasing );
    SetPainterCompat( m_ownPainter.get() );
    SetFont( GetFont() );
}
