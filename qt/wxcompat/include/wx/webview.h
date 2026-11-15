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
 * @file webview.h
 * The web view (wxWebView) of the wxWidgets compatibility layer.  As wxWidgets, it shows the pages
 * with the web engine of the system: Qt WebEngine on Linux, WKWebView on macOS, WebView2 on
 * Windows (see kiwx_webview_backend.h).  Without one, a QTextBrowser shows the HTML pages of
 * KiCad (rich text HTML: no scripts, no remote pages).
 */

#pragma once

#include <memory>

#include <wx/kiwx_gui.h>

#include <QWidget>

class KIWX_WEBVIEW_BACKEND;


/// The events of the web view.
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NAVIGATING;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NAVIGATED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_LOADED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_ERROR;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NEWWINDOW;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_TITLE_CHANGED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_SCRIPT_RESULT;

/// The backends of wxWidgets: the web engine of the system is the default one.
inline constexpr const char wxWebViewBackendDefault[] = "";
inline constexpr const char wxWebViewBackendIE[] = "wxWebViewIE";
inline constexpr const char wxWebViewBackendEdge[] = "wxWebViewEdge";
inline constexpr const char wxWebViewBackendWebKit[] = "wxWebViewWebKit";

enum wxWebViewNavigationActionFlags
{
    wxWEBVIEW_NAV_ACTION_NONE,
    wxWEBVIEW_NAV_ACTION_USER,
    wxWEBVIEW_NAV_ACTION_OTHER
};

enum wxWebViewReloadFlags
{
    wxWEBVIEW_RELOAD_DEFAULT,
    wxWEBVIEW_RELOAD_NO_CACHE
};

/// When a user script runs (wxWebView::AddUserScript()).
enum wxWebViewUserScriptInjectionTime
{
    wxWEBVIEW_INJECT_AT_DOCUMENT_START,
    wxWEBVIEW_INJECT_AT_DOCUMENT_END
};


/// An event of a web view (wxWebViewEvent).
class wxWebViewEvent : public wxNotifyEvent
{
public:
    wxWebViewEvent( wxEventType aType = wxEVT_NULL, int aId = 0, const wxString& aUrl = wxString(),
                    const wxString& aTarget = wxString(),
                    wxWebViewNavigationActionFlags aFlags = wxWEBVIEW_NAV_ACTION_NONE,
                    const wxString& aMessageHandler = wxString() ) :
            wxNotifyEvent( aType, aId ),
            m_url( aUrl ),
            m_target( aTarget ),
            m_actionFlags( aFlags ),
            m_messageHandler( aMessageHandler )
    {
    }

    const wxString& GetURL() const { return m_url; }
    const wxString& GetTarget() const { return m_target; }
    wxWebViewNavigationActionFlags GetNavigationAction() const { return m_actionFlags; }
    const wxString& GetMessageHandler() const { return m_messageHandler; }
    void SetMessageHandler( const wxString& aHandler ) { m_messageHandler = aHandler; }
    bool IsError() const { return GetInt() != 0; }

    wxWebViewEvent* Clone() const { return new wxWebViewEvent( *this ); }

private:
    wxString                       m_url;
    wxString                       m_target;
    wxWebViewNavigationActionFlags m_actionFlags;
    wxString                       m_messageHandler;
};

#define wxWebViewEventHandler( func ) ( &func )


/**
 * A web view (wxWebView): the page of the web engine of the system (or of a QTextBrowser).
 *
 * The events are the ones of wxWidgets: wxEVT_WEBVIEW_NAVIGATING before a page is loaded (a
 * handler can veto it), wxEVT_WEBVIEW_NAVIGATED and wxEVT_WEBVIEW_LOADED once it is,
 * wxEVT_WEBVIEW_ERROR if it can't be, wxEVT_WEBVIEW_NEWWINDOW for a link to a new window,
 * wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED for the messages of the page to a handler
 * (window.<handler>.postMessage()), wxEVT_WEBVIEW_SCRIPT_RESULT for the result of
 * RunScriptAsync().  They go to the parents of the view up to its top level window, as the
 * command events of wxWidgets: the panels bind them.
 */
class KIQT_API wxWebView : public wxWindowCompat<QWidget>
{
public:
    explicit wxWebView( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                        const wxString& aUrl = wxString(), const wxPoint& aPos = wxDefaultPosition,
                        const wxSize& aSize = wxDefaultSize, long aStyle = 0 );
    ~wxWebView() override;

    static wxWebView* New( wxWidgetArgCompat aParent, int aId = wxID_ANY,
                           const wxString& aUrl = wxString(),
                           const wxPoint& aPos = wxDefaultPosition,
                           const wxSize& aSize = wxDefaultSize,
                           const wxString& aBackend = wxString(), long aStyle = 0,
                           const wxString& aName = wxString() )
    {
        return new wxWebView( aParent, aId, aUrl, aPos, aSize, aStyle );
    }

    static bool IsBackendAvailable( const wxString& aBackend ) { return aBackend.IsEmpty(); }

    /// True if the pages run their scripts (a web engine, not the QTextBrowser).
    static bool HasWebEngineCompat();

    /// Show a page: an URL, or a local file (file:// or a path).
    void LoadURL( const wxString& aUrl );

    /// Show an HTML text.
    void SetPage( const wxString& aHtml, const wxString& aBaseUrl = wxString() );

    wxString GetCurrentURL() const;
    wxString GetCurrentTitle() const;
    wxString GetPageSource() const { return m_html; }

    void Reload( wxWebViewReloadFlags = wxWEBVIEW_RELOAD_DEFAULT );
    bool IsBusy() const;
    void Stop();

    /**
     * Run a script in the page; its result as a string (an object as JSON).  RunScript() waits
     * for it, RunScriptAsync() sends wxEVT_WEBVIEW_SCRIPT_RESULT (with the client data).  False,
     * or an event with an error, if the page has no scripts or the script fails.
     */
    bool RunScript( const wxString& aScript, wxString* aOutput = nullptr ) const;
    void RunScriptAsync( const wxString& aScript, void* aClientData = nullptr ) const;

    /// window.<aName>.postMessage( message ) in the pages: wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED.
    bool AddScriptMessageHandler( const wxString& aName );
    bool RemoveScriptMessageHandler( const wxString& aName );

    /// A script run in each page (before its own scripts, or once it is loaded).
    bool AddUserScript( const wxString& aScript,
                        wxWebViewUserScriptInjectionTime aInjectionTime =
                                wxWEBVIEW_INJECT_AT_DOCUMENT_START );
    void RemoveAllUserScripts();

    void EnableContextMenu( bool aEnable = true );
    void EnableAccessToDevTools( bool aEnable = true );
    bool SetUserAgent( const wxString& aUserAgent );

    void SetZoomFactor( float aZoom );
    float GetZoomFactor() const { return m_zoom; }

    void* GetNativeBackend() const { return nullptr; }

    /// The backend of the view (its widget, its pages).
    KIWX_WEBVIEW_BACKEND* GetBackendCompat() const { return m_backend.get(); }

    /**
     * Send an event of the view (its id and object are set): to the view, then to its parents up
     * to its top level window until one handles it.  False if a handler vetoed it.
     */
    bool SendEventCompat( wxWebViewEvent& aEvent );

    /// The source of the page shown (the backends keep it for GetPageSource()).
    void SetPageSourceCompat( const wxString& aHtml ) { m_html = aHtml; }

private:
    std::unique_ptr<KIWX_WEBVIEW_BACKEND> m_backend;
    wxString                              m_html;
    float                                 m_zoom = 1.0f;
};
