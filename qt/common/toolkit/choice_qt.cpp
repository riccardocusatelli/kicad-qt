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
#include <wx/defs.h>        // wxNOT_FOUND
#include <wx/string.h>

#include <qt_strings.h>
#include <ui_hooks.h>


namespace TOOLKIT
{

TOOLKIT_CHOICE* CreateChoice( TOOLKIT_WINDOW* aParent, int aId, std::function<void()> aOnSelect )
{
    QComboBox* choice = new QComboBox( aParent );

    // A choice box: the user picks one of the items, without editing them
    choice->setEditable( false );
    choice->setSizeAdjustPolicy( QComboBox::AdjustToContents );
    KIQT::SetWindowId( choice, aId );

    if( aOnSelect )
    {
        QObject::connect( choice, &QComboBox::activated, choice,
                          [aOnSelect]( int )
                          {
                              aOnSelect();
                          } );
    }

    return choice;
}


void ChoiceClear( TOOLKIT_CHOICE* aChoice )
{
    aChoice->clear();
}


void ChoiceAppend( TOOLKIT_CHOICE* aChoice, const wxString& aItem )
{
    aChoice->addItem( ToQString( aItem ) );
}


void ChoiceSet( TOOLKIT_CHOICE* aChoice, const wxArrayString& aItems )
{
    aChoice->clear();

    for( const wxString& item : aItems )
        aChoice->addItem( ToQString( item ) );
}


int ChoiceGetCount( const TOOLKIT_CHOICE* aChoice )
{
    return aChoice->count();
}


int ChoiceGetSelection( const TOOLKIT_CHOICE* aChoice )
{
    int index = aChoice->currentIndex();
    return index < 0 ? wxNOT_FOUND : index;
}


void ChoiceSetSelection( TOOLKIT_CHOICE* aChoice, int aIndex )
{
    aChoice->setCurrentIndex( aIndex );
}


wxString ChoiceGetString( const TOOLKIT_CHOICE* aChoice, int aIndex )
{
    return ToWxString( aChoice->itemText( aIndex ) );
}


int ChoiceFindString( const TOOLKIT_CHOICE* aChoice, const wxString& aItem )
{
    int index = aChoice->findText( ToQString( aItem ) );
    return index < 0 ? wxNOT_FOUND : index;
}


void ChoiceSetToolTip( TOOLKIT_CHOICE* aChoice, const wxString& aTip )
{
    aChoice->setToolTip( ToQString( aTip ) );
}

} // namespace TOOLKIT
