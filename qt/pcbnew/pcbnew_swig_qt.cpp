/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcbnew_swig_qt.cpp
 * The legacy Python module of pcbnew (the SWIG bindings, KIFACE_SCRIPTING_LEGACY) in the Qt
 * build: the bindings of pcbnew are generated with the wxWidgets types (wx_python_helpers), they
 * are not built with Qt yet.  Importing the module fails with ImportError; the IPC API of KiCad
 * is the scripting interface of the Qt build.
 */

#include <Python.h>


extern "C" PyObject* PyInit__pcbnew( void )
{
    PyErr_SetString( PyExc_ImportError,
                     "the pcbnew module of the SWIG bindings is not built with Qt" );
    return nullptr;
}
