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
 * @file kiway.h
 * KIWAY of the Qt build: the interface of include/kiway.h for the frames of Qt.
 *
 * The KIWAY loads the KIFACEs (KIWAY_BASE), makes their frames (the KIWAY_PLAYERs) and carries
 * the mail between them.
 */

#ifndef KIWAY_H_
#define KIWAY_H_

#include <array>
#include <string>
#include <vector>

#include <kiqt.h>
#include <QMainWindow>
#include <QPointer>

#include <import_export.h>
#include <search_stack.h>
#include <project.h>
#include <frame_type.h>
#include <mail_type.h>
#include <ki_exception.h>
#include <kiface.h>
#include <kiway_base.h>

class QMainWindow;
class QWidget;
class KIWAY_PLAYER;
class TOOL_ACTION;
class LOCAL_HISTORY;


/**
 * A minimalistic software bus for communications between various DLLs/DSOs (DSOs) within
 * the same KiCad process: the KIFACEs, and the frames they make (the players).
 */
class KICOMMON_API KIWAY : public KIWAY_BASE
{
public:
    KIWAY( int aCtlBits, QMainWindow* aTop = nullptr );

    ~KIWAY();

    /**
     * A simple mapping function which returns the FACE_T which is known to implement
     * @a aFrameType.
     *
     * @return a valid value #KIWAY::FACE_T or FACE_T(-1) if given a bad @a aFrameType.
     */
    static FACE_T KifaceType( FRAME_T aFrameType );

    /**
     * Return the #KIWAY_PLAYER* given a FRAME_T.
     *
     * If it is not already created, the required KIFACE is found and loaded and initialized
     * if needed, then the KIWAY_PLAYER window is created but not shown.
     *
     * @param aFrameType is from #FRAME_T.
     * @param doCreate when true asks that the player be created if it is not already created,
     *                 false means do not create and maybe return NULL.
     * @param aParent is a parent for modal #KIWAY_PLAYER frames, otherwise NULL.
     * @return a valid opened #KIWAY_PLAYER or NULL if there is something wrong or doCreate
     *         was false and the player has yet to be created.
     */
    virtual KIWAY_PLAYER* Player( FRAME_T aFrameType, bool doCreate = true,
                                  QWidget* aParent = nullptr );

    /**
     * Call the KIWAY_PLAYER::Close( bool force ) function on the window and if not vetoed,
     * returns true, else false.
     *
     * If window actually closes, then this KIWAY marks it as not opened internally.
     *
     * @return true if the window is closed and not vetoed, else false.
     */
    virtual bool PlayerClose( FRAME_T aFrameType, bool doForce );

    /**
     * Call the KIWAY_PLAYER::Close( bool force ) function on all the windows and if none
     * are vetoed, returns true, else false.
     *
     * If any window actually closes, then* this KIWAY marks it as not opened internally.
     *
     * @return true if no window vetoed the close
     */
    virtual bool PlayersClose( bool doForce );

    /**
     * Notifies a Kiway that a player has been closed.
     */
    void PlayerDidClose( FRAME_T aFrameType );

    /**
     * Send @a aPayload to @a aDestination from @a aSource.
     *
     * The recipient receives this in its #KIWAY_PLAYER::KiwayMailIn() function and can
     * efficiently switch() based on @a aCommand in there.
     *
     * @param aFromOtherThread the mail is delivered later, by the event loop of the GUI.
     */
    virtual void ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                              QWidget* aSource = nullptr, bool aFromOtherThread = false );

    /**
     * Append all registered actions to the given list.
     */
    virtual void GetActions( std::vector<TOOL_ACTION*>& aActions ) const;

    LOCAL_HISTORY& LocalHistory() { return *m_local_history; }

    /**
     * Change the language and then calls ShowChangedLanguage() on all #KIWAY_PLAYERs.
     */
    virtual void SetLanguage( int aLanguage );

    void CommonSettingsChanged( int aFlags = 0 ) override;

    /**
     * Calls ProjectChanged() on all KIWAY_PLAYERs.
     *
     * Used after changing the project to ensure all players are updated correctly.
     */
    void ProjectChanged() override;

    /**
     * Tell this KIWAY about the top most frame in the program and optionally allows it to
     * play the role of one of the KIWAY_PLAYERs if launched from single_top.cpp.
     *
     * @param aTop is the top most frame of the program.
     */
    void SetTop( QMainWindow* aTop );
    QMainWindow* GetTop() { return m_top; }

    void OnKiCadExit();

    QWidget* GetBlockingDialog();
    void SetBlockingDialog( QWidget* aWin );

private:
    /// Get the frame of a player, or nullptr if it doesn't exist (anymore).
    KIWAY_PLAYER* GetPlayerFrame( FRAME_T aFrameType );

    QPointer<QMainWindow> m_top;            // Usually m_top is the Project manager
    QPointer<QWidget>     m_blockingDialog;

    std::array<QPointer<QWidget>, KIWAY_PLAYER_COUNT> m_players;

    LOCAL_HISTORY*  m_local_history;
};


#ifndef SWIG
// provided by single_top.cpp and kicad.cpp;
extern KIWAY Kiway;
#endif

#endif  // KIWAY_H_
