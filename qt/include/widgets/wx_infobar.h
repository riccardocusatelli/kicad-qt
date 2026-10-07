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


/**
 * @file wx_infobar.h
 * WX_INFOBAR of the Qt build: the bar of messages above the canvas of the frames (the name of
 * the class of KiCad is kept, so that the tools find it).
 */

#ifndef INFOBAR_H_
#define INFOBAR_H_

#include <functional>
#include <optional>

#include <kiqt.h>
#include <QFrame>

#include <wx/string.h>
#include <wx/intl.h>
#include <wx/defs.h>                // wxICON_ flags (kiwx)
#include <reporter.h>
#include <toolkit/gui_timer.h>
#include <widgets/wx_infobar_message_type.h>

class QAbstractButton;
class QHBoxLayout;
class QLabel;
class QToolButton;


/**
 * A modified version of the wxInfoBar class that allows us to:
 *     * Show the close button along with the other buttons
 *     * Remove all user-provided buttons at once
 *     * Allow automatically hiding the infobar after a time period
 *
 * This inherits from the generic infobar because the native infobar on GTK doesn't
 * work with the AUI manager.  In Qt it is a frame above the canvas, hidden until a message is
 * shown.
 */
class KIQT_API WX_INFOBAR : public QFrame
{
public:
    WX_INFOBAR( QWidget* aParent );

    ~WX_INFOBAR();

    using MESSAGE_TYPE = INFOBAR_MESSAGE_TYPE;

    MESSAGE_TYPE GetMessageType() const { return m_type; }

    /**
     * Set the time period to show the infobar.
     *
     * This only applies for the next showing of the infobar, so it must be reset every time.
     * A value of 0 disables the automatic hiding (this is the default).
     *
     * @param aTime is the number of milliseconds to show the infobar
     */
    void SetShowTime( int aTime );

    /**
     * Add the default close button to the infobar on the right side.
     *
     * @param aTooltip is the tooltip to give the close button
     */
    void AddCloseButton( const wxString& aTooltip = _( "Hide this message." ) );

    /**
     * Add an already created button to the infobar.
     * New buttons are added in the right-most position.
     *
     * @param aButton is the button to add
     */
    void AddButton( QAbstractButton* aButton );

    /**
     * Add a button with a label (shown as a link) that runs a function when it is clicked.
     */
    void AddButton( const wxString& aLabel, std::function<void()> aCallback );

    /**
     * Remove all the buttons that have been added by the user.
     */
    void RemoveAllButtons();

    /**
     * @return true if the infobar has a close button.
     */
    bool HasCloseButton() const;

    /**
     * Provide a callback to be called when the infobar is dismissed (either by user action
     * or timer).
     *
     * @param aCallback
     */
    void SetCallback( std::function<void(void)> aCallback )
    {
        m_callback = aCallback;
    }

    /**
     * Show the infobar with the provided message and icon for a specific period
     * of time.
     *
     * @param aMessage is the message to display
     * @param aTime is the amount of time in milliseconds to show the infobar
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     * @param aType is the type of message being displayed
     */
    void ShowMessageFor( const wxString& aMessage, int aTime, int aFlags = wxICON_INFORMATION,
                         MESSAGE_TYPE aType = WX_INFOBAR::MESSAGE_TYPE::GENERIC );

    /**
     * Show the info bar with the provided message and icon.
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void ShowMessage( const wxString& aMessage, int aFlags = wxICON_INFORMATION );

    /**
     * Show the info bar with the provided message and icon, setting the type
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     * @param aType is the type of message being displayed
     */
    void ShowMessage( const wxString& aMessage, int aFlags, MESSAGE_TYPE aType );

    /**
     * Dismisses the infobar and updates the containing layout (AUI manager if one is provided).
     */
    void Dismiss();

    /**
     * Send the infobar an event telling it to show a message.
     *
     * This is useful when the infobar must be shown from another thread: the message is shown
     * by the event loop.
     *
     * @param aMessage is the message to display
     * @param aFlags is the flag containing the icon to display on the left side of the infobar
     */
    void QueueShowMessage( const wxString& aMessage, int aFlags = wxICON_INFORMATION );

    /**
     * Send the infobar an event telling it to hide itself.
     */
    void QueueDismiss();

    /**
     * Returns true if the infobar is being updated.
     */
    bool IsLocked()
    {
        return m_updateLock;
    }

    /// The message shown.
    const wxString& GetMessage() const { return m_message; }

protected:
    void onCloseButton();

    void onTimer();

protected:
    int           m_showTime;       ///< The time to show the infobar. 0 = don't auto hide
    bool          m_updateLock;     ///< True if this infobar requested the UI update
    GUI_TIMER     m_showTimer;      ///< The timer counting the autoclose period
    MESSAGE_TYPE  m_type;           ///< The type of message being displayed
    wxString      m_message;        ///< The original message without wrapping

    QLabel*       m_icon;
    QLabel*       m_text;
    QHBoxLayout*  m_buttons;        ///< The buttons added by the user
    QToolButton*  m_closeButton;

    std::optional<std::function<void(void)>> m_callback;   ///< Optional callback made when closing infobar
};


/**
 * A wrapper for reporting to a specific text location in a statusbar.
 */
class KIQT_API INFOBAR_REPORTER : public REPORTER
{
public:
    INFOBAR_REPORTER( WX_INFOBAR* aInfoBar ) :
            REPORTER(),
            m_messageSet( false ),
            m_infoBar( aInfoBar ),
            m_severity( RPT_SEVERITY_UNDEFINED )
    {
    }

    virtual ~INFOBAR_REPORTER() {};

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override;

    /**
     * Update the infobar with the reported text.
     */
    void Finalize();

private:
    bool                      m_messageSet;
    WX_INFOBAR*               m_infoBar;
    std::unique_ptr<wxString> m_message;
    SEVERITY                  m_severity;
};

#endif // INFOBAR_H_
