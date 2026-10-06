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
 * @file test_clipboard_io.cpp
 * CLIPBOARD_IO without the clipboard of wxWidgets: the items written to a text and parsed back.
 */

#include <boost/test/unit_test.hpp>

#include <memory>

#include <base_units.h>
#include <board.h>
#include <kicad_clipboard.h>
#include <pcb_track.h>


BOOST_AUTO_TEST_SUITE( ClipboardIo )


BOOST_AUTO_TEST_CASE( SaveItemsAroundReferencePoint )
{
    BOARD board;

    PCB_TRACK* track = new PCB_TRACK( &board );
    track->SetStart( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );
    track->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    track->SetLayer( F_Cu );
    board.Add( track );

    wxString     text;
    CLIPBOARD_IO io( [&text]( const wxString& aText ) { text = aText; },
                     [&text]() { return text; } );

    io.SetBoard( &board );
    io.SaveItems( { track }, VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ), false );

    BOOST_REQUIRE( !text.empty() );

    // The board on the clipboard has the track, with the reference point at (0, 0)
    std::unique_ptr<BOARD_ITEM> parsed( io.Parse() );

    BOOST_REQUIRE( parsed );
    BOOST_REQUIRE_EQUAL( parsed->Type(), PCB_T );

    BOARD* clipBoard = static_cast<BOARD*>( parsed.get() );

    BOOST_REQUIRE_EQUAL( clipBoard->Tracks().size(), 1 );
    BOOST_CHECK( clipBoard->Tracks().front()->GetStart() == VECTOR2I( 0, 0 ) );
    BOOST_CHECK( clipBoard->Tracks().front()->GetEnd() == VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );

    // The source board is not changed
    BOOST_CHECK( track->GetStart() == VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
