/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Graphics Abstraction Layer (GAL) for OpenGL
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
 * @file opengl_gal_qt.cpp
 * The parts of the OpenGL GAL that use the GUI toolkit, for Qt (see
 * common/gal/opengl/opengl_gal_wx.cpp for the wxWidgets ones).
 */

#include <kicad_gl/kiglu.h> // Must be included first

#include <gal/opengl/opengl_gal.h>

#include <optional>

#include <QCoreApplication>
#include <QWidget>

using namespace KIGFX;


wxString OPENGL_GAL::CheckFeatures( GAL_DISPLAY_OPTIONS& aOptions )
{
    static std::optional<wxString> cached;

    if( cached.has_value() )
        return *cached;

    wxString retVal = wxEmptyString;

    // A small window, not seen by the user, with the canvas of the test
    QWidget* testWindow = new QWidget( nullptr, Qt::Tool | Qt::FramelessWindowHint );
    testWindow->resize( 1, 1 );

    KIGFX::OPENGL_GAL* opengl_gal = nullptr;

    try
    {
        KIGFX::VC_SETTINGS dummy;
        opengl_gal = new KIGFX::OPENGL_GAL( dummy, aOptions, testWindow );

        testWindow->show();

        // Let the window system realize the surface before the context is bound to it
        QCoreApplication::processEvents();

        GAL_CONTEXT_LOCKER lock( opengl_gal );
        opengl_gal->init();
    }
    catch( std::runtime_error& err )
    {
        //Test failed
        retVal = wxString( err.what() );
    }

    delete opengl_gal;
    delete testWindow;

    cached = retVal;
    return retVal;
}
