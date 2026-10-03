/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Rivos
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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

#include <settings/aui_settings.h>
#include <json_common.h>

#include <wx/gdicmn.h>


void to_json( nlohmann::json& aJson, const wxPoint& aPoint )
{
    aJson = nlohmann::json
            {
                { "x", aPoint.x },
                { "y", aPoint.y }
            };
}


void from_json( const nlohmann::json& aJson, wxPoint& aPoint )
{
    aPoint.x = aJson.at( "x" ).get<int>();
    aPoint.y = aJson.at( "y" ).get<int>();
}


bool operator<( const wxPoint& aLhs, const wxPoint& aRhs )
{
    int xDelta = aLhs.x - aRhs.x;

    if( xDelta < 0 )
        return true;

    if( ( xDelta == 0 ) && (aLhs.y < aRhs.y ) )
        return true;

    return false;
}


void to_json( nlohmann::json& aJson, const wxSize& aSize )
{
    aJson = nlohmann::json
            {
                { "width", aSize.x },
                { "height", aSize.y }
            };
}


void from_json( const nlohmann::json& aJson, wxSize& aSize )
{
    aSize.SetWidth( aJson.at( "width" ).get<int>() );
    aSize.SetHeight( aJson.at( "height" ).get<int>() );
}


bool operator<( const wxSize& aLhs, const wxSize& aRhs )
{
    int xDelta = aLhs.x - aRhs.x;

    if( xDelta < 0 )
        return true;

    if( ( xDelta == 0 ) && (aLhs.y < aRhs.y ) )
        return true;

    return false;
}


void to_json( nlohmann::json& aJson, const wxRect& aRect )
{
    aJson = nlohmann::json
            {
                { "position", aRect.GetPosition() },
                { "size", aRect.GetSize() }
            };
}


void from_json( const nlohmann::json& aJson, wxRect& aRect )
{
    aRect.SetPosition( aJson.at( "position" ).get<wxPoint>() );
    aRect.SetSize( aJson.at( "size" ).get<wxSize>() );
}


bool operator<( const wxRect& aLhs, const wxRect& aRhs )
{
    if( aLhs.GetSize() != aRhs.GetSize() )
        return aLhs.GetSize() < aRhs.GetSize();

    return aLhs.GetPosition() < aRhs.GetPosition();
}
