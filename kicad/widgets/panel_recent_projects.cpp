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

#include "panel_recent_projects.h"

#include <kicad_manager_frame.h>
#include <file_history.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <wildcards_and_files_ext.h>

#include <wx/button.h>
#include <wx/control.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


namespace
{

/// Linear blend of two colours; aAmount = 0 gives aA, 1 gives aB.
wxColour blend( const wxColour& aA, const wxColour& aB, double aAmount )
{
    auto mix = [&]( unsigned char a, unsigned char b )
               {
                   return (unsigned char) ( a + ( b - a ) * aAmount );
               };

    return wxColour( mix( aA.Red(), aB.Red() ), mix( aA.Green(), aB.Green() ),
                     mix( aA.Blue(), aB.Blue() ) );
}


wxColour bgColour()      { return wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ); }
wxColour fgColour()      { return wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ); }
wxColour mutedColour()   { return blend( bgColour(), fgColour(), 0.55 ); }
wxColour borderColour()  { return blend( bgColour(), fgColour(), 0.14 ); }
wxColour hoverColour()   { return blend( bgColour(), fgColour(), 0.05 ); }
wxColour accentColour()  { return wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ); }

} // namespace


// ---------------------------------------------------------------------------------------------
// RECENT_PROJECT_ITEM
// ---------------------------------------------------------------------------------------------

RECENT_PROJECT_ITEM::RECENT_PROJECT_ITEM( wxWindow* aParent, KICAD_MANAGER_FRAME* aFrame,
                                          const wxString& aProjectFile ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE ),
        m_frame( aFrame ),
        m_projectFile( aProjectFile ),
        m_exists( false ),
        m_hasSchematic( false ),
        m_hasBoard( false ),
        m_hover( false )
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );

    wxFileName fn( aProjectFile );
    m_name = fn.GetName();
    m_folder = fn.GetPath();
    m_exists = fn.FileExists();

    // Only cheap stat() calls here: the list must stay instant even with a long history.
    if( m_exists )
    {
        m_modified = fn.GetModificationTime();

        wxFileName sch( fn );
        sch.SetExt( FILEEXT::KiCadSchematicFileExtension );
        m_hasSchematic = sch.FileExists();

        wxFileName pcb( fn );
        pcb.SetExt( FILEEXT::KiCadPcbFileExtension );
        m_hasBoard = pcb.FileExists();

        // The project file is rarely touched; the documents tell when work really happened.
        for( const wxFileName& doc : { sch, pcb } )
        {
            if( doc.FileExists() )
            {
                wxDateTime t = doc.GetModificationTime();

                if( t.IsValid() && ( !m_modified.IsValid() || t > m_modified ) )
                    m_modified = t;
            }
        }
    }

    // Three text lines plus padding.
    int lineH = GetCharHeight();
    SetMinSize( wxSize( FromDIP( 180 ), lineH * 3 + FromDIP( 26 ) ) );

    SetToolTip( m_exists ? m_projectFile
                         : wxString::Format( _( "Project not found: %s" ), m_projectFile ) );

    if( m_exists )
        SetCursor( wxCursor( wxCURSOR_HAND ) );

    Bind( wxEVT_PAINT, &RECENT_PROJECT_ITEM::onPaint, this );
    Bind( wxEVT_ENTER_WINDOW, &RECENT_PROJECT_ITEM::onEnter, this );
    Bind( wxEVT_LEAVE_WINDOW, &RECENT_PROJECT_ITEM::onLeave, this );
    Bind( wxEVT_LEFT_UP, &RECENT_PROJECT_ITEM::onClick, this );
}


void RECENT_PROJECT_ITEM::onPaint( wxPaintEvent& aEvent )
{
    wxAutoBufferedPaintDC dc( this );

    const wxSize size = GetClientSize();
    const int    pad = FromDIP( 10 );
    const int    radius = FromDIP( 6 );

    dc.SetBackground( wxBrush( GetParent()->GetBackgroundColour() ) );
    dc.Clear();

    // Card
    wxRect card( FromDIP( 2 ), FromDIP( 2 ), size.x - FromDIP( 4 ), size.y - FromDIP( 4 ) );
    dc.SetPen( wxPen( m_hover ? accentColour() : borderColour(), 1 ) );
    dc.SetBrush( wxBrush( m_hover ? hoverColour() : bgColour() ) );
    dc.DrawRoundedRectangle( card, radius );

    const int textLeft = card.x + pad;
    const int textRight = card.GetRight() - pad;
    int       y = card.y + pad - FromDIP( 2 );

    // Line 1: name (bold) + relative time (right aligned)
    wxFont base = GetFont();
    wxFont bold = base.Bold();

    wxString when = m_exists ? PANEL_RECENT_PROJECTS::FormatRelativeTime( m_modified )
                             : _( "not found" );

    dc.SetFont( base );
    wxSize whenSize = dc.GetTextExtent( when );
    dc.SetTextForeground( mutedColour() );
    dc.DrawText( when, textRight - whenSize.x, y );

    dc.SetFont( bold );
    dc.SetTextForeground( m_exists ? fgColour() : mutedColour() );
    int nameWidth = std::max( 10, textRight - textLeft - whenSize.x - pad );
    dc.DrawText( wxControl::Ellipsize( m_name, dc, wxELLIPSIZE_END, nameWidth ), textLeft, y );

    y += dc.GetCharHeight() + FromDIP( 3 );

    // Line 2: folder, shortened in the middle so both the root and the leaf stay visible
    dc.SetFont( base );
    dc.SetTextForeground( mutedColour() );
    dc.DrawText( wxControl::Ellipsize( m_folder, dc, wxELLIPSIZE_MIDDLE, textRight - textLeft ),
                 textLeft, y );

    y += dc.GetCharHeight() + FromDIP( 5 );

    // Line 3: small tags for the documents that exist
    if( m_exists )
    {
        wxFont small = base.Smaller();
        dc.SetFont( small );

        int x = textLeft;

        auto drawTag =
                [&]( const wxString& aText )
                {
                    wxSize ts = dc.GetTextExtent( aText );
                    wxRect tag( x, y, ts.x + FromDIP( 10 ), ts.y + FromDIP( 4 ) );

                    dc.SetPen( wxPen( borderColour(), 1 ) );
                    dc.SetBrush( *wxTRANSPARENT_BRUSH );
                    dc.DrawRoundedRectangle( tag, FromDIP( 3 ) );
                    dc.SetTextForeground( mutedColour() );
                    dc.DrawText( aText, tag.x + FromDIP( 5 ), tag.y + FromDIP( 2 ) );

                    x = tag.GetRight() + FromDIP( 6 );
                };

        if( m_hasSchematic )
            drawTag( _( "Schematic" ) );

        if( m_hasBoard )
            drawTag( _( "PCB" ) );
    }
}


void RECENT_PROJECT_ITEM::onEnter( wxMouseEvent& aEvent )
{
    if( m_exists )
    {
        m_hover = true;
        Refresh();
    }

    aEvent.Skip();
}


void RECENT_PROJECT_ITEM::onLeave( wxMouseEvent& aEvent )
{
    m_hover = false;
    Refresh();
    aEvent.Skip();
}


void RECENT_PROJECT_ITEM::onClick( wxMouseEvent& aEvent )
{
    if( !m_exists )
        return;

    // Deferred: loading a project rebuilds this list, which would delete us mid-handler.
    KICAD_MANAGER_FRAME* frame = m_frame;
    wxString             file = m_projectFile;

    frame->CallAfter( [frame, file]()
                      {
                          frame->LoadProject( wxFileName( file ) );
                      } );
}


// ---------------------------------------------------------------------------------------------
// PANEL_RECENT_PROJECTS
// ---------------------------------------------------------------------------------------------

PANEL_RECENT_PROJECTS::PANEL_RECENT_PROJECTS( KICAD_MANAGER_FRAME* aFrame, wxWindow* aParent ) :
        wxPanel( aParent, wxID_ANY ),
        m_frame( aFrame )
{
    SetBackgroundColour( bgColour() );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // Header: title on the left, primary actions on the right
    wxBoxSizer*   header = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* title = new wxStaticText( this, wxID_ANY, _( "Recent Projects" ) );
    wxFont        titleFont = title->GetFont().Bold();
    titleFont.SetFractionalPointSize( titleFont.GetFractionalPointSize() * 1.25 );
    title->SetFont( titleFont );

    wxButton* newBtn = new wxButton( this, wxID_ANY, _( "New Project..." ) );
    wxButton* openBtn = new wxButton( this, wxID_ANY, _( "Open..." ) );

    newBtn->Bind( wxEVT_BUTTON,
                  [this]( wxCommandEvent& )
                  {
                      m_frame->GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::newProject );
                  } );

    openBtn->Bind( wxEVT_BUTTON,
                   [this]( wxCommandEvent& )
                   {
                       m_frame->GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::openProject );
                   } );

    header->Add( title, 1, wxALIGN_CENTER_VERTICAL );
    header->Add( newBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP( 6 ) );
    header->Add( openBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP( 6 ) );

    mainSizer->Add( header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP( 10 ) );

    // Card list
    m_list = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    m_list->SetScrollRate( 0, FromDIP( 8 ) );
    m_list->SetBackgroundColour( bgColour() );
    m_listSizer = new wxBoxSizer( wxVERTICAL );
    m_list->SetSizer( m_listSizer );

    mainSizer->Add( m_list, 1, wxEXPAND | wxALL, FromDIP( 6 ) );

    // Footer
    m_footer = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_footer->SetForegroundColour( mutedColour() );
    mainSizer->Add( m_footer, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP( 10 ) );

    SetSizer( mainSizer );

    Bind( wxEVT_SYS_COLOUR_CHANGED, &PANEL_RECENT_PROJECTS::onThemeChanged, this );

    RebuildList();
}


void PANEL_RECENT_PROJECTS::RebuildList()
{
    m_list->Freeze();
    m_listSizer->Clear( true );

    FILE_HISTORY& history = m_frame->GetFileHistory();
    int           count = 0;

    for( size_t i = 0; i < history.GetCount(); ++i )
    {
        wxString file = history.GetHistoryFile( i );

        if( file.IsEmpty() )
            continue;

        RECENT_PROJECT_ITEM* item = new RECENT_PROJECT_ITEM( m_list, m_frame, file );
        m_listSizer->Add( item, 0, wxEXPAND | wxBOTTOM, FromDIP( 2 ) );
        count++;
    }

    if( count == 0 )
    {
        wxStaticText* empty = new wxStaticText( m_list, wxID_ANY,
                                                _( "No recent projects.\n"
                                                   "Create a new project or open an existing one." ) );
        empty->SetForegroundColour( mutedColour() );
        m_listSizer->Add( empty, 0, wxALL, FromDIP( 8 ) );
    }

    m_footer->SetLabel( wxString::Format( wxPLURAL( "%d project", "%d projects", count ), count ) );

    m_list->FitInside();
    m_list->Thaw();
    Layout();
}


wxString PANEL_RECENT_PROJECTS::FormatRelativeTime( const wxDateTime& aTime )
{
    if( !aTime.IsValid() )
        return wxEmptyString;

    wxTimeSpan span = wxDateTime::Now() - aTime;
    long       minutes = span.GetMinutes();
    long       hours = span.GetHours();
    long       days = span.GetDays();

    if( minutes < 1 )
        return _( "just now" );

    if( minutes < 60 )
        return wxString::Format( wxPLURAL( "%ld minute ago", "%ld minutes ago", minutes ), minutes );

    if( hours < 24 )
        return wxString::Format( wxPLURAL( "%ld hour ago", "%ld hours ago", hours ), hours );

    if( days == 1 )
        return _( "yesterday" );

    if( days < 7 )
        return wxString::Format( wxPLURAL( "%ld day ago", "%ld days ago", days ), days );

    if( days < 35 )
    {
        long weeks = days / 7;
        return wxString::Format( wxPLURAL( "%ld week ago", "%ld weeks ago", weeks ), weeks );
    }

    return aTime.FormatDate();
}


void PANEL_RECENT_PROJECTS::onThemeChanged( wxSysColourChangedEvent& aEvent )
{
    SetBackgroundColour( bgColour() );
    m_list->SetBackgroundColour( bgColour() );
    m_footer->SetForegroundColour( mutedColour() );
    RebuildList();
    aEvent.Skip();
}
