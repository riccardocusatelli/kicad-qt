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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file weakref.h
 * The weak references of the wxWidgets compatibility layer (wxWeakRef): a QPointer, that Qt
 * clears when the object (a QObject, as the windows of the layer) is destroyed.
 */

#pragma once

#include <QObject>
#include <QPointer>

#include <type_traits>


/**
 * A weak reference to an object: to a QObject, cleared by Qt when it is destroyed; to a window of
 * the layer (wxWindow, which is not a QObject), cleared when its widget is destroyed; to any other
 * object, a plain pointer.
 */
template <class T>
class wxWeakRef
{
public:
    wxWeakRef( T* aObject = nullptr ) { assign( aObject ); }

    wxWeakRef& operator=( T* aObject )
    {
        assign( aObject );
        return *this;
    }

    T* get() const
    {
        if( m_tracked )
            return m_guard ? m_object : nullptr;

        return m_object;
    }

    operator T*() const { return get(); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    /// The reference as the argument of a function that takes the object (the parent window of
    /// a dialog, a wxWidgetArgCompat)
    template <typename U, typename = std::enable_if_t<!std::is_pointer_v<U> && !std::is_same_v<U, bool>
                                                      && std::is_constructible_v<U, T*>>>
    operator U() const
    {
        return U( get() );
    }

    void Release() { assign( nullptr ); }

private:
    void assign( T* aObject )
    {
        m_object = aObject;
        m_guard = nullptr;
        m_tracked = false;

        if( !aObject )
            return;

        if constexpr( std::is_base_of_v<QObject, T> )
        {
            m_guard = aObject;
            m_tracked = true;
        }
        else if constexpr( requires( T* aT ) { aT->GetHandle(); } )
        {
            if( auto* widget = aObject->GetHandle() )
            {
                m_guard = widget;
                m_tracked = true;
            }
        }
    }

    T*               m_object = nullptr;
    QPointer<QObject> m_guard;
    bool             m_tracked = false;
};
