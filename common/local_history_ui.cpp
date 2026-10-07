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

#include <wx/msgdlg.h>

#include <confirm.h>
#include <kicad_message_dialog.h>
#include <dialogs/dialog_restore_local_history.h>
#include <local_history.h>
#include <local_history_ui.h>


bool RestoreLocalHistoryCommit( LOCAL_HISTORY& aHistory, const wxString& aProjectPath,
                                const wxString& aHash, wxWindow* aParent, bool aConfirm )
{
    LOCAL_HISTORY::RESTORE_HOOKS hooks;

    hooks.reportLockedFiles =
            [&]( const wxString& aMessage )
            {
                wxMessageBox( aMessage, _( "Restore Failed" ), wxOK | wxICON_WARNING, aParent );
            };

    if( aConfirm )
    {
        hooks.confirmRestore =
                [&]( const wxDateTime& aWhen ) -> bool
                {
                    KICAD_MESSAGE_DIALOG dlg( aParent,
                                              wxString::Format( _( "Restore the project to the version from %s?" ),
                                                                aWhen.Format( wxS( "%Y-%m-%d %H:%M:%S" ) ) ),
                                              _( "Restore Version" ), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );

                    dlg.SetYesNoLabels( _( "Restore" ), _( "Cancel" ) );
                    dlg.SetExtendedMessage( _( "Your current files are backed up first so you can undo the "
                                               "restore. Files that are not part of this version are left "
                                               "untouched." ) );

                    return dlg.ShowModal() == wxID_YES;
                };
    }

    return aHistory.RestoreCommit( aProjectPath, aHash, &hooks );
}


void ShowLocalHistoryRestoreDialog( LOCAL_HISTORY& aHistory, const wxString& aProjectPath,
                                    wxWindow* aParent )
{
    if( !aHistory.HistoryExists( aProjectPath ) )
        return;

    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> snapshots = aHistory.LoadSnapshots( aProjectPath );

    if( snapshots.empty() )
        return;

    DIALOG_RESTORE_LOCAL_HISTORY dlg( aParent, snapshots );

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString selectedHash = dlg.GetSelectedHash();

        if( !selectedHash.IsEmpty() )
            RestoreLocalHistoryCommit( aHistory, aProjectPath, selectedHash, aParent );
    }
}
