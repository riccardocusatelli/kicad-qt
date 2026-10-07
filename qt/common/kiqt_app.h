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

#pragma once

/**
 * @file kiqt_app.h
 *
 * The application object of the Qt programs of KiCad.
 */

#include <wx/app.h>     // kiwx


/**
 * The application object (wxAppConsole of kiwx) of a program whose user interface is Qt: the
 * code of KiCad asks it whether there is a user interface (PGM_BASE::IsGUI()), as it asks the
 * wxApp of the wxWidgets build.
 *
 * It is created before the wxInitializer of the program, which then uses it.
 */
class KIQT_APP : public wxAppConsole
{
public:
    bool IsGUI() const override { return true; }
};
