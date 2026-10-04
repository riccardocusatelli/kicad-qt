/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "spice_validator.h"
#include "spice_value.h"

#include <wx/textentry.h>
#include <confirm.h>


bool SPICE_VALIDATOR::Validate( wxWindow* aParent )
{
    wxTextEntry* const text = GetTextEntry();

    if( !text )
        return false;

    if( text->IsEmpty() )
    {
        if( m_emptyAllowed )
            return true;

        DisplayError( aParent, wxString::Format( _( "Please, fill required fields" ) ) );
        return false;
    }

    wxString svalue = text->GetValue();

    // In countries where the decimal separator is not a point, if the user
    // has not used a point, replace the decimal separator by the point, as needed
    // by spice simulator which uses the "C" decimal separator
    svalue.Replace(",", "." );

    try
    {
        // If SPICE_VALUE can be constructed, then it is a valid Spice value
        SPICE_VALUE val( svalue );
    }
    catch( ... )
    {
        DisplayError( aParent, wxString::Format( _( "'%s' is not a valid SPICE value." ),
                                                 text->GetValue() ) );

        return false;
    }

    if( svalue != text->GetValue() )
        text->SetValue( svalue );

    return true;
}
