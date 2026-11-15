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
 * @file webview.cpp
 * KIPLATFORM::WEBVIEW of the Qt build: the cookies are the ones of the web engine of the view
 * (see wx/kiwx_webview_backend.h); a view without a web engine has none.
 */

#include <kiplatform/webview.h>

#include <wx/webview.h>
#include <wx/kiwx_webview_backend.h>


namespace
{
KIWX_WEBVIEW_BACKEND* backendOf( wxWebView* aWebView )
{
    return aWebView ? aWebView->GetBackendCompat() : nullptr;
}
} // namespace


// Exported from kiqt as the other functions of KIPLATFORM (their header has no export macro)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

bool KIPLATFORM::WEBVIEW::SaveCookies( wxWebView* aWebView, const wxString& aTargetFile )
{
    KIWX_WEBVIEW_BACKEND* backend = backendOf( aWebView );
    return backend && backend->SaveCookies( aTargetFile );
}


bool KIPLATFORM::WEBVIEW::LoadCookies( wxWebView* aWebView, const wxString& aSourceFile )
{
    KIWX_WEBVIEW_BACKEND* backend = backendOf( aWebView );
    return backend && backend->LoadCookies( aSourceFile );
}


bool KIPLATFORM::WEBVIEW::DeleteCookies( wxWebView* aWebView )
{
    KIWX_WEBVIEW_BACKEND* backend = backendOf( aWebView );
    return backend && backend->DeleteCookies();
}

#ifndef _WIN32
#pragma GCC visibility pop
#endif
