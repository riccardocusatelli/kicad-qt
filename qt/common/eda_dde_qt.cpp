/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
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
 * @file eda_dde_qt.cpp
 * The client of the remote commands (cross-probing between the standalone editors) with Qt
 * (see common/eda_dde.cpp; the server is KIWAY_PLAYER::CreateServer()).
 */

#include <eda_dde.h>

#include <QHostAddress>
#include <QPointer>
#include <QTcpSocket>
#include <QTimer>


/// The time given to a message to reach the other editor (as the wxWidgets build)
static constexpr int SEND_TIMEOUT_MS = 250;


/// The socket of the message being sent: one message at a time, as the wxWidgets build
static QPointer<QTcpSocket> s_pendingSocket;


bool SendCommand( int aService, const std::string& aMessage )
{
    if( s_pendingSocket )
        return false;

    QTcpSocket* socket = new QTcpSocket();
    QByteArray  message( aMessage.data(), static_cast<qsizetype>( aMessage.size() ) );

    s_pendingSocket = socket;

    QObject::connect( socket, &QTcpSocket::connected, socket,
            [socket, message]()
            {
                socket->write( message );
                socket->disconnectFromHost();
            } );

    QObject::connect( socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater );
    QObject::connect( socket, &QTcpSocket::errorOccurred, socket, &QObject::deleteLater );

    // The other editor isn't listening: the message is dropped
    QTimer::singleShot( SEND_TIMEOUT_MS, socket,
            [socket]()
            {
                if( socket->state() != QAbstractSocket::ConnectedState )
                {
                    socket->abort();
                    socket->deleteLater();
                }
            } );

    socket->connectToHost( QHostAddress( QHostAddress::LocalHost ),
                           static_cast<quint16>( aService ) );

    return true;
}


void SocketCleanup()
{
    if( s_pendingSocket )
    {
        s_pendingSocket->abort();
        delete s_pendingSocket.data();
    }
}
