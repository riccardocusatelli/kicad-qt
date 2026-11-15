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
 * @file kiwx_webview_webview2.cpp
 * The backend of the web view on Windows: WebView2 (Microsoft Edge, the web engine of the
 * system: its runtime comes with Windows), in a native child widget.  The pages post their
 * messages with window.chrome.webview.postMessage(); the environment is made asynchronously, the
 * calls made before it is ready wait for it.
 */

#include <wx/webview.h>
#include <wx/kiwx_webview_backend.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QUrl>
#include <QWidget>
#include <qt_strings.h>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wrl.h>
#include <WebView2.h>


using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;


namespace
{

/// The function of the pages posting a message to the view.
const char POST_FUNCTION[] =
        "(function( n, m ) { window.chrome.webview.postMessage( JSON.stringify( [ n, m ] ) ); })";


/// Process the events until @a aDone (at most @a aMilliseconds).
bool waitForCompat( const std::function<bool()>& aDone, int aMilliseconds )
{
    QElapsedTimer timer;
    timer.start();

    while( !aDone() && timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20 );

    return aDone();
}


std::wstring toW( const QString& aText )
{
    return aText.toStdWString();
}


std::wstring toW( const wxString& aText )
{
    return ToQString( aText ).toStdWString();
}


/// A string of WebView2 (freed with CoTaskMemFree()).
QString takeString( LPWSTR aText )
{
    QString text = aText ? QString::fromWCharArray( aText ) : QString();

    if( aText )
        CoTaskMemFree( aText );

    return text;
}


/// The result of the wrapped script: its JSON ([ ok, string ]).
void scriptResultCompat( HRESULT aError, LPCWSTR aJson, bool& aOk, wxString& aText )
{
    aOk = false;

    if( FAILED( aError ) || !aJson )
    {
        aText = wxS( "the script can't be run" );
        return;
    }

    QJsonArray list = QJsonDocument::fromJson( QString::fromWCharArray( aJson ).toUtf8() ).array();

    if( list.size() == 2 )
    {
        aOk = list[0].toBool();
        aText = ToWxString( list[1].toString() );
    }
    else
    {
        aText = wxS( "the script gave no result" );
    }
}


/// The native widget of WebView2: it gives its size to the controller.
class HOST_WIDGET : public QWidget
{
public:
    explicit HOST_WIDGET( QWidget* aParent ) : QWidget( aParent )
    {
        setAttribute( Qt::WA_NativeWindow );
        setAttribute( Qt::WA_DontCreateNativeAncestors );
        setMinimumSize( 1, 1 );
    }

    std::function<void()> m_resized;
    std::function<void( bool )> m_shown;

protected:
    void resizeEvent( QResizeEvent* aEvent ) override
    {
        QWidget::resizeEvent( aEvent );

        if( m_resized )
            m_resized();
    }

    void showEvent( QShowEvent* aEvent ) override
    {
        QWidget::showEvent( aEvent );

        if( m_shown )
            m_shown( true );
    }

    void hideEvent( QHideEvent* aEvent ) override
    {
        QWidget::hideEvent( aEvent );

        if( m_shown )
            m_shown( false );
    }
};


class WEBVIEW2_BACKEND : public KIWX_WEBVIEW_BACKEND
{
public:
    explicit WEBVIEW2_BACKEND( wxWebView* aView ) :
            KIWX_WEBVIEW_BACKEND( aView ),
            m_widget( new HOST_WIDGET( aView ) )
    {
        m_widget->m_resized = [this]() { updateBounds(); };
        m_widget->m_shown = [this]( bool aShown )
        {
            if( m_controller )
                m_controller->put_IsVisible( aShown ? TRUE : FALSE );
        };

        create();
    }

    ~WEBVIEW2_BACKEND() override
    {
        m_widget->m_resized = nullptr;
        m_widget->m_shown = nullptr;

        if( m_controller )
            m_controller->Close();

        m_webView.Reset();
        m_controller.Reset();
        delete m_widget.data();
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

        std::wstring url = toW( qurl.toString() );

        whenReady( [this, url]()
                   {
                       m_expectLoad = true;
                       m_webView->Navigate( url.c_str() );
                   } );
    }

    void SetPage( const wxString& aHtml, const wxString& aBaseUrl ) override
    {
        std::wstring html = toW( aHtml );

        whenReady( [this, html]()
                   {
                       m_expectLoad = true;
                       m_webView->NavigateToString( html.c_str() );
                   } );
    }

    wxString GetCurrentURL() const override
    {
        LPWSTR url = nullptr;

        if( m_webView && SUCCEEDED( m_webView->get_Source( &url ) ) )
            return ToWxString( takeString( url ) );

        return wxString();
    }

    wxString GetCurrentTitle() const override
    {
        LPWSTR title = nullptr;

        if( m_webView && SUCCEEDED( m_webView->get_DocumentTitle( &title ) ) )
            return ToWxString( takeString( title ) );

        return wxString();
    }

    void Reload() override
    {
        whenReady( [this]() { m_webView->Reload(); } );
    }

    bool IsBusy() const override { return m_busy || !m_webView; }

    void Stop() override
    {
        if( m_webView )
            m_webView->Stop();
    }

    void RunScriptAsync( const wxString& aScript, void* aClientData ) override
    {
        std::wstring      script = toW( WrapScriptCompat( aScript ) );
        QPointer<QWidget> widget( m_widget );

        whenReady( [this, widget, script, aClientData]()
                   {
                       m_webView->ExecuteScript(
                               script.c_str(),
                               Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                                       [this, widget, aClientData]( HRESULT aError,
                                                                    LPCWSTR aJson ) -> HRESULT
                                       {
                                           if( !widget )
                                               return S_OK;

                                           bool     ok = false;
                                           wxString text;
                                           scriptResultCompat( aError, aJson, ok, text );
                                           sendScriptResult( aClientData, ok, text );
                                           return S_OK;
                                       } )
                                       .Get() );
                   } );
    }

    bool RunScript( const wxString& aScript, wxString* aOutput ) override
    {
        if( !waitForCompat( [this]() { return m_webView || m_failed; }, 10000 ) || !m_webView )
            return false;

        // Shared with the completion, which can come after the timeout
        struct RESULT
        {
            bool     done = false;
            bool     ok = false;
            wxString text;
        };

        auto result = std::make_shared<RESULT>();

        m_webView->ExecuteScript(
                toW( WrapScriptCompat( aScript ) ).c_str(),
                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [result]( HRESULT aError, LPCWSTR aJson ) -> HRESULT
                        {
                            scriptResultCompat( aError, aJson, result->ok, result->text );
                            result->done = true;
                            return S_OK;
                        } )
                        .Get() );

        waitForCompat( [result]() { return result->done; }, 5000 );

        if( aOutput )
            *aOutput = result->text;

        return result->done && result->ok;
    }

    bool AddScriptMessageHandler( const wxString& aName ) override
    {
        if( m_handlers.count( aName ) )
            return true;

        m_handlers.insert( aName );

        std::wstring source =
                toW( MessageHandlerScriptCompat( aName, wxString::FromUTF8( POST_FUNCTION ) ) );

        addDocumentScript( wxS( "handler:" ) + aName, source, true );
        return true;
    }

    bool RemoveScriptMessageHandler( const wxString& aName ) override
    {
        if( !m_handlers.erase( aName ) )
            return false;

        removeDocumentScript( wxS( "handler:" ) + aName );
        return true;
    }

    bool AddUserScript( const wxString& aScript, bool aAtDocumentStart ) override
    {
        QString source = ToQString( aScript );

        if( !aAtDocumentStart )
        {
            source = QStringLiteral( "document.addEventListener( 'DOMContentLoaded', function() {\n" )
                     + source + QStringLiteral( "\n} );" );
        }

        addDocumentScript( wxString::Format( wxS( "user:%d" ), m_userScriptCount++ ),
                           toW( source ), false );
        return true;
    }

    void RemoveAllUserScripts() override
    {
        std::vector<wxString> keys;

        for( const auto& [key, id] : m_scriptIds )
        {
            if( key.StartsWith( wxS( "user:" ) ) )
                keys.push_back( key );
        }

        for( const wxString& key : keys )
            removeDocumentScript( key );
    }

    bool SetUserAgent( const wxString& aUserAgent ) override
    {
        std::wstring agent = toW( aUserAgent );

        whenReady( [this, agent]()
                   {
                       ComPtr<ICoreWebView2Settings>  settings;
                       ComPtr<ICoreWebView2Settings2> settings2;

                       if( SUCCEEDED( m_webView->get_Settings( &settings ) )
                           && SUCCEEDED( settings.As( &settings2 ) ) )
                       {
                           settings2->put_UserAgent( agent.c_str() );
                       }
                   } );
        return true;
    }

    void SetZoomFactor( float aZoom ) override
    {
        whenReady( [this, aZoom]() { m_controller->put_ZoomFactor( aZoom ); } );
    }

    void EnableContextMenu( bool aEnable ) override
    {
        whenReady( [this, aEnable]()
                   {
                       ComPtr<ICoreWebView2Settings> settings;

                       if( SUCCEEDED( m_webView->get_Settings( &settings ) ) )
                           settings->put_AreDefaultContextMenusEnabled( aEnable ? TRUE : FALSE );
                   } );
    }

    void EnableAccessToDevTools( bool aEnable ) override
    {
        whenReady( [this, aEnable]()
                   {
                       ComPtr<ICoreWebView2Settings> settings;

                       if( SUCCEEDED( m_webView->get_Settings( &settings ) ) )
                           settings->put_AreDevToolsEnabled( aEnable ? TRUE : FALSE );
                   } );
    }

    bool SaveCookies( const wxString& aFile ) override
    {
        ComPtr<ICoreWebView2CookieManager> manager = cookieManager();

        if( !manager )
            return false;

        struct RESULT
        {
            bool       done = false;
            QJsonArray list;
        };

        auto result = std::make_shared<RESULT>();

        manager->GetCookies(
                L"",
                Callback<ICoreWebView2GetCookiesCompletedHandler>(
                        [result]( HRESULT aError, ICoreWebView2CookieList* aList ) -> HRESULT
                        {
                            UINT count = 0;

                            if( SUCCEEDED( aError ) && aList )
                                aList->get_Count( &count );

                            for( UINT ii = 0; ii < count; ++ii )
                            {
                                ComPtr<ICoreWebView2Cookie> cookie;

                                if( FAILED( aList->GetValueAtIndex( ii, &cookie ) ) )
                                    continue;

                                LPWSTR text = nullptr;
                                BOOL   flag = FALSE;
                                double expires = 0;
                                BOOL   session = TRUE;

                                QJsonObject item;
                                cookie->get_Name( &text );
                                item[QStringLiteral( "name" )] = takeString( text );
                                cookie->get_Value( &text );
                                item[QStringLiteral( "value" )] = takeString( text );
                                cookie->get_Domain( &text );
                                item[QStringLiteral( "domain" )] = takeString( text );
                                cookie->get_Path( &text );
                                item[QStringLiteral( "path" )] = takeString( text );
                                cookie->get_IsSecure( &flag );
                                item[QStringLiteral( "secure" )] = bool( flag );
                                cookie->get_IsHttpOnly( &flag );
                                item[QStringLiteral( "httpOnly" )] = bool( flag );
                                cookie->get_IsSession( &session );
                                cookie->get_Expires( &expires );
                                item[QStringLiteral( "expires" )] = session ? 0.0 : expires;
                                result->list.append( item );
                            }

                            result->done = true;
                            return S_OK;
                        } )
                        .Get() );

        if( !waitForCompat( [result]() { return result->done; }, 5000 ) )
            return false;

        QFile file( ToQString( aFile ) );

        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
            return false;

        return file.write( QJsonDocument( result->list ).toJson() ) >= 0;
    }

    bool LoadCookies( const wxString& aFile ) override
    {
        ComPtr<ICoreWebView2CookieManager> manager = cookieManager();
        QFile                              file( ToQString( aFile ) );

        if( !manager || !file.open( QIODevice::ReadOnly ) )
            return false;

        const QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );

        if( !doc.isArray() )
            return false;

        for( const QJsonValue& value : doc.array() )
        {
            const QJsonObject           item = value.toObject();
            ComPtr<ICoreWebView2Cookie> cookie;

            if( FAILED( manager->CreateCookie(
                        toW( item[QStringLiteral( "name" )].toString() ).c_str(),
                        toW( item[QStringLiteral( "value" )].toString() ).c_str(),
                        toW( item[QStringLiteral( "domain" )].toString() ).c_str(),
                        toW( item[QStringLiteral( "path" )].toString( QStringLiteral( "/" ) ) )
                                .c_str(),
                        &cookie ) ) )
            {
                continue;
            }

            cookie->put_IsSecure( item[QStringLiteral( "secure" )].toBool() ? TRUE : FALSE );
            cookie->put_IsHttpOnly( item[QStringLiteral( "httpOnly" )].toBool() ? TRUE : FALSE );

            const double expires = item[QStringLiteral( "expires" )].toDouble();

            if( expires > 0 )
                cookie->put_Expires( expires );

            manager->AddOrUpdateCookie( cookie.Get() );
        }

        return true;
    }

    bool DeleteCookies() override
    {
        ComPtr<ICoreWebView2CookieManager> manager = cookieManager();

        return manager && SUCCEEDED( manager->DeleteAllCookies() );
    }

private:
    /// The environment and the controller of WebView2, asynchronously.
    void create()
    {
        QString folder = QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation );

        if( folder.isEmpty() )
            folder = QDir::tempPath();

        folder = QDir( folder ).filePath( QStringLiteral( "WebView2" ) );
        QDir().mkpath( folder );

        HWND              hwnd = reinterpret_cast<HWND>( m_widget->winId() );
        QPointer<QWidget> widget( m_widget );

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
                nullptr, toW( QDir::toNativeSeparators( folder ) ).c_str(), nullptr,
                Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                        [this, widget, hwnd]( HRESULT aError,
                                              ICoreWebView2Environment* aEnvironment ) -> HRESULT
                        {
                            if( !widget )
                                return S_OK;

                            if( FAILED( aError ) || !aEnvironment )
                                return failed( wxS( "the WebView2 runtime is not available" ) );

                            return aEnvironment->CreateCoreWebView2Controller(
                                    hwnd,
                                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                            [this, widget]( HRESULT aResult,
                                                            ICoreWebView2Controller* aController )
                                                    -> HRESULT
                                            {
                                                if( !widget )
                                                    return S_OK;

                                                if( FAILED( aResult ) || !aController )
                                                    return failed( wxS( "WebView2 can't be made" ) );

                                                ready( aController );
                                                return S_OK;
                                            } )
                                            .Get() );
                        } )
                        .Get() );

        if( FAILED( hr ) )
            failed( wxS( "the WebView2 runtime is not available" ) );
    }

    HRESULT failed( const wxString& aWhy )
    {
        m_failed = true;
        m_pending.clear();
        sendError( wxString(), aWhy );
        return S_OK;
    }

    void ready( ICoreWebView2Controller* aController )
    {
        m_controller = aController;
        m_controller->get_CoreWebView2( &m_webView );

        updateBounds();
        m_controller->put_IsVisible( m_widget->isVisible() ? TRUE : FALSE );

        EventRegistrationToken token;

        m_webView->add_NavigationStarting(
                Callback<ICoreWebView2NavigationStartingEventHandler>(
                        [this]( ICoreWebView2*,
                                ICoreWebView2NavigationStartingEventArgs* aArgs ) -> HRESULT
                        {
                            m_busy = true;

                            // The navigation of LoadURL() and SetPage() was sent already
                            if( m_expectLoad )
                            {
                                m_expectLoad = false;
                                return S_OK;
                            }

                            LPWSTR uri = nullptr;
                            BOOL   byUser = FALSE;
                            aArgs->get_Uri( &uri );
                            aArgs->get_IsUserInitiated( &byUser );

                            if( !sendNavigating( ToWxString( takeString( uri ) ), byUser != FALSE ) )
                                aArgs->put_Cancel( TRUE );

                            return S_OK;
                        } )
                        .Get(),
                &token );

        m_webView->add_NavigationCompleted(
                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [this]( ICoreWebView2*,
                                ICoreWebView2NavigationCompletedEventArgs* aArgs ) -> HRESULT
                        {
                            m_busy = false;
                            m_expectLoad = false;

                            BOOL                            success = FALSE;
                            COREWEBVIEW2_WEB_ERROR_STATUS status =
                                    COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
                            aArgs->get_IsSuccess( &success );
                            aArgs->get_WebErrorStatus( &status );

                            if( success )
                                sendLoaded( GetCurrentURL() );
                            else if( status != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED )
                                sendError( GetCurrentURL(), wxS( "the page can't be loaded" ) );

                            return S_OK;
                        } )
                        .Get(),
                &token );

        m_webView->add_NewWindowRequested(
                Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                        [this]( ICoreWebView2*,
                                ICoreWebView2NewWindowRequestedEventArgs* aArgs ) -> HRESULT
                        {
                            // No new window: wxEVT_WEBVIEW_NEWWINDOW
                            LPWSTR uri = nullptr;
                            aArgs->get_Uri( &uri );
                            aArgs->put_Handled( TRUE );
                            sendNewWindow( ToWxString( takeString( uri ) ) );
                            return S_OK;
                        } )
                        .Get(),
                &token );

        m_webView->add_DocumentTitleChanged(
                Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                        [this]( ICoreWebView2*, IUnknown* ) -> HRESULT
                        {
                            sendTitleChanged( GetCurrentTitle() );
                            return S_OK;
                        } )
                        .Get(),
                &token );

        m_webView->add_WebMessageReceived(
                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [this]( ICoreWebView2*,
                                ICoreWebView2WebMessageReceivedEventArgs* aArgs ) -> HRESULT
                        {
                            LPWSTR text = nullptr;

                            if( FAILED( aArgs->TryGetWebMessageAsString( &text ) ) )
                                return S_OK;

                            QJsonArray message =
                                    QJsonDocument::fromJson( takeString( text ).toUtf8() ).array();

                            if( message.size() != 2 )
                                return S_OK;

                            wxString handler = ToWxString( message[0].toString() );

                            if( m_handlers.count( handler ) )
                                sendScriptMessage( handler, ToWxString( message[1].toString() ) );

                            return S_OK;
                        } )
                        .Get(),
                &token );

        std::vector<std::function<void()>> pending;
        pending.swap( m_pending );

        for( const std::function<void()>& call : pending )
            call();
    }

    /// Run @a aCall now if WebView2 is ready, or once it is.
    void whenReady( std::function<void()> aCall )
    {
        if( m_webView )
            aCall();
        else if( !m_failed )
            m_pending.push_back( std::move( aCall ) );
    }

    void updateBounds()
    {
        if( !m_controller )
            return;

        const qreal scale = m_widget->devicePixelRatioF();
        RECT        bounds{ 0, 0, LONG( m_widget->width() * scale ),
                     LONG( m_widget->height() * scale ) };
        m_controller->put_Bounds( bounds );
    }

    /// A script run at the creation of each document (and now in the page shown).
    void addDocumentScript( const wxString& aKey, const std::wstring& aSource, bool aRunNow )
    {
        whenReady(
                [this, aKey, aSource, aRunNow]()
                {
                    m_webView->AddScriptToExecuteOnDocumentCreated(
                            aSource.c_str(),
                            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                                    [this, aKey]( HRESULT aError, LPCWSTR aId ) -> HRESULT
                                    {
                                        if( SUCCEEDED( aError ) && aId )
                                            m_scriptIds[aKey] = aId;

                                        return S_OK;
                                    } )
                                    .Get() );

                    if( aRunNow )
                    {
                        m_webView->ExecuteScript(
                                aSource.c_str(),
                                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                                        []( HRESULT, LPCWSTR ) -> HRESULT { return S_OK; } )
                                        .Get() );
                    }
                } );
    }

    void removeDocumentScript( const wxString& aKey )
    {
        auto it = m_scriptIds.find( aKey );

        if( it == m_scriptIds.end() )
            return;

        if( m_webView )
            m_webView->RemoveScriptToExecuteOnDocumentCreated( it->second.c_str() );

        m_scriptIds.erase( it );
    }

    ComPtr<ICoreWebView2CookieManager> cookieManager()
    {
        if( !waitForCompat( [this]() { return m_webView || m_failed; }, 10000 ) || !m_webView )
            return nullptr;

        ComPtr<ICoreWebView2_2>            webView2;
        ComPtr<ICoreWebView2CookieManager> manager;

        if( SUCCEEDED( m_webView.As( &webView2 ) ) )
            webView2->get_CookieManager( &manager );

        return manager;
    }

    QPointer<HOST_WIDGET>                 m_widget;
    ComPtr<ICoreWebView2Controller>       m_controller;
    ComPtr<ICoreWebView2>                 m_webView;
    std::vector<std::function<void()>>    m_pending;
    std::set<wxString>                    m_handlers;
    std::map<wxString, std::wstring>      m_scriptIds;
    int                                   m_userScriptCount = 0;
    bool                                  m_busy = false;
    bool                                  m_expectLoad = false;
    bool                                  m_failed = false;
};

} // namespace


std::unique_ptr<KIWX_WEBVIEW_BACKEND> KiwxCreateWebEngineBackend( wxWebView* aView )
{
    return std::make_unique<WEBVIEW2_BACKEND>( aView );
}
