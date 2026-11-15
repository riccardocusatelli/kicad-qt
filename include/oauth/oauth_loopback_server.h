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

#pragma once

#include <wx/event.h>
#include <wx/string.h>

#ifndef KICAD_WX_FREE
#include <wx/socket.h>
#include <wx/timer.h>
#endif

#include <memory>
#include <optional>
#include <string>

#include <kicommon.h>

#ifdef KICAD_WX_FREE
class QTcpServer;
class QTcpSocket;
class QTimer;
#endif


wxDECLARE_EXPORTED_EVENT( KIQT_COMMON_API, EVT_OAUTH_LOOPBACK_RESULT, wxCommandEvent );


struct KIQT_COMMON_API OAUTH_AUTHORIZATION_RESPONSE
{
    wxString code;
    wxString state;
    wxString error;
    wxString error_description;
};


class KIQT_COMMON_API OAUTH_LOOPBACK_SERVER : public wxEvtHandler
{
public:
    OAUTH_LOOPBACK_SERVER( wxEvtHandler* aOwner, const wxString& aCallbackPath,
                           const wxString& aExpectedState );
    ~OAUTH_LOOPBACK_SERVER() override;

    bool Start();
    unsigned short GetPort() const { return m_port; }
    wxString GetRedirectUri() const;

    static bool ParseAuthorizationResponse( const wxString& aRequestLine,
                                            const wxString& aExpectedPath,
                                            const wxString& aExpectedState,
                                            OAUTH_AUTHORIZATION_RESPONSE& aResponse,
                                            wxString& aError );

private:
#ifdef KICAD_WX_FREE
    // The Qt build: a QTcpServer; the request of a client is read as it arrives
    void HandleRequest( QTcpSocket* aClient, const std::string& aRequest );
    void SendHttpResponse( QTcpSocket* aClient, bool aSuccess );
#else
    void OnSocketEvent( wxSocketEvent& aEvent );
    void OnTimeout( wxTimerEvent& aEvent );
    void HandleClient( wxSocketBase* aClient );
    void SendHttpResponse( wxSocketBase* aClient, bool aSuccess );
#endif
    void Finish( bool aSuccess );
    void Shutdown();

    wxEvtHandler*                             m_owner;
    wxString                                  m_callbackPath;
    wxString                                  m_expectedState;
#ifdef KICAD_WX_FREE
    QTcpServer*                               m_server;
    QTimer*                                   m_timeout;
#else
    std::unique_ptr<wxSocketServer>           m_server;
    wxTimer                                   m_timeout;
#endif
    unsigned short                            m_port;
    bool                                      m_done;
    std::optional<OAUTH_AUTHORIZATION_RESPONSE> m_response;
};
