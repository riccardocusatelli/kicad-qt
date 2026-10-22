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
 * @file wx_progress_reporters.h
 * The progress reporters of the GUI with Qt (include/widgets/wx_progress_reporters.h with
 * wxWidgets): a progress dialog and a progress bar.  The names of the classes of KiCad are kept,
 * so that the tools find them.
 */

#ifndef WX_PROGRESS_REPORTERS_H
#define WX_PROGRESS_REPORTERS_H

#include <atomic>

#include <kiqt.h>
#include <QProgressBar>
#include <QProgressDialog>

#include <core/throttle.h>
#include <wx/kiwx_window.h>      // wxWidgetArgCompat
#include <widgets/progress_reporter_base.h>

#define PR_NO_ABORT 0
#define PR_CAN_ABORT 1


/**
 * Multi-thread safe progress reporter dialog, intended for use of tasks that parallel reporting
 * back of work status.
 *
 * @see PROGRESS_REPORTER.
 */
class KIQT_API WX_PROGRESS_REPORTER : public PROGRESS_REPORTER_BASE, public QProgressDialog
{
public:
    /**
     * The #PROGRESS_REPORTER will stay on top of \a aParent.
     *
     * @param aParent is the dialog or the frame that manages this.
     * @param aTitle is the dialog progress title
     * @param aNumPhases is the number of "virtual sections" of the progress bar
     *   aNumPhases = 1 is the usual progress bar
     *   aNumPhases = n creates n virtual progress bar zones: a 0 to 100 percent width
     *   of a virtual zone fills 0 to 1/n progress bar full size of the nth virtual zone index
     * @param aCanAbort indicates if the Cancel button should be shown (PR_CAN_ABORT)
     * @param aReserveSpaceForMessage lays the dialog out for status messages from the start
     */
    WX_PROGRESS_REPORTER( wxWidgetArgCompat aParent, const wxString& aTitle, int aNumPhases,
                          int aCanAbort, bool aReserveSpaceForMessage = true );
    ~WX_PROGRESS_REPORTER();

    /// Hide the dialog (wxWindow::Hide()).
    void Hide() { hide(); }

    /**
     * Change the title displayed on the window caption.
     */
    void SetTitle( const wxString& aTitle ) override;

    void ReportIndeterminate( const wxString& aMessage ) override;

    void SetVisible( bool aVisible ) override { setVisible( aVisible ); }

    ///< Gate expensive dialog work while always allowing the final tick.
    static bool shouldRefresh( int aProgress, int aMaxProgress, int aPhase, int aNumPhases,
                               THROTTLE& aThrottle )
    {
        bool finalTick = aMaxProgress > 0 && aProgress >= aMaxProgress && aPhase + 1 >= aNumPhases;

        return finalTick || aThrottle.Ready();
    }

private:
    bool updateUI() override;

private:
    // Rate-limit expensive dialog repaint and event draining.
    THROTTLE               m_updateThrottle;
    std::atomic_bool       m_lastUpdateResult;
};


class KIQT_API GAUGE_PROGRESS_REPORTER : public PROGRESS_REPORTER_BASE, public QProgressBar
{
public:
    /**
     * @param aParent is the parent of the progress bar
     * @param aNumPhases is the number of "virtual sections" of the progress bar
     *   aNumPhases = 1 is the usual progress bar
     *   aNumPhases = n creates n virtual progress bar zones: a 0 to 100 percent width
     *   of a virtual zone fills 0 to 1/n progress bar full size of the nth virtual zone index
     */
    GAUGE_PROGRESS_REPORTER( QWidget* aParent, int aNumPhases );

private:
    bool updateUI() override;
};

#endif
