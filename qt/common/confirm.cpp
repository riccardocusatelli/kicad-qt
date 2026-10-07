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
 * @file confirm.cpp
 * The message dialogs of confirm.h on Qt (QMessageBox), with the texts of the wxWidgets build.
 */

#include <confirm.h>

#include <cstdio>

#include <QApplication>
#include <QCheckBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include <wx/defs.h>        // wxID_ codes (kiwx)
#include <wx/intl.h>
#include <core/ki_log.h>
#include <kiplatform/app.h>

#include <qt_strings.h>


/**
 * Flag to enable confirmation dialog debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceConfirm[] = wxT( "KICAD_CONFIRM" );


/// A message box with the texts of KiCad (the mnemonics of the labels are kept).
static QMessageBox* newMessageBox( TOOLKIT_WINDOW* aParent, QMessageBox::Icon aIcon,
                                   const wxString& aTitle, const wxString& aMessage,
                                   const wxString& aExtendedMessage = wxEmptyString )
{
    QMessageBox* box = new QMessageBox( aIcon, ToQString( aTitle ), ToQString( aMessage ),
                                        QMessageBox::NoButton, aParent );

    if( !aExtendedMessage.IsEmpty() )
        box->setInformativeText( ToQString( aExtendedMessage ) );

    box->setAttribute( Qt::WA_DeleteOnClose, false );
    return box;
}


/// @return true if there is no GUI to show a dialog: the message goes to the log.
static bool noGui()
{
    return !qApp || !qobject_cast<QApplication*>( qApp );
}


bool AskOverrideLock( TOOLKIT_WINDOW* aParent, const wxString& aMessage )
{
    std::unique_ptr<QMessageBox> dlg( newMessageBox( aParent, QMessageBox::Critical,
            _( "File Open Warning" ), aMessage,
            _( "Interleaved saves may produce very unexpected results." ) ) );

    QPushButton* cancel = dlg->addButton( ToQString( _( "&Cancel" ) ), QMessageBox::RejectRole );
    QPushButton* open = dlg->addButton( ToQString( _( "&Open Anyway" ) ), QMessageBox::AcceptRole );
    dlg->setDefaultButton( cancel );

    dlg->exec();

    return dlg->clickedButton() == open;
}


int UnsavedChangesDialog( TOOLKIT_WINDOW* parent, const wxString& aMessage, bool* aApplyToAll )
{
    static bool s_apply_to_all = false;

    std::unique_ptr<QMessageBox> dlg( newMessageBox( parent, QMessageBox::Warning,
            _( "Save Changes?" ), aMessage,
            _( "If you don't save, all your changes will be permanently lost." ) ) );

    QPushButton* save = dlg->addButton( ToQString( _( "&Save" ) ), QMessageBox::AcceptRole );
    QPushButton* discard = dlg->addButton( ToQString( _( "&Discard Changes" ) ),
                                           QMessageBox::DestructiveRole );
    dlg->addButton( QMessageBox::Cancel );
    dlg->setDefaultButton( save );

    QCheckBox* applyToAll = nullptr;

    if( aApplyToAll )
    {
        applyToAll = new QCheckBox( ToQString( _( "&Apply to all" ) ) );
        applyToAll->setChecked( s_apply_to_all );
        dlg->setCheckBox( applyToAll );
    }

    dlg->exec();

    if( aApplyToAll )
    {
        *aApplyToAll = applyToAll->isChecked();
        s_apply_to_all = applyToAll->isChecked();
    }

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    if( dlg->clickedButton() == save )
        return wxID_YES;
    else if( dlg->clickedButton() == discard )
        return wxID_NO;
    else
        return wxID_CANCEL;
}


int UnsavedChangesDialog( TOOLKIT_WINDOW* parent, const wxString& aMessage )
{
    return UnsavedChangesDialog( parent, aMessage, nullptr );
}


bool ConfirmRevertDialog( TOOLKIT_WINDOW* parent, const wxString& aMessage )
{
    std::unique_ptr<QMessageBox> dlg( newMessageBox( parent, QMessageBox::Warning,
            wxEmptyString, aMessage, _( "Your current changes will be permanently lost." ) ) );

    QPushButton* revert = dlg->addButton( ToQString( _( "&Revert" ) ), QMessageBox::AcceptRole );
    dlg->addButton( ToQString( _( "&Cancel" ) ), QMessageBox::RejectRole );
    dlg->setDefaultButton( revert );

    dlg->exec();

    return dlg->clickedButton() == revert;
}


static int g_lastUnsavedChangesResult = -1;

bool HandleUnsavedChanges( TOOLKIT_WINDOW* aParent, const wxString& aMessage,
                           const std::function<bool()>& aSaveFunction )
{
    g_lastUnsavedChangesResult = UnsavedChangesDialog( aParent, aMessage );

    switch( g_lastUnsavedChangesResult )
    {
    case wxID_YES:    return aSaveFunction();
    case wxID_NO:     return true; // proceed without saving
    default:
    case wxID_CANCEL: return false;
    }
}


int GetLastUnsavedChangesResponse()
{
    return g_lastUnsavedChangesResult;
}


int OKOrCancelDialog( TOOLKIT_WINDOW* aParent, const wxString& aWarning, const wxString& aMessage,
                      const wxString& aDetailedMessage, const wxString& aOKLabel,
                      const wxString& aCancelLabel, bool* aApplyToAll )
{
    std::unique_ptr<QMessageBox> dlg( newMessageBox( aParent, QMessageBox::Warning, aWarning,
                                                     aMessage, aDetailedMessage ) );

    QPushButton* ok = dlg->addButton( ToQString( aOKLabel.IsEmpty() ? _( "&OK" ) : aOKLabel ),
                                      QMessageBox::AcceptRole );
    dlg->addButton( ToQString( aCancelLabel.IsEmpty() ? _( "&Cancel" ) : aCancelLabel ),
                    QMessageBox::RejectRole );
    dlg->setDefaultButton( ok );

    QCheckBox* applyToAll = nullptr;

    if( aApplyToAll )
    {
        applyToAll = new QCheckBox( ToQString( _( "&Apply to all" ) ) );
        applyToAll->setChecked( true );
        dlg->setCheckBox( applyToAll );
    }

    dlg->exec();

    if( aApplyToAll )
        *aApplyToAll = applyToAll->isChecked();

    // Returns wxID_OK or wxID_CANCEL
    return dlg->clickedButton() == ok ? wxID_OK : wxID_CANCEL;
}


// DisplayError should be deprecated, use DisplayErrorMessage instead
void DisplayError( TOOLKIT_WINDOW* aParent, const wxString& aText )
{
    DisplayErrorMessage( aParent, aText );
}


void DisplayErrorMessage( TOOLKIT_WINDOW* aParent, const wxString& aText, const wxString& aExtraInfo )
{
    if( noGui() )
    {
        fprintf( stderr, "%s %s\n", aText.utf8_str().data(), aExtraInfo.utf8_str().data() );
        return;
    }

    std::unique_ptr<QMessageBox> dlg( newMessageBox( aParent, QMessageBox::Critical, _( "Error" ),
                                                     aText, aExtraInfo ) );
    dlg->addButton( QMessageBox::Ok );
    dlg->exec();
}


void DisplayInfoMessage( TOOLKIT_WINDOW* aParent, const wxString& aMessage, const wxString& aExtraInfo )
{
    if( noGui() )
    {
        KI_LOG_TRACE( traceConfirm, wxS( "%s %s" ), aMessage, aExtraInfo );
        return;
    }

    std::unique_ptr<QMessageBox> dlg( newMessageBox( aParent, QMessageBox::Information,
                                                     _( "Information" ), aMessage, aExtraInfo ) );
    dlg->addButton( QMessageBox::Ok );
    dlg->exec();
}


bool IsOK( TOOLKIT_WINDOW* aParent, const wxString& aMessage )
{
#ifdef __APPLE__
    // Why is the question icon a light-bulb on Mac?  That has more of a hint or info connotation.
    QMessageBox::Icon icon = QMessageBox::Warning;
#else
    QMessageBox::Icon icon = QMessageBox::Question;
#endif

    std::unique_ptr<QMessageBox> dlg( newMessageBox( aParent, icon, _( "Confirmation" ),
                                                     aMessage ) );

    QPushButton* yes = dlg->addButton( ToQString( _( "&Yes" ) ), QMessageBox::AcceptRole );
    QPushButton* no = dlg->addButton( ToQString( _( "&No" ) ), QMessageBox::RejectRole );
    dlg->setDefaultButton( yes );
    dlg->setEscapeButton( no );

    dlg->exec();

    return dlg->clickedButton() == yes;
}


int SelectSingleOption( TOOLKIT_WINDOW* aParent, const wxString& aTitle,
                        const wxString& aMessage, const wxArrayString& aOptions )
{
    QStringList options;

    for( const wxString& option : aOptions )
        options << ToQString( option );

    bool    ok = false;
    QString choice = QInputDialog::getItem( aParent, ToQString( aTitle ), ToQString( aMessage ),
                                            options, 0, false, &ok );

    if( !ok )
        return -1;

    return options.indexOf( choice );
}


bool WarnUserIfOperatingSystemUnsupported()
{
    if( !KIPLATFORM::APP::IsOperatingSystemUnsupported() )
        return false;

    std::unique_ptr<QMessageBox> dlg( newMessageBox( nullptr, QMessageBox::Warning,
            _( "Unsupported Operating System" ),
            _( "This operating system is not supported by KiCad and its dependencies." ),
            _( "Any issues with KiCad on this system cannot be reported to the official "
               "bugtracker." ) ) );

    dlg->addButton( QMessageBox::Ok );
    dlg->exec();

    return true;
}
