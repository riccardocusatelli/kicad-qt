/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

// The window of the list of the notifications with wxWidgets (see notifications_list.h).

#include <unordered_map>

#include <wx/frame.h>
#include <wx/hyperlink.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/time.h>
#include <wx/utils.h>

#include <kiplatform/ui.h>
#include <notifications_manager.h>
#include <widgets/ui_common.h>

#include "notifications_list.h"


static long long g_last_closed_timer = 0;


class NOTIFICATION_PANEL : public wxPanel
{
public:
    NOTIFICATION_PANEL( wxWindow* aParent, NOTIFICATIONS_MANAGER* aManager, NOTIFICATION* aNoti ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, 75 ), wxBORDER_SIMPLE ),
            m_hlDetails( nullptr ),
            m_notification( aNoti ),
            m_manager( aManager )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        wxColour fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );
        SetBackgroundColour( bg );
        SetForegroundColour( fg );

        m_stTitle = new wxStaticText( this, wxID_ANY, aNoti->title );
        m_stTitle->Wrap( -1 );
        m_stTitle->SetFont( KIUI::GetControlFont( this ).Bold() );
        mainSizer->Add( m_stTitle, 0, wxALL | wxEXPAND, 1 );

        m_stDescription = new wxStaticText( this, wxID_ANY, aNoti->description );
        m_stDescription->Wrap( -1 );
        mainSizer->Add( m_stDescription, 0, wxALL | wxEXPAND, 1 );

        wxBoxSizer* tailSizer;
        tailSizer = new wxBoxSizer( wxHORIZONTAL );

        if( !aNoti->href.IsEmpty() )
        {
            m_hlDetails = new wxHyperlinkCtrl( this, wxID_ANY, _( "View Details" ), aNoti->href );
            tailSizer->Add( m_hlDetails, 0, wxALL, 2 );
        }

        m_hlDismiss = new wxHyperlinkCtrl( this, wxID_ANY, _( "Dismiss" ), aNoti->href );
        tailSizer->Add( m_hlDismiss, 0, wxALL, 2 );

        mainSizer->Add( tailSizer, 1, wxEXPAND, 5 );

        if( m_hlDetails != nullptr )
            m_hlDetails->Bind( wxEVT_HYPERLINK, &NOTIFICATION_PANEL::onDetails, this );

        m_hlDismiss->Bind( wxEVT_HYPERLINK, &NOTIFICATION_PANEL::onDismiss, this );

        SetSizer( mainSizer );
        Layout();
    }

private:
    void onDetails( wxHyperlinkEvent& aEvent )
    {
        wxString url = aEvent.GetURL();

        if( url.StartsWith( wxS( "kicad://" ) ) )
        {
            url.Replace( wxS( "kicad://" ), wxS( "" ) );

            if( url == wxS( "pcm" ) )
            {
                // TODO
            }
        }
        else
        {
            wxLaunchDefaultBrowser( aEvent.GetURL(), wxBROWSER_NEW_WINDOW );
        }
    }

    void onDismiss( wxHyperlinkEvent& aEvent )
    {
        CallAfter(
                [this]()
                {
                    // This will cause this panel to get deleted
                    m_manager->Remove( m_notification->key );
                } );
    }

private:
    wxStaticText*          m_stTitle;
    wxStaticText*          m_stDescription;
    wxHyperlinkCtrl*       m_hlDetails;
    wxHyperlinkCtrl*       m_hlDismiss;
    NOTIFICATION*          m_notification;
    NOTIFICATIONS_MANAGER* m_manager;
};


class NOTIFICATIONS_LIST_WX : public wxFrame, public NOTIFICATIONS_LIST
{
public:
    NOTIFICATIONS_LIST_WX( NOTIFICATIONS_MANAGER* aManager, wxWindow* parent, const wxPoint& pos ) :
            wxFrame( parent, wxID_ANY, _( "Notifications" ), pos, wxSize( 300, 150 ),
                     wxFRAME_NO_TASKBAR | wxBORDER_SIMPLE ),
            m_manager( aManager )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* bSizer1;
        bSizer1 = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition,
                                                 wxSize( -1, -1 ), wxVSCROLL | wxBORDER_SIMPLE );
        wxColour fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );
        m_scrolledWindow->SetBackgroundColour( bg );
        m_scrolledWindow->SetForegroundColour( fg );

        m_scrolledWindow->SetScrollRate( 5, 5 );
        m_contentSizer = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow->SetSizer( m_contentSizer );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );
        bSizer1->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 0 );

        m_noNotificationsText = new wxStaticText( m_scrolledWindow, wxID_ANY,
                                                  _( "There are no notifications available" ),
                                                  wxDefaultPosition, wxDefaultSize,
                                                  wxALIGN_CENTER_HORIZONTAL );
        m_noNotificationsText->Wrap( -1 );
        m_contentSizer->Add( m_noNotificationsText, 1, wxALL | wxEXPAND, 5 );

        Bind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST_WX::onFocusLoss, this );
        m_scrolledWindow->Bind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST_WX::onFocusLoss, this );

        SetSizer( bSizer1 );
        Layout();

        SetFocus();
    }


    ~NOTIFICATIONS_LIST_WX()
    {
        // Unbind focus handlers before base class destructors run. GTK fires focus-out signals
        // during window teardown, which would invoke onFocusLoss through a partially-destroyed
        // object and crash in the wxWidgets event handler chain.
        Unbind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST_WX::onFocusLoss, this );
        m_scrolledWindow->Unbind( wxEVT_KILL_FOCUS, &NOTIFICATIONS_LIST_WX::onFocusLoss, this );
    }


    void onFocusLoss( wxFocusEvent& aEvent )
    {
        if( IsDescendant( aEvent.GetWindow() ) )
        {
            // Child (such as the hyperlink texts) got focus
        }
        else
        {
            Close( true );
            g_last_closed_timer = wxGetLocalTimeMillis().GetValue();
        }

        aEvent.Skip();
    }


    void Add( NOTIFICATION* aNoti ) override
    {
        m_noNotificationsText->Hide();

        NOTIFICATION_PANEL* panel = new NOTIFICATION_PANEL( m_scrolledWindow, m_manager, aNoti );
        m_contentSizer->Add( panel, 0, wxEXPAND | wxALL, 2 );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );

        // call this at this window otherwise the child panels don't resize width properly
        Layout();

        m_panelMap[aNoti] = panel;
    }


    void Remove( NOTIFICATION* aNoti ) override
    {
        auto it = m_panelMap.find( aNoti );

        if( it != m_panelMap.end() )
        {
            NOTIFICATION_PANEL* panel = m_panelMap[aNoti];
            m_contentSizer->Detach( panel );
            panel->Destroy();

            m_panelMap.erase( it );

            // ensure the window contents get shifted as needed
            m_scrolledWindow->Layout();
            Layout();
        }

        if( m_panelMap.size() == 0 )
        {
            m_noNotificationsText->Show();
        }
    }

private:
    wxScrolledWindow*                                      m_scrolledWindow;

    /// Inner content of the scrolled window, add panels here.
    wxBoxSizer*                                            m_contentSizer;
    std::unordered_map<NOTIFICATION*, NOTIFICATION_PANEL*> m_panelMap;
    NOTIFICATIONS_MANAGER*                                 m_manager;

public:
    /// Tell the manager that the window closes.
    void NotifyClosed() { notifyClosed( m_manager, this ); }

private:
    /// Text to be displayed when no notifications are present, this gets a Show/Hide call as
    /// needed.
    wxStaticText*                                          m_noNotificationsText;
};


void NOTIFICATIONS_MANAGER::ShowList( wxWindow* aParent, wxPoint aPos )
{
    // Debounce clicking on the icon with a list already showing.  The button will get focus
    // first, which will cause a focus-loss on the list (thereby closing it), and then we'd open
    // it again without this guard.
    if( wxGetLocalTimeMillis().GetValue() - g_last_closed_timer < 300 )
    {
        g_last_closed_timer = 0;
        return;
    }

    NOTIFICATIONS_LIST_WX* list = new NOTIFICATIONS_LIST_WX( this, aParent, aPos );

    for( NOTIFICATION& job : m_notifications )
        list->Add( &job );

    m_shownDialogs.push_back( list );

    list->Bind( wxEVT_CLOSE_WINDOW,
                [list]( wxCloseEvent& aEvent )
                {
                    list->NotifyClosed();
                    aEvent.Skip();
                } );

    // correct the position
    wxSize windowSize = list->GetSize();
    list->SetPosition( aPos - windowSize );

    list->Show();
    KIPLATFORM::UI::ForceFocus( list );
}
