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
 * @file opengl_gal_wx.cpp
 * The parts of the OpenGL GAL that use wxWidgets (the Qt build has its own).
 */

#include <kicad_gl/kiglu.h> // Must be included first

#include <gal/opengl/opengl_gal.h>

#include <optional>

#include <wx/app.h>
#include <wx/frame.h>

using namespace KIGFX;


wxString OPENGL_GAL::CheckFeatures( GAL_DISPLAY_OPTIONS& aOptions )
{
    static std::optional<wxString> cached;

    if( cached.has_value() )
        return *cached;

    wxString retVal = wxEmptyString;

    wxFrame* testFrame = new wxFrame( nullptr, wxID_ANY, wxT( "" ), wxDefaultPosition,
                                      wxSize( 1, 1 ), wxFRAME_TOOL_WINDOW | wxNO_BORDER );

    KIGFX::OPENGL_GAL* opengl_gal = nullptr;

    try
    {
        KIGFX::VC_SETTINGS dummy;
        opengl_gal = new KIGFX::OPENGL_GAL( dummy, aOptions, testFrame );

        testFrame->Raise();
        testFrame->Show();

#ifdef __WXGTK__
        // On GTK, Show() only queues realization. The GDK drawing window
        // needed by SetCurrent() may not exist yet. Yield to let the event
        // loop process the realize signal before we try to lock the context.
        wxYield();
#endif

        GAL_CONTEXT_LOCKER lock( opengl_gal );
        opengl_gal->init();
    }
    catch( std::runtime_error& err )
    {
        //Test failed
        retVal = wxString( err.what() );
    }

    delete opengl_gal;
    delete testFrame;

    cached = retVal;
    return retVal;
}
