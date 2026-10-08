/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <toolkit/choice.h>

#include <wx/arrstr.h>
#include <wx/string.h>


namespace TOOLKIT
{

TOOLKIT_CHOICE* CreateChoice( TOOLKIT_WINDOW* aParent, int aId, std::function<void()> aOnSelect )
{
    wxChoice* choice = new wxChoice( aParent, aId, wxDefaultPosition, wxDefaultSize, 0, nullptr );

    if( aOnSelect )
    {
        choice->Bind( wxEVT_CHOICE,
                      [aOnSelect]( wxCommandEvent& )
                      {
                          aOnSelect();
                      } );
    }

    return choice;
}


void ChoiceClear( TOOLKIT_CHOICE* aChoice )
{
    aChoice->Clear();
}


void ChoiceAppend( TOOLKIT_CHOICE* aChoice, const wxString& aItem )
{
    aChoice->Append( aItem );
}


void ChoiceSet( TOOLKIT_CHOICE* aChoice, const wxArrayString& aItems )
{
    aChoice->Set( aItems );
}


int ChoiceGetCount( const TOOLKIT_CHOICE* aChoice )
{
    return (int) aChoice->GetCount();
}


int ChoiceGetSelection( const TOOLKIT_CHOICE* aChoice )
{
    return aChoice->GetSelection();
}


void ChoiceSetSelection( TOOLKIT_CHOICE* aChoice, int aIndex )
{
    aChoice->SetSelection( aIndex );
}


wxString ChoiceGetString( const TOOLKIT_CHOICE* aChoice, int aIndex )
{
    return aChoice->GetString( aIndex );
}


int ChoiceFindString( const TOOLKIT_CHOICE* aChoice, const wxString& aItem )
{
    return aChoice->FindString( aItem );
}


void ChoiceSetToolTip( TOOLKIT_CHOICE* aChoice, const wxString& aTip )
{
    aChoice->SetToolTip( aTip );
}

} // namespace TOOLKIT
