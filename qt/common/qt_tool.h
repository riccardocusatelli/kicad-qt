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

#include "kiqt.h"     // Qt (Qt::KeyboardModifiers), without its METHOD macro

#include <math/vector2d.h>


/**
 * An interactive tool of a document of the Qt shell (drawing a wire, a track...).
 *
 * While the tool is active, the document gives it the clicks and the motion of the cursor,
 * on the grid, instead of selecting; Escape cancels what the tool is doing, then stops it.
 * The tools are made by the documents of the KIFACEs, with their model.
 */
class QT_TOOL
{
public:
    virtual ~QT_TOOL() = default;

    /// The tool stops: what is in progress is dropped.
    virtual void Stop() = 0;

    /// A click at a point on the grid.
    virtual void Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) = 0;

    /// A double click (after the click of its first press) at a point on the grid.
    virtual void DoubleClick( const VECTOR2I& aPoint ) {}

    /// The cursor moved to a point on the grid.
    virtual void Motion( const VECTOR2I& aPoint ) {}

    /**
     * Escape: drop what is in progress.
     * @return false if nothing was in progress: the tool stops.
     */
    virtual bool Cancel() = 0;
};
