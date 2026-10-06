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

#include "board_tools.h"

#include <QObject>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <geometry/direction45.h>
#include <geometry/shape_line_chain.h>
#include <pcb_track.h>
#include <pcb_view.h>

#include <qt_strings.h>

#include "board_document.h"


/// The segments of a track from @a aStart to @a aEnd: a straight part, then a diagonal one.
static SHAPE_LINE_CHAIN trackPath( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    return DIRECTION_45().BuildInitialTrace( aStart, aEnd );
}


void TRACK_TOOL::Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    BOARD* board = m_doc.Board();

    if( !m_start )
    {
        // A new track, in the net of what it starts from
        m_layer = m_doc.ActiveCopperLayer();
        m_width = board->GetDesignSettings().GetCurrentTrackWidth();
        m_netCode = 0;

        if( BOARD_CONNECTED_ITEM* item = m_doc.ConnectionAt( aPoint, m_layer ) )
            m_netCode = item->GetNetCode();

        m_start = aPoint;
        preview( aPoint );
        return;
    }

    if( aPoint == *m_start )
        return;

    // A click on a pad, a track end or a via ends the track there
    BOARD_CONNECTED_ITEM* target = m_doc.ConnectionAt( aPoint, m_layer );

    fix( aPoint );

    if( target )
        clear();
    else
        preview( aPoint );
}


void TRACK_TOOL::Motion( const VECTOR2I& aPoint )
{
    if( m_start )
        preview( aPoint );
}


bool TRACK_TOOL::Cancel()
{
    if( !m_start )
        return false;

    clear();
    return true;
}


void TRACK_TOOL::preview( const VECTOR2I& aEnd )
{
    KIGFX::PCB_VIEW* view = m_doc.View();
    view->ClearPreview();

    const SHAPE_LINE_CHAIN path = trackPath( *m_start, aEnd );

    for( int ii = 0; ii < path.SegmentCount(); ++ii )
    {
        auto* segment = new PCB_TRACK( m_doc.Board() );
        segment->SetStart( path.CSegment( ii ).A );
        segment->SetEnd( path.CSegment( ii ).B );
        segment->SetWidth( m_width );
        segment->SetLayer( m_layer );
        segment->SetNetCode( m_netCode );

        view->AddToPreview( segment, true );
    }

    view->MarkDirty();
}


void TRACK_TOOL::fix( const VECTOR2I& aEnd )
{
    const SHAPE_LINE_CHAIN        path = trackPath( *m_start, aEnd );
    std::unique_ptr<BOARD_COMMIT> commit = m_doc.MakeCommit();
    bool                          added = false;

    for( int ii = 0; ii < path.SegmentCount(); ++ii )
    {
        const SEG& seg = path.CSegment( ii );

        if( seg.A == seg.B )
            continue;

        auto* track = new PCB_TRACK( m_doc.Board() );
        track->SetStart( seg.A );
        track->SetEnd( seg.B );
        track->SetWidth( m_width );
        track->SetLayer( m_layer );
        track->SetNetCode( m_netCode );

        commit->Add( track );
        added = true;
    }

    m_doc.View()->ClearPreview();

    if( added )
        commit->Push( ToWxString( QObject::tr( "Route Track" ) ) );

    m_start = aEnd;
}


void TRACK_TOOL::clear()
{
    m_start.reset();
    m_doc.View()->ClearPreview();
    m_doc.View()->MarkDirty();
}


void VIA_TOOL::Stop()
{
    m_doc.View()->ClearPreview();
    m_doc.View()->MarkDirty();
}


/// A through via at @a aPoint with the current size and drill.
static PCB_VIA* makeVia( BOARD* aBoard, const VECTOR2I& aPoint )
{
    const BOARD_DESIGN_SETTINGS& settings = aBoard->GetDesignSettings();

    auto* via = new PCB_VIA( aBoard );
    via->SetViaType( VIATYPE::THROUGH );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetPosition( aPoint );
    via->SetWidth( settings.GetCurrentViaSize() );
    via->SetDrill( settings.GetCurrentViaDrill() );

    return via;
}


void VIA_TOOL::Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    PCB_VIA* via = makeVia( m_doc.Board(), aPoint );

    if( BOARD_CONNECTED_ITEM* item = m_doc.ConnectionAt( aPoint, UNDEFINED_LAYER ) )
        via->SetNetCode( item->GetNetCode() );

    std::unique_ptr<BOARD_COMMIT> commit = m_doc.MakeCommit();
    commit->Add( via );
    commit->Push( ToWxString( QObject::tr( "Place Via" ) ) );
}


void VIA_TOOL::Motion( const VECTOR2I& aPoint )
{
    KIGFX::PCB_VIEW* view = m_doc.View();

    view->ClearPreview();
    view->AddToPreview( makeVia( m_doc.Board(), aPoint ), true );
    view->MarkDirty();
}
