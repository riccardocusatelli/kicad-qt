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
 * @file test_board_undo_redo.cpp
 * Undo and redo of board changes without an editor: BOARD_COMMIT with a listener records the
 * change, BOARD_UNDO_REDO applies it back and forth.
 */

#include <boost/test/unit_test.hpp>

#include <memory>
#include <vector>

#include <board.h>
#include <board_commit.h>
#include <board_undo_redo.h>
#include <footprint.h>
#include <pcb_field.h>
#include <pcb_track.h>


namespace
{

/// Records the undo entries of the commits, as an editor does.
class RECORDING_LISTENER : public BOARD_COMMIT_LISTENER
{
public:
    explicit RECORDING_LISTENER( BOARD* aBoard, std::vector<PICKED_ITEMS_LIST*>& aEntries ) :
            m_board( aBoard ),
            m_entries( aEntries )
    {
    }

    BOARD*     Board() const override { return m_board; }
    PCB_GROUP* EnteredGroup() const override { return nullptr; }
    bool       AutoRefillZones() const override { return false; }
    void       ItemAdded( BOARD_ITEM* ) override {}
    void       ItemRemoved( BOARD_ITEM* ) override {}
    void       ItemChanged( BOARD_ITEM* ) override {}
    void       Deselect( BOARD_ITEM* ) override {}
    bool       PruneSelection( const std::unordered_set<EDA_ITEM*>& ) override { return false; }
    void       RatsnestChanged() override {}
    void       SolderMaskChanged() override {}
    void       BoardOutlineChanged() override {}
    void       RuleAreasChanged( const std::vector<BOX2I>& ) override {}
    void       Reverted() override {}

    void Pushed( PICKED_ITEMS_LIST& aUndoList, int, const PUSH_RESULT& ) override
    {
        auto* entry = new PICKED_ITEMS_LIST();

        for( unsigned ii = 0; ii < aUndoList.GetCount(); ii++ )
            entry->PushItem( aUndoList.GetItemWrapper( ii ) );

        BOARD_UNDO_REDO::CompleteEntry( *entry, 0, UNDO_REDO::UNSPECIFIED );
        m_entries.push_back( entry );
    }

private:
    BOARD*                           m_board;
    std::vector<PICKED_ITEMS_LIST*>& m_entries;
};


/// An editor without a view.
class NULL_UNDO_LISTENER : public BOARD_UNDO_REDO::LISTENER
{
public:
    void ItemRemoved( EDA_ITEM* ) override {}
    void ItemAdded( EDA_ITEM* ) override {}
    bool ShowRatsnest() const override { return false; }
    void SetOrigin( UNDO_REDO, EDA_ITEM*, const VECTOR2I& ) override {}
    void SwapPageSettings( DS_PROXY_UNDO_ITEM* ) override {}
};


struct UNDO_FIXTURE
{
    ~UNDO_FIXTURE()
    {
        for( PICKED_ITEMS_LIST* entry : m_entries )
        {
            BOARD_UNDO_REDO::DeleteItems( *entry );
            delete entry;
        }
    }

    std::unique_ptr<BOARD_COMMIT> makeCommit()
    {
        return std::make_unique<BOARD_COMMIT>(
                std::make_unique<RECORDING_LISTENER>( &m_board, m_entries ), true, false );
    }

    /// Undo (or redo) the last entry recorded, as an editor does.
    BOARD_UNDO_REDO::RESULT apply()
    {
        NULL_UNDO_LISTENER      listener;
        BOARD_UNDO_REDO::RESULT result = BOARD_UNDO_REDO::Apply( m_board, *m_entries.back(),
                                                                 listener, true );
        m_entries.back()->ReversePickersListOrder();
        return result;
    }

    BOARD                           m_board;
    std::vector<PICKED_ITEMS_LIST*> m_entries;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( BoardUndoRedo, UNDO_FIXTURE )


BOOST_AUTO_TEST_CASE( MoveFootprint )
{
    FOOTPRINT* footprint = new FOOTPRINT( &m_board );
    footprint->SetPosition( VECTOR2I( 0, 0 ) );
    m_board.Add( footprint );

    const KIID referenceUuid = footprint->GetField( FIELD_T::REFERENCE )->m_Uuid;

    std::unique_ptr<BOARD_COMMIT> commit = makeCommit();
    commit->Modify( footprint );
    footprint->Move( VECTOR2I( 1000000, 2000000 ) );
    commit->Push( wxT( "Move" ) );

    BOOST_REQUIRE_EQUAL( m_entries.size(), 1 );

    BOARD_UNDO_REDO::RESULT result = apply();       // undo

    BOOST_CHECK( !result.itemsNotFound );
    BOOST_CHECK( result.shapesChanged );
    BOOST_CHECK_EQUAL( m_board.Footprints().size(), 1 );
    BOOST_CHECK( m_board.Footprints().front() == footprint );
    BOOST_CHECK( footprint->GetPosition() == VECTOR2I( 0, 0 ) );

    // The data of the footprint come from its copy: the fields keep their UUIDs
    BOOST_CHECK( footprint->GetField( FIELD_T::REFERENCE )->m_Uuid == referenceUuid );

    apply();                                        // redo

    BOOST_CHECK( footprint->GetPosition() == VECTOR2I( 1000000, 2000000 ) );
    BOOST_CHECK( footprint->GetField( FIELD_T::REFERENCE )->m_Uuid == referenceUuid );
}


BOOST_AUTO_TEST_CASE( DeleteTrack )
{
    PCB_TRACK* track = new PCB_TRACK( &m_board );
    track->SetStart( VECTOR2I( 0, 0 ) );
    track->SetEnd( VECTOR2I( 1000000, 0 ) );
    track->SetLayer( F_Cu );
    m_board.Add( track );

    std::unique_ptr<BOARD_COMMIT> commit = makeCommit();
    commit->Remove( track );
    commit->Push( wxT( "Delete" ) );

    BOOST_CHECK( m_board.Tracks().empty() );

    BOARD_UNDO_REDO::RESULT result = apply();       // undo: the track is back on the board

    BOOST_CHECK( result.connectivityChanged );
    BOOST_REQUIRE_EQUAL( m_board.Tracks().size(), 1 );
    BOOST_CHECK( m_board.Tracks().front() == track );
    BOOST_CHECK( !track->HasFlag( UR_TRANSIENT ) );

    apply();                                        // redo: the entry owns the track again

    BOOST_CHECK( m_board.Tracks().empty() );
    BOOST_CHECK( track->HasFlag( UR_TRANSIENT ) );
}


BOOST_AUTO_TEST_CASE( FootprintCopyKeepsFieldUuids )
{
    FOOTPRINT footprint( &m_board );
    FOOTPRINT copy( footprint );

    for( FIELD_T id : { FIELD_T::REFERENCE, FIELD_T::VALUE, FIELD_T::DATASHEET,
                        FIELD_T::DESCRIPTION } )
    {
        BOOST_REQUIRE( copy.GetField( id ) );
        BOOST_CHECK( copy.GetField( id )->m_Uuid == footprint.GetField( id )->m_Uuid );
        BOOST_CHECK( copy.GetField( id )->GetParent() == &copy );
    }
}


BOOST_AUTO_TEST_SUITE_END()
