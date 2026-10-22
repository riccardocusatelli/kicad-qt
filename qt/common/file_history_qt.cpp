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
 * @file file_history_qt.cpp
 * FILE_HISTORY of the Qt build (see common/file_history.cpp).
 */

#include <file_history.h>

#include <algorithm>

#include <bitmaps.h>
#include <id.h>
#include <settings/app_settings.h>
#include <tool/action_menu.h>
#include <tool/selection.h>


FILE_HISTORY::FILE_HISTORY( size_t aMaxFiles, int aBaseFileId, int aClearId, wxString aClearText ) :
        m_fileMaxFiles( std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE ) ),
        m_baseId( aBaseFileId ),
        m_clearId( aClearId ),
        m_clearText( aClearText )
{
}


void FILE_HISTORY::Load( const APP_SETTINGS_BASE& aSettings )
{
    Load( aSettings.m_System.file_history );
}


void FILE_HISTORY::Load( const std::vector<wxString>& aList )
{
    // The list has the most recent file first, as the history
    m_fileHistory.clear();

    for( const wxString& file : aList )
    {
        if( m_fileHistory.size() < m_fileMaxFiles
            && std::find( m_fileHistory.begin(), m_fileHistory.end(), file ) == m_fileHistory.end() )
        {
            m_fileHistory.push_back( file );
        }
    }

    updateMenus();
}


void FILE_HISTORY::Save( APP_SETTINGS_BASE& aSettings )
{
    Save( &aSettings.m_System.file_history );
}


void FILE_HISTORY::Save( std::vector<wxString>* aList )
{
    *aList = m_fileHistory;
}


void FILE_HISTORY::AddFileToHistory( const wxString& aFile )
{
    std::erase( m_fileHistory, aFile );
    m_fileHistory.insert( m_fileHistory.begin(), aFile );

    if( m_fileHistory.size() > m_fileMaxFiles )
        m_fileHistory.resize( m_fileMaxFiles );

    updateMenus();
}


void FILE_HISTORY::RemoveFileFromHistory( size_t aIndex )
{
    if( aIndex < m_fileHistory.size() )
        m_fileHistory.erase( m_fileHistory.begin() + aIndex );

    updateMenus();
}


void FILE_HISTORY::UseMenu( ACTION_MENU* aMenu )
{
    if( std::find( m_menus.begin(), m_menus.end(), aMenu ) == m_menus.end() )
        m_menus.emplace_back( aMenu );
}


void FILE_HISTORY::RemoveMenu( ACTION_MENU* aMenu )
{
    std::erase( m_menus, aMenu );
}


void FILE_HISTORY::AddFilesToMenu()
{
    updateMenus();
}


void FILE_HISTORY::AddFilesToMenu( ACTION_MENU* aMenu )
{
    aMenu->Clear();

    for( size_t i = 0; i < m_fileHistory.size(); ++i )
    {
        // The number of the file is its mnemonic, as wxWidgets
        wxString label = wxString::Format( wxS( "&%d %s" ), int( i + 1 ), m_fileHistory[i] );
        aMenu->Add( label, m_baseId + int( i ), BITMAPS::INVALID_BITMAP );
    }

    if( m_fileHistory.empty() )
    {
        // If the history is empty, an item says there are no files
        aMenu->Add( _( "No Files" ), ID_FILE_LIST_EMPTY, BITMAPS::INVALID_BITMAP );
        aMenu->Enable( ID_FILE_LIST_EMPTY, false );
    }

    aMenu->AppendSeparator();
    aMenu->Add( m_clearText, m_clearId, BITMAPS::INVALID_BITMAP );
}


void FILE_HISTORY::updateMenus()
{
    std::erase_if( m_menus, []( const QPointer<ACTION_MENU>& aMenu ) { return aMenu.isNull(); } );

    for( const QPointer<ACTION_MENU>& menu : m_menus )
        AddFilesToMenu( menu.data() );
}


void FILE_HISTORY::SetMaxFiles( size_t aMaxFiles )
{
    m_fileMaxFiles = std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE );

    if( m_fileHistory.size() > m_fileMaxFiles )
        m_fileHistory.resize( m_fileMaxFiles );

    updateMenus();
}


void FILE_HISTORY::UpdateClearText( ACTION_MENU* aMenu, wxString aClearText )
{
    m_clearText = aClearText;
    AddFilesToMenu( aMenu );
}


void FILE_HISTORY::ClearFileHistory()
{
    m_fileHistory.clear();
    updateMenus();
}


SELECTION_CONDITION FILE_HISTORY::FileHistoryNotEmpty( const FILE_HISTORY& aHistory )
{
    return [&aHistory]( const SELECTION& )
           {
               return aHistory.GetCount() != 0;
           };
}
