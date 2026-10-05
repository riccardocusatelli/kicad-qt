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

#pragma once

#include <list>
#include <string>
#include <vector>

#include <frame_type.h>
#include <kicommon.h>
#include <wx/string.h>
#include <core/ki_assert.h>


/**
 * Class to hold basic information about controls that can be added to the toolbars.
 */
class KICOMMON_API ACTION_TOOLBAR_CONTROL
{
public:
    ACTION_TOOLBAR_CONTROL( const std::string& aName, const wxString& aUiName,
                            const wxString& aDescription, std::vector<FRAME_T> aSupportedContexts ) :
        m_name( aName ),
        m_uiname( aUiName ),
        m_description( aDescription ),
        m_supportedContexts( aSupportedContexts )
    {
        KI_ASSERT_MSG( aName.starts_with( "control" ),
                      wxString::Format( "Control name \"%s\" must start with \"control\"", aName ) );

        GetAll().push_back( this );
    }

    /// All the controls (they register themselves).
    static std::list<ACTION_TOOLBAR_CONTROL*>& GetAll();

    const std::string& GetName() const { return m_name; }
    const wxString& GetUiName() const { return m_uiname; }
    const wxString& GetDescription() const { return m_description; }

    bool SupportedFor( FRAME_T aFrame ) const
    {
        for( FRAME_T candidate : m_supportedContexts )
        {
            if( aFrame == candidate )
                return true;
        }

        return false;
    }

protected:
    /**
     * Name of the control - must start with "control."
     */
    std::string m_name;

    /**
     * Short description to show for the control
     */
    wxString m_uiname;

    /**
     * User-visible tooltip for the control
     */
    wxString m_description;

    /**
     * List of frame types that support the control.
     */
    std::vector<FRAME_T> m_supportedContexts;
};

class ACTION_TOOLBAR_CONTROLS
{
public:
    static ACTION_TOOLBAR_CONTROL gridSelect;
    static ACTION_TOOLBAR_CONTROL zoomSelect;
    static ACTION_TOOLBAR_CONTROL ipcScripting;
    static ACTION_TOOLBAR_CONTROL unitSelector;
    static ACTION_TOOLBAR_CONTROL bodyStyleSelector;
    static ACTION_TOOLBAR_CONTROL layerSelector;
    static ACTION_TOOLBAR_CONTROL overrideLocks;
};
