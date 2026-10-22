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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file file_history.h
 * FILE_HISTORY of the Qt build: the recently used files (wxFileHistory and the menus of
 * include/file_history.h), shown in ACTION_MENUs.  The item of a file has the id
 * GetBaseId() + its index: the menu gives it to the frame (EDA_BASE_FRAME::RunMenuCommand()).
 */

#ifndef FILE_HISTORY_H_
#define FILE_HISTORY_H_

#include <vector>

#include <QPointer>

#include <kicommon.h>
#include <tool/selection_conditions.h>
#include <wx/intl.h>
#include <wx/string.h>

class ACTION_MENU;
class APP_SETTINGS_BASE;


class KIQT_COMMON_API FILE_HISTORY
{
public:
    /**
     * Create a file history object to store a list of files and add them to a menu.
     *
     * @param aMaxFiles is the number of files to store in the history
     * @param aBaseFileId is the ID to use for the first file menu item
     * @param aClearId is the ID to use for the clear menu menu item
     * @param aClearText is the text to use for the menu item that clears the history.
     */
    FILE_HISTORY( size_t aMaxFiles, int aBaseFileId, int aClearId,
                  wxString aClearText = _( "Clear Recent Files" ) );

    /// Load the history from the settings of an application, or from a list of files.
    void Load( const APP_SETTINGS_BASE& aSettings );
    void Load( const std::vector<wxString>& aList );

    /// Save the history into the settings of an application, or into a list of files.
    void Save( APP_SETTINGS_BASE& aSettings );
    void Save( std::vector<wxString>* aList );

    /// Add a file to the history (first), and to the menus.
    void AddFileToHistory( const wxString& aFile );
    void RemoveFileFromHistory( size_t aIndex );

    wxString GetHistoryFile( size_t aIndex ) const { return m_fileHistory.at( aIndex ); }
    size_t GetCount() const { return m_fileHistory.size(); }
    size_t GetMaxFiles() const { return m_fileMaxFiles; }

    int GetBaseId() const { return m_baseId; }
    void SetBaseId( int aBaseId ) { m_baseId = aBaseId; }

    /// The menus showing the history (they are updated when the history changes).
    void UseMenu( ACTION_MENU* aMenu );
    void RemoveMenu( ACTION_MENU* aMenu );

    /// Add the files to all registered menus, or to the specified one.
    void AddFilesToMenu();
    void AddFilesToMenu( ACTION_MENU* aMenu );

    /// Update the number of files that will be contained inside the file history.
    void SetMaxFiles( size_t aMaxFiles );

    /// Set the text displayed on the menu item that clears the entire menu.
    void SetClearText( wxString aClearText ) { m_clearText = aClearText; }

    /// Update the text displayed on the menu item that clears the menu (after a language change).
    void UpdateClearText( ACTION_MENU* aMenu, wxString aClearText );

    /// Clear all entries from the file history.
    void ClearFileHistory();

    /// A #SELECTION_CONDITION enabling a menu item when the file history has items in it.
    static SELECTION_CONDITION FileHistoryNotEmpty( const FILE_HISTORY& aHistory );

private:
    /// Update the menus of the history.
    void updateMenus();

    std::vector<wxString>             m_fileHistory;   ///< The most recent file first
    size_t                            m_fileMaxFiles;
    int                               m_baseId;
    int                               m_clearId;
    wxString                          m_clearText;
    std::vector<QPointer<ACTION_MENU>> m_menus;
};

#endif
