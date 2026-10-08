/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file layer_box_selector.h
 * LAYER_BOX_SELECTOR of the Qt build: the list of the layers, with their color swatches, in a
 * combo box (see include/widgets/layer_box_selector.h).
 */

#ifndef LAYER_BOX_SELECTOR_H
#define LAYER_BOX_SELECTOR_H

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QComboBox>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>


/**
 * Base class to build a layer list.
 */
class LAYER_SELECTOR
{
public:
    LAYER_SELECTOR();

    virtual ~LAYER_SELECTOR() { }

    bool SetLayersHotkeys( bool value );

protected:
    /// Return true if the layer id is enabled (i.e. is it should be displayed).
    virtual bool isLayerEnabled( int aLayer ) const = 0;

    bool m_layerhotkeys;
};


/**
 * Display a layer list in a combo box: each item has the color swatch of its layer, and the
 * layer as data.
 */
class LAYER_BOX_SELECTOR : public QComboBox, public LAYER_SELECTOR
{
public:
    LAYER_BOX_SELECTOR( QWidget* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize, int n = 0,
                        const wxString choices[] = nullptr );

    ~LAYER_BOX_SELECTOR() override;

    int GetLayerSelection() const;

    int SetLayerSelection( int layer );

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;
};

#endif // LAYER_BOX_SELECTOR_H
