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

#include "undo_stack.h"


UNDO_STACK::UNDO_STACK( DELETE_ITEMS aDeleteItems ) :
        m_deleteItems( std::move( aDeleteItems ) )
{
}


UNDO_STACK::~UNDO_STACK()
{
    Clear();
}


void UNDO_STACK::Push( PICKED_ITEMS_LIST* aEntry )
{
    // The saved state was among the changes undone: it cannot be reached any more
    if( m_savedDepth > static_cast<int>( m_undo.m_CommandsList.size() ) )
        m_savedDepth = -1;

    clear( m_redo );
    m_undo.PushCommand( aEntry );
}


PICKED_ITEMS_LIST* UNDO_STACK::LastChange() const
{
    return m_undo.m_CommandsList.empty() ? nullptr : m_undo.m_CommandsList.back();
}


void UNDO_STACK::move( UNDO_REDO_CONTAINER& aFrom, UNDO_REDO_CONTAINER& aTo, const APPLY& aApply )
{
    PICKED_ITEMS_LIST* entry = aFrom.PopCommand();

    if( !entry )
        return;

    aApply( *entry );

    // The pickers were applied last to first: the reverse entry applies them first to last
    entry->ReversePickersListOrder();
    aTo.PushCommand( entry );
}


void UNDO_STACK::Undo( const APPLY& aApply )
{
    move( m_undo, m_redo, aApply );
}


void UNDO_STACK::Redo( const APPLY& aApply )
{
    move( m_redo, m_undo, aApply );
}


bool UNDO_STACK::IsModified() const
{
    return static_cast<int>( m_undo.m_CommandsList.size() ) != m_savedDepth;
}


void UNDO_STACK::SetSaved()
{
    m_savedDepth = static_cast<int>( m_undo.m_CommandsList.size() );
}


void UNDO_STACK::Clear()
{
    clear( m_undo );
    clear( m_redo );
    m_savedDepth = 0;
}


void UNDO_STACK::clear( UNDO_REDO_CONTAINER& aList )
{
    for( PICKED_ITEMS_LIST* entry : aList.m_CommandsList )
    {
        m_deleteItems( *entry );
        delete entry;
    }

    aList.m_CommandsList.clear();
}
