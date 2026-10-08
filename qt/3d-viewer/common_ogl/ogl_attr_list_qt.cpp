/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file ogl_attr_list_qt.cpp
 * The attributes of the OpenGL surface of the 3D canvases, for Qt (QSurfaceFormat); see
 * 3d-viewer/common_ogl/ogl_attr_list_wx.cpp for the wxWidgets ones.
 */

#include "common_ogl/ogl_attr_list.h"

#include <QSurfaceFormat>

#include <core/ki_assert.h>


KIGL_ATTRIBUTES OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE aAntiAliasingMode,
                                                 bool              aAlpha )
{
    KI_ASSERT( aAntiAliasingMode <= ANTIALIASING_MODE::AA_8X );

    static const int aaSamples[4] = { 0, 2, 4, 8 };

    QSurfaceFormat format;
    format.setRenderableType( QSurfaceFormat::OpenGL );
    format.setProfile( QSurfaceFormat::CompatibilityProfile );
    format.setSwapBehavior( QSurfaceFormat::DoubleBuffer );
    format.setRedBufferSize( 8 );
    format.setGreenBufferSize( 8 );
    format.setBlueBufferSize( 8 );
    format.setAlphaBufferSize( aAlpha ? 8 : 0 );
    format.setDepthBufferSize( 16 );
    format.setStencilBufferSize( 8 );

    // The platform takes the nearest number of samples it supports
    format.setSamples( aaSamples[static_cast<int>( aAntiAliasingMode )] );
    format.setSwapInterval( 1 );

    return format;
}
