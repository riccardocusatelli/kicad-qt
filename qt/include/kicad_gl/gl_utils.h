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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


/**
 * @file gl_utils.h
 * GL_UTILS of the Qt build: the interface of include/kicad_gl/gl_utils.h, for the OpenGL
 * canvas of Qt.
 */

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <wx/string.h>

#include <kiqt.h>

class HIDPI_GL_CANVAS;

class KIQT_API GL_UTILS
{
public:
    /// The OpenGL interface of the window system ("GLX", "EGL"...), for the information about
    /// the graphics.
    static wxString DetectGLBackend( HIDPI_GL_CANVAS* aCanvas );

    /**
     * The OpenGL swap interval of a canvas (the vertical synchronization is set in the format
     * of its surface).
     *
     * @param aCanvas the canvas for which to set the swap interval
     * @param aVal if -1 = try to set adaptive swapping, 0 = sync off, 1 = sync with VSYNC rate.
     * @return actual value set
     */
    static int SetSwapInterval( HIDPI_GL_CANVAS* aCanvas, int aVal );
};

#endif /* GL_UTILS_H */
