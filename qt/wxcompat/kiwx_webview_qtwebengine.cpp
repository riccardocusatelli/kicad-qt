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
 * @file kiwx_webview_qtwebengine.cpp
 * The backend of the web view on Qt WebEngine (Linux): a QWebEngineView.  The messages of the
 * pages reach the view through a QWebChannel (qwebchannel.js, injected in each page).
 */

#include <wx/kiwx_webview_backend.h>

#include <map>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkCookie>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <qt_strings.h>


// Qt WebEngine shares its OpenGL contexts with the ones of the application: the attribute is set
// before the QApplication is made (when the library is loaded).
//
// Before Qt 6.7 the first web view shown in a window made Qt create again the native window of
// the window (to draw it with OpenGL) without keeping aside the OpenGL windows of the canvases,
// destroyed with it: their containers were left without a window, and Qt crashed when the window
// moved.  The windows are drawn with OpenGL from the start then (QT_WIDGETS_RHI, unless the
// environment chooses), and none has to be made again.
static void kiwxShareOpenGLContexts()
{
    QCoreApplication::setAttribute( Qt::AA_ShareOpenGLContexts );

#if QT_VERSION < QT_VERSION_CHECK( 6, 7, 0 )
    if( !qEnvironmentVariableIsSet( "QT_WIDGETS_RHI" ) )
    {
        qputenv( "QT_WIDGETS_RHI", "1" );

        if( !qEnvironmentVariableIsSet( "QT_WIDGETS_RHI_BACKEND" ) )
            qputenv( "QT_WIDGETS_RHI_BACKEND", "opengl" );
    }
#endif
}

Q_CONSTRUCTOR_FUNCTION( kiwxShareOpenGLContexts )


namespace
{

const char BRIDGE_NAME[] = "kiwxBridge";
const char BRIDGE_SCRIPT_NAME[] = "kiwxBridge";

/// The function of the pages posting a message to the view (queued until the channel is ready).
const char POST_FUNCTION[] = "window.__kiwxPost";


class QTWEBENGINE_BACKEND;


/// The object of the channel: the messages of the pages.
class KIWX_WEBVIEW_BRIDGE : public QObject
{
    Q_OBJECT

public:
    explicit KIWX_WEBVIEW_BRIDGE( std::function<void( const QString&, const QString& )> aPost ) :
            m_post( std::move( aPost ) )
    {
    }

public Q_SLOTS:
    void postMessage( const QString& aHandler, const QString& aMessage )
    {
        m_post( aHandler, aMessage );
    }

private:
    std::function<void( const QString&, const QString& )> m_post;
};


/// The page: its navigations go to the view first (wxEVT_WEBVIEW_NAVIGATING).
class KIWX_WEBENGINE_PAGE : public QWebEnginePage
{
public:
    KIWX_WEBENGINE_PAGE( QWebEngineProfile* aProfile, QObject* aParent,
                         std::function<bool( const QUrl&, bool )> aNavigating,
                         std::function<void( const QUrl& )> aNewWindow ) :
            QWebEnginePage( aProfile, aParent ),
            m_navigating( std::move( aNavigating ) ),
            m_newWindow( std::move( aNewWindow ) )
    {
    }

    /// The navigation of SetPage() (its data: URL) is not one of the user.
    void ExpectPageCompat() { m_settingPage = true; }

protected:
    bool acceptNavigationRequest( const QUrl& aUrl, NavigationType aType,
                                  bool aIsMainFrame ) override
    {
        if( !aIsMainFrame )
            return true;

        if( m_settingPage )
        {
            m_settingPage = false;
            return true;
        }

        return m_navigating( aUrl, aType == NavigationTypeLinkClicked );
    }

    QWebEnginePage* createWindow( WebWindowType aType ) override
    {
        // A page for the URL only: wxEVT_WEBVIEW_NEWWINDOW, the page shows nothing
        class NEW_WINDOW_PAGE : public QWebEnginePage
        {
        public:
            NEW_WINDOW_PAGE( QWebEngineProfile* aProfile, QObject* aParent,
                             std::function<void( const QUrl& )> aNewWindow ) :
                    QWebEnginePage( aProfile, aParent ),
                    m_newWindow( std::move( aNewWindow ) )
            {
            }

        protected:
            bool acceptNavigationRequest( const QUrl& aUrl, NavigationType, bool ) override
            {
                if( m_newWindow )
                    m_newWindow( aUrl );

                m_newWindow = nullptr;
                deleteLater();
                return false;
            }

        private:
            std::function<void( const QUrl& )> m_newWindow;
        };

        return new NEW_WINDOW_PAGE( profile(), this, m_newWindow );
    }

private:
    std::function<bool( const QUrl&, bool )> m_navigating;
    std::function<void( const QUrl& )>       m_newWindow;
    bool                                     m_settingPage = false;
};


class QTWEBENGINE_BACKEND : public KIWX_WEBVIEW_BACKEND
{
public:
    explicit QTWEBENGINE_BACKEND( wxWebView* aView ) :
            KIWX_WEBVIEW_BACKEND( aView ),
            m_widget( new QWebEngineView( aView ) )
    {
        // A profile of the view (in memory): its cookies are the ones that
        // KIPLATFORM::WEBVIEW::SaveCookies() writes and LoadCookies() gives back
        m_profile = new QWebEngineProfile( m_widget );
        QWebEngineCookieStore* cookies = m_profile->cookieStore();

        QObject::connect( cookies, &QWebEngineCookieStore::cookieAdded, m_widget,
                          [this]( const QNetworkCookie& aCookie )
                          {
                              m_cookies[cookieKey( aCookie )] = aCookie;
                          } );
        QObject::connect( cookies, &QWebEngineCookieStore::cookieRemoved, m_widget,
                          [this]( const QNetworkCookie& aCookie )
                          {
                              m_cookies.erase( cookieKey( aCookie ) );
                          } );

        m_page = new KIWX_WEBENGINE_PAGE(
                m_profile, m_widget,
                [this]( const QUrl& aUrl, bool aByUser )
                {
                    return sendNavigating( ToWxString( aUrl.toString() ), aByUser );
                },
                [this]( const QUrl& aUrl )
                {
                    sendNewWindow( ToWxString( aUrl.toString() ) );
                } );

        m_widget->setPage( m_page );

        // The messages of the pages
        m_bridge = new KIWX_WEBVIEW_BRIDGE(
                [this]( const QString& aHandler, const QString& aMessage )
                {
                    if( m_handlers.count( ToWxString( aHandler ) ) )
                        sendScriptMessage( ToWxString( aHandler ), ToWxString( aMessage ) );
                } );
        m_bridge->setParent( m_widget );

        QWebChannel* channel = new QWebChannel( m_page );
        channel->registerObject( QString::fromLatin1( BRIDGE_NAME ), m_bridge );
        m_page->setWebChannel( channel );
        addBridgeScript();

        QObject::connect( m_page, &QWebEnginePage::loadFinished, m_widget,
                          [this]( bool aOk )
                          {
                              m_busy = false;
                              wxString url = GetCurrentURL();

                              if( aOk )
                                  sendLoaded( url );
                              else
                                  sendError( url, wxS( "the page can't be loaded" ) );
                          } );

        QObject::connect( m_page, &QWebEnginePage::loadStarted, m_widget,
                          [this]() { m_busy = true; } );

        QObject::connect( m_page, &QWebEnginePage::titleChanged, m_widget,
                          [this]( const QString& aTitle )
                          {
                              sendTitleChanged( ToWxString( aTitle ) );
                          } );
    }

    ~QTWEBENGINE_BACKEND() override
    {
        // The page and its signals go with the backend (the view deletes it first); the page
        // goes before its profile
        delete m_page;
        m_page = nullptr;
        delete m_widget;
    }

    bool SaveCookies( const wxString& aFile ) override
    {
        QJsonArray list;

        for( const auto& [key, cookie] : m_cookies )
            list.append( QString::fromUtf8( cookie.toRawForm( QNetworkCookie::Full ) ) );

        QFile file( ToQString( aFile ) );

        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
            return false;

        return file.write( QJsonDocument( list ).toJson() ) >= 0;
    }

    bool LoadCookies( const wxString& aFile ) override
    {
        QFile file( ToQString( aFile ) );

        if( !file.open( QIODevice::ReadOnly ) )
            return false;

        const QJsonDocument doc = QJsonDocument::fromJson( file.readAll() );

        if( !doc.isArray() )
            return false;

        for( const QJsonValue& value : doc.array() )
        {
            for( const QNetworkCookie& cookie :
                 QNetworkCookie::parseCookies( value.toString().toUtf8() ) )
            {
                QString host = cookie.domain();

                if( host.startsWith( QLatin1Char( '.' ) ) )
                    host.remove( 0, 1 );

                QUrl origin;
                origin.setScheme( cookie.isSecure() ? QStringLiteral( "https" )
                                                    : QStringLiteral( "http" ) );
                origin.setHost( host );
                m_profile->cookieStore()->setCookie( cookie, host.isEmpty() ? QUrl() : origin );
            }
        }

        return true;
    }

    bool DeleteCookies() override
    {
        m_profile->cookieStore()->deleteAllCookies();
        m_cookies.clear();
        return true;
    }

    QWidget* Widget() override { return m_widget; }
    bool HasScripts() const override { return true; }

    void LoadURL( const wxString& aUrl ) override
    {
        QString url = ToQString( aUrl );
        QUrl    qurl = QFileInfo::exists( url ) ? QUrl::fromLocalFile( url )
                                                : QUrl::fromUserInput( url );

        // As wxWidgets: the navigation of the code goes to the handlers too
        if( !sendNavigating( ToWxString( qurl.toString() ), false ) )
            return;

        m_page->ExpectPageCompat();
        m_page->load( qurl );
    }

    void SetPage( const wxString& aHtml, const wxString& aBaseUrl ) override
    {
        QUrl base = aBaseUrl.IsEmpty() ? QUrl() : QUrl( ToQString( aBaseUrl ) );

        // file:// alone is no base
        if( base.isLocalFile() && base.toLocalFile().isEmpty() )
            base = QUrl::fromLocalFile( QStringLiteral( "/" ) );

        m_page->ExpectPageCompat();
        m_page->setHtml( ToQString( aHtml ), base );
    }

    wxString GetCurrentURL() const override { return ToWxString( m_page->url().toString() ); }
    wxString GetCurrentTitle() const override { return ToWxString( m_page->title() ); }

    void Reload() override { m_page->triggerAction( QWebEnginePage::Reload ); }
    bool IsBusy() const override { return m_busy; }
    void Stop() override { m_page->triggerAction( QWebEnginePage::Stop ); }

    void RunScriptAsync( const wxString& aScript, void* aClientData ) override
    {
        QPointer<QWebEngineView> widget( m_widget );

        m_page->runJavaScript( ToQString( WrapScriptCompat( aScript ) ),
                               [this, widget, aClientData]( const QVariant& aResult )
                               {
                                   if( !widget )
                                       return;

                                   bool    ok = false;
                                   QString result;
                                   resultCompat( aResult, ok, result );
                                   sendScriptResult( aClientData, ok, ToWxString( result ) );
                               } );
    }

    bool RunScript( const wxString& aScript, wxString* aOutput ) override
    {
        QEventLoop loop;
        bool       done = false;
        bool       ok = false;
        QString    result;

        m_page->runJavaScript( ToQString( WrapScriptCompat( aScript ) ),
                               [&]( const QVariant& aResult )
                               {
                                   resultCompat( aResult, ok, result );
                                   done = true;
                                   loop.quit();
                               } );

        if( !done )
        {
            QTimer::singleShot( 5000, &loop, &QEventLoop::quit );
            loop.exec( QEventLoop::ExcludeUserInputEvents );
        }

        if( aOutput )
            *aOutput = ToWxString( result );

        return done && ok;
    }

    bool AddScriptMessageHandler( const wxString& aName ) override
    {
        if( m_handlers.count( aName ) )
            return true;

        QWebEngineScript script;
        script.setName( QStringLiteral( "kiwxHandler:" ) + ToQString( aName ) );
        script.setSourceCode( ToQString( MessageHandlerScriptCompat(
                aName, wxString::FromUTF8( POST_FUNCTION ) ) ) );
        script.setInjectionPoint( QWebEngineScript::DocumentCreation );
        script.setWorldId( QWebEngineScript::MainWorld );
        script.setRunsOnSubFrames( false );

        m_page->scripts().insert( script );
        m_handlers[aName] = script;

        // The page shown has it too
        m_page->runJavaScript( script.sourceCode() );
        return true;
    }

    bool RemoveScriptMessageHandler( const wxString& aName ) override
    {
        auto it = m_handlers.find( aName );

        if( it == m_handlers.end() )
            return false;

        m_page->scripts().remove( it->second );
        m_handlers.erase( it );
        return true;
    }

    bool AddUserScript( const wxString& aScript, bool aAtDocumentStart ) override
    {
        QWebEngineScript script;
        script.setName( QStringLiteral( "kiwxUserScript" ) );
        script.setSourceCode( ToQString( aScript ) );
        script.setInjectionPoint( aAtDocumentStart ? QWebEngineScript::DocumentCreation
                                                   : QWebEngineScript::DocumentReady );
        script.setWorldId( QWebEngineScript::MainWorld );
        m_page->scripts().insert( script );
        return true;
    }

    void RemoveAllUserScripts() override
    {
        for( const QWebEngineScript& script :
             m_page->scripts().find( QStringLiteral( "kiwxUserScript" ) ) )
        {
            m_page->scripts().remove( script );
        }
    }

    bool SetUserAgent( const wxString& aUserAgent ) override
    {
        m_page->profile()->setHttpUserAgent( ToQString( aUserAgent ) );
        return true;
    }

    void SetZoomFactor( float aZoom ) override { m_widget->setZoomFactor( aZoom ); }

    void EnableContextMenu( bool aEnable ) override
    {
        m_widget->setContextMenuPolicy( aEnable ? Qt::DefaultContextMenu : Qt::NoContextMenu );
    }

private:
    /// The result of the wrapped script: [ ok, string ].
    static void resultCompat( const QVariant& aResult, bool& aOk, QString& aText )
    {
        QVariantList list = aResult.toList();

        if( list.size() == 2 )
        {
            aOk = list[0].toBool();
            aText = list[1].toString();
        }
        else
        {
            aOk = false;
            aText = QStringLiteral( "the script gave no result" );
        }
    }

    /// qwebchannel.js and the function posting the messages, in each page before its scripts.
    void addBridgeScript()
    {
        QFile   file( QStringLiteral( ":/qtwebchannel/qwebchannel.js" ) );
        QString channelJs;

        if( file.open( QIODevice::ReadOnly ) )
            channelJs = QString::fromUtf8( file.readAll() );

        QString source = channelJs + QStringLiteral(
                "\n(function() {"
                " var queue = [];"
                " var bridge = null;"
                " %1 = function( handler, message ) {"
                "  if( bridge ) bridge.postMessage( handler, message );"
                "  else queue.push( [ handler, message ] );"
                " };"
                " if( typeof qt === 'undefined' || !qt.webChannelTransport ) return;"
                " new QWebChannel( qt.webChannelTransport, function( channel ) {"
                "  bridge = channel.objects.%2;"
                "  queue.forEach( function( m ) { bridge.postMessage( m[0], m[1] ); } );"
                "  queue = [];"
                " } );"
                "})();" ).arg( QString::fromLatin1( POST_FUNCTION ),
                                QString::fromLatin1( BRIDGE_NAME ) );

        QWebEngineScript script;
        script.setName( QString::fromLatin1( BRIDGE_SCRIPT_NAME ) );
        script.setSourceCode( source );
        script.setInjectionPoint( QWebEngineScript::DocumentCreation );
        script.setWorldId( QWebEngineScript::MainWorld );
        script.setRunsOnSubFrames( false );
        m_page->scripts().insert( script );
    }

    static QString cookieKey( const QNetworkCookie& aCookie )
    {
        return QString::fromUtf8( aCookie.name() ) + QLatin1Char( '\n' ) + aCookie.domain()
               + QLatin1Char( '\n' ) + aCookie.path();
    }

    QWebEngineView*                           m_widget;
    QWebEngineProfile*                        m_profile = nullptr;
    std::map<QString, QNetworkCookie>         m_cookies;
    KIWX_WEBENGINE_PAGE*                      m_page = nullptr;
    KIWX_WEBVIEW_BRIDGE*                      m_bridge = nullptr;
    std::map<wxString, QWebEngineScript>      m_handlers;
    bool                                      m_busy = false;
};

} // namespace


std::unique_ptr<KIWX_WEBVIEW_BACKEND> KiwxCreateWebEngineBackend( wxWebView* aView )
{
    return std::make_unique<QTWEBENGINE_BACKEND>( aView );
}


#include "kiwx_webview_qtwebengine.moc"
