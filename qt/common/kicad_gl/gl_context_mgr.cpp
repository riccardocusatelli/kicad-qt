/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @file gl_context_mgr.cpp
 * GL_CONTEXT_MANAGER of the Qt build (see common/kicad_gl/gl_context_mgr.cpp for the wxWidgets
 * one).
 */

#include <kicad_gl/gl_context_mgr.h>

#include <QOpenGLContext>
#include <QWindow>

#include <gal/hidpi_gl_canvas.h>
#include <core/ki_assert.h>


GL_CONTEXT_MANAGER* GL_CONTEXT_MANAGER::Get()
{
    static GL_CONTEXT_MANAGER manager;
    return &manager;
}


KIGL_CONTEXT* GL_CONTEXT_MANAGER::CreateCtx( KIGL_CANVAS* aCanvas, const KIGL_CONTEXT* aOther )
{
    QOpenGLContext* context = new QOpenGLContext();
    context->setFormat( HIDPI_GL_CANVAS::SurfaceFormat() );

    if( aOther )
        context->setShareContext( const_cast<QOpenGLContext*>( aOther ) );

    if( !context->create() )
    {
        delete context;
        return nullptr;
    }

    m_glContexts.insert( std::make_pair( context, aCanvas ) );

    return context;
}


void GL_CONTEXT_MANAGER::DestroyCtx( KIGL_CONTEXT* aContext )
{
    if( m_glContexts.count( aContext ) )
    {
        m_glContexts.erase( aContext );
        delete aContext;
    }
    else
    {
        // Do not delete unknown GL contexts
        KI_FAIL();
    }

    if( m_glCtx == aContext )
        m_glCtx = nullptr;
}


void GL_CONTEXT_MANAGER::DeleteAll()
{
    m_glCtxMutex.lock();

    for( auto& ctx : m_glContexts )
        delete ctx.first;

    m_glContexts.clear();
    m_glCtx = nullptr;
    m_glCtxMutex.unlock();
}


void GL_CONTEXT_MANAGER::LockCtx( KIGL_CONTEXT* aContext, KIGL_CANVAS* aCanvas )
{
    KI_CHECK( aContext && m_glContexts.count( aContext ) > 0, /* void */ );

    m_glCtxMutex.lock();
    KIGL_CANVAS* canvas = aCanvas ? aCanvas : m_glContexts.at( aContext );

    if( canvas && canvas->GetSurface() )
        aContext->makeCurrent( canvas->GetSurface() );

    m_glCtx = aContext;
}


void GL_CONTEXT_MANAGER::UnlockCtx( KIGL_CONTEXT* aContext )
{
    KI_CHECK( aContext && m_glContexts.count( aContext ) > 0, /* void */ );

    if( m_glCtx == aContext )
    {
        m_glCtxMutex.unlock();
        m_glCtx = nullptr;
    }
    else
    {
        KI_FAIL_MSG( wxString::Format( wxS( "Trying to unlock GL context mutex from "
                                            "a wrong context: aContext %p m_glCtx %p" ),
                                       aContext, m_glCtx ) );
    }
}
