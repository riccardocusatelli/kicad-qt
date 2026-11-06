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

#include <wx/webview.h>

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <qt_strings.h>


const wxEventType wxEVT_WEBVIEW_NAVIGATING = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_NAVIGATED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_LOADED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_ERROR = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_NEWWINDOW = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_TITLE_CHANGED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_SCRIPT_RESULT = wxNewEventType();


wxWebView::wxWebView( wxWidgetArgCompat aParent, int aId, const wxString& aUrl,
                      const wxPoint& aPos, const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QTextBrowser>( aParent )
{
    SetId( aId );
    setOpenLinks( false );
    setOpenExternalLinks( false );

    // A link clicked: wxEVT_WEBVIEW_NAVIGATING, which a handler can veto; else the page is
    // shown (a local file) or the URL opens in the browser of the system
    QObject::connect( this, &QTextBrowser::anchorClicked, this,
                      [this]( const QUrl& aLink )
                      {
                          QUrl     link = QUrl( ToQString( m_url ) ).resolved( aLink );
                          wxString url = ToWxString( link.toString() );

                          wxWebViewEvent event( wxEVT_WEBVIEW_NAVIGATING, GetId(), url,
                                                wxString(), wxWEBVIEW_NAV_ACTION_USER );
                          event.SetEventObject( this );
                          ProcessEvent( event );

                          if( !event.IsAllowed() )
                              return;

                          if( aLink.scheme().isEmpty() && aLink.hasFragment()
                              && aLink.path().isEmpty() )
                          {
                              scrollToAnchor( aLink.fragment() );
                          }
                          else if( link.isLocalFile() )
                          {
                              LoadURL( url );
                          }
                          else
                          {
                              QDesktopServices::openUrl( link );
                          }
                      } );

    if( !aUrl.IsEmpty() )
        LoadURL( aUrl );
}


void wxWebView::LoadURL( const wxString& aUrl )
{
    QString url = ToQString( aUrl );
    QUrl    qurl( url );
    QString path;

    if( qurl.isLocalFile() )
        path = qurl.toLocalFile();
    else if( QFileInfo::exists( url ) )
        path = url;

    m_url = aUrl;

    QFile file( path );

    if( path.isEmpty() || !file.open( QIODevice::ReadOnly ) )
    {
        // No remote pages: an error, as a page not found
        wxWebViewEvent event( wxEVT_WEBVIEW_ERROR, GetId(), aUrl );
        event.SetEventObject( this );
        event.SetInt( 1 );
        ProcessEvent( event );
        return;
    }

    m_html = ToWxString( QString::fromUtf8( file.readAll() ) );

    QUrl base = QUrl::fromLocalFile( QFileInfo( path ).absoluteFilePath() );
    document()->setBaseUrl( base );
    setSearchPaths( { QFileInfo( path ).absolutePath() } );
    setHtml( ToQString( m_html ) );
    sendLoaded();
}


void wxWebView::SetPage( const wxString& aHtml, const wxString& aBaseUrl )
{
    m_html = aHtml;
    m_url = aBaseUrl;

    if( !aBaseUrl.IsEmpty() )
        document()->setBaseUrl( QUrl( ToQString( aBaseUrl ) ) );

    setHtml( ToQString( aHtml ) );
    sendLoaded();
}


void wxWebView::sendLoaded()
{
    QPointer<QTextBrowser> self( this );
    wxString               url = m_url;

    // Later, as the pages of wxWidgets, which load asynchronously
    QTimer::singleShot( 0, this,
                        [this, self, url]()
                        {
                            if( !self )
                                return;

                            wxWebViewEvent navigated( wxEVT_WEBVIEW_NAVIGATED, GetId(), url );
                            navigated.SetEventObject( this );
                            ProcessEvent( navigated );

                            wxWebViewEvent loaded( wxEVT_WEBVIEW_LOADED, GetId(), url );
                            loaded.SetEventObject( this );
                            ProcessEvent( loaded );
                        } );
}
