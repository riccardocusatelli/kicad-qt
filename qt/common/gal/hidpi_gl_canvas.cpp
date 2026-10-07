/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Bernhard Stegmaier <stegmaier@sw-systems.de>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Base class for HiDPI aware wxGLCanvas implementations.
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
 * @file hidpi_gl_canvas.cpp
 * HIDPI_GL_CANVAS of the Qt build (see common/gal/hidpi_gl_canvas.cpp for the wxWidgets one).
 */

#include <kicad_gl/kiglad.h> // Must be included first

#include <gal/hidpi_gl_canvas.h>

#include <QApplication>
#include <QExposeEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QWindow>

#include <gal/cursors.h>
#include <qt_strings.h>


HIDPI_GL_CANVAS::HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, TOOLKIT_WINDOW* aParent,
                                  const wxString& aName ) :
        QWidget( aParent ),
        m_settings( aSettings ),
        m_window( new QWindow() ),
        m_container( nullptr )
{
    setObjectName( ToQString( aName ) );

    // The native OpenGL surface, created now: the contexts can be bound to it before it is
    // shown (as a wxGLCanvas)
    m_window->setSurfaceType( QSurface::OpenGLSurface );
    m_window->setFormat( SurfaceFormat() );
    m_window->create();
    m_window->installEventFilter( this );

    m_container = QWidget::createWindowContainer( m_window, this );
    m_container->setFocusPolicy( Qt::NoFocus );
    m_container->setMouseTracking( true );

    setAttribute( Qt::WA_NativeWindow, false );

    if( aParent )
        ResizeCanvas( aParent->width(), aParent->height() );
}


HIDPI_GL_CANVAS::~HIDPI_GL_CANVAS()
{
    // The window goes with its container
    m_window->removeEventFilter( this );
}


QSurfaceFormat HIDPI_GL_CANVAS::SurfaceFormat()
{
    QSurfaceFormat format;
    format.setRenderableType( QSurfaceFormat::OpenGL );
    format.setProfile( QSurfaceFormat::CompatibilityProfile );
    format.setSwapBehavior( QSurfaceFormat::DoubleBuffer );
    format.setRedBufferSize( 8 );
    format.setGreenBufferSize( 8 );
    format.setBlueBufferSize( 8 );
    format.setAlphaBufferSize( 8 );
    format.setDepthBufferSize( 8 );
    format.setStencilBufferSize( 8 );
    format.setSwapInterval( 1 );
    return format;
}


wxSize HIDPI_GL_CANVAS::GetNativePixelSize() const
{
    const double scaleFactor = GetScaleFactor();

    return wxSize( width() * scaleFactor, height() * scaleFactor );
}


wxPoint HIDPI_GL_CANVAS::GetNativePosition( const wxPoint& aPoint ) const
{
    const double scaleFactor = GetScaleFactor();

    return wxPoint( aPoint.x * scaleFactor, aPoint.y * scaleFactor );
}


double HIDPI_GL_CANVAS::GetScaleFactor() const
{
    return m_window->devicePixelRatio();
}


void HIDPI_GL_CANVAS::PostPaint()
{
    // The panel draws the canvas from its paint event
    if( m_paintListener )
        m_paintListener->update();
}


void HIDPI_GL_CANVAS::SetCanvasCursor( KICURSOR aCursor, bool aHiDPI )
{
    m_window->setCursor( CURSOR_STORE::GetCursor( aCursor, aHiDPI ) );
}


void HIDPI_GL_CANVAS::ResizeCanvas( int aWidth, int aHeight )
{
    resize( aWidth, aHeight );
    m_container->setGeometry( 0, 0, aWidth, aHeight );
}


bool HIDPI_GL_CANVAS::ShowCanvas( bool aShow )
{
    bool changed = isVisible() != aShow;

    setVisible( aShow );

    if( aShow )
        raise();

    return changed;
}


bool HIDPI_GL_CANVAS::IsCanvasVisible() const
{
    return isVisible() && m_window->isExposed() && width() > 0 && height() > 0;
}


int HIDPI_GL_CANVAS::SetupSwapInterval()
{
    return m_window->format().swapInterval();
}


void HIDPI_GL_CANVAS::SwapBuffers()
{
    if( QOpenGLContext* context = QOpenGLContext::currentContext() )
        context->swapBuffers( m_window );
}


void HIDPI_GL_CANVAS::Refresh()
{
    PostPaint();
}


bool HIDPI_GL_CANVAS::eventFilter( QObject* aWatched, QEvent* aEvent )
{
    if( aWatched != m_window )
        return QWidget::eventFilter( aWatched, aEvent );

    switch( aEvent->type() )
    {
    case QEvent::Expose:
        // The window shows again: the panel draws it
        if( m_window->isExposed() )
            PostPaint();

        return false;

    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::NativeGesture:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
    case QEvent::Enter:
    case QEvent::Leave:
        // The positions of the window are the ones of the canvas, and of its panel
        if( m_mouseListener )
        {
            if( aEvent->type() == QEvent::MouseButtonPress )
                m_mouseListener->setFocus( Qt::MouseFocusReason );

            QCoreApplication::sendEvent( m_mouseListener, aEvent );
            return true;
        }

        return false;

    default:
        return false;
    }
}
