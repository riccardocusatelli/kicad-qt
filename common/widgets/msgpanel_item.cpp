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


#include <widgets/msgpanel_item.h>

#include <advanced_config.h>
#include <kiid.h>


std::optional<wxString> GetMsgPanelDisplayUuid( const KIID& aKiid )
{
    const static int        showUuids = ADVANCED_CFG::GetCfg().m_MsgPanelShowUuids;
    std::optional<wxString> uuid;

    if( showUuids > 0 )
    {
        uuid = aKiid.AsString();

        if( showUuids == 2 )
            uuid = uuid->SubString( 0, 7 );
    }

    return uuid;
}
