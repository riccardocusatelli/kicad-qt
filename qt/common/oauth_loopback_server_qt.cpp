/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * @file oauth_loopback_server_qt.cpp
 * OAUTH_LOOPBACK_SERVER of the Qt build: a QTcpServer on 127.0.0.1 waits the redirection of the
 * browser after the sign in; the request is read as it arrives, without blocking the GUI.  The
 * parsing of the request is in common/oauth/oauth_loopback_server.cpp.
 */

#include <oauth/oauth_loopback_server.h>

#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <memory>


OAUTH_LOOPBACK_SERVER::OAUTH_LOOPBACK_SERVER( wxEvtHandler* aOwner, const wxString& aCallbackPath,
                                              const wxString& aExpectedState ) :
        m_owner( aOwner ),
        m_callbackPath( aCallbackPath ),
        m_expectedState( aExpectedState ),
        m_server( nullptr ),
        m_timeout( new QTimer ),
        m_port( 0 ),
        m_done( false )
{
    m_timeout->setSingleShot( true );
    QObject::connect( m_timeout, &QTimer::timeout, [this]() { Finish( false ); } );
}


OAUTH_LOOPBACK_SERVER::~OAUTH_LOOPBACK_SERVER()
{
    m_timeout->stop();
    delete m_timeout;
    Shutdown();
}


bool OAUTH_LOOPBACK_SERVER::Start()
{
    auto server = std::make_unique<QTcpServer>();

    if( !server->listen( QHostAddress::LocalHost, 0 ) )
        return false;

    QTcpServer* srv = server.get();

    QObject::connect( srv, &QTcpServer::newConnection,
            [this, srv]()
            {
                while( QTcpSocket* client = srv->nextPendingConnection() )
                {
                    auto request = std::make_shared<std::string>();
                    QPointer<QTcpSocket> guard( client );

                    QObject::connect( client, &QTcpSocket::disconnected, client,
                                      &QObject::deleteLater );
                    QObject::connect( client, &QTcpSocket::readyRead, client,
                            [this, guard, request]()
                            {
                                if( !guard )
                                    return;

                                const QByteArray data = guard->readAll();
                                request->append( data.constData(), data.size() );

                                if( request->find( "\r\n\r\n" ) != std::string::npos
                                        || request->size() > 4096 )
                                {
                                    QObject::disconnect( guard, &QTcpSocket::readyRead, nullptr,
                                                         nullptr );
                                    HandleRequest( guard, *request );
                                }
                            } );
                }
            } );

    m_port = server->serverPort();
    m_server = server.release();
    m_timeout->start( 120000 );
    return m_port != 0;
}


void OAUTH_LOOPBACK_SERVER::HandleRequest( QTcpSocket* aClient, const std::string& aRequest )
{
    const wxString requestWx = wxString::FromUTF8( aRequest.data(), aRequest.size() );
    const int endOfLine = requestWx.Find( wxS( "\r\n" ) );
    const wxString requestLine = endOfLine == wxNOT_FOUND ? requestWx : requestWx.Mid( 0, endOfLine );

    OAUTH_AUTHORIZATION_RESPONSE response;
    wxString error;
    const bool success = ParseAuthorizationResponse( requestLine, m_callbackPath, m_expectedState,
                                                     response, error );

    if( success )
        m_response = response;

    SendHttpResponse( aClient, success );
    Finish( success );
}


void OAUTH_LOOPBACK_SERVER::SendHttpResponse( QTcpSocket* aClient, bool aSuccess )
{
    if( !aClient )
        return;

    const QByteArray body = aSuccess
            ? QByteArrayLiteral( "<!DOCTYPE html><html><body><p>Authentication complete.</p></body></html>" )
            : QByteArrayLiteral( "<!DOCTYPE html><html><body><p>Authentication failed.</p></body></html>" );

    QByteArray response = QByteArrayLiteral( "HTTP/1.1 200 OK\r\n"
                                             "Content-Type: text/html; charset=utf-8\r\n"
                                             "Cache-Control: no-store\r\n"
                                             "Connection: close\r\n"
                                             "Content-Length: " );
    response += QByteArray::number( body.size() ) + "\r\n\r\n" + body;

    // The socket outlives the server: it closes (and deletes itself) once the answer is written
    aClient->setParent( nullptr );
    aClient->write( response );
    aClient->disconnectFromHost();
}


void OAUTH_LOOPBACK_SERVER::Finish( bool aSuccess )
{
    if( m_done )
        return;

    m_done = true;
    m_timeout->stop();

    if( m_owner )
    {
        wxCommandEvent evt( EVT_OAUTH_LOOPBACK_RESULT );
        evt.SetInt( aSuccess ? 1 : 0 );

        if( aSuccess && m_response.has_value() )
            evt.SetString( m_response->code );

        wxQueueEvent( m_owner, evt.Clone() );
    }

    Shutdown();
}


void OAUTH_LOOPBACK_SERVER::Shutdown()
{
    if( m_server )
    {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}
