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

#ifndef HOME_WIDGETS_H
#define HOME_WIDGETS_H

#include <functional>
#include <wx/control.h>
#include <wx/panel.h>

class wxStaticText;


/**
 * Flat, rounded push button drawn with the UI_THEME palette.  Emits wxEVT_BUTTON.
 */
class FLAT_BUTTON : public wxControl
{
public:
    FLAT_BUTTON( wxWindow* aParent, const wxString& aLabel, bool aPrimary = false );

    void SetLabel( const wxString& aLabel ) override;

protected:
    wxSize DoGetBestSize() const override;

private:
    void onPaint( wxPaintEvent& aEvent );
    void onMouse( wxMouseEvent& aEvent );

    wxString m_label;
    bool     m_primary;
    bool     m_hover;
    bool     m_pressed;
};


/**
 * Clickable launcher card: monogram badge, title, one-line description, optional update badge.
 */
class LAUNCHER_CARD : public wxPanel
{
public:
    LAUNCHER_CARD( wxWindow* aParent, const wxString& aMonogram, const wxString& aTitle,
                   const wxString& aDescription, std::function<void()> aOnClick,
                   bool aCompact = false );

    void SetShowBadge( bool aShow ) { m_showCount = aShow; Refresh(); }
    void SetBadgeText( const wxString& aText ) { m_countText = aText; Refresh(); }

    bool Enable( bool aEnable = true ) override;

protected:
    wxSize DoGetBestSize() const override;

private:
    void onPaint( wxPaintEvent& aEvent );
    void onMouse( wxMouseEvent& aEvent );

    wxString              m_monogram;
    wxString              m_title;
    wxString              m_description;
    std::function<void()> m_onClick;
    bool                  m_compact;
    bool                  m_hover;
    bool                  m_showCount;
    wxString              m_countText;
};


/**
 * Section label in the accent colour, small caps (e.g. "DESIGN").
 */
wxStaticText* CreateSectionLabel( wxWindow* aParent, const wxString& aText );

/**
 * Large title used at the top of Home columns.
 */
wxStaticText* CreateHomeTitle( wxWindow* aParent, const wxString& aText );

#endif // HOME_WIDGETS_H
