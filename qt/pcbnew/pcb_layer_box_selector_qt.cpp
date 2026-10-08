/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * @file pcb_layer_box_selector_qt.cpp
 * The list of PCB_LAYER_BOX_SELECTOR with Qt (see pcbnew/pcb_layer_box_selector_wx.cpp).
 */

#include "pcb_layer_box_selector.h"

#include <QIcon>
#include <QPixmap>
#include <QSignalBlocker>

#include <board.h>
#include <layer_ids.h>
#include <pcb_layer_presentation.h>
#include <qt_strings.h>


// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    // The list is rebuilt: not a selection of the user
    QSignalBlocker blocker( this );

    clear();

    const int size = 14;

    LSET show = ( LSET::AllCuMask() | LSET::AllNonCuMask() ) & ~m_layerMaskDisable;
    LSET activated = getEnabledLayers() & ~m_layerMaskDisable;

    for( PCB_LAYER_ID layerid : show.UIOrder() )
    {
        if( !m_showNotEnabledBrdlayers && !activated[layerid] )
            continue;

        QIcon swatch;

        for( int scale = 1; scale <= 3; scale++ )
        {
            QPixmap bmp( size * scale, size * scale );

            m_layerPresentation->DrawColorSwatch( bmp, layerid );

            bmp.setDevicePixelRatio( scale );
            swatch.addPixmap( bmp );
        }

        addItem( swatch, ToQString( layerItemName( layerid, activated[layerid] ) ),
                 (int) layerid );
    }

    if( !m_undefinedLayerName.IsEmpty() )
        addItem( ToQString( m_undefinedLayerName ), (int) UNDEFINED_LAYER );

    setIconSize( QSize( size, size ) );

    // The list starts without a selection
    setCurrentIndex( -1 );
}
