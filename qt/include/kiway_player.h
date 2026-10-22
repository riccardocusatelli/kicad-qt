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
 * @file kiway_player.h
 * KIWAY_PLAYER of the Qt build: the interface of include/kiway_player.h, on the Qt
 * EDA_BASE_FRAME.
 */

#ifndef KIWAY_PLAYER_H_
#define KIWAY_PLAYER_H_

#include <vector>

#include <kiway_holder.h>
#include <eda_base_frame.h>
#include <kictl_flags.h>

class QEventLoop;
class QTcpServer;
class KIWAY;
class PROJECT;
struct KIFACE;
class KIFACE_BASE;
class TOOL_MANAGER;
class KIWAY_MAIL_EVENT;


/**
 * A wxFrame capable of the OpenProjectFiles function, meaning it can load a portion of a
 * KiCad project.
 *
 * Because this class provides a dummy implementation,it is not a certainty that all
 * classes which inherit from this class intend to OpenProjectFiles() or KiwayMailIn().
 * In the Qt build it is a QMainWindow, as the EDA_BASE_FRAME.
 */
class KIWAY_PLAYER : public EDA_BASE_FRAME
{
public:
    KIWAY_PLAYER( KIWAY* aKiway, QWidget* aParent, FRAME_T aFrameType, const wxString& aTitle,
                  const wxPoint& aPos, const wxSize& aSize, long aStyle,
                  const wxString& aFrameName, const EDA_IU_SCALE& aIuScale );

    ~KIWAY_PLAYER() throw();

    /**
     * Open a project or set of files given by @a aFileList.
     *
     * This is generalized in the direction of worst case.  In a typical single document
     * interface application, the aFileList will only be a single file.  However, it is
     * conceivable that a spreadsheet type application would have more than one file.
     *
     * @param aFileList includes files that this frame should open according to the knowledge
     *                  in the derived wxFrame.  In almost every case, the list will have only
     *                  a single file in it.
     * @param aCtl is a set of bit flags ORed together from the set of KICTL_* \#defined above.
     * @return true if all requested files were opened OK, else false.
     */
    virtual bool OpenProjectFiles( const std::vector<wxString>& aFileList, int aCtl = 0 )
    {
        return false;
    }

    /**
     * Show this wxFrame as if it were a modal dialog, with all other instantiated wxFrames
     * disabled until this KIWAY_PLAYER derivative calls DismissModal().
     *
     * This is behavior is similar to a modal dialog window.  Not all KIWAY_PLAYERs use this
     * interface, so don't call this unless the implementation knows how to call
     * DismissModal() on a button click or double click or some special event which ends
     * the modal behavior.
     *
     * @param aResult if not NULL, indicates a place to put a resultant string.
     * @param aResultantFocusWindow if not NULL, indicates what window to pass focus to on return.
     * @return true if frame implementation called KIWAY_PLAYER::DismissModal() with
     *         aRetVal of true.
     */
    virtual bool ShowModal( wxString* aResult = nullptr,
                            wxWindow* aResultantFocusWindow = nullptr );

    /**
     * Receive #KIWAY_MAIL_EVENT messages from other players.
     *
     * Override it in derived classes.
     */
    virtual void KiwayMailIn( KIWAY_MAIL_EVENT& aEvent );

    bool IsModal() const override       { return m_modal; }
    void SetModal( bool aIsModal )      { m_modal = aIsModal; }

    /**
     * @return false only if both the frame is acting in modal mode and it has not been
     *         dismissed yet with DismissModal().  True if the dialog is not modal or if it
     *         is modal and has been dismissed.
     */
    bool IsDismissed();

    void DismissModal( bool aRetVal, const wxString& aResult = wxEmptyString );

    /**
     * Execute a remote command sent via socket (to port KICAD_PCB_PORT_SERVICE_NUMBER,
     * currently 4242).
     *
     * Subclasses should override to implement actual command handlers.
     */
    virtual void ExecuteRemoteCommand( const char* cmdline ){}

    /**
     * Listen on the TCP port @a service for the remote commands (ExecuteRemoteCommand()).
     *
     * @param local is true to accept only the connections of the local host.
     */
    void CreateServer( int service, bool local = true );

protected:
    bool            m_modal;        // true if frame is intended to be modal, not modeless

    /// Points to nested event_loop. NULL means not modal and dismissed.
    QEventLoop*     m_modal_loop;
    QWidget*        m_modal_resultant_parent; // the window caller in modal mode
    wxString        m_modal_string;
    bool            m_modal_ret_val;    // true if a selection was made

    QTcpServer*     m_socketServer;     // the server of the remote commands, or nullptr
};

#endif // KIWAY_PLAYER_H_
