/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/typeinfo.h, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>

#include <cstring>
#include <typeinfo>

#define WX_DECLARE_TYPEINFO_INLINE( CLS )
#define WX_DECLARE_TYPEINFO( CLS )
#define WX_DEFINE_TYPEINFO( CLS )
#define WX_DECLARE_ABSTRACT_TYPEINFO( CLS )

/// A type identifier compared by name, as wxWidgets does with gcc (the type_info objects
/// of a class may differ between shared libraries).
class wxTypeIdentifier
{
public:
    explicit wxTypeIdentifier( const char* aClassName ) : m_className( aClassName ) {}

    bool operator==( const wxTypeIdentifier& aOther ) const
    {
        return std::strcmp( m_className, aOther.m_className ) == 0;
    }

    bool operator!=( const wxTypeIdentifier& aOther ) const { return !( *this == aOther ); }

private:
    const char* m_className;
};

#define wxTypeId( OBJ ) wxTypeIdentifier( typeid( OBJ ).name() )

#ifndef _WIN32
#pragma GCC visibility pop
#endif
