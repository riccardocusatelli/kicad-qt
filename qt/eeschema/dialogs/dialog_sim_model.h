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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file dialog_sim_model.h
 * DIALOG_SIM_MODEL of the Qt build: the editor of the simulation models of the symbols needs the
 * property grid of wxWidgets (wxPropertyGridManager), not on Qt yet.  The dialogs of the symbol
 * properties open it as on wxWidgets; it tells that the editor is not available.
 */

#ifndef DIALOG_SIM_MODEL_H
#define DIALOG_SIM_MODEL_H

#include <vector>

#include <confirm.h>
#include <sch_field.h>
#include <wx/defs.h>

class EDA_BASE_FRAME;


template <typename T>
class DIALOG_SIM_MODEL
{
public:
    DIALOG_SIM_MODEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame, T& aSymbol,
                      std::vector<SCH_FIELD>& aFields ) :
            m_parent( aParent )
    {
    }

    int ShowModal()
    {
        DisplayInfoMessage( m_parent, _( "The simulation model editor is not available yet in "
                                         "this version of KiCad." ) );
        return wxID_CANCEL;
    }

    /// See the dialog of the wxWidgets build (eeschema/dialogs/dialog_sim_model.h).
    static void RestoreInferredValue( std::vector<SCH_FIELD>& aFields, const wxString& aOriginalValue,
                                      bool aOverwritten, bool aStoredInValue )
    {
        if( !aOverwritten || aStoredInValue )
            return;

        if( SCH_FIELD* valueField = FindField( aFields, FIELD_T::VALUE ) )
            valueField->SetText( aOriginalValue );
    }

private:
    wxWindow* m_parent;
};

#endif /* DIALOG_SIM_MODEL_H */
