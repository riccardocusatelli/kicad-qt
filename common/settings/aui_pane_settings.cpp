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
 * @file aui_pane_settings.cpp
 * JSON serialization of wxAuiPaneInfo (GUI part of aui_settings.cpp).
 */

#include <json_common.h>
#include <settings/aui_settings.h>
#include <settings/json_settings_templates.h>

#include <wx/aui/framemanager.h>
#include <wx/gdicmn.h>


void to_json( nlohmann::json& aJson, const wxAuiPaneInfo& aPaneInfo )
{
    aJson = nlohmann::json
    {
        { "name", aPaneInfo.name },
        { "caption", aPaneInfo.caption },
        { "state", aPaneInfo.state },
        { "dock_direction", aPaneInfo.dock_direction },
        { "dock_layer", aPaneInfo.dock_layer },
        { "dock_row", aPaneInfo.dock_row },
        { "dock_pos", aPaneInfo.dock_pos },
        { "dock_proportion", aPaneInfo.dock_proportion },
        { "best_size", aPaneInfo.best_size },
        { "min_size", aPaneInfo.min_size },
        { "max_size", aPaneInfo.max_size },
        { "floating_pos", aPaneInfo.floating_pos },
        { "floating_size", aPaneInfo.floating_size },
        { "rect", aPaneInfo.rect }
    };
}


void from_json( const nlohmann::json& aJson, wxAuiPaneInfo& aPaneInfo )
{
    aPaneInfo.name = aJson.at( "name" ).get<wxString>();
    aPaneInfo.caption = aJson.at( "caption" ).get<wxString>();
    aPaneInfo.state = aJson.at( "state" ).get<int>();
    aPaneInfo.dock_direction = aJson.at( "dock_direction" ).get<unsigned int>();
    aPaneInfo.dock_layer = aJson.at( "dock_layer" ).get<int>();
    aPaneInfo.dock_row = aJson.at( "dock_row" ).get<int>();
    aPaneInfo.dock_pos = aJson.at( "dock_pos" ).get<int>();
    aPaneInfo.dock_proportion = aJson.at( "dock_proportion" ).get<int>();
    aPaneInfo.best_size = aJson.at( "best_size" ).get<wxSize>();
    aPaneInfo.min_size = aJson.at( "min_size" ).get<wxSize>();
    aPaneInfo.max_size = aJson.at( "max_size" ).get<wxSize>();
    aPaneInfo.floating_pos = aJson.at( "floating_pos" ).get<wxPoint>();
    aPaneInfo.floating_size = aJson.at( "floating_size" ).get<wxSize>();
    aPaneInfo.rect = aJson.at( "rect" ).get<wxRect>();
}


bool operator<( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs )
{
    if( aLhs.name != aRhs.name )
        return aLhs.name < aRhs.name;

    if( aLhs.caption != aRhs.caption )
        return aLhs.caption < aRhs.caption;

    if( aLhs.state != aRhs.state )
        return aLhs.state < aRhs.state;

    if( aLhs.dock_direction != aRhs.dock_direction )
        return aLhs.dock_direction < aRhs.dock_direction;

    if( aLhs.dock_layer != aRhs.dock_layer )
        return aLhs.dock_layer < aRhs.dock_layer;

    if( aLhs.dock_row != aRhs.dock_row )
        return aLhs.dock_row < aRhs.dock_row;

    if( aLhs.dock_pos != aRhs.dock_pos )
        return aLhs.dock_pos < aRhs.dock_pos;

    if( aLhs.dock_proportion != aRhs.dock_proportion )
        return aLhs.dock_proportion < aRhs.dock_proportion;

    if( aLhs.best_size != aRhs.best_size )
        return aLhs.best_size < aRhs.best_size;

    if( aLhs.min_size != aRhs.min_size )
        return aLhs.min_size < aRhs.min_size;

    if( aLhs.max_size != aRhs.max_size )
        return aLhs.max_size < aRhs.max_size;

    if( aLhs.floating_pos != aRhs.floating_pos )
        return aLhs.floating_pos < aRhs.floating_pos;

    if( aLhs.floating_size != aRhs.floating_size )
        return aLhs.floating_size < aRhs.floating_size;

    return aLhs.rect < aRhs.rect;
}


bool operator==( const wxAuiPaneInfo& aLhs, const wxAuiPaneInfo& aRhs )
{
    if( aLhs.name != aRhs.name )
        return false;

    if( aLhs.caption != aRhs.caption )
        return false;

    if( aLhs.state != aRhs.state )
        return false;

    if( aLhs.dock_direction != aRhs.dock_direction )
        return false;

    if( aLhs.dock_layer != aRhs.dock_layer )
        return false;

    if( aLhs.dock_row != aRhs.dock_row )
        return false;

    if( aLhs.dock_pos != aRhs.dock_pos )
        return false;

    if( aLhs.dock_proportion != aRhs.dock_proportion )
        return false;

    if( aLhs.best_size != aRhs.best_size )
        return false;

    if( aLhs.min_size != aRhs.min_size )
        return false;

    if( aLhs.max_size != aRhs.max_size )
        return false;

    if( aLhs.floating_pos != aRhs.floating_pos )
        return false;

    if( aLhs.floating_size != aRhs.floating_size )
        return false;

    if( aLhs.rect != aRhs.rect )
        return false;

    return true;
}


template KICOMMON_API std::optional<wxAuiPaneInfo>
                      JSON_SETTINGS::Get<wxAuiPaneInfo>( const std::string& aPath ) const;
template KICOMMON_API void JSON_SETTINGS::Set<wxAuiPaneInfo>( const std::string& aPath,
                                                              wxAuiPaneInfo      aValue );
