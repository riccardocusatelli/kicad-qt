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
 * @file webview_panel.h
 * The panel of a web view (WEBVIEW_PANEL) of the Qt build: a wxPanel holding the wxWebView of
 * the compatibility layer (a QTextBrowser: rich text HTML, no scripts).  The API is the one of
 * include/widgets/webview_panel.h.
 */

#pragma once

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/utils.h>
#include <wx/webview.h>
#include <functional>
#include <map>

class TOOL_MANAGER;
class TOOL_BASE;


class WEBVIEW_PANEL : public wxPanel
{
public:
    using MESSAGE_HANDLER = std::function<void( const wxString& )>;

    explicit WEBVIEW_PANEL( wxWindow* aParent, wxWindowID aId = wxID_ANY,
                            const wxPoint& aPos = wxDefaultPosition,
                            const wxSize& aSize = wxDefaultSize, const int aStyle = 0,
                            TOOL_MANAGER* aToolManager = nullptr, TOOL_BASE* aTool = nullptr ) :
            wxPanel( aParent, aId, aPos, aSize, aStyle )
    {
        m_browser = new wxWebView( this );

        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( m_browser, 1, wxEXPAND );
        SetSizer( sizer );

        Bind( wxEVT_WEBVIEW_NAVIGATING,
              [this]( wxWebViewEvent& aEvent )
              {
                  m_loadError = false;

                  bool external = aEvent.GetURL().StartsWith( wxS( "http://" ) )
                                  || aEvent.GetURL().StartsWith( wxS( "https://" ) );

                  // The external links open in the browser of the system
                  if( external && !m_handleExternalLinks )
                  {
                      wxLaunchDefaultBrowser( aEvent.GetURL() );
                      aEvent.Veto();
                  }
              },
              m_browser->GetId() );

        Bind( wxEVT_WEBVIEW_ERROR,
              [this]( wxWebViewEvent& aEvent )
              {
                  m_loadError = true;
                  aEvent.Skip();
              },
              m_browser->GetId() );
    }

    WEBVIEW_PANEL( const WEBVIEW_PANEL& ) = delete;
    WEBVIEW_PANEL& operator=( const WEBVIEW_PANEL& ) = delete;

    wxWebView* GetWebView() const { return m_browser; }
    const wxString& GetBackend() const { return m_backend; }

    void LoadURL( const wxString& aUrl )
    {
        m_loadError = false;
        m_browser->LoadURL( aUrl );
    }

    void SetPage( const wxString& aHtml )
    {
        m_loadError = false;
        m_browser->SetPage( aHtml, wxS( "file://" ) );
    }

    /// No scripts in the pages: the message handlers are never called.
    bool AddMessageHandler( const wxString& aName, MESSAGE_HANDLER aHandler )
    {
        m_msgHandlers[aName] = std::move( aHandler );
        return false;
    }

    void ClearMessageHandlers() { m_msgHandlers.clear(); }

    void SetHandleExternalLinks( bool aHandle ) { m_handleExternalLinks = aHandle; }
    bool GetHandleExternalLinks() const { return m_handleExternalLinks; }

    void RunScriptAsync( const wxString& aScript, void* aClientData = nullptr ) const
    {
        m_browser->RunScriptAsync( aScript, aClientData );
    }

    bool HasLoadError() const { return m_loadError; }

    /// The loaded event of the web view reaches the handlers of the panel without binding.
    void BindLoadedEvent() {}

private:
    wxWebView*                          m_browser = nullptr;
    wxString                            m_backend = wxWebViewBackendDefault;
    bool                                m_handleExternalLinks = false;
    bool                                m_loadError = false;
    std::map<wxString, MESSAGE_HANDLER> m_msgHandlers;
};
