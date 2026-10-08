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

#include <wx/filename.h>
#include <wx/string.h>
#include <wx/utils.h>

#include <paths.h>

#include <notifications_manager.h>
#include <widgets/kistatusbar.h>

#include "notifications_list.h"

#include <algorithm>
#include <json_common.h>

#include <core/wx_stl_compat.h>
#include <core/json_serializers.h>
#include <core/kicad_algo.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <tuple>
#include <vector>
#include <core/ki_assert.h>


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( NOTIFICATION, title, description, href, key, date )

NOTIFICATIONS_MANAGER::NOTIFICATIONS_MANAGER()
{
    m_destFileName = wxFileName( PATHS::GetUserCachePath(), wxT( "notifications.json" ) );
}


void NOTIFICATIONS_MANAGER::Load()
{
    nlohmann::json saved_json;

    std::ifstream saved_json_stream( m_destFileName.GetFullPath().fn_str() );

    try
    {
        saved_json_stream >> saved_json;

        m_notifications = saved_json.get<std::vector<NOTIFICATION>>();
    }
    catch( std::exception& )
    {
        // failed to load the json, which is fine, default to no notifications
    }

    if( wxGetEnv( wxT( "KICAD_TEST_NOTI" ), nullptr ) )
    {
        CreateOrUpdate( wxS( "test" ), wxS( "Test Notification" ), wxS( "Test please ignore" ),
                        wxS( "https://kicad.org" ) );
    }
}


void NOTIFICATIONS_MANAGER::Save()
{
    std::ofstream jsonFileStream( m_destFileName.GetFullPath().fn_str() );

    nlohmann::json saveJson = nlohmann::json( m_notifications );
    jsonFileStream << std::setw( 4 ) << saveJson << std::endl;
    jsonFileStream.flush();
    jsonFileStream.close();
}


void NOTIFICATIONS_MANAGER::CreateOrUpdate( const wxString& aKey,
                                            const wxString& aTitle,
                                            const wxString& aDescription,
                                            const wxString& aHref )
{
    KI_CHECK_RET( !aKey.IsEmpty(), wxS( "Notification key must not be empty" ) );

    auto it = std::find_if( m_notifications.begin(), m_notifications.end(),
                            [&]( const NOTIFICATION& noti )
                            {
                                return noti.key == aKey;
                            } );

    if( it != m_notifications.end() )
    {
        NOTIFICATION& noti = *it;

        noti.title = aTitle;
        noti.description = aDescription;
        noti.href = aHref;
    }
    else
    {
        m_notifications.emplace_back( NOTIFICATION{ aTitle, aDescription, aHref,
                                                    aKey, wxEmptyString } );
    }

    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs
        for( NOTIFICATIONS_LIST* list : m_shownDialogs )
            list->Add( &m_notifications.back() );
    }

    for( KISTATUSBAR* statusBar : m_statusBars )
        statusBar->SetNotificationCount( m_notifications.size() );

    Save();
}


void NOTIFICATIONS_MANAGER::Remove( const wxString& aKey )
{
    auto it = std::find_if( m_notifications.begin(), m_notifications.end(),
                            [&]( const NOTIFICATION& noti )
                            {
                                return noti.key == aKey;
                            } );

    if( it == m_notifications.end() )
        return;

    if( m_shownDialogs.size() > 0 )
    {
        // update dialogs

        for( NOTIFICATIONS_LIST* list : m_shownDialogs )
            list->Remove( &(*it) );
    }

    m_notifications.erase( it );

    Save();

    for( KISTATUSBAR* statusBar : m_statusBars )
        statusBar->SetNotificationCount( m_notifications.size() );
}


void NOTIFICATIONS_MANAGER::listClosed( NOTIFICATIONS_LIST* aList )
{
    std::erase_if( m_shownDialogs, [&]( NOTIFICATIONS_LIST* dialog )
                                    {
                                        return dialog == aList;
                                    } );
}


void NOTIFICATIONS_MANAGER::RegisterStatusBar( KISTATUSBAR* aStatusBar )
{
    m_statusBars.push_back( aStatusBar );

    // notifications should already be loaded so set the initial notification count
    aStatusBar->SetNotificationCount( m_notifications.size() );
}


void NOTIFICATIONS_MANAGER::UnregisterStatusBar( KISTATUSBAR* aStatusBar )
{
    std::erase_if( m_statusBars, [&]( KISTATUSBAR* statusBar )
                                  {
                                      return statusBar == aStatusBar;
                                  } );
}
