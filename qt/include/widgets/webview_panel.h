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
 * the compatibility layer (the web engine of the system, see wx/webview.h).  The API is the one
 * of include/widgets/webview_panel.h.
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

                  // The external links clicked open in the browser of the system (the pages
                  // loaded by the code are shown)
                  if( external && !m_handleExternalLinks
                      && aEvent.GetNavigationAction() == wxWEBVIEW_NAV_ACTION_USER )
                  {
                      wxLaunchDefaultBrowser( aEvent.GetURL() );
                      aEvent.Veto();
                  }
              },
              m_browser->GetId() );

        // The links to a new window: in the browser of the system, or in the view
        Bind( wxEVT_WEBVIEW_NEWWINDOW,
              [this]( wxWebViewEvent& aEvent )
              {
                  if( m_handleExternalLinks )
                      m_browser->LoadURL( aEvent.GetURL() );
                  else
                      wxLaunchDefaultBrowser( aEvent.GetURL() );
              },
              m_browser->GetId() );

        // The messages of the pages to their handlers (window.<name>.postMessage())
        Bind( wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
              [this]( wxWebViewEvent& aEvent )
              {
                  auto it = m_msgHandlers.find( aEvent.GetMessageHandler() );

                  if( it != m_msgHandlers.end() && it->second )
                      it->second( aEvent.GetString() );
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

    /// A handler of the messages of the pages: window.<aName>.postMessage( message ).  False if
    /// the pages run no scripts (no web engine).
    bool AddMessageHandler( const wxString& aName, MESSAGE_HANDLER aHandler )
    {
        m_msgHandlers[aName] = std::move( aHandler );
        return m_browser->AddScriptMessageHandler( aName );
    }

    void ClearMessageHandlers()
    {
        for( const auto& [name, handler] : m_msgHandlers )
            m_browser->RemoveScriptMessageHandler( name );

        m_msgHandlers.clear();
    }

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
