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
 * @file test_sch_undo_redo.cpp
 * Undo and redo of schematic changes without an editor: SCH_COMMIT with a listener records
 * the change, SCH_UNDO_REDO applies it back and forth.
 */

#include <boost/test/unit_test.hpp>

#include <memory>
#include <vector>

#include <project.h>
#include <sch_commit.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_text.h>
#include <sch_undo_redo.h>
#include <schematic.h>
#include <settings/settings_manager.h>


namespace
{

/// Records the undo entries of the commits on the screen shown, as an editor does.
class RECORDING_LISTENER : public SCH_COMMIT_LISTENER
{
public:
    RECORDING_LISTENER( SCHEMATIC& aSchematic, std::vector<PICKED_ITEMS_LIST*>& aEntries ) :
            m_schematic( aSchematic ),
            m_entries( aEntries )
    {
    }

    SCH_GROUP*  EnteredGroup() const override { return nullptr; }
    SCH_SCREEN* CurrentScreen() const override { return m_schematic.GetCurrentScreen(); }

    bool GetCurrentSheet( SCH_SHEET_PATH& aSheet ) const override
    {
        aSheet = m_schematic.CurrentSheet();
        return true;
    }

    void UpdateHopOveredWires( SCH_ITEM* ) override {}
    void ItemAdded( SCH_ITEM* ) override {}
    void ItemRemoved( SCH_ITEM* ) override {}
    void UpdateItem( SCH_ITEM* aItem, SCH_SCREEN* aScreen, bool ) override
    {
        aScreen->Update( aItem );
    }

    void Deselect( SCH_ITEM* ) override {}
    void HierarchyChanged() override {}
    void Reverted() override {}

    void Pushed( SCH_COMMIT&, PICKED_ITEMS_LIST& aUndoList, int, const PUSH_RESULT& ) override
    {
        auto* entry = new PICKED_ITEMS_LIST();
        entry->CopyList( aUndoList );
        m_entries.push_back( entry );
    }

    LIB_SYMBOL* CurrentSymbol() const override { return nullptr; }
    void SymbolPushed( LIB_SYMBOL*, LIB_SYMBOL* aCopy, const wxString&, int ) override
    {
        delete aCopy;
    }

    void SymbolReverted( LIB_SYMBOL* aCopy ) override { delete aCopy; }
    void Committed( int ) override {}

private:
    SCHEMATIC&                       m_schematic;
    std::vector<PICKED_ITEMS_LIST*>& m_entries;
};


/// An editor without a view: the items go to and from their screen.
class SCREEN_UNDO_LISTENER : public SCH_UNDO_REDO::LISTENER
{
public:
    explicit SCREEN_UNDO_LISTENER( SCHEMATIC& aSchematic ) :
            m_schematic( aSchematic )
    {
    }

    const SCH_SHEET_PATH& CurrentSheet() const override { return m_schematic.CurrentSheet(); }

    void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        aScreen->Append( static_cast<SCH_ITEM*>( aItem ) );
    }

    void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        aScreen->Remove( static_cast<SCH_ITEM*>( aItem ) );
    }

    void SheetRemoving( SCH_SHEET* ) override {}
    void SwapPageSettings( DS_PROXY_UNDO_ITEM*, const SCH_SHEET_PATH& ) override {}
    void ClearRepeatItems() override {}
    void AddRepeatItem( SCH_ITEM* ) override {}

private:
    SCHEMATIC& m_schematic;
};


struct UNDO_FIXTURE
{
    UNDO_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();
    }

    ~UNDO_FIXTURE()
    {
        for( PICKED_ITEMS_LIST* entry : m_entries )
        {
            SCH_UNDO_REDO::DeleteItems( *entry );
            delete entry;
        }

        m_schematic.reset();
    }

    SCH_SCREEN* screen() const { return m_schematic->GetCurrentScreen(); }

    std::unique_ptr<SCH_COMMIT> makeCommit()
    {
        return std::make_unique<SCH_COMMIT>(
                std::make_unique<RECORDING_LISTENER>( *m_schematic, m_entries ) );
    }

    /// Undo (or redo) the last entry recorded, as an editor does.
    SCH_UNDO_REDO::RESULT apply()
    {
        SCREEN_UNDO_LISTENER  listener( *m_schematic );
        SCH_UNDO_REDO::RESULT result = SCH_UNDO_REDO::Apply( *m_schematic, *m_entries.back(),
                                                             listener );
        m_entries.back()->ReversePickersListOrder();
        return result;
    }

    bool onScreen( SCH_ITEM* aItem ) const
    {
        for( SCH_ITEM* item : screen()->Items() )
        {
            if( item == aItem )
                return true;
        }

        return false;
    }

    SETTINGS_MANAGER                m_settingsManager;
    std::unique_ptr<SCHEMATIC>      m_schematic;
    std::vector<PICKED_ITEMS_LIST*> m_entries;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( SchUndoRedo, UNDO_FIXTURE )


BOOST_AUTO_TEST_CASE( MoveText )
{
    SCH_TEXT* text = new SCH_TEXT( VECTOR2I( 0, 0 ), wxT( "text" ) );
    screen()->Append( text );

    std::unique_ptr<SCH_COMMIT> commit = makeCommit();
    commit->Modify( text, screen() );
    text->Move( VECTOR2I( 1270000, 0 ) );
    commit->Push( wxT( "Move" ) );

    BOOST_REQUIRE_EQUAL( m_entries.size(), 1 );

    SCH_UNDO_REDO::RESULT result = apply();         // undo

    BOOST_CHECK( !result.connectivityChanged );
    BOOST_CHECK( onScreen( text ) );
    BOOST_CHECK( text->GetPosition() == VECTOR2I( 0, 0 ) );

    apply();                                        // redo

    BOOST_CHECK( text->GetPosition() == VECTOR2I( 1270000, 0 ) );
}


BOOST_AUTO_TEST_CASE( DeleteWire )
{
    SCH_LINE* wire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire->SetEndPoint( VECTOR2I( 2540000, 0 ) );
    screen()->Append( wire );

    std::unique_ptr<SCH_COMMIT> commit = makeCommit();
    commit->Remove( wire, screen() );
    commit->Push( wxT( "Delete" ) );

    BOOST_REQUIRE_EQUAL( m_entries.size(), 1 );
    BOOST_CHECK( !onScreen( wire ) );

    SCH_UNDO_REDO::RESULT result = apply();         // undo: the wire is back

    BOOST_CHECK( result.connectivityChanged );
    BOOST_CHECK( result.connectablesChanged );
    BOOST_CHECK( onScreen( wire ) );

    apply();                                        // redo: the entry owns the wire again

    BOOST_CHECK( !onScreen( wire ) );
}


BOOST_AUTO_TEST_SUITE_END()
