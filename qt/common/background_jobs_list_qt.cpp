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

/**
 * @file background_jobs_list_qt.cpp
 * The window of the list of the background jobs with Qt (see common/background_job_list.h and
 * common/background_jobs_list_wx.cpp).
 */

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QVBoxLayout>

#include <background_jobs_monitor.h>
#include <wx/intl.h>

#include "../../common/background_job_list.h"

#include <qt_strings.h>


namespace
{

/// A job of the list: its name, its status and its progress.
class BACKGROUND_JOB_PANEL : public QFrame
{
public:
    BACKGROUND_JOB_PANEL( QWidget* aParent, std::shared_ptr<BACKGROUND_JOB> aJob ) :
            QFrame( aParent ),
            m_job( aJob )
    {
        setFrameShape( QFrame::Box );
        setMinimumHeight( 75 );
        setAutoFillBackground( true );
        setBackgroundRole( QPalette::Midlight );

        QVBoxLayout* mainSizer = new QVBoxLayout( this );
        mainSizer->setContentsMargins( 1, 1, 1, 1 );
        mainSizer->setSpacing( 1 );

        m_stName = new QLabel( ToQString( aJob->m_name ), this );
        QFont bold = m_stName->font();
        bold.setBold( true );
        m_stName->setFont( bold );
        mainSizer->addWidget( m_stName );

        m_stStatus = new QLabel( ToQString( aJob->m_status ), this );
        mainSizer->addWidget( m_stStatus );

        m_progress = new QProgressBar( this );
        m_progress->setTextVisible( false );
        mainSizer->addWidget( m_progress );

        UpdateFromJob();
    }

    void UpdateFromJob()
    {
        m_stStatus->setText( ToQString( m_job->m_status ) );
        m_progress->setRange( 0, m_job->m_maxProgress );
        m_progress->setValue( m_job->m_currentProgress );
    }

private:
    QProgressBar*                   m_progress;
    QLabel*                         m_stName;
    QLabel*                         m_stStatus;
    std::shared_ptr<BACKGROUND_JOB> m_job;
};


/// The list: a popup, closed (and deleted) by a click outside of it.
class BACKGROUND_JOB_LIST_QT : public QFrame, public BACKGROUND_JOB_LIST
{
public:
    BACKGROUND_JOB_LIST_QT( QWidget* aParent ) :
            QFrame( aParent, Qt::Popup )
    {
        setAttribute( Qt::WA_DeleteOnClose );
        setFrameShape( QFrame::Box );
        setWindowTitle( ToQString( _( "Background Jobs" ) ) );
        resize( 300, 150 );

        QVBoxLayout* bSizer1 = new QVBoxLayout( this );
        bSizer1->setContentsMargins( 0, 0, 0, 0 );

        QScrollArea* scrolledWindow = new QScrollArea( this );
        scrolledWindow->setWidgetResizable( true );
        scrolledWindow->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

        m_content = new QWidget( scrolledWindow );
        m_contentSizer = new QVBoxLayout( m_content );
        m_contentSizer->setContentsMargins( 2, 2, 2, 2 );
        m_contentSizer->addStretch( 1 );
        scrolledWindow->setWidget( m_content );

        bSizer1->addWidget( scrolledWindow );
    }

    ~BACKGROUND_JOB_LIST_QT() override
    {
        if( m_onClose )
            m_onClose();
    }

    /// Called when the list closes.
    void SetOnClose( std::function<void()> aOnClose ) { m_onClose = std::move( aOnClose ); }

    void Add( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        BACKGROUND_JOB_PANEL* panel = new BACKGROUND_JOB_PANEL( m_content, aJob );

        // Before the stretch at the end
        m_contentSizer->insertWidget( m_contentSizer->count() - 1, panel );
        m_jobPanels[aJob] = panel;
    }

    void Remove( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        auto it = m_jobPanels.find( aJob );

        if( it != m_jobPanels.end() )
        {
            it->second->deleteLater();
            m_jobPanels.erase( it );
        }
    }

    void UpdateJob( std::shared_ptr<BACKGROUND_JOB> aJob ) override
    {
        auto it = m_jobPanels.find( aJob );

        if( it != m_jobPanels.end() )
            it->second->UpdateFromJob();
    }

private:
    std::function<void()> m_onClose;
    QWidget*              m_content;
    QVBoxLayout*          m_contentSizer;
    std::unordered_map<std::shared_ptr<BACKGROUND_JOB>, BACKGROUND_JOB_PANEL*> m_jobPanels;
};

}


void BACKGROUND_JOBS_MONITOR::ShowList( QWidget* aParent, wxPoint aPos )
{
    BACKGROUND_JOB_LIST_QT* list = new BACKGROUND_JOB_LIST_QT( aParent );

    std::shared_lock<std::shared_mutex> lock( m_mutex, std::try_to_lock );

    for( const std::shared_ptr<BACKGROUND_JOB>& job : m_jobs )
        list->Add( job );

    lock.unlock();

    m_shownDialogs.push_back( list );

    list->SetOnClose(
            [this, list]()
            {
                listClosed( list );
            } );

    // correct the position: the bottom right corner at aPos
    list->move( aPos.x - list->width(), aPos.y - list->height() );
    list->show();
}
