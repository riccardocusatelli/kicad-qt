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
 * @file kiwx_webview_wkwebview.mm
 * The backend of the web view on macOS: a WKWebView (the web engine of the system), in a
 * QWindowContainer.  The messages of the pages come from the handlers of its
 * WKUserContentController (window.webkit.messageHandlers); the cookies are the ones of a data
 * store of the view (not persistent: KIPLATFORM::WEBVIEW saves them).
 *
 * Compiled with ARC (-fobjc-arc).
 */

#include <wx/webview.h>
#include <wx/kiwx_webview_backend.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QUrl>
#include <QWidget>
#include <QWindow>
#include <qt_strings.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

#import <WebKit/WebKit.h>


class KIWX_WKWEBVIEW_BACKEND;


/// The delegate of the WKWebView: its navigations, new windows and script messages.
@interface KiwxWKWebViewDelegate : NSObject <WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>
@property( nonatomic, assign ) KIWX_WKWEBVIEW_BACKEND* backend;
@end


namespace
{

/// The function of the pages posting a message to the view.
const char POST_FUNCTION[] =
        "(function( n, m ) { window.webkit.messageHandlers[n].postMessage( m ); })";


/// Process the events until @a aDone (at most @a aMilliseconds).
bool waitForCompat( const std::function<bool()>& aDone, int aMilliseconds )
{
    QElapsedTimer timer;
    timer.start();

    while( !aDone() && timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20 );

    return aDone();
}


NSString* toNS( const wxString& aText )
{
    return ToQString( aText ).toNSString();
}


wxString fromNS( NSString* aText )
{
    return aText ? ToWxString( QString::fromNSString( aText ) ) : wxString();
}


/// The result of the wrapped script ([ ok, string ]).
void scriptResultCompat( id aResult, NSError* aError, bool& aOk, wxString& aText )
{
    aOk = false;

    if( aError )
    {
        aText = fromNS( aError.localizedDescription );
        return;
    }

    if( [aResult isKindOfClass:[NSArray class]] && [(NSArray*) aResult count] == 2 )
    {
        NSArray* list = (NSArray*) aResult;
        aOk = [list[0] boolValue];
        aText = [list[1] isKindOfClass:[NSString class]] ? fromNS( list[1] )
                                                          : fromNS( [list[1] description] );
        return;
    }

    aText = wxS( "the script gave no result" );
}

} // namespace


class KIWX_WKWEBVIEW_BACKEND : public KIWX_WEBVIEW_BACKEND
{
public:
    explicit KIWX_WKWEBVIEW_BACKEND( wxWebView* aView ) :
            KIWX_WEBVIEW_BACKEND( aView )
    {
        m_delegate = [[KiwxWKWebViewDelegate alloc] init];
        m_delegate.backend = this;

        WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
        config.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];

        m_webView = [[WKWebView alloc] initWithFrame:NSMakeRect( 0, 0, 400, 300 )
                                       configuration:config];
        m_webView.navigationDelegate = m_delegate;
        m_webView.UIDelegate = m_delegate;

        m_window = QWindow::fromWinId( reinterpret_cast<WId>( (__bridge void*) m_webView ) );
        m_widget = QWidget::createWindowContainer( m_window, aView );
        m_widget->setMinimumSize( 1, 1 );
    }

    ~KIWX_WKWEBVIEW_BACKEND() override
    {
        m_delegate.backend = nullptr;

        [m_webView stopLoading];
        m_webView.navigationDelegate = nil;
        m_webView.UIDelegate = nil;

        WKUserContentController* scripts = m_webView.configuration.userContentController;

        for( const auto& [name, source] : m_handlers )
            [scripts removeScriptMessageHandlerForName:toNS( name )];

        // The container deletes the window of the view
        delete m_widget.data();
        m_webView = nil;
        m_delegate = nil;
    }

    QWidget* Widget() override { return m_widget; }
    bool HasScripts() const override { return true; }

    void LoadURL( const wxString& aUrl ) override
    {
        QString path = ToQString( aUrl );
        QUrl    qurl = QFileInfo::exists( path ) ? QUrl::fromLocalFile( path )
                                                 : QUrl::fromUserInput( path );

        // As wxWidgets: the navigation of the code goes to the handlers too
        if( !sendNavigating( ToWxString( qurl.toString() ), false ) )
            return;

        m_expectLoad = true;
        NSURL* url = qurl.toNSURL();

        if( url.isFileURL )
        {
            [m_webView loadFileURL:url
                allowingReadAccessToURL:[url URLByDeletingLastPathComponent]];
        }
        else
        {
            [m_webView loadRequest:[NSURLRequest requestWithURL:url]];
        }
    }

    void SetPage( const wxString& aHtml, const wxString& aBaseUrl ) override
    {
        NSURL* base = nil;

        if( aBaseUrl == wxS( "file://" ) )
            base = [NSURL fileURLWithPath:@"/"];
        else if( !aBaseUrl.IsEmpty() )
            base = [NSURL URLWithString:toNS( aBaseUrl )];

        m_expectLoad = true;
        [m_webView loadHTMLString:toNS( aHtml ) baseURL:base];
    }

    wxString GetCurrentURL() const override { return fromNS( m_webView.URL.absoluteString ); }
    wxString GetCurrentTitle() const override { return fromNS( m_webView.title ); }

    void Reload() override { [m_webView reload]; }
    bool IsBusy() const override { return m_webView.isLoading; }
    void Stop() override { [m_webView stopLoading]; }

    void RunScriptAsync( const wxString& aScript, void* aClientData ) override
    {
        QPointer<QWidget>       widget( m_widget );
        KIWX_WKWEBVIEW_BACKEND* backend = this;

        [m_webView evaluateJavaScript:toNS( WrapScriptCompat( aScript ) )
                    completionHandler:^( id aResult, NSError* aError ) {
                        if( !widget )
                            return;

                        bool     ok = false;
                        wxString text;
                        scriptResultCompat( aResult, aError, ok, text );
                        backend->scriptResult( aClientData, ok, text );
                    }];
    }

    bool RunScript( const wxString& aScript, wxString* aOutput ) override
    {
        // Shared with the completion, which can come after the timeout
        struct RESULT
        {
            bool     done = false;
            bool     ok = false;
            wxString text;
        };

        auto result = std::make_shared<RESULT>();

        [m_webView evaluateJavaScript:toNS( WrapScriptCompat( aScript ) )
                    completionHandler:^( id aResult, NSError* aError ) {
                        scriptResultCompat( aResult, aError, result->ok, result->text );
                        result->done = true;
                    }];

        waitForCompat( [result]() { return result->done; }, 5000 );

        if( aOutput )
            *aOutput = result->text;

        return result->done && result->ok;
    }

    bool AddScriptMessageHandler( const wxString& aName ) override
    {
        if( m_handlers.count( aName ) )
            return true;

        wxString source = MessageHandlerScriptCompat( aName, wxString::FromUTF8( POST_FUNCTION ) );
        WKUserContentController* scripts = m_webView.configuration.userContentController;

        [scripts addScriptMessageHandler:m_delegate name:toNS( aName )];
        [scripts addUserScript:[[WKUserScript alloc]
                                         initWithSource:toNS( source )
                                          injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                       forMainFrameOnly:YES]];
        m_handlers[aName] = source;

        // The page shown has it too
        [m_webView evaluateJavaScript:toNS( source ) completionHandler:nil];
        return true;
    }

    bool RemoveScriptMessageHandler( const wxString& aName ) override
    {
        if( !m_handlers.erase( aName ) )
            return false;

        [m_webView.configuration.userContentController
                removeScriptMessageHandlerForName:toNS( aName )];
        resetUserScripts();
        return true;
    }

    bool AddUserScript( const wxString& aScript, bool aAtDocumentStart ) override
    {
        m_userScripts.emplace_back( aScript, aAtDocumentStart );
        addUserScript( aScript, aAtDocumentStart );
        return true;
    }

    void RemoveAllUserScripts() override
    {
        m_userScripts.clear();
        resetUserScripts();
    }

    bool SetUserAgent( const wxString& aUserAgent ) override
    {
        m_webView.customUserAgent = toNS( aUserAgent );
        return true;
    }

    void SetZoomFactor( float aZoom ) override
    {
        if( @available( macOS 11.0, * ) )
            m_webView.pageZoom = aZoom;
        else
            m_webView.magnification = aZoom;
    }

    void EnableAccessToDevTools( bool aEnable ) override
    {
        if( @available( macOS 13.3, * ) )
            m_webView.inspectable = aEnable;
    }

    bool SaveCookies( const wxString& aFile ) override
    {
        struct RESULT
        {
            bool       done = false;
            QJsonArray list;
        };

        auto result = std::make_shared<RESULT>();

        [m_webView.configuration.websiteDataStore.httpCookieStore
                getAllCookies:^( NSArray<NSHTTPCookie*>* aCookies ) {
                    for( NSHTTPCookie* cookie in aCookies )
                    {
                        QJsonObject item;
                        item[QStringLiteral( "name" )] = QString::fromNSString( cookie.name );
                        item[QStringLiteral( "value" )] = QString::fromNSString( cookie.value );
                        item[QStringLiteral( "domain" )] = QString::fromNSString( cookie.domain );
                        item[QStringLiteral( "path" )] = QString::fromNSString( cookie.path );
                        item[QStringLiteral( "secure" )] = bool( cookie.isSecure );
                        item[QStringLiteral( "httpOnly" )] = bool( cookie.isHTTPOnly );
                        item[QStringLiteral( "expires" )] =
                                cookie.expiresDate
                                        ? double( [cookie.expiresDate timeIntervalSince1970] )
                                        : 0.0;
                        result->list.append( item );
                    }

                    result->done = true;
                }];

        if( !waitForCompat( [result]() { return result->done; }, 5000 ) )
            return false;

        const QJsonArray cookies = result->list;

        QFile file( ToQString( aFile ) );

        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
            return false;

        return file.write( QJsonDocument( cookies ).toJson() ) >= 0;
    }

    bool LoadCookies( const wxString& aFile ) override
    {
        QFile file( ToQString( aFile ) );

        if( !file.open( QIODevice::ReadOnly ) )
            return false;

        const QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );

        if( !doc.isArray() )
            return false;

        WKHTTPCookieStore* store = m_webView.configuration.websiteDataStore.httpCookieStore;
        auto               pending = std::make_shared<int>( 0 );

        for( const QJsonValue& value : doc.array() )
        {
            const QJsonObject    item = value.toObject();
            NSMutableDictionary* properties = [NSMutableDictionary dictionary];

            properties[NSHTTPCookieName] = item[QStringLiteral( "name" )].toString().toNSString();
            properties[NSHTTPCookieValue] =
                    item[QStringLiteral( "value" )].toString().toNSString();
            properties[NSHTTPCookieDomain] =
                    item[QStringLiteral( "domain" )].toString().toNSString();
            properties[NSHTTPCookiePath] =
                    item[QStringLiteral( "path" )].toString( QStringLiteral( "/" ) ).toNSString();

            const double expires = item[QStringLiteral( "expires" )].toDouble();

            if( expires > 0 )
                properties[NSHTTPCookieExpires] = [NSDate dateWithTimeIntervalSince1970:expires];

            if( item[QStringLiteral( "secure" )].toBool() )
                properties[NSHTTPCookieSecure] = @"TRUE";

            if( NSHTTPCookie* cookie = [NSHTTPCookie cookieWithProperties:properties] )
            {
                ++*pending;
                [store setCookie:cookie completionHandler:^{ --*pending; }];
            }
        }

        waitForCompat( [pending]() { return *pending <= 0; }, 5000 );
        return true;
    }

    bool DeleteCookies() override
    {
        auto done = std::make_shared<bool>( false );

        [m_webView.configuration.websiteDataStore
                removeDataOfTypes:[NSSet setWithObject:WKWebsiteDataTypeCookies]
                    modifiedSince:[NSDate distantPast]
                completionHandler:^{ *done = true; }];

        return waitForCompat( [done]() { return *done; }, 5000 );
    }

    // The events of the delegate

    bool OnNavigating( NSURL* aUrl, bool aByUser )
    {
        // The navigation of LoadURL() and SetPage() was sent already
        if( m_expectLoad )
        {
            m_expectLoad = false;
            return true;
        }

        return sendNavigating( fromNS( aUrl.absoluteString ), aByUser );
    }

    void OnNewWindow( NSURL* aUrl ) { sendNewWindow( fromNS( aUrl.absoluteString ) ); }

    void OnLoaded()
    {
        m_expectLoad = false;
        sendLoaded( GetCurrentURL() );

        wxString title = GetCurrentTitle();

        if( title != m_title )
        {
            m_title = title;
            sendTitleChanged( title );
        }
    }

    void OnError( NSError* aError )
    {
        m_expectLoad = false;

        // A navigation refused by the handlers is no error
        if( aError.code == NSURLErrorCancelled )
            return;

        sendError( GetCurrentURL(), fromNS( aError.localizedDescription ) );
    }

    void OnScriptMessage( WKScriptMessage* aMessage )
    {
        if( !m_handlers.count( fromNS( aMessage.name ) ) )
            return;

        id       body = aMessage.body;
        wxString text;

        if( [body isKindOfClass:[NSString class]] )
        {
            text = fromNS( (NSString*) body );
        }
        else if( body && [NSJSONSerialization isValidJSONObject:body] )
        {
            NSData* data = [NSJSONSerialization dataWithJSONObject:body options:0 error:nil];
            text = wxString::FromUTF8( (const char*) data.bytes, data.length );
        }
        else if( body )
        {
            text = fromNS( [body description] );
        }

        sendScriptMessage( fromNS( aMessage.name ), text );
    }

    void scriptResult( void* aClientData, bool aOk, const wxString& aText )
    {
        sendScriptResult( aClientData, aOk, aText );
    }

private:
    void addUserScript( const wxString& aScript, bool aAtDocumentStart )
    {
        WKUserScriptInjectionTime when = aAtDocumentStart ? WKUserScriptInjectionTimeAtDocumentStart
                                                          : WKUserScriptInjectionTimeAtDocumentEnd;

        [m_webView.configuration.userContentController
                addUserScript:[[WKUserScript alloc] initWithSource:toNS( aScript )
                                                     injectionTime:when
                                                  forMainFrameOnly:YES]];
    }

    /// The user scripts again (WebKit removes them all at once): the handlers, then the others.
    void resetUserScripts()
    {
        [m_webView.configuration.userContentController removeAllUserScripts];

        for( const auto& [name, source] : m_handlers )
            addUserScript( source, true );

        for( const auto& [script, atStart] : m_userScripts )
            addUserScript( script, atStart );
    }

    WKWebView*                               m_webView = nil;
    KiwxWKWebViewDelegate*                   m_delegate = nil;
    QWindow*                                 m_window = nullptr;
    QPointer<QWidget>                        m_widget;
    std::map<wxString, wxString>             m_handlers;
    std::vector<std::pair<wxString, bool>>   m_userScripts;
    wxString                                 m_title;
    bool                                     m_expectLoad = false;
};


@implementation KiwxWKWebViewDelegate

- (void)webView:(WKWebView*)aWebView
        decidePolicyForNavigationAction:(WKNavigationAction*)aAction
                        decisionHandler:(void ( ^ )( WKNavigationActionPolicy ))aHandler
{
    // The frames, and the new windows (createWebViewWithConfiguration below)
    if( !self.backend || !aAction.targetFrame || !aAction.targetFrame.isMainFrame )
    {
        aHandler( WKNavigationActionPolicyAllow );
        return;
    }

    bool allowed = self.backend->OnNavigating(
            aAction.request.URL, aAction.navigationType == WKNavigationTypeLinkActivated );
    aHandler( allowed ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel );
}

- (WKWebView*)webView:(WKWebView*)aWebView
        createWebViewWithConfiguration:(WKWebViewConfiguration*)aConfiguration
                   forNavigationAction:(WKNavigationAction*)aAction
                        windowFeatures:(WKWindowFeatures*)aFeatures
{
    // No new window: wxEVT_WEBVIEW_NEWWINDOW
    if( self.backend )
        self.backend->OnNewWindow( aAction.request.URL );

    return nil;
}

- (void)webView:(WKWebView*)aWebView didFinishNavigation:(WKNavigation*)aNavigation
{
    if( self.backend )
        self.backend->OnLoaded();
}

- (void)webView:(WKWebView*)aWebView
        didFailNavigation:(WKNavigation*)aNavigation
                withError:(NSError*)aError
{
    if( self.backend )
        self.backend->OnError( aError );
}

- (void)webView:(WKWebView*)aWebView
        didFailProvisionalNavigation:(WKNavigation*)aNavigation
                           withError:(NSError*)aError
{
    if( self.backend )
        self.backend->OnError( aError );
}

- (void)userContentController:(WKUserContentController*)aController
      didReceiveScriptMessage:(WKScriptMessage*)aMessage
{
    if( self.backend )
        self.backend->OnScriptMessage( aMessage );
}

@end


std::unique_ptr<KIWX_WEBVIEW_BACKEND> KiwxCreateWebEngineBackend( wxWebView* aView )
{
    return std::make_unique<KIWX_WKWEBVIEW_BACKEND>( aView );
}
