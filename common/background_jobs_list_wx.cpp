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

// The window of the list of the background jobs with wxWidgets (see background_job_list.h).

#include <unordered_map>

#include <wx/gauge.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <background_jobs_monitor.h>

#include "background_job_list.h"


class BACKGROUND_JOB_PANEL : public wxPanel
{
public:
    BACKGROUND_JOB_PANEL( wxWindow* aParent, std::shared_ptr<BACKGROUND_JOB> aJob ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, 75 ),
                     wxBORDER_SIMPLE ),
            m_job( aJob )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* mainSizer;
        mainSizer = new wxBoxSizer( wxVERTICAL );

        SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

        m_stName = new wxStaticText( this, wxID_ANY, aJob->m_name );
        m_stName->Wrap( -1 );
        m_stName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT,
                                   wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false,
                                   wxEmptyString ) );
        mainSizer->Add( m_stName, 0, wxALL | wxEXPAND, 1 );

        m_stStatus = new wxStaticText( this, wxID_ANY, aJob->m_status, wxDefaultPosition,
                                       wxDefaultSize, 0 );
        m_stStatus->Wrap( -1 );
        mainSizer->Add( m_stStatus, 0, wxALL | wxEXPAND, 1 );

        m_progress = new wxGauge( this, wxID_ANY, aJob->m_maxProgress, wxDefaultPosition,
                                  wxDefaultSize, wxGA_HORIZONTAL );
        m_progress->SetValue( 0 );
        mainSizer->Add( m_progress, 0, wxALL | wxEXPAND, 1 );

        SetSizer( mainSizer );
        Layout();

        UpdateFromJob();
    }


    void UpdateFromJob()
    {
        m_stStatus->SetLabelText( m_job->m_status );
        m_progress->SetValue( m_job->m_currentProgress );
        m_progress->SetRange( m_job->m_maxProgress );
    }

private:
    wxGauge* m_progress;
    wxStaticText* m_stName;
    wxStaticText* m_stStatus;
    std::shared_ptr<BACKGROUND_JOB> m_job;
};


class BACKGROUND_JOB_LIST_WX : public wxFrame, public BACKGROUND_JOB_LIST
{
public:
    BACKGROUND_JOB_LIST_WX( wxWindow* parent, const wxPoint& pos ) :
            wxFrame( parent, wxID_ANY, _( "Background Jobs" ), pos, wxSize( 300, 150 ),
                     wxFRAME_NO_TASKBAR | wxBORDER_SIMPLE )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

	    wxBoxSizer* bSizer1;
        bSizer1 = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition,
                                                 wxSize( -1, -1 ), wxVSCROLL );
        m_scrolledWindow->SetScrollRate( 5, 5 );
        m_contentSizer = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow->SetSizer( m_contentSizer );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );
        bSizer1->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 0 );

        Bind( wxEVT_KILL_FOCUS, &BACKGROUND_JOB_LIST_WX::onFocusLoss, this );

	    SetSizer( bSizer1 );
        Layout();

        SetFocus();
    }

    void onFocusLoss( wxFocusEvent& aEvent )
    {
        Close( true );
        aEvent.Skip();
    }


    void Add( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        BACKGROUND_JOB_PANEL* panel = new BACKGROUND_JOB_PANEL( m_scrolledWindow, aJob );
        m_contentSizer->Add( panel, 0, wxEXPAND | wxALL, 2 );
        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );

        // call this at this window otherwise the child panels don't resize width properly
        Layout();

        m_jobPanels[aJob] = panel;
    }


    void Remove( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        auto it = m_jobPanels.find( aJob );
        if( it != m_jobPanels.end() )
        {
            BACKGROUND_JOB_PANEL* panel = m_jobPanels[aJob];
            m_contentSizer->Detach( panel );
            panel->Destroy();

            m_jobPanels.erase( it );
        }
    }

    void UpdateJob( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        auto it = m_jobPanels.find( aJob );
        if( it != m_jobPanels.end() )
        {
            BACKGROUND_JOB_PANEL* panel = m_jobPanels[aJob];
            panel->UpdateFromJob();
        }
    }

private:
    wxScrolledWindow* m_scrolledWindow;
    wxBoxSizer*       m_contentSizer;
    std::unordered_map<std::shared_ptr<BACKGROUND_JOB>, BACKGROUND_JOB_PANEL*> m_jobPanels;
};


void BACKGROUND_JOBS_MONITOR::ShowList( wxWindow* aParent, wxPoint aPos )
{
    BACKGROUND_JOB_LIST_WX* list = new BACKGROUND_JOB_LIST_WX( aParent, aPos );

    std::shared_lock<std::shared_mutex> lock( m_mutex, std::try_to_lock );

    for( const std::shared_ptr<BACKGROUND_JOB>& job : m_jobs )
        list->Add( job );

    lock.unlock();

    m_shownDialogs.push_back( list );

    list->Bind( wxEVT_CLOSE_WINDOW,
                [this, list]( wxCloseEvent& aEvent )
                {
                    listClosed( list );
                    aEvent.Skip();
                } );

    // correct the position
    wxSize windowSize = list->GetSize();
    list->SetPosition( aPos - windowSize );

    list->Show();
}
