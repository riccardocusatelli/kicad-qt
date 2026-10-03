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

#ifndef LOCAL_HISTORY_UI_H
#define LOCAL_HISTORY_UI_H

#include <kicommon.h>

class LOCAL_HISTORY;
class wxString;
class wxWindow;


/**
 * Restore a local history snapshot, with message boxes for the errors and (when @a aConfirm
 * is true) a confirmation before the files are overwritten.  Callers that have already asked
 * the user (the startup recovery prompt) pass false to avoid a double prompt.
 */
KICOMMON_API bool RestoreLocalHistoryCommit( LOCAL_HISTORY& aHistory, const wxString& aProjectPath,
                                             const wxString& aHash, wxWindow* aParent,
                                             bool aConfirm = true );

/// Show a dialog allowing the user to choose a snapshot to restore.
KICOMMON_API void ShowLocalHistoryRestoreDialog( LOCAL_HISTORY& aHistory,
                                                 const wxString& aProjectPath, wxWindow* aParent );

#endif // LOCAL_HISTORY_UI_H
