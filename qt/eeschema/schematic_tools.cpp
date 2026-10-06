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

#include "schematic_tools.h"

#include <set>

#include <QInputDialog>

#include <sch_commit.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_view.h>
#include <schematic.h>

#include <qt_strings.h>

#include "schematic_document.h"


void WIRE_TOOL::Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    if( m_points.empty() )
    {
        m_points.push_back( aPoint );
        preview( aPoint );
        return;
    }

    if( aPoint == m_points.back() )
        return;

    m_points = pointsTo( aPoint );

    // A click on a pin, a wire end or a label ends the wire there
    if( m_doc.CurrentScreen()->IsTerminalPoint( aPoint, LAYER_WIRE ) )
        finish();
    else
        preview( aPoint );
}


void WIRE_TOOL::Motion( const VECTOR2I& aPoint )
{
    if( !m_points.empty() )
        preview( aPoint );
}


bool WIRE_TOOL::Cancel()
{
    if( m_points.empty() )
        return false;

    clear();
    return true;
}


std::vector<VECTOR2I> WIRE_TOOL::pointsTo( const VECTOR2I& aEnd ) const
{
    std::vector<VECTOR2I> points = m_points;
    const VECTOR2I        last = points.back();

    // Horizontally, then vertically
    const VECTOR2I corner( aEnd.x, last.y );

    if( corner != last && corner != aEnd )
        points.push_back( corner );

    if( aEnd != last )
        points.push_back( aEnd );

    return points;
}


void WIRE_TOOL::preview( const VECTOR2I& aEnd )
{
    KIGFX::SCH_VIEW* view = m_doc.View();
    view->ClearPreview();

    const std::vector<VECTOR2I> points = pointsTo( aEnd );

    for( size_t ii = 1; ii < points.size(); ++ii )
    {
        auto* wire = new SCH_LINE( points[ii - 1], LAYER_WIRE );
        wire->SetEndPoint( points[ii] );
        wire->SetParent( m_doc.CurrentScreen() );

        view->AddToPreview( wire, true );
    }

    view->MarkDirty();
}


void WIRE_TOOL::finish()
{
    if( m_points.size() < 2 )
    {
        clear();
        return;
    }

    // The wire is on the screen before the commit: its ends tell where junctions are needed
    SCH_SCREEN*                 screen = m_doc.CurrentScreen();
    KIGFX::SCH_VIEW*            view = m_doc.View();
    std::unique_ptr<SCH_COMMIT> commit = m_doc.MakeCommit();
    std::set<VECTOR2I>          ends;

    view->ClearPreview();

    for( size_t ii = 1; ii < m_points.size(); ++ii )
    {
        if( m_points[ii - 1] == m_points[ii] )
            continue;

        auto* wire = new SCH_LINE( m_points[ii - 1], LAYER_WIRE );
        wire->SetEndPoint( m_points[ii] );

        screen->Append( wire );
        view->Add( wire );
        commit->Added( wire, screen );

        ends.insert( m_points[ii - 1] );
        ends.insert( m_points[ii] );
    }

    for( const VECTOR2I& point : ends )
    {
        if( !screen->IsExplicitJunctionNeeded( point ) )
            continue;

        auto* junction = new SCH_JUNCTION( point );

        screen->Append( junction );
        view->Add( junction );
        commit->Added( junction, screen );
    }

    commit->Push( ToWxString( QObject::tr( "Draw Wire" ) ) );
    m_points.clear();
}


void WIRE_TOOL::clear()
{
    m_points.clear();
    m_doc.View()->ClearPreview();
    m_doc.View()->MarkDirty();
}


void LABEL_TOOL::Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    bool          ok = false;
    const QString name = QInputDialog::getText( &m_doc, QObject::tr( "Place Label" ),
                                                QObject::tr( "Net name:" ), QLineEdit::Normal,
                                                QString(), &ok ).trimmed();

    if( !ok || name.isEmpty() )
        return;

    const int size = m_doc.Schematic()->Settings().m_DefaultTextSize;

    auto* label = new SCH_LABEL( aPoint, ToWxString( name ) );
    label->SetTextSize( VECTOR2I( size, size ) );

    std::unique_ptr<SCH_COMMIT> commit = m_doc.MakeCommit();
    commit->Add( label, m_doc.CurrentScreen() );
    commit->Push( ToWxString( QObject::tr( "Place Label" ) ) );
}
