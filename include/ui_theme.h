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

#pragma once

#include <kiplatform/ui.h>
#include <paths.h>
#include <wx/colour.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/font.h>
#include <wx/settings.h>
#include <wx/utils.h>

/**
 * Central colour tokens for the parts of the UI that KiCad draws itself (AUI frames, toolbars,
 * the project manager home, custom widgets).  Native controls keep the system look.
 *
 * Mode: the KICAD_UI_THEME environment variable ("dark" / "light") forces a variant; otherwise
 * the system dark-mode setting decides.
 */
namespace UI_THEME
{

struct PALETTE
{
    wxColour bg;            ///< Window / dock background
    wxColour surface;       ///< Cards, panels raised above the background
    wxColour surfaceHover;  ///< Hovered card
    wxColour border;        ///< Hairline separators and card outlines
    wxColour text;          ///< Primary text
    wxColour muted;         ///< Secondary text, captions
    wxColour accent;        ///< Primary action, focus, section labels
    wxColour accentText;    ///< Text on top of the accent colour
    wxColour badge;         ///< Background of monogram badges
};


inline bool IsDark()
{
    wxString mode;

    if( wxGetEnv( wxS( "KICAD_UI_THEME" ), &mode ) )
    {
        if( mode.CmpNoCase( wxS( "dark" ) ) == 0 )
            return true;

        if( mode.CmpNoCase( wxS( "light" ) ) == 0 )
            return false;
    }

    return KIPLATFORM::UI::IsDarkTheme();
}


inline const PALETTE& Get()
{
    // Dark "workbench" and light "airy" variants, matching the Home design mockups.
    static const PALETTE dark = {
        wxColour( 0x0F, 0x12, 0x17 ),   // bg
        wxColour( 0x16, 0x1A, 0x21 ),   // surface
        wxColour( 0x1D, 0x23, 0x2C ),   // surfaceHover
        wxColour( 0x27, 0x2E, 0x39 ),   // border
        wxColour( 0xE6, 0xE8, 0xEC ),   // text
        wxColour( 0x8A, 0x94, 0xA3 ),   // muted
        wxColour( 0x4C, 0x8D, 0xF6 ),   // accent
        wxColour( 0xFF, 0xFF, 0xFF ),   // accentText
        wxColour( 0x22, 0x29, 0x33 )    // badge
    };

    static const PALETTE light = {
        wxColour( 0xF6, 0xF7, 0xF9 ),
        wxColour( 0xFF, 0xFF, 0xFF ),
        wxColour( 0xF0, 0xF3, 0xF8 ),
        wxColour( 0xE1, 0xE4, 0xEA ),
        wxColour( 0x1C, 0x20, 0x26 ),
        wxColour( 0x6B, 0x72, 0x80 ),
        wxColour( 0x2F, 0x6F, 0xE4 ),
        wxColour( 0xFF, 0xFF, 0xFF ),
        wxColour( 0xEC, 0xEF, 0xF4 )
    };

    return IsDark() ? dark : light;
}


// ---------------------------------------------------------------------------------------------
// Fonts
//
// Inter (UI text) and JetBrains Mono (labels, badges, code-like values) ship in
// <stock data>/resources/fonts/ui and are registered as private fonts, so nothing is installed
// system-wide.  If loading fails everything falls back to the system fonts.
// ---------------------------------------------------------------------------------------------

inline const wxString UI_FACE = wxS( "Inter" );
inline const wxString MONO_FACE = wxS( "JetBrains Mono" );


/// Register the bundled fonts.  Call once, early, before windows are created.
inline bool LoadUiFonts()
{
    static int loaded = -1;

    if( loaded >= 0 )
        return loaded > 0;

    loaded = 0;

#if wxUSE_PRIVATE_FONTS
    wxString disable;

    if( wxGetEnv( wxS( "KICAD_UI_FONT" ), &disable ) && disable.CmpNoCase( wxS( "system" ) ) == 0 )
        return false;

    wxFileName dir( PATHS::GetStockDataPath(), wxEmptyString );
    dir.AppendDir( wxS( "resources" ) );
    dir.AppendDir( wxS( "fonts" ) );
    dir.AppendDir( wxS( "ui" ) );

    wxArrayString files;

    if( dir.DirExists() )
    {
        wxDir::GetAllFiles( dir.GetPath(), &files, wxS( "*.otf" ), wxDIR_FILES );
        wxDir::GetAllFiles( dir.GetPath(), &files, wxS( "*.ttf" ), wxDIR_FILES );
    }

    for( const wxString& file : files )
    {
        if( wxFont::AddPrivateFont( file ) )
            loaded = 1;
    }
#endif

    return loaded > 0;
}


/// Point size of the platform GUI font; the bundled font is used at the same size.
inline int BaseFontSize()
{
    return wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();
}


/// Main UI font (Inter), or the system GUI font if the bundled fonts are not available.
inline wxFont GetUiFont( int aRelativeSize = 0 )
{
    wxFont sys = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( !LoadUiFonts() )
    {
        sys.SetPointSize( sys.GetPointSize() + aRelativeSize );
        return sys;
    }

    wxFont font( BaseFontSize() + aRelativeSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                 wxFONTWEIGHT_NORMAL, false, UI_FACE );

    return font.IsOk() ? font : sys;
}


/// Monospaced font for labels and badges (JetBrains Mono), or the system monospace font.
inline wxFont GetMonoFont( int aRelativeSize = 0, bool aBold = false )
{
    wxFont font( BaseFontSize() + aRelativeSize, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                 aBold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL, false,
                 LoadUiFonts() ? MONO_FACE : wxString() );

    return font;
}

} // namespace UI_THEME
