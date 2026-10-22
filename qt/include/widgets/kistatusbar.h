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
 * @file kistatusbar.h
 * KISTATUSBAR of the Qt build: the status bar of the frames, with the fields of the frame and
 * the controls of the background jobs, of the load messages and of the notifications (see
 * include/widgets/kistatusbar.h).
 */

#ifndef KISTATUSBAR_H
#define KISTATUSBAR_H

#include <kicommon.h>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QStatusBar>

#include <string_utils.h>       // LOAD_MESSAGE
#include <wx/defs.h>
#include <wx/string.h>

class QLabel;
class QProgressBar;
class QToolButton;
class BITMAP_BUTTON;


class KIQT_COMMON_API KISTATUSBAR : public QStatusBar
{
public:
    enum STYLE_FLAGS : int
    {
        NONE_STYLE        = 0x00,
        NOTIFICATION_ICON = 0x01,
        CANCEL_BUTTON     = 0x02,
        WARNING_ICON      = 0x04,
    };

    static constexpr auto DEFAULT_STYLE =
            static_cast<STYLE_FLAGS>( NOTIFICATION_ICON | CANCEL_BUTTON );

    KISTATUSBAR( int aNumberFields, QWidget* parent, wxWindowID id = wxID_ANY,
                 STYLE_FLAGS aFlags = DEFAULT_STYLE );

    ~KISTATUSBAR() override;

    /// A destroyed status bar is not referenced any more (the wxWeakRef of the layer are
    /// QPointer): it is never seen while being deleted.
    bool IsBeingDeleted() const { return false; }

    /// Show a text in a field of the frame (as wxStatusBar::SetStatusText()).
    void SetStatusText( const wxString& aText, int aField = 0 );
    wxString GetStatusText( int aField = 0 ) const;

    /// The number of the fields of the frame.
    int GetFieldsCount() const { return m_normalFieldsCount; }

    /**
     * Set the widths of the fields of the frame (as wxStatusBar::SetStatusWidths()): a positive
     * width is fixed, a negative one is a stretch factor.
     */
    virtual void SetStatusWidths( int aSize, const int* aWidths );

    /**
     * Show the background progress bar.
     */
    void ShowBackgroundProgressBar( bool aCancellable = false );

    /**
     * Hide the background progress bar.
     */
    void HideBackgroundProgressBar();

    /**
     * Set the current progress of the progress bar.
     */
    void SetBackgroundProgress( int aAmount );

    /**
     * Set the max progress of the progress bar.
     */
    void SetBackgroundProgressMax( int aAmount );

    /**
     * Set the status text that displays next to the progress bar.
     */
    void SetBackgroundStatusText( const wxString& aTxt );

    /**
     * Set the notification count on the notifications button.
     *
     * A value of 0 will hide the count.
     */
    void SetNotificationCount( int aCount );

    /**
     * Clears all warning messages from the given source (or all sources if aSource is empty)
     */
    void ClearWarningMessages( const wxString& aSource = wxEmptyString );

    /**
     * Add warning/error messages (not thread-safe, use the std::vector<LOAD_MESSAGE> variant
     * from other threads)
     */
    void AddWarningMessages( const wxString& aSource, const wxString& aMessages );

    /**
     * Add warning/error messages thread-safely.
     * Can be called from any thread. UI update is deferred to main thread.
     */
    void AddWarningMessages( const wxString& aSource, const std::vector<LOAD_MESSAGE>& aMessages );

    /**
     * Get current message count (thread-safe).
     */
    size_t GetLoadWarningCount() const;

protected:
    void resizeEvent( QResizeEvent* aEvent ) override;

private:
    void onBackgroundProgressClick();
    void onNotificationsIconClick();
    void onLoadWarningsIconClick();
    void updateWarningUI();  ///< Update warning button visibility and badge (main thread only)
    void updateBackgroundText();

private:
    std::vector<QLabel*> m_fields;          ///< The fields of the frame
    QLabel*        m_backgroundTxt;
    QProgressBar*  m_backgroundProgressBar;
    QToolButton*   m_backgroundStopButton;
    BITMAP_BUTTON* m_notificationsButton;
    BITMAP_BUTTON* m_warningButton;
    mutable std::mutex m_warningMutex;  ///< Protects m_warningMessages
    std::unordered_map<wxString, std::vector<LOAD_MESSAGE>> m_warningMessages;
    int            m_normalFieldsCount;
    STYLE_FLAGS    m_styleFlags;
    wxString       m_savedStatusText;       ///< Saved text from adjacent field during background jobs
    wxString       m_backgroundRawText;     ///< Unellipsized background status text
};

#endif
