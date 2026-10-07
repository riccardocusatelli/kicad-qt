/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 CERN
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <bitmaps.h>
#include <tool/conditional_menu.h>
#include <tool/action_menu.h>
#include <tool/selection.h>
#include <kiface_base.h>
#include <widgets/ui_common.h>
#include <core/ki_assert.h>


CONDITIONAL_MENU::CONDITIONAL_MENU( TOOL_INTERACTIVE* aTool ) :
        ACTION_MENU( true, aTool )
{
}


ACTION_MENU* CONDITIONAL_MENU::create() const
{
    CONDITIONAL_MENU* clone = new CONDITIONAL_MENU( m_tool );
    clone->m_entries = m_entries;
    return clone;
}


void CONDITIONAL_MENU::AddItem( const TOOL_ACTION& aAction, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    KI_ASSERT( aAction.GetId() > 0 ); // Check if action was previously registered in ACTION_MANAGER
    addEntry( ENTRY( &aAction, aCondition, aOrder, false ) );
}


void CONDITIONAL_MENU::AddCheckItem( const TOOL_ACTION& aAction,
                                     const SELECTION_CONDITION& aCondition, int aOrder )
{
    KI_ASSERT( aAction.GetId() > 0 ); // Check if action was previously registered in ACTION_MANAGER
    addEntry( ENTRY( &aAction, aCondition, aOrder, true ) );
}


void CONDITIONAL_MENU::AddItem( int aId, const wxString& aText, const wxString& aTooltip,
                                BITMAPS aIcon, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    addEntry( ENTRY( ITEM{ aId, aText, aTooltip }, aIcon, aCondition, aOrder, false ) );
}


void CONDITIONAL_MENU::AddCheckItem( int aId, const wxString& aText, const wxString& aTooltip,
                                     BITMAPS aIcon, const SELECTION_CONDITION& aCondition,
                                     int aOrder )
{
    addEntry( ENTRY( ITEM{ aId, aText, aTooltip }, aIcon, aCondition, aOrder, true ) );
}


void CONDITIONAL_MENU::AddMenu( ACTION_MENU* aMenu, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    addEntry( ENTRY( aMenu, aCondition, aOrder ) );
}


void CONDITIONAL_MENU::AddSeparator( int aOrder )
{
    addEntry( ENTRY( SELECTION_CONDITIONS::ShowAlways, aOrder ) );
}


void CONDITIONAL_MENU::AddSeparator( const SELECTION_CONDITION& aCondition, int aOrder )
{
    addEntry( ENTRY( aCondition, aOrder ) );
}


SELECTION g_resolveDummySelection;


void CONDITIONAL_MENU::Resolve()
{
    Evaluate( g_resolveDummySelection );
    UpdateAll();

    runOnSubmenus(
            [] ( ACTION_MENU* aMenu )
            {
                CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( aMenu );

                if( conditionalMenu )
                    conditionalMenu->Resolve();
            } );
}


void CONDITIONAL_MENU::Evaluate( const SELECTION& aSelection )
{
    Clear();

    // We try to avoid adding useless separators (when no menuitems between separators)
    int menu_count = 0;     // number of menus since the latest separator

    for( const ENTRY& entry : m_entries )
    {
        const SELECTION_CONDITION& cond = entry.Condition();
        bool                       result;

        try
        {
            result = cond( aSelection );
        }
        catch( std::exception& )
        {
            continue;
        }

        if( !result )
            continue;

        switch( entry.Type() )
        {
        case ENTRY::ACTION:
            Add( *entry.Action(), entry.IsCheckmarkEntry() );
            menu_count++;
            break;

        case ENTRY::MENU:
            entry.Menu()->UpdateTitle();
            Add( entry.Menu()->Clone() );
            menu_count++;
            break;

        case ENTRY::ITEM_ENTRY:
            Add( wxGetTranslation( entry.Item().m_label ),
                 wxGetTranslation( entry.Item().m_tooltip ),
                 entry.Item().m_id, entry.GetIcon(), entry.IsCheckmarkEntry() );
            menu_count++;
            break;

        case ENTRY::SEPARATOR:
            if( menu_count )
                AppendSeparator();

            menu_count = 0;
            break;

        default:
            KI_ASSERT( false );
            break;
        }
    }

    // Recursively call Evaluate on all the submenus that are CONDITIONAL_MENUs to ensure
    // they are updated. This is also required on GTK to make sure the menus have the proper
    // size when created.
    runOnSubmenus(
            [&aSelection]( ACTION_MENU* aMenu )
            {
                CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( aMenu );

                if( conditionalMenu )
                    conditionalMenu->Evaluate( aSelection );
            } );
}


void CONDITIONAL_MENU::addEntry( ENTRY aEntry )
{
    if( aEntry.Order() < 0 )        // Any order, so give it any order number
        aEntry.SetOrder( m_entries.size() );

    std::list<ENTRY>::iterator it = m_entries.begin();

    // Find the right spot for the entry
    while( it != m_entries.end() && it->Order() <= aEntry.Order() )
        ++it;

    m_entries.insert( it, aEntry );
}
