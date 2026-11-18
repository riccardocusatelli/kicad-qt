/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <layer_ids.h>
#include <view/view.h>
#include <board_item.h>

class BOARD;
class COLOR_SETTINGS;
class PCB_DISPLAY_OPTIONS;
class PROGRESS_REPORTER;

namespace KIGFX {

class PCB_VIEW : public VIEW
{
public:
    PCB_VIEW();
    virtual ~PCB_VIEW();

    /// @copydoc VIEW::Add()
    virtual void Add( VIEW_ITEM* aItem, int aDrawPriority = -1 ) override;

    /// @copydoc VIEW::Remove()
    virtual void Remove( VIEW_ITEM* aItem ) override;

    /// @copydoc VIEW::Update()
    virtual void Update( const VIEW_ITEM* aItem, int aUpdateFlags ) const override;

    virtual void Update( const VIEW_ITEM* aItem ) const override;

    /**
     * Sets the KIGFX::REPAINT on all items matching \a aTypes which intersect \a aStaleAreas.
     */
    void UpdateCollidingItems( const std::vector<BOX2I>& aStaleAreas,
                               std::initializer_list<KICAD_T> aTypes );

    void UpdateDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions );

    /// Set the default drawing order of the layers.
    void SetDefaultLayerOrder();

    /**
     * Set the default layer dependencies and render targets.
     *
     * @param aCached is true for a renderer that caches the items (OpenGL); a software
     *                renderer (Cairo) draws them each time.
     */
    void SetDefaultLayerDeps( bool aCached );

    /// Bring a layer, and the layers drawn with it, on top of the others.
    void SetActiveLayerOnTop( PCB_LAYER_ID aLayer );

    /// Set the layer shown in high contrast mode (and bring it on top).
    void SetHighContrastLayer( PCB_LAYER_ID aLayer );

    /// Show the layers and the elements visible in the board.
    void SyncLayersVisibility( const BOARD* aBoard );

    /// Show the items of a board (replacing the current ones).
    void ShowBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );

    /// Use the colors of a color theme for the items, the grid and the cursor.
    void LoadColors( const COLOR_SETTINGS* aSettings );
};

}
