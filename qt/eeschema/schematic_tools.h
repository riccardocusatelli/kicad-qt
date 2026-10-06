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

#include <vector>

#include <math/vector2d.h>

#include <qt_tool.h>

class SCHEMATIC_DOCUMENT;


/**
 * Draw a wire: each click fixes a point, the segments to it go horizontally then vertically.
 * The wire ends on a click on a pin, a wire end or a label (a terminal point), or on a double
 * click; the junctions it needs are added with it.  Escape drops the wire being drawn.
 */
class WIRE_TOOL : public QT_TOOL
{
public:
    explicit WIRE_TOOL( SCHEMATIC_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    void Stop() override { clear(); }
    void Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    void DoubleClick( const VECTOR2I& aPoint ) override { finish(); }
    void Motion( const VECTOR2I& aPoint ) override;
    bool Cancel() override;

private:
    /// The points of the wire, with the ones to @a aEnd.
    std::vector<VECTOR2I> pointsTo( const VECTOR2I& aEnd ) const;

    void preview( const VECTOR2I& aEnd );

    /// Add the wire fixed so far to the screen, as one change.
    void finish();

    void clear();

    SCHEMATIC_DOCUMENT&   m_doc;
    std::vector<VECTOR2I> m_points;     ///< The points fixed
};


/// Place local labels: a click asks for the net name.
class LABEL_TOOL : public QT_TOOL
{
public:
    explicit LABEL_TOOL( SCHEMATIC_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    void Stop() override {}
    void Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    bool Cancel() override { return false; }

private:
    SCHEMATIC_DOCUMENT& m_doc;
};
