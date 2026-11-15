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
 * @file kiwx_webview_backend.h
 * The backends of the web view of the wxWidgets compatibility layer: the web engine that shows
 * the pages of a wxWebView.  Qt WebEngine (kiwx_webview_qtwebengine.cpp, Linux), WKWebView
 * (kiwx_webview_wkwebview.mm, macOS), WebView2 (kiwx_webview_webview2.cpp, Windows), or a
 * QTextBrowser without scripts (kiwx_webview.cpp) when the build has none of them.
 */

#pragma once

#include <memory>

#include <wx/webview.h>

class QWidget;


class KIQT_API KIWX_WEBVIEW_BACKEND
{
public:
    explicit KIWX_WEBVIEW_BACKEND( wxWebView* aView ) : m_view( aView ) {}
    virtual ~KIWX_WEBVIEW_BACKEND() = default;

    /// The widget showing the pages (a child of the view).
    virtual QWidget* Widget() = 0;

    /// True if the pages run their scripts.
    virtual bool HasScripts() const { return false; }

    virtual void LoadURL( const wxString& aUrl ) = 0;
    virtual void SetPage( const wxString& aHtml, const wxString& aBaseUrl ) = 0;
    virtual wxString GetCurrentURL() const = 0;
    virtual wxString GetCurrentTitle() const = 0;
    virtual void Reload() = 0;
    virtual bool IsBusy() const { return false; }
    virtual void Stop() {}

    virtual void RunScriptAsync( const wxString& aScript, void* aClientData );
    virtual bool RunScript( const wxString& aScript, wxString* aOutput ) { return false; }

    virtual bool AddScriptMessageHandler( const wxString& aName ) { return false; }
    virtual bool RemoveScriptMessageHandler( const wxString& aName ) { return false; }
    virtual bool AddUserScript( const wxString& aScript, bool aAtDocumentStart ) { return false; }
    virtual void RemoveAllUserScripts() {}

    virtual bool SetUserAgent( const wxString& aUserAgent ) { return false; }
    virtual void SetZoomFactor( float aZoom ) {}
    virtual void EnableContextMenu( bool aEnable ) {}
    virtual void EnableAccessToDevTools( bool aEnable ) {}

    /// The cookies of the view (KIPLATFORM::WEBVIEW): saved to a file, given back, deleted.
    virtual bool SaveCookies( const wxString& aFile ) { return false; }
    virtual bool LoadCookies( const wxString& aFile ) { return false; }
    virtual bool DeleteCookies() { return false; }

    /**
     * The script of RunScript(): @a aScript evaluated in a function that returns
     * [ true, result as a string (an object as JSON) ] or [ false, error ], for the engines that
     * give the value of a script.
     */
    static wxString WrapScriptCompat( const wxString& aScript );

    /**
     * The script defining window.<aName>.postMessage() in the pages, which calls
     * @a aPostFunction( aName, message as a string ) (the bridge of the engine to the view).
     */
    static wxString MessageHandlerScriptCompat( const wxString& aName,
                                                const wxString& aPostFunction );

protected:
    /// The events of the view.
    bool sendNavigating( const wxString& aUrl, bool aByUser );
    void sendLoaded( const wxString& aUrl );
    void sendError( const wxString& aUrl, const wxString& aDescription );
    void sendNewWindow( const wxString& aUrl );
    void sendTitleChanged( const wxString& aTitle );
    void sendScriptMessage( const wxString& aHandler, const wxString& aMessage );
    void sendScriptResult( void* aClientData, bool aOk, const wxString& aResult );

    wxWebView* m_view;
};


/// The backend of the web engine of the build (nullptr if none: the QTextBrowser).
std::unique_ptr<KIWX_WEBVIEW_BACKEND> KiwxCreateWebEngineBackend( wxWebView* aView );
