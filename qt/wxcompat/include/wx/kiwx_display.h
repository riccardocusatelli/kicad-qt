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
 * @file kiwx_display.h
 * The screens (wxDisplay) and the fonts of the system (wxFontEnumerator) of the wxWidgets
 * compatibility layer.
 */

#ifndef KIWX_DISPLAY_H
#define KIWX_DISPLAY_H

#include <kiqt.h>

#include <wx/arrstr.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

class QScreen;
class wxWindow;


/// A mode of a screen (wxVideoMode).
struct wxVideoMode
{
    explicit wxVideoMode( int aWidth = 0, int aHeight = 0, int aDepth = 0, int aRefresh = 0 ) :
            w( aWidth ), h( aHeight ), bpp( aDepth ), refresh( aRefresh )
    {
    }

    int GetWidth() const { return w; }
    int GetHeight() const { return h; }
    int GetDepth() const { return bpp; }
    int GetRefresh() const { return refresh; }
    bool IsOk() const { return w > 0 && h > 0; }

    int w;
    int h;
    int bpp;
    int refresh;
};


/// A screen (wxDisplay): the screens of Qt, by their index.
class KIQT_API wxDisplay
{
public:
    explicit wxDisplay( unsigned int aIndex = 0 );
    explicit wxDisplay( const wxWindow* aWindow );

    static unsigned int GetCount();

    /// The index of the screen of a point (wxNOT_FOUND if none).
    static int GetFromPoint( const wxPoint& aPoint );

    /// The index of the screen of a window (wxNOT_FOUND if none).
    static int GetFromWindow( const wxWindow* aWindow );

    bool IsOk() const { return m_screen != nullptr; }
    wxRect GetGeometry() const;
    wxRect GetClientArea() const;
    wxString GetName() const;
    bool IsPrimary() const;
    double GetScaleFactor() const;
    wxSize GetPPI() const;
    wxVideoMode GetCurrentMode() const;

private:
    QScreen* m_screen;
};


/// The fonts of the system (wxFontEnumerator).
class KIQT_API wxFontEnumerator
{
public:
    virtual ~wxFontEnumerator() = default;

    /// Call OnFacename() for each font family (until it returns false).
    virtual bool EnumerateFacenames( int aEncoding = 0, bool aFixedWidthOnly = false );
    virtual bool OnFacename( const wxString& aFacename ) { return true; }

    static wxArrayString GetFacenames( int aEncoding = 0, bool aFixedWidthOnly = false );
    static bool IsValidFacename( const wxString& aFacename );
    static void InvalidateCache() {}
};

#endif // KIWX_DISPLAY_H
