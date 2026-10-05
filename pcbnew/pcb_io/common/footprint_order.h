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

#include <unordered_map>

#include <board.h>
#include <board_item.h>
#include <footprint.h>


/**
 * The order of the footprints of a board, for exporters that group items by footprint.
 *
 * Ordering by footprint pointer would make the output depend on where the footprints were
 * allocated, so it would change from one run or build to another.  The footprints are ordered
 * as in the board instead; items without a footprint come first.
 */
class FOOTPRINT_ORDER
{
public:
    explicit FOOTPRINT_ORDER( const BOARD* aBoard )
    {
        size_t index = 0;

        for( const FOOTPRINT* fp : aBoard->Footprints() )
            m_index.emplace( fp, ++index );
    }

    /// Position of a footprint in the board (0 for no footprint).
    size_t Index( const FOOTPRINT* aFootprint ) const
    {
        auto it = m_index.find( aFootprint );
        return it != m_index.end() ? it->second : 0;
    }

    /// Strict weak ordering of items: by parent footprint, then by type.
    bool operator()( const BOARD_ITEM* aA, const BOARD_ITEM* aB ) const
    {
        size_t indexA = Index( aA->GetParentFootprint() );
        size_t indexB = Index( aB->GetParentFootprint() );

        if( indexA == indexB )
            return aA->Type() < aB->Type();

        return indexA < indexB;
    }

private:
    std::unordered_map<const FOOTPRINT*, size_t> m_index;
};
