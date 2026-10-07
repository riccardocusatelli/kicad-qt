/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file gl_utils.cpp
 * GL_UTILS of the Qt build (see common/kicad_gl/gl_utils.cpp for the wxWidgets one).
 */

#include <kicad_gl/kiglad.h> // Must be included first

#include <kicad_gl/gl_utils.h>

#include <QOpenGLContext>
#include <QWindow>

#include <gal/hidpi_gl_canvas.h>


wxString GL_UTILS::DetectGLBackend( HIDPI_GL_CANVAS* aCanvas )
{
    QOpenGLContext* context = QOpenGLContext::currentContext();

    if( !context )
        return wxEmptyString;

#if defined( __unix__ ) && !defined( __APPLE__ )
    if( context->nativeInterface<QNativeInterface::QGLXContext>() )
        return wxS( "GLX" );

    if( context->nativeInterface<QNativeInterface::QEGLContext>() )
        return wxS( "EGL" );
#endif

    return wxEmptyString;
}


int GL_UTILS::SetSwapInterval( HIDPI_GL_CANVAS* aCanvas, int aVal )
{
    return aCanvas->GetSurface()->format().swapInterval();
}
