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
#include <wx/kiwx_webview_backend.h>

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <qt_strings.h>


const wxEventType wxEVT_WEBVIEW_NAVIGATING = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_NAVIGATED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_LOADED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_ERROR = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_NEWWINDOW = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_TITLE_CHANGED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED = wxNewEventType();
const wxEventType wxEVT_WEBVIEW_SCRIPT_RESULT = wxNewEventType();


// The backends ------------------------------------------------------------------------------

namespace
{

/// A string as a literal of JavaScript (JSON).
QString jsonStringCompat( const QString& aText )
{
    QByteArray array = QJsonDocument( QJsonArray{ aText } ).toJson( QJsonDocument::Compact );
    return QString::fromUtf8( array.mid( 1, array.size() - 2 ) );
}

} // namespace


wxString KIWX_WEBVIEW_BACKEND::WrapScriptCompat( const wxString& aScript )
{
    QString script = QStringLiteral(
            "(function() {"
            " try {"
            "  var r = ( 0, eval )( %1 );"
            "  return [ true, ( r !== null && typeof r === 'object' ) ? JSON.stringify( r )"
            "                                                           : String( r ) ];"
            " } catch( e ) { return [ false, String( e ) ]; }"
            "})()" ).arg( jsonStringCompat( ToQString( aScript ) ) );

    return ToWxString( script );
}


wxString KIWX_WEBVIEW_BACKEND::MessageHandlerScriptCompat( const wxString& aName,
                                                           const wxString& aPostFunction )
{
    QString script = QStringLiteral(
            "(function() {"
            " var name = %1;"
            " window[name] = { postMessage: function( m ) {"
            "  %2( name, typeof m === 'string' ? m : JSON.stringify( m ) ); } };"
            "})();" ).arg( jsonStringCompat( ToQString( aName ) ), ToQString( aPostFunction ) );

    return ToWxString( script );
}


void KIWX_WEBVIEW_BACKEND::RunScriptAsync( const wxString& aScript, void* aClientData )
{
    // No scripts: the result is an error, later (as the result of a script)
    QPointer<QWidget> widget( Widget() );

    QTimer::singleShot( 0, Widget(),
                        [this, widget, aClientData]()
                        {
                            if( widget )
                                sendScriptResult( aClientData, false,
                                                  wxS( "the page runs no scripts" ) );
                        } );
}


bool KIWX_WEBVIEW_BACKEND::sendNavigating( const wxString& aUrl, bool aByUser )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_NAVIGATING, 0, aUrl, wxString(),
                          aByUser ? wxWEBVIEW_NAV_ACTION_USER : wxWEBVIEW_NAV_ACTION_OTHER );
    return m_view->SendEventCompat( event );
}


void KIWX_WEBVIEW_BACKEND::sendLoaded( const wxString& aUrl )
{
    wxWebViewEvent navigated( wxEVT_WEBVIEW_NAVIGATED, 0, aUrl );
    m_view->SendEventCompat( navigated );

    wxWebViewEvent loaded( wxEVT_WEBVIEW_LOADED, 0, aUrl );
    m_view->SendEventCompat( loaded );
}


void KIWX_WEBVIEW_BACKEND::sendError( const wxString& aUrl, const wxString& aDescription )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_ERROR, 0, aUrl );
    event.SetString( aDescription );
    event.SetInt( 1 );
    m_view->SendEventCompat( event );
}


void KIWX_WEBVIEW_BACKEND::sendNewWindow( const wxString& aUrl )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_NEWWINDOW, 0, aUrl, wxString(), wxWEBVIEW_NAV_ACTION_USER );
    m_view->SendEventCompat( event );
}


void KIWX_WEBVIEW_BACKEND::sendTitleChanged( const wxString& aTitle )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_TITLE_CHANGED, 0, GetCurrentURL() );
    event.SetString( aTitle );
    m_view->SendEventCompat( event );
}


void KIWX_WEBVIEW_BACKEND::sendScriptMessage( const wxString& aHandler, const wxString& aMessage )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, 0, GetCurrentURL(), wxString(),
                          wxWEBVIEW_NAV_ACTION_NONE, aHandler );
    event.SetString( aMessage );
    m_view->SendEventCompat( event );
}


void KIWX_WEBVIEW_BACKEND::sendScriptResult( void* aClientData, bool aOk, const wxString& aResult )
{
    wxWebViewEvent event( wxEVT_WEBVIEW_SCRIPT_RESULT, 0, GetCurrentURL() );
    event.SetString( aResult );
    event.SetInt( aOk ? 0 : 1 );
    event.SetClientData( aClientData );
    m_view->SendEventCompat( event );
}


namespace
{

/**
 * The backend without a web engine: a QTextBrowser shows the local HTML pages (rich text, no
 * scripts).  The links clicked send wxEVT_WEBVIEW_NAVIGATING; the remote ones open in the
 * browser of the system.
 */
class TEXT_BROWSER_BACKEND : public KIWX_WEBVIEW_BACKEND
{
public:
    explicit TEXT_BROWSER_BACKEND( wxWebView* aView ) :
            KIWX_WEBVIEW_BACKEND( aView ),
            m_browser( new QTextBrowser( aView ) )
    {
        m_browser->setOpenLinks( false );
        m_browser->setOpenExternalLinks( false );

        QObject::connect( m_browser, &QTextBrowser::anchorClicked, m_browser,
                          [this]( const QUrl& aLink )
                          {
                              QUrl     link = QUrl( ToQString( m_url ) ).resolved( aLink );
                              wxString url = ToWxString( link.toString() );

                              if( !sendNavigating( url, true ) )
                                  return;

                              if( aLink.scheme().isEmpty() && aLink.hasFragment()
                                  && aLink.path().isEmpty() )
                              {
                                  m_browser->scrollToAnchor( aLink.fragment() );
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
    }

    QWidget* Widget() override { return m_browser; }

    void LoadURL( const wxString& aUrl ) override
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
            sendError( aUrl, wxS( "the page can't be shown" ) );
            return;
        }

        wxString html = ToWxString( QString::fromUtf8( file.readAll() ) );
        m_view->SetPageSourceCompat( html );

        QUrl base = QUrl::fromLocalFile( QFileInfo( path ).absoluteFilePath() );
        m_browser->document()->setBaseUrl( base );
        m_browser->setSearchPaths( { QFileInfo( path ).absolutePath() } );
        m_browser->setHtml( ToQString( html ) );
        loadedLater();
    }

    void SetPage( const wxString& aHtml, const wxString& aBaseUrl ) override
    {
        m_url = aBaseUrl;

        if( !aBaseUrl.IsEmpty() )
            m_browser->document()->setBaseUrl( QUrl( ToQString( aBaseUrl ) ) );

        m_browser->setHtml( ToQString( aHtml ) );
        loadedLater();
    }

    wxString GetCurrentURL() const override { return m_url; }
    wxString GetCurrentTitle() const override { return ToWxString( m_browser->documentTitle() ); }
    void Reload() override { LoadURL( m_url ); }

    void SetZoomFactor( float aZoom ) override
    {
        if( m_zoom > 0 && aZoom > 0 )
        {
            QFont font = m_browser->font();
            font.setPointSizeF( font.pointSizeF() * aZoom / m_zoom );
            m_browser->setFont( font );
        }

        m_zoom = aZoom;
    }

private:
    /// The page is shown: wxEVT_WEBVIEW_LOADED later, as the pages of wxWidgets load.
    void loadedLater()
    {
        QPointer<QTextBrowser> browser( m_browser );
        wxString               url = m_url;

        QTimer::singleShot( 0, m_browser,
                            [this, browser, url]()
                            {
                                if( browser )
                                    sendLoaded( url );
                            } );
    }

    QTextBrowser* m_browser;
    wxString      m_url;
    float         m_zoom = 1.0f;
};

} // namespace


#if !defined( KIWX_WEBVIEW_QTWEBENGINE ) && !defined( KIWX_WEBVIEW_WKWEBVIEW ) \
        && !defined( KIWX_WEBVIEW_WEBVIEW2 )
std::unique_ptr<KIWX_WEBVIEW_BACKEND> KiwxCreateWebEngineBackend( wxWebView* aView )
{
    return nullptr;
}
#endif


// The view ---------------------------------------------------------------------------------

wxWebView::wxWebView( wxWidgetArgCompat aParent, int aId, const wxString& aUrl,
                      const wxPoint& aPos, const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QWidget>( aParent )
{
    SetId( aId );

    m_backend = KiwxCreateWebEngineBackend( this );

    if( !m_backend )
        m_backend = std::make_unique<TEXT_BROWSER_BACKEND>( this );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_backend->Widget() );

    if( aSize.IsFullySpecified() )
        resize( aSize.x, aSize.y );

    if( !aUrl.IsEmpty() )
        LoadURL( aUrl );
}


wxWebView::~wxWebView()
{
    // The backend before the widgets (its widget is a child of the view)
    m_backend.reset();
}


bool wxWebView::HasWebEngineCompat()
{
#if defined( KIWX_WEBVIEW_QTWEBENGINE ) || defined( KIWX_WEBVIEW_WKWEBVIEW ) \
        || defined( KIWX_WEBVIEW_WEBVIEW2 )
    return true;
#else
    return false;
#endif
}


bool wxWebView::SendEventCompat( wxWebViewEvent& aEvent )
{
    aEvent.SetId( GetId() );
    aEvent.SetEventObject( this );

    for( wxWindow* window = this; window; window = window->GetParent() )
    {
        if( window->ProcessEvent( aEvent ) || window->IsTopLevel() )
            break;
    }

    return aEvent.IsAllowed();
}


void wxWebView::LoadURL( const wxString& aUrl )
{
    m_backend->LoadURL( aUrl );
}


void wxWebView::SetPage( const wxString& aHtml, const wxString& aBaseUrl )
{
    m_html = aHtml;
    m_backend->SetPage( aHtml, aBaseUrl );
}


wxString wxWebView::GetCurrentURL() const
{
    return m_backend->GetCurrentURL();
}


wxString wxWebView::GetCurrentTitle() const
{
    return m_backend->GetCurrentTitle();
}


void wxWebView::Reload( wxWebViewReloadFlags )
{
    m_backend->Reload();
}


bool wxWebView::IsBusy() const
{
    return m_backend->IsBusy();
}


void wxWebView::Stop()
{
    m_backend->Stop();
}


bool wxWebView::RunScript( const wxString& aScript, wxString* aOutput ) const
{
    return m_backend->RunScript( aScript, aOutput );
}


void wxWebView::RunScriptAsync( const wxString& aScript, void* aClientData ) const
{
    m_backend->RunScriptAsync( aScript, aClientData );
}


bool wxWebView::AddScriptMessageHandler( const wxString& aName )
{
    return m_backend->AddScriptMessageHandler( aName );
}


bool wxWebView::RemoveScriptMessageHandler( const wxString& aName )
{
    return m_backend->RemoveScriptMessageHandler( aName );
}


bool wxWebView::AddUserScript( const wxString& aScript,
                               wxWebViewUserScriptInjectionTime aInjectionTime )
{
    return m_backend->AddUserScript( aScript,
                                     aInjectionTime == wxWEBVIEW_INJECT_AT_DOCUMENT_START );
}


void wxWebView::RemoveAllUserScripts()
{
    m_backend->RemoveAllUserScripts();
}


void wxWebView::EnableContextMenu( bool aEnable )
{
    m_backend->EnableContextMenu( aEnable );
}


void wxWebView::EnableAccessToDevTools( bool aEnable )
{
    m_backend->EnableAccessToDevTools( aEnable );
}


bool wxWebView::SetUserAgent( const wxString& aUserAgent )
{
    return m_backend->SetUserAgent( aUserAgent );
}


void wxWebView::SetZoomFactor( float aZoom )
{
    m_zoom = aZoom;
    m_backend->SetZoomFactor( aZoom );
}
