/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file dialog_shim.h
 * DIALOG_SHIM of the Qt build: the base of the dialogs of KiCad, on a QDialog.
 */

#ifndef DIALOG_SHIM_
#define DIALOG_SHIM_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <kiqt.h>
#include <QDialog>

#include <kicommon.h>
#include <eda_units.h>
#include <kiway_holder.h>
#include <wx/defs.h>        // wxID_ codes (kiwx)

class QEventLoop;
class EDA_BASE_FRAME;
class UNIT_BINDER;


/**
 * Dialog helper object to sit in the inheritance tree between the dialogs of KiCad and
 * QDialog.
 *
 * It returns the codes of the buttons of KiCad (wxID_OK, wxID_CANCEL...) from ShowModal(), has
 * the quasi-modal mode of KiCad, and remembers the geometry of the dialog (with the same
 * settings as the wxWidgets build).
 */
class KICOMMON_API DIALOG_SHIM : public QDialog, public KIWAY_HOLDER
{
public:
    DIALOG_SHIM( QWidget* aParent, const wxString& aTitle );

    ~DIALOG_SHIM();

    /**
     * Set the window (usually a QLineEdit) that should be focused when the dialog is
     * shown.
     */
    void SetInitialFocus( QWidget* aWindow )
    {
        m_initialFocusTarget = aWindow;
    }

    /// The title of the dialog.
    void SetTitle( const wxString& aTitle );
    wxString GetTitle() const;

    /**
     * Show the dialog modally.
     *
     * @return the code given to EndModal() (wxID_OK, wxID_CANCEL...).
     */
    int ShowModal();

    /// End the modal dialog with a return code.
    void EndModal( int aRetCode );

    int ShowQuasiModal();      // disable only the parent window, otherwise modal.

    void EndQuasiModal( int retCode );  // End quasi-modal mode

    bool IsQuasiModal() const { return m_qmodal_loop != nullptr; }

    /// The code the dialog was ended with.
    int GetReturnCode() const { return m_returnCode; }

    EDA_UNITS GetUserUnits() const
    {
        return m_units;
    }

    /**
     * Set the labels of the standard buttons (OK, Cancel, Apply...) of the dialog, and make
     * them end it with their code.
     *
     * @param aLabels gives the labels of some buttons, by their code (wxID_OK...); the other
     *                ones get the standard labels of KiCad.
     */
    void SetupStandardButtons( std::map<int, wxString> aLabels = {} );

    /**
     * Opt out of control state saving.
     *
     * @param aWindow is the control to not save the state of.
     */
    void OptOut( QWidget* aWindow );

    /**
     * Register a UNIT_BINDER so that it can handle units in control state save/restore.
     */
    void RegisterUnitBinder( UNIT_BINDER* aUnitBinder, QWidget* aWindow );

    void UnregisterUnitBinder( UNIT_BINDER* aUnitBinder );

protected:
    /**
     * In all dialogs, we must call the same functions to fix minimal dlg size, the default
     * position and perhaps some other to do in all dialogs.
     *
     * It must be called from the constructor of derived classes, when all widgets are
     * initialized.
     */
    void finishDialogSettings();

    void showEvent( QShowEvent* aEvent ) override;
    void hideEvent( QHideEvent* aEvent ) override;
    void done( int aResult ) override;

    /// The key of the dialog in the settings (the title, or m_hash_key).
    std::string dialogKey() const;

    /// Remember the geometry of the dialog.
    void saveGeometry();

    /// Show the dialog where it was (and with the size it had).
    void restoreGeometry();

    EDA_UNITS              m_units;    // userUnits for display and parsing
    std::string            m_hash_key; // optional custom key for persistence

    QWidget*               m_initialFocusTarget;
    QEventLoop*            m_qmodal_loop;  // points to nested event_loop, NULL means not qmodal
    QWidget*               m_qmodal_parent; // the window disabled by the quasi-modal mode

    EDA_BASE_FRAME*        m_parentFrame;

    int                    m_returnCode;

    std::set<QWidget*>                m_optOut;
    std::map<QWidget*, UNIT_BINDER*>  m_unitBinders;
};

#endif  // DIALOG_SHIM_
