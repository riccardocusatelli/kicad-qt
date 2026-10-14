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

#include <wx/kiwx_display.h>

#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

#include <qt_strings.h>
#include <wx/kiwx_window.h>


namespace
{
wxRect rectOf( const QRect& aRect )
{
    return wxRect( aRect.x(), aRect.y(), aRect.width(), aRect.height() );
}


QScreen* screenOf( const wxWindow* aWindow )
{
    if( !aWindow || !aWindow->GetHandle() )
        return nullptr;

    return aWindow->GetHandle()->screen();
}
} // namespace


wxDisplay::wxDisplay( unsigned int aIndex )
{
    QList<QScreen*> screens = QGuiApplication::screens();
    m_screen = aIndex < unsigned( screens.size() ) ? screens[int( aIndex )] : nullptr;
}


wxDisplay::wxDisplay( const wxWindow* aWindow ) :
        m_screen( screenOf( aWindow ) )
{
    if( !m_screen )
        m_screen = QGuiApplication::primaryScreen();
}


unsigned int wxDisplay::GetCount()
{
    return unsigned( QGuiApplication::screens().size() );
}


int wxDisplay::GetFromPoint( const wxPoint& aPoint )
{
    QScreen* screen = QGuiApplication::screenAt( QPoint( aPoint.x, aPoint.y ) );
    return screen ? int( QGuiApplication::screens().indexOf( screen ) ) : wxNOT_FOUND;
}


int wxDisplay::GetFromWindow( const wxWindow* aWindow )
{
    QScreen* screen = screenOf( aWindow );
    return screen ? int( QGuiApplication::screens().indexOf( screen ) ) : wxNOT_FOUND;
}


wxRect wxDisplay::GetGeometry() const
{
    return m_screen ? rectOf( m_screen->geometry() ) : wxRect();
}


wxRect wxDisplay::GetClientArea() const
{
    return m_screen ? rectOf( m_screen->availableGeometry() ) : wxRect();
}


wxString wxDisplay::GetName() const
{
    return m_screen ? ToWxString( m_screen->name() ) : wxString();
}


bool wxDisplay::IsPrimary() const
{
    return m_screen && m_screen == QGuiApplication::primaryScreen();
}


double wxDisplay::GetScaleFactor() const
{
    return m_screen ? m_screen->devicePixelRatio() : 1.0;
}


wxSize wxDisplay::GetPPI() const
{
    if( !m_screen )
        return wxSize( 96, 96 );

    return wxSize( int( m_screen->logicalDotsPerInchX() ), int( m_screen->logicalDotsPerInchY() ) );
}


wxVideoMode wxDisplay::GetCurrentMode() const
{
    if( !m_screen )
        return wxVideoMode();

    QSize size = m_screen->size();
    return wxVideoMode( size.width(), size.height(), m_screen->depth(),
                        int( m_screen->refreshRate() ) );
}


bool wxFontEnumerator::EnumerateFacenames( int aEncoding, bool aFixedWidthOnly )
{
    for( const wxString& name : GetFacenames( aEncoding, aFixedWidthOnly ) )
    {
        if( !OnFacename( name ) )
            break;
    }

    return true;
}


wxArrayString wxFontEnumerator::GetFacenames( int aEncoding, bool aFixedWidthOnly )
{
    wxArrayString names;

    for( const QString& family : QFontDatabase::families() )
    {
        if( !aFixedWidthOnly || QFontDatabase::isFixedPitch( family ) )
            names.Add( ToWxString( family ) );
    }

    return names;
}


bool wxFontEnumerator::IsValidFacename( const wxString& aFacename )
{
    return QFontDatabase::families().contains( ToQString( aFacename ), Qt::CaseInsensitive );
}
