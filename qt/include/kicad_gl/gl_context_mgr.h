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
 * @file gl_context_mgr.h
 * GL_CONTEXT_MANAGER of the Qt build: the interface of include/kicad_gl/gl_context_mgr.h, with
 * the OpenGL contexts of Qt.
 */

#ifndef GL_CONTEXT_MANAGER_H
#define GL_CONTEXT_MANAGER_H

#include <kicad_gl/kiglad.h> // Must be included first

#include <map>
#include <mutex>
#include <type_traits>
#include <utility>

#include <kiqt.h>

class QOpenGLContext;
class HIDPI_GL_CANVAS;

/// The OpenGL context and canvas of the GUI toolkit (each GUI build has its own
/// gl_context_mgr.h).
using KIGL_CONTEXT = QOpenGLContext;
using KIGL_CANVAS  = HIDPI_GL_CANVAS;


class KIQT_API GL_CONTEXT_MANAGER
{
public:
    GL_CONTEXT_MANAGER() : m_glCtx( nullptr ) {}

    /**
     * The manager of the OpenGL contexts of the program.
     */
    static GL_CONTEXT_MANAGER* Get();

    /**
     * Create a managed OpenGL context for a canvas.
     *
     * It is assured that the created context is freed upon exit.
     *
     * @param aCanvas is the canvas the context draws in.
     * @param aOther is a context to share the objects (textures, buffers) with, or nullptr.
     * @return Created OpenGL context, or nullptr.
     */
    KIGL_CONTEXT* CreateCtx( KIGL_CANVAS* aCanvas, const KIGL_CONTEXT* aOther = nullptr );

    /**
     * Destroy a managed OpenGL context.
     *
     * The context to be removed has to be created using GL_CONTEXT_MANAGER::CreateCtx() first.
     *
     * @param aContext is the OpenGL context to be destroyed. It will not be managed anymore.
     */
    void DestroyCtx( KIGL_CONTEXT* aContext );

    /**
     * Destroy all managed OpenGL contexts.
     *
     * This method should be called in the final deinitialization routine.
     */
    void DeleteAll();

    /**
     * Set a context as current and prevents other canvases from switching it.
     *
     * Requires calling UnlockCtx() when there are no more GL calls for the context.  If
     * another canvas has already locked a GL context, then the calling process is blocked.
     *
     * @param aContext is the GL context to be bound.
     * @param aCanvas (optional) allows caller to bind the context to a non-parent canvas
     *                (e.g. when a few canvases share a single GL context).
     */
    void LockCtx( KIGL_CONTEXT* aContext, KIGL_CANVAS* aCanvas );

    /**
     * Allow other canvases to bind an OpenGL context.
     *
     * @param aContext is the currently bound context. It is only a check to assure the right
     *                 canvas wants to unlock GL context.
     */
    void UnlockCtx( KIGL_CONTEXT* aContext );

    /**
     * Get the currently bound GL context.
     *
     * @return the currently bound GL context.
     */
    KIGL_CONTEXT* GetCurrentCtx() const
    {
        return m_glCtx;
    }

    /**
     * Get the currently bound GL canvas.
     *
     * @return the currently bound GL canvas.
     */
    KIGL_CANVAS* GetCurrentCanvas() const
    {
        auto it = m_glContexts.find( m_glCtx );
        return it != m_glContexts.end() ? it->second : nullptr;
    }

    /**
     * Run the given function first releasing the GL context lock, then restoring it.
     *
     * @param aFunction is the function to be executed.
     */
    template<typename Func, typename... Args>
    auto RunWithoutCtxLock( Func&& aFunction, Args&&... args )
    {
        KIGL_CONTEXT* currentCtx = GetCurrentCtx();
        KIGL_CANVAS* currentCanvas = GetCurrentCanvas();
        UnlockCtx( currentCtx );

        if constexpr (std::is_void_v<decltype(aFunction(std::forward<Args>(args)...))>)
        {
            std::forward<Func>(aFunction)(std::forward<Args>(args)...);
            LockCtx( currentCtx, currentCanvas );
            return;
        }
        else
        {
            auto result = std::forward<Func>(aFunction)(std::forward<Args>(args)...);
            LockCtx( currentCtx, currentCanvas );
            return result;
        }
    }

private:
    ///< Map of GL contexts & their parent canvases.
    std::map<KIGL_CONTEXT*, KIGL_CANVAS*> m_glContexts;

    ///< Currently bound GL context.
    KIGL_CONTEXT* m_glCtx;

    ///< Lock to prevent unexpected GL context switching.
    std::mutex m_glCtxMutex;
};

#endif /* GL_CONTEXT_MANAGER_H */
