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
 * @file wx_progress_reporters.cpp
 * The progress reporters of the GUI with Qt (common/widgets/wx_progress_reporters.cpp with
 * wxWidgets).
 */

#include <widgets/wx_progress_reporters.h>

#include <QCoreApplication>
#include <QPushButton>

#include <qt_strings.h>


WX_PROGRESS_REPORTER::WX_PROGRESS_REPORTER( QWidget* aParent, const wxString& aTitle,
                                            int aNumPhases, int aCanAbort,
                                            bool aReserveSpaceForMessage ) :
        PROGRESS_REPORTER_BASE( aNumPhases ),
        QProgressDialog( aParent ),
        m_updateThrottle( std::chrono::milliseconds( 100 ) ),
        m_lastUpdateResult( true )
{
    setWindowTitle( ToQString( aTitle ) );
    setRange( 0, 1000 );
    setValue( 0 );

    // Shown at once (the work is reported by its caller, the dialog doesn't wait for it), hidden
    // at the end of the work (wxPD_AUTO_HIDE)
    setMinimumDuration( 0 );
    setAutoClose( true );
    setAutoReset( false );

    // Allowing interaction with other windows has unintended consequences
    setWindowModality( Qt::ApplicationModal );

    if( aCanAbort != PR_CAN_ABORT )
        setCancelButton( nullptr );

    // Laid out for the status messages from the start, so that it doesn't grow with them
    if( aReserveSpaceForMessage )
        setMinimumWidth( fontMetrics().averageCharWidth() * 80 );
}


WX_PROGRESS_REPORTER::~WX_PROGRESS_REPORTER()
{
}


void WX_PROGRESS_REPORTER::SetTitle( const wxString& aTitle )
{
    setWindowTitle( ToQString( aTitle ) );
}


void WX_PROGRESS_REPORTER::ReportIndeterminate( const wxString& aMessage )
{
    // A busy indicator: no range
    setRange( 0, 0 );
    setLabelText( ToQString( aMessage ) );

    QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
}


bool WX_PROGRESS_REPORTER::updateUI()
{
    if( !shouldRefresh( m_progress.load(), m_maxProgress.load(), m_phase.load(), m_numPhases.load(),
                        m_updateThrottle ) )
    {
        return !m_cancelled.load() && m_lastUpdateResult.load();
    }

    int cur = CurrentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    wxString message;
    bool     messageChanged;

    {
        std::lock_guard<std::mutex> guard( m_mutex );
        message = m_rptMessage;
        messageChanged = m_messageChanged.exchange( false );
    }

    if( maximum() != 1000 )
        setRange( 0, 1000 );

    if( messageChanged )
        setLabelText( ToQString( message ) );

    setValue( cur );

    // The input of the user goes to the dialog only (it is modal): the cancel button
    QCoreApplication::processEvents();

    // Returns false when cancelled (if it's a cancellable dialog)
    bool result = !wasCanceled();

    m_lastUpdateResult.store( result );

    return result;
}


GAUGE_PROGRESS_REPORTER::GAUGE_PROGRESS_REPORTER( QWidget* aParent, int aNumPhases ) :
        PROGRESS_REPORTER_BASE( aNumPhases ),
        QProgressBar( aParent )
{
    setRange( 0, 1000 );
    setTextVisible( false );
}


bool GAUGE_PROGRESS_REPORTER::updateUI()
{
    int cur = CurrentProgress();

    if( cur < 0 || cur > 1000 )
        cur = 0;

    setValue( cur );

    QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents );

    return true;  // No cancel button on a progress bar
}
