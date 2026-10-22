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

#include <QPointer>


template <class T>
class wxWeakRef
{
public:
    wxWeakRef( T* aObject = nullptr ) :
            m_object( aObject )
    {
    }

    wxWeakRef& operator=( T* aObject )
    {
        m_object = aObject;
        return *this;
    }

    T* get() const { return m_object.data(); }

    operator T*() const { return get(); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    void Release() { m_object.clear(); }

private:
    QPointer<T> m_object;
};
