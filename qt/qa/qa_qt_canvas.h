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
 * @file qa_qt_canvas.h
 * The QA of the canvas of an editor (the schematic editor, the PCB editor): its GAL has the size
 * of the canvas, the cross of the cursor is under the mouse, and the OpenGL window is over the
 * canvas and drawn whole (the bugs of the screens scaled to 125% on Windows). Included after the
 * headers of KiCad (kiglad.h first) and qa_qt_frames.h.
 */

#pragma once

#include <cmath>
#include <cstdio>
#include <string>

#include <QImage>
#include <QMargins>
#include <QPoint>
#include <QWindow>

#include <class_draw_panel_gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>


namespace QA_QT_FRAMES
{

/// The geometry of the canvas of @a aName (a shown editor).
inline void checkCanvasGeometry( EDA_DRAW_PANEL_GAL* aCanvas, const std::string& aName )
{
    KIGFX::GAL* gal = aCanvas ? aCanvas->GetGAL() : nullptr;

    check( gal != nullptr, ( "the canvas of the " + aName + " has its GAL" ).c_str() );

    if( !gal )
        return;

    const bool     opengl = aCanvas->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    const VECTOR2I screen = gal->GetScreenPixelSize();
    const VECTOR2I client = aCanvas->GetClientSizeDevice();

    std::printf( "  %s canvas: %s GAL, %dx%d at ratio %g, GAL %dx%d\n", aName.c_str(),
                 opengl ? "OpenGL" : "Cairo", aCanvas->width(), aCanvas->height(),
                 aCanvas->devicePixelRatioF(), screen.x, screen.y );

    check( screen == client, ( "the GAL of the " + aName + " has the size of its canvas" ).c_str() );

    // The mouse at a third of the canvas is at a third of the screen of the GAL: the cross of
    // the cursor is drawn under the mouse (at 125% it was at 1.25 times its distance)
    if( screen.x > 0 && screen.y > 0 )
    {
        const QPointF  at( aCanvas->width() / 3.0, aCanvas->height() / 3.0 );
        const VECTOR2D device = aCanvas->ToDevice( at );
        const double   fx = device.x / screen.x;
        const double   fy = device.y / screen.y;

        std::printf( "  %s: the mouse at 0.333, 0.333 of the canvas is at %.3f, %.3f of the GAL\n",
                     aName.c_str(), fx, fy );
        check( std::abs( fx - 1.0 / 3 ) < 0.01 && std::abs( fy - 1.0 / 3 ) < 0.01,
               ( "the cursor of the " + aName + " is under the mouse" ).c_str() );
    }

    if( !opengl )
        return;

    // The OpenGL window is where the canvas is, without the frame of a top level window (on
    // Windows it was 8 pixels up and left, with a black band at the top and at the right)
    QWindow* glWindow = static_cast<KIGFX::OPENGL_GAL*>( gal )->GetSurface();

    check( glWindow != nullptr, ( "the " + aName + " has its OpenGL window" ).c_str() );

    if( !glWindow )
        return;

    const QPoint   glOrigin = glWindow->mapToGlobal( QPoint( 0, 0 ) );
    const QPoint   canvasOrigin = aCanvas->mapToGlobal( QPoint( 0, 0 ) );
    const QMargins margins = glWindow->frameMargins();

    std::printf( "  %s: OpenGL window at %d,%d, %dx%d, frame margins %d %d %d %d; canvas at %d,%d\n",
                 aName.c_str(), glOrigin.x(), glOrigin.y(), glWindow->width(), glWindow->height(),
                 margins.left(), margins.top(), margins.right(), margins.bottom(),
                 canvasOrigin.x(), canvasOrigin.y() );

    check( glOrigin == canvasOrigin,
           ( "the OpenGL window of the " + aName + " is over its canvas" ).c_str() );
    check( glWindow->size() == aCanvas->size(),
           ( "the OpenGL window of the " + aName + " has the size of its canvas" ).c_str() );
#ifdef _WIN32
    // (xcb reports for a child window the margins of the frame of its top level window)
    check( margins.isNull(), ( "the OpenGL window of the " + aName + " has no frame" ).c_str() );
#endif

    // The buffers have the size of the native window (or twice it, with the supersampling
    // antialiasing): a smaller one leaves a black line at the top and at the right
    QImage image;

    if( aCanvas->GetScreenshot( image ) )
    {
        const QSize native = aCanvas->size() * glWindow->devicePixelRatio();

        std::printf( "  %s: native window %dx%d, drawing %dx%d\n", aName.c_str(), native.width(),
                     native.height(), image.width(), image.height() );
        check( image.size() == native || image.size() == native * 2,
               ( "the drawing of the " + aName + " covers its window" ).c_str() );
    }
}

} // namespace QA_QT_FRAMES
