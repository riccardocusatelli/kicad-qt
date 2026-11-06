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
 * The web view (wxWebView) of the wxWidgets compatibility layer: a QTextBrowser, which shows
 * the HTML pages of KiCad (rich text HTML: no scripts, no remote pages).
 */

#pragma once

#include <wx/kiwx_gui.h>

#include <QTextBrowser>


/// The events of the web view.
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NAVIGATING;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NAVIGATED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_LOADED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_ERROR;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_NEWWINDOW;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_TITLE_CHANGED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED;
KIQT_API extern const wxEventType wxEVT_WEBVIEW_SCRIPT_RESULT;

/// The backends of wxWidgets (the QTextBrowser is the only one here).
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


/// An event of a web view (wxWebViewEvent).
class wxWebViewEvent : public wxNotifyEvent
{
public:
    wxWebViewEvent( wxEventType aType = wxEVT_NULL, int aId = 0, const wxString& aUrl = wxString(),
                    const wxString& aTarget = wxString(),
                    wxWebViewNavigationActionFlags aFlags = wxWEBVIEW_NAV_ACTION_NONE ) :
            wxNotifyEvent( aType, aId ),
            m_url( aUrl ),
            m_target( aTarget ),
            m_actionFlags( aFlags )
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
 * A web view (wxWebView): the pages are shown by a QTextBrowser.  The links clicked send
 * wxEVT_WEBVIEW_NAVIGATING (a handler can veto them), the pages loaded wxEVT_WEBVIEW_LOADED.
 */
class KIQT_API wxWebView : public wxWindowCompat<QTextBrowser>
{
public:
    explicit wxWebView( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                        const wxString& aUrl = wxString(), const wxPoint& aPos = wxDefaultPosition,
                        const wxSize& aSize = wxDefaultSize, long aStyle = 0 );

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

    /// Show a page: a local file (file:// or a path), else nothing (no remote pages).
    void LoadURL( const wxString& aUrl );

    /// Show an HTML text.
    void SetPage( const wxString& aHtml, const wxString& aBaseUrl = wxString() );

    wxString GetCurrentURL() const { return m_url; }
    wxString GetPageSource() const { return m_html; }
    wxString GetCurrentTitle() const { return ToWxString( documentTitle() ); }

    void Reload( wxWebViewReloadFlags = wxWEBVIEW_RELOAD_DEFAULT ) { LoadURL( m_url ); }
    bool IsBusy() const { return false; }
    void Stop() {}

    /// No scripts in the pages: the scripts do nothing.
    bool RunScript( const wxString& aScript, wxString* aOutput = nullptr ) const { return false; }
    void RunScriptAsync( const wxString& aScript, void* aClientData = nullptr ) const {}
    bool AddScriptMessageHandler( const wxString& aName ) { return false; }
    bool RemoveScriptMessageHandler( const wxString& aName ) { return false; }
    bool AddUserScript( const wxString& aScript, int = 0 ) { return false; }
    void RemoveAllUserScripts() {}
    void EnableContextMenu( bool aEnable = true ) {}
    void EnableAccessToDevTools( bool aEnable = true ) {}
    bool SetUserAgent( const wxString& ) { return false; }

    void SetZoomFactor( float aZoom ) { m_zoom = aZoom; }
    float GetZoomFactor() const { return m_zoom; }

    void* GetNativeBackend() const { return nullptr; }

private:
    /// The page is shown: send wxEVT_WEBVIEW_LOADED (later, as wxWidgets).
    void sendLoaded();

    wxString m_url;
    wxString m_html;
    float    m_zoom = 1.0f;
};
