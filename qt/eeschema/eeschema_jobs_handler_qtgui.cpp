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

// The jobs handler of eeschema with its GUI parts: the diff viewer (OpenDiffDialog,
// AttachDiffCanvases) and the interactive 3-way merge.
//
// eeschema_jobs_handler.cpp is compiled twice.  The headless KIFACE (kicad-cli) builds it without those
// parts, which report that they are not available.  The KIFACE with the Qt GUI builds this
// copy, with KICAD_JOBS_GUI: its objects define every symbol of the headless object, so the
// headless one, in an archive that the KIFACE links after them, is never pulled in.

#define KICAD_JOBS_GUI

#include <wx/app.h>     // wxTheApp (the parent of the dialogs)

#include "../../eeschema/eeschema_jobs_handler.cpp"
