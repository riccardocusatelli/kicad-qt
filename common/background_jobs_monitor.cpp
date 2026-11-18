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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <mutex>


#include <background_jobs_monitor.h>
#include <toolkit/process.h>
#include <widgets/kistatusbar.h>

#include "background_job_list.h"


BACKGROUND_JOB_REPORTER::BACKGROUND_JOB_REPORTER( BACKGROUND_JOBS_MONITOR* aMonitor,
                                                  const std::shared_ptr<BACKGROUND_JOB>& aJob ) :
        PROGRESS_REPORTER_BASE( 1 ),
        m_monitor( aMonitor ),
        m_job( aJob )
{

}


bool BACKGROUND_JOB_REPORTER::updateUI()
{
    return !m_cancelled;
}


void BACKGROUND_JOB_REPORTER::Report( const wxString& aMessage )
{
    m_job->SetStatus( aMessage );
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::SetNumPhases( int aNumPhases )
{
    PROGRESS_REPORTER_BASE::SetNumPhases( aNumPhases );
    m_job->m_maxProgress.store( m_numPhases.load() );
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::AdvancePhase()
{
    PROGRESS_REPORTER_BASE::AdvancePhase();
    m_job->m_currentProgress.store( m_phase.load() );
    m_monitor->jobUpdated( m_job );
}


void BACKGROUND_JOB_REPORTER::SetCurrentProgress( double aProgress )
{
    PROGRESS_REPORTER_BASE::SetCurrentProgress( aProgress );
    m_job->m_maxProgress.store( 1000 );
    m_job->m_currentProgress.store( static_cast<int>( 1000 * aProgress ) );
    m_monitor->jobUpdated( m_job );
}


BACKGROUND_JOBS_MONITOR::BACKGROUND_JOBS_MONITOR()
{

}


std::shared_ptr<BACKGROUND_JOB> BACKGROUND_JOBS_MONITOR::Create( const wxString& aName )
{
    std::shared_ptr<BACKGROUND_JOB> job = std::make_shared<BACKGROUND_JOB>();

    job->m_name = aName;
    job->m_reporter = std::make_shared<BACKGROUND_JOB_REPORTER>( this, job );

    std::lock_guard<std::shared_mutex> lock( m_mutex );
    m_jobs.push_back( job );

    // The lists are updated from the event loop, if they are still shown then
    for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
    {
        TOOLKIT::RunOnMainThread(
                [=, this]()
                {
                    if( isShown( list ) )
                        list->Add( job );
                } );
    }

    return job;
}


void BACKGROUND_JOBS_MONITOR::Remove( std::shared_ptr<BACKGROUND_JOB> aJob )
{
    std::shared_ptr<BACKGROUND_JOB> frontJob;

    {
        std::lock_guard<std::shared_mutex> lock( m_mutex );

        m_jobs.erase( std::remove( m_jobs.begin(), m_jobs.end(), aJob ), m_jobs.end() );

        for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
        {
            TOOLKIT::RunOnMainThread(
                    [=, this]()
                    {
                        if( isShown( list ) )
                            list->Remove( aJob );
                    } );
        }

        if( m_jobs.empty() )
        {
            for( KISTATUSBAR* statusBar : m_statusBars )
            {
                TOOLKIT::RunOnMainThread(
                        [=, this]()
                        {
                            if( !isRegistered( statusBar ) )
                                return;

                            statusBar->HideBackgroundProgressBar();
                            statusBar->SetBackgroundStatusText( wxT( "" ) );
                        } );
            }
        }
        else
        {
            frontJob = m_jobs.front();
        }
    }

    // jobUpdated takes the mutex so call it after we release ours
    if( frontJob )
        jobUpdated( frontJob );
}


void BACKGROUND_JOBS_MONITOR::listClosed( BACKGROUND_JOB_LIST* aList )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );

    m_shownDialogs.erase( std::remove_if( m_shownDialogs.begin(), m_shownDialogs.end(),
                                          [&]( BACKGROUND_JOB_LIST* dialog )
                                          {
                                              return dialog == aList;
                                          } ),
                          m_shownDialogs.end() );
}


bool BACKGROUND_JOBS_MONITOR::isRegistered( KISTATUSBAR* aStatusBar ) const
{
    std::shared_lock<std::shared_mutex> lock( m_mutex );

    return std::find( m_statusBars.begin(), m_statusBars.end(), aStatusBar ) != m_statusBars.end();
}


bool BACKGROUND_JOBS_MONITOR::isShown( BACKGROUND_JOB_LIST* aList ) const
{
    std::shared_lock<std::shared_mutex> lock( m_mutex );

    return std::find( m_shownDialogs.begin(), m_shownDialogs.end(), aList ) != m_shownDialogs.end();
}


void BACKGROUND_JOBS_MONITOR::jobUpdated( std::shared_ptr<BACKGROUND_JOB> aJob )
{
    // Reporters call this from other threads so all UI work runs from the event loop
    std::shared_lock<std::shared_mutex> lock( m_mutex );

    //for now, we go and update the status bar if its the first job in the vector
    if( !m_jobs.empty() && m_jobs.front() == aJob )
    {
        // update all status bar entries
        for( KISTATUSBAR* statusBar : m_statusBars )
        {
            TOOLKIT::RunOnMainThread(
                    [=, this]()
                    {
                        if( !isRegistered( statusBar ) )
                            return;

                        statusBar->ShowBackgroundProgressBar();
                        statusBar->SetBackgroundProgressMax( aJob->m_maxProgress );
                        statusBar->SetBackgroundProgress( aJob->m_currentProgress );
                        statusBar->SetBackgroundStatusText( aJob->GetStatus() );
                    } );
        }
    }

    for( BACKGROUND_JOB_LIST* list : m_shownDialogs )
    {
        TOOLKIT::RunOnMainThread(
                [=, this]()
                {
                    if( isShown( list ) )
                        list->UpdateJob( aJob );
                } );
    }
}


void BACKGROUND_JOBS_MONITOR::RegisterStatusBar( KISTATUSBAR* aStatusBar )
{
    std::shared_ptr<BACKGROUND_JOB> frontJob;

    {
        std::lock_guard<std::shared_mutex> lock( m_mutex );
        m_statusBars.push_back( aStatusBar );

        // Capture front job while holding lock
        if( !m_jobs.empty() )
            frontJob = m_jobs.front();
    }

    // Make sure the newly-registered bar gets the active job, if any
    if( frontJob )
        jobUpdated( frontJob );
}


void BACKGROUND_JOBS_MONITOR::UnregisterStatusBar( KISTATUSBAR* aStatusBar )
{
    std::lock_guard<std::shared_mutex> lock( m_mutex );
    m_statusBars.erase( std::remove_if( m_statusBars.begin(), m_statusBars.end(),
                                        [&]( KISTATUSBAR* statusBar )
                                        {
                                            return statusBar == aStatusBar;
                                        } ),
                        m_statusBars.end() );
}
