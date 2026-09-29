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

#include "home_widgets.h"

#include <ui_theme.h>

#include <wx/dcbuffer.h>
#include <wx/stattext.h>


// ---------------------------------------------------------------------------------------------
// FLAT_BUTTON
// ---------------------------------------------------------------------------------------------

FLAT_BUTTON::FLAT_BUTTON( wxWindow* aParent, const wxString& aLabel, bool aPrimary ) :
        wxControl( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE ),
        m_label( aLabel ),
        m_primary( aPrimary ),
        m_hover( false ),
        m_pressed( false )
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetCursor( wxCursor( wxCURSOR_HAND ) );

    Bind( wxEVT_PAINT, &FLAT_BUTTON::onPaint, this );
    Bind( wxEVT_ENTER_WINDOW, &FLAT_BUTTON::onMouse, this );
    Bind( wxEVT_LEAVE_WINDOW, &FLAT_BUTTON::onMouse, this );
    Bind( wxEVT_LEFT_DOWN, &FLAT_BUTTON::onMouse, this );
    Bind( wxEVT_LEFT_UP, &FLAT_BUTTON::onMouse, this );

    InvalidateBestSize();
    SetMinSize( GetBestSize() );
}


void FLAT_BUTTON::SetLabel( const wxString& aLabel )
{
    m_label = aLabel;
    InvalidateBestSize();
    SetMinSize( GetBestSize() );
    Refresh();
}


wxSize FLAT_BUTTON::DoGetBestSize() const
{
    wxClientDC dc( const_cast<FLAT_BUTTON*>( this ) );
    dc.SetFont( GetFont() );
    wxSize text = dc.GetTextExtent( m_label );

    return wxSize( text.x + FromDIP( 24 ), text.y + FromDIP( 12 ) );
}


void FLAT_BUTTON::onPaint( wxPaintEvent& aEvent )
{
    wxAutoBufferedPaintDC    dc( this );
    const UI_THEME::PALETTE& theme = UI_THEME::Get();
    wxSize                   size = GetClientSize();

    dc.SetBackground( wxBrush( GetParent()->GetBackgroundColour() ) );
    dc.Clear();

    wxColour fill, border, text;

    if( m_primary )
    {
        fill = m_hover ? theme.accent.ChangeLightness( 110 ) : theme.accent;
        border = fill;
        text = theme.accentText;
    }
    else
    {
        fill = m_hover ? theme.surfaceHover : theme.surface;
        border = m_hover ? theme.muted : theme.border;
        text = theme.text;
    }

    if( m_pressed )
        fill = fill.ChangeLightness( 92 );

    if( !IsEnabled() )
        text = theme.muted;

    dc.SetPen( wxPen( border, 1 ) );
    dc.SetBrush( wxBrush( fill ) );
    dc.DrawRoundedRectangle( 0, 0, size.x, size.y, FromDIP( 5 ) );

    dc.SetFont( GetFont() );
    dc.SetTextForeground( text );
    wxSize ts = dc.GetTextExtent( m_label );
    dc.DrawText( m_label, ( size.x - ts.x ) / 2, ( size.y - ts.y ) / 2 );
}


void FLAT_BUTTON::onMouse( wxMouseEvent& aEvent )
{
    if( aEvent.Entering() )
    {
        m_hover = true;
    }
    else if( aEvent.Leaving() )
    {
        m_hover = false;
        m_pressed = false;
    }
    else if( aEvent.LeftDown() )
    {
        m_pressed = true;
    }
    else if( aEvent.LeftUp() )
    {
        bool fire = m_pressed && IsEnabled();
        m_pressed = false;
        Refresh();

        if( fire )
        {
            wxCommandEvent evt( wxEVT_BUTTON, GetId() );
            evt.SetEventObject( this );
            ProcessWindowEvent( evt );
        }

        return;
    }

    Refresh();
    aEvent.Skip();
}


// ---------------------------------------------------------------------------------------------
// LAUNCHER_CARD
// ---------------------------------------------------------------------------------------------

LAUNCHER_CARD::LAUNCHER_CARD( wxWindow* aParent, const wxString& aMonogram, const wxString& aTitle,
                              const wxString& aDescription, std::function<void()> aOnClick,
                              bool aCompact ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE ),
        m_monogram( aMonogram ),
        m_title( aTitle ),
        m_description( aDescription ),
        m_onClick( std::move( aOnClick ) ),
        m_compact( aCompact ),
        m_hover( false ),
        m_showCount( false )
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetCursor( wxCursor( wxCURSOR_HAND ) );
    SetToolTip( aDescription );

    Bind( wxEVT_PAINT, &LAUNCHER_CARD::onPaint, this );
    Bind( wxEVT_ENTER_WINDOW, &LAUNCHER_CARD::onMouse, this );
    Bind( wxEVT_LEAVE_WINDOW, &LAUNCHER_CARD::onMouse, this );
    Bind( wxEVT_LEFT_UP, &LAUNCHER_CARD::onMouse, this );

    InvalidateBestSize();
    SetMinSize( GetBestSize() );
}


bool LAUNCHER_CARD::Enable( bool aEnable )
{
    bool changed = wxPanel::Enable( aEnable );
    SetCursor( wxCursor( aEnable ? wxCURSOR_HAND : wxCURSOR_ARROW ) );
    Refresh();
    return changed;
}


wxSize LAUNCHER_CARD::DoGetBestSize() const
{
    int lineH = GetCharHeight();

    if( m_compact )
        return wxSize( FromDIP( 150 ), FromDIP( 30 ) + lineH * 2 + FromDIP( 26 ) );

    return wxSize( FromDIP( 240 ), lineH * 2 + FromDIP( 26 ) );
}


void LAUNCHER_CARD::onPaint( wxPaintEvent& aEvent )
{
    wxAutoBufferedPaintDC    dc( this );
    const UI_THEME::PALETTE& theme = UI_THEME::Get();
    const wxSize             size = GetClientSize();
    const bool               enabled = IsEnabled();
    const int                pad = FromDIP( 12 );
    const int                badgeSize = FromDIP( 30 );

    dc.SetBackground( wxBrush( GetParent()->GetBackgroundColour() ) );
    dc.Clear();

    // Card
    bool hover = m_hover && enabled;
    dc.SetPen( wxPen( hover ? theme.accent : theme.border, 1 ) );
    dc.SetBrush( wxBrush( hover ? theme.surfaceHover : theme.surface ) );
    dc.DrawRoundedRectangle( 0, 0, size.x, size.y, FromDIP( 6 ) );

    // Monogram badge
    wxRect badge( pad, m_compact ? pad : ( size.y - badgeSize ) / 2, badgeSize, badgeSize );
    dc.SetPen( wxPen( theme.border, 1 ) );
    dc.SetBrush( wxBrush( theme.badge ) );
    dc.DrawRoundedRectangle( badge, FromDIP( 5 ) );

    wxFont mono = UI_THEME::GetMonoFont( -1, true );
    dc.SetFont( mono );
    dc.SetTextForeground( enabled ? theme.accent : theme.muted );
    wxSize ms = dc.GetTextExtent( m_monogram );
    dc.DrawText( m_monogram, badge.x + ( badge.width - ms.x ) / 2,
                 badge.y + ( badge.height - ms.y ) / 2 );

    // Update-count pill (plugin manager)
    if( m_showCount && !m_countText.IsEmpty() )
    {
        dc.SetFont( GetFont().Smaller().Bold() );
        wxSize cs = dc.GetTextExtent( m_countText );
        int    d = std::max( cs.x, cs.y ) + FromDIP( 4 );
        wxRect pill( badge.GetRight() - d / 2, badge.y - d / 3, d, d );

        dc.SetPen( *wxTRANSPARENT_PEN );
        dc.SetBrush( wxBrush( wxColour( 0xE5, 0x48, 0x4D ) ) );
        dc.DrawEllipse( pill );
        dc.SetTextForeground( *wxWHITE );
        dc.DrawText( m_countText, pill.x + ( pill.width - cs.x ) / 2,
                     pill.y + ( pill.height - cs.y ) / 2 );
    }

    // Text
    int textX = m_compact ? pad : badge.GetRight() + pad;
    int textW = size.x - textX - pad;
    int textY = m_compact ? badge.GetBottom() + FromDIP( 10 )
                          : ( size.y - ( GetCharHeight() * 2 + FromDIP( 2 ) ) ) / 2;

    dc.SetFont( GetFont().Bold() );
    dc.SetTextForeground( enabled ? theme.text : theme.muted );
    dc.DrawText( wxControl::Ellipsize( m_title, dc, wxELLIPSIZE_END, textW ), textX, textY );

    textY += dc.GetCharHeight() + FromDIP( 2 );

    dc.SetFont( GetFont().Smaller() );
    dc.SetTextForeground( theme.muted );
    dc.DrawText( wxControl::Ellipsize( m_description, dc, wxELLIPSIZE_END, textW ), textX, textY );
}


void LAUNCHER_CARD::onMouse( wxMouseEvent& aEvent )
{
    if( aEvent.Entering() )
    {
        m_hover = true;
        Refresh();
    }
    else if( aEvent.Leaving() )
    {
        m_hover = false;
        Refresh();
    }
    else if( aEvent.LeftUp() && IsEnabled() && m_onClick )
    {
        m_onClick();
        return;
    }

    aEvent.Skip();
}


// ---------------------------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------------------------

wxStaticText* CreateSectionLabel( wxWindow* aParent, const wxString& aText )
{
    wxStaticText* label = new wxStaticText( aParent, wxID_ANY, aText.Upper() );
    wxFont        font = UI_THEME::GetMonoFont( -2, true );

    label->SetFont( font );
    label->SetForegroundColour( UI_THEME::Get().accent );
    return label;
}


wxStaticText* CreateHomeTitle( wxWindow* aParent, const wxString& aText )
{
    wxStaticText* title = new wxStaticText( aParent, wxID_ANY, aText );
    wxFont        font = aParent->GetFont().Bold();

    font.SetFractionalPointSize( font.GetFractionalPointSize() * 1.35 );
    title->SetFont( font );
    title->SetForegroundColour( UI_THEME::Get().text );
    return title;
}
