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

#include <optional>

#include <layer_ids.h>
#include <math/vector2d.h>

#include <qt_tool.h>

class BOARD_DOCUMENT;


/**
 * Route a track: each click fixes the segments from the last point, in 45 degree postures,
 * on the active copper layer, with the current track width and the net of the pad, track or
 * via it starts from.  The track ends on a click on a pad, a track end or a via, or on a
 * double click; Escape drops the segments not fixed.
 */
class TRACK_TOOL : public QT_TOOL
{
public:
    explicit TRACK_TOOL( BOARD_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    void Stop() override { clear(); }
    void Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    void DoubleClick( const VECTOR2I& aPoint ) override { clear(); }
    void Motion( const VECTOR2I& aPoint ) override;
    bool Cancel() override;

private:
    /// Show the segments from the last point to @a aEnd.
    void preview( const VECTOR2I& aEnd );

    /// Add the segments from the last point to @a aEnd to the board, as one change.
    void fix( const VECTOR2I& aEnd );

    /// End the track (the segments fixed stay).
    void clear();

    BOARD_DOCUMENT&         m_doc;
    std::optional<VECTOR2I> m_start;            ///< The last point fixed
    PCB_LAYER_ID            m_layer = F_Cu;
    int                     m_netCode = 0;
    int                     m_width = 0;
};


/// Place through vias with the current size and drill, in the net of the track under them.
class VIA_TOOL : public QT_TOOL
{
public:
    explicit VIA_TOOL( BOARD_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    void Stop() override;
    void Click( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers ) override;
    void Motion( const VECTOR2I& aPoint ) override;
    bool Cancel() override { return false; }

private:
    BOARD_DOCUMENT& m_doc;
};
