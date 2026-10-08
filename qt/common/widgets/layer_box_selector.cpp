/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file layer_box_selector.cpp
 * LAYER_BOX_SELECTOR of the Qt build (see common/widgets/layer_box_selector.cpp).
 */

#include <widgets/layer_box_selector.h>

#include <layer_ids.h>
#include <qt_strings.h>
#include <ui_hooks.h>


LAYER_SELECTOR::LAYER_SELECTOR()
{
    m_layerhotkeys = true;
}


bool LAYER_SELECTOR::SetLayersHotkeys( bool value )
{
    m_layerhotkeys = value;
    return m_layerhotkeys;
}


LAYER_BOX_SELECTOR::LAYER_BOX_SELECTOR( QWidget* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, int n, const wxString choices[] ) :
        QComboBox( parent ),
        LAYER_SELECTOR()
{
    // A read only list (wxCB_READONLY)
    setEditable( false );
    setSizeAdjustPolicy( QComboBox::AdjustToContents );

    if( id != wxID_ANY )
        KIQT::SetWindowId( this, id );

    if( size.x > 0 && size.y > 0 )
        resize( size.x, size.y );

    for( int ii = 0; ii < n && choices; ++ii )
        addItem( ToQString( choices[ii] ) );
}


LAYER_BOX_SELECTOR::~LAYER_BOX_SELECTOR()
{
}


int LAYER_BOX_SELECTOR::GetLayerSelection() const
{
    if( currentIndex() < 0 )
        return UNDEFINED_LAYER;

    return currentData().toInt();
}


int LAYER_BOX_SELECTOR::SetLayerSelection( int layer )
{
    int i = findData( layer );

    // Element (i) is not selected (or not found: no selection)
    if( currentIndex() != i )
        setCurrentIndex( i );

    return i;
}
