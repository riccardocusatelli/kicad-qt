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
 * @file json_settings_templates.h
 * Definitions of JSON_SETTINGS::Get() and Set(), for the files that instantiate them:
 * json_settings.cpp (model types) and aui_pane_settings.cpp (wxAuiPaneInfo, GUI).
 */

#ifndef JSON_SETTINGS_TEMPLATES_H
#define JSON_SETTINGS_TEMPLATES_H

#include <settings/json_settings.h>
#include <settings/json_settings_internals.h>


template<typename ValueType>
std::optional<ValueType> JSON_SETTINGS::Get( const std::string& aPath ) const
{
    if( std::optional<nlohmann::json> ret = GetJson( aPath ) )
    {
        try
        {
            return ret->get<ValueType>();
        }
        catch( ... )
        {
        }
    }

    return std::nullopt;
}


template<typename ValueType>
void JSON_SETTINGS::Set( const std::string& aPath, ValueType aVal )
{
    m_internals->SetFromString( aPath, std::move( aVal ) );
}

#endif // JSON_SETTINGS_TEMPLATES_H
