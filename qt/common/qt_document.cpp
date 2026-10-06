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

#include "qt_document.h"

#include <algorithm>

#include <math/util.h>

#include "gal_canvas.h"


void QT_DOCUMENT::setupEditing( GAL_CANVAS* aCanvas )
{
    m_editCanvas = aCanvas;

    connect( aCanvas, &GAL_CANVAS::leftPressed, this, &QT_DOCUMENT::onLeftPressed );
    connect( aCanvas, &GAL_CANVAS::leftDragStarted, this, &QT_DOCUMENT::onLeftDragStarted );
    connect( aCanvas, &GAL_CANVAS::leftDragged, this, &QT_DOCUMENT::onLeftDragged );
    connect( aCanvas, &GAL_CANVAS::leftReleased, this, &QT_DOCUMENT::onLeftReleased );
    connect( aCanvas, &GAL_CANVAS::cancelRequested, this, &QT_DOCUMENT::onCancel );
}


bool QT_DOCUMENT::isSelected( EDA_ITEM* aItem ) const
{
    return std::find( m_selection.begin(), m_selection.end(), aItem ) != m_selection.end();
}


void QT_DOCUMENT::setSelection( const std::vector<EDA_ITEM*>& aItems )
{
    for( EDA_ITEM* item : m_selection )
    {
        if( std::find( aItems.begin(), aItems.end(), item ) == aItems.end() )
            setItemSelected( item, false );
    }

    std::vector<EDA_ITEM*> selection;

    for( EDA_ITEM* item : aItems )
    {
        if( std::find( selection.begin(), selection.end(), item ) != selection.end() )
            continue;

        if( !isSelected( item ) )
            setItemSelected( item, true );

        selection.push_back( item );
    }

    m_selection = std::move( selection );

    selectionChanged();

    if( m_editCanvas )
        m_editCanvas->update();

    emit stateChanged();
}


void QT_DOCUMENT::forgetSelected( EDA_ITEM* aItem )
{
    auto it = std::find( m_selection.begin(), m_selection.end(), aItem );

    if( it == m_selection.end() )
        return;

    setItemSelected( aItem, false );
    m_selection.erase( it );

    selectionChanged();
    emit stateChanged();
}


void QT_DOCUMENT::dropSelection()
{
    m_selection.clear();
    m_pressItem = nullptr;
    selectionChanged();
    emit stateChanged();
}


void QT_DOCUMENT::SelectAll()
{
    cancelInteraction();
    setSelection( allItems() );
}


void QT_DOCUMENT::ClearSelection()
{
    cancelInteraction();
    setSelection( {} );
}


void QT_DOCUMENT::DeleteSelection()
{
    cancelInteraction();

    if( m_selection.empty() )
        return;

    std::vector<EDA_ITEM*> items = m_selection;

    // The items leave the selection before they leave the document
    setSelection( {} );
    deleteItems( items );

    emit stateChanged();
}


void QT_DOCUMENT::RotateSelection()
{
    cancelInteraction();

    if( m_selection.empty() )
        return;

    rotateItems( m_selection );
    selectionChanged();     // the properties have changed

    if( m_editCanvas )
        m_editCanvas->update();

    emit stateChanged();
}


void QT_DOCUMENT::Undo()
{
    if( !CanUndo() )
        return;

    cancelInteraction();
    setSelection( {} );
    undo();

    if( m_editCanvas )
        m_editCanvas->update();

    emit stateChanged();
}


void QT_DOCUMENT::Redo()
{
    if( !CanRedo() )
        return;

    cancelInteraction();
    setSelection( {} );
    redo();

    if( m_editCanvas )
        m_editCanvas->update();

    emit stateChanged();
}


void QT_DOCUMENT::cancelInteraction()
{
    switch( m_drag )
    {
    case DRAG::MOVE:
        endMove( false );
        break;

    case DRAG::AREA:
        m_editCanvas->SetSelectionArea( std::nullopt, false );
        break;

    case DRAG::NONE:
        break;
    }

    m_drag = DRAG::NONE;
    m_pressItem = nullptr;
}


BOX2I QT_DOCUMENT::dragArea( const VECTOR2D& aPoint, bool& aCrossing ) const
{
    // As in the editors: dragging to the left selects the items the area touches
    aCrossing = aPoint.x < m_pressPoint.x;

    BOX2D area( m_pressPoint, aPoint - m_pressPoint );
    area.Normalize();

    return BOX2I( VECTOR2I( KiROUND( area.GetX() ), KiROUND( area.GetY() ) ),
                  VECTOR2I( KiROUND( area.GetWidth() ), KiROUND( area.GetHeight() ) ) );
}


void QT_DOCUMENT::onLeftPressed( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers )
{
    cancelInteraction();

    m_pressPoint = aPoint;
    m_pressItem = itemAt( VECTOR2I( KiROUND( aPoint.x ), KiROUND( aPoint.y ) ), aModifiers );
    m_pressSelected = false;

    if( !m_pressItem )
        return;

    if( aModifiers & Qt::ShiftModifier )
    {
        // Shift: add the item to the selection, or remove it
        std::vector<EDA_ITEM*> selection = m_selection;
        auto                   it = std::find( selection.begin(), selection.end(), m_pressItem );

        if( it == selection.end() )
            selection.push_back( m_pressItem );
        else
            selection.erase( it );

        setSelection( selection );
        m_pressSelected = true;
    }
    else if( !isSelected( m_pressItem ) )
    {
        setSelection( { m_pressItem } );
        m_pressSelected = true;
    }

    // A press on a selected item keeps the selection: a drag moves all of it
}


void QT_DOCUMENT::onLeftDragStarted( const VECTOR2D& aStart, Qt::KeyboardModifiers aModifiers )
{
    m_pressPoint = aStart;

    if( m_pressItem && isSelected( m_pressItem ) )
    {
        if( beginMove() )
        {
            m_drag = DRAG::MOVE;
            m_moved = VECTOR2I( 0, 0 );
        }
    }
    else
    {
        m_drag = DRAG::AREA;
    }
}


void QT_DOCUMENT::onLeftDragged( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers )
{
    switch( m_drag )
    {
    case DRAG::MOVE:
    {
        const VECTOR2I moved = snapToGrid( aPoint - m_pressPoint );

        if( moved != m_moved )
        {
            moveBy( moved - m_moved );
            m_moved = moved;
            m_editCanvas->update();
        }

        break;
    }

    case DRAG::AREA:
    {
        bool  crossing = false;
        BOX2I area = dragArea( aPoint, crossing );

        m_editCanvas->SetSelectionArea( BOX2D( area.GetOrigin(), area.GetSize() ), crossing );
        break;
    }

    case DRAG::NONE:
        break;
    }
}


void QT_DOCUMENT::onLeftReleased( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers,
                                  bool aDragged )
{
    const DRAG drag = m_drag;
    m_drag = DRAG::NONE;

    if( drag == DRAG::MOVE )
    {
        endMove( m_moved != VECTOR2I( 0, 0 ) );
        selectionChanged();     // the positions have changed
        emit stateChanged();
    }
    else if( drag == DRAG::AREA )
    {
        bool  crossing = false;
        BOX2I area = dragArea( aPoint, crossing );

        m_editCanvas->SetSelectionArea( std::nullopt, false );

        std::vector<EDA_ITEM*> selection;

        if( aModifiers & Qt::ShiftModifier )
            selection = m_selection;

        for( EDA_ITEM* item : itemsInArea( area, crossing ) )
            selection.push_back( item );

        setSelection( selection );
    }
    else if( !aDragged )
    {
        if( !m_pressItem )
        {
            // A click on an empty place clears the selection
            if( !( aModifiers & Qt::ShiftModifier ) )
                setSelection( {} );
        }
        else if( !m_pressSelected && !( aModifiers & Qt::ShiftModifier ) )
        {
            // A click on an item of the selection selects only it
            setSelection( { m_pressItem } );
        }
    }

    m_pressItem = nullptr;
}


void QT_DOCUMENT::onCancel()
{
    if( m_drag != DRAG::NONE )
        cancelInteraction();
    else
        setSelection( {} );
}
