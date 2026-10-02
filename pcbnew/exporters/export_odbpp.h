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

#ifndef EXPORT_ODBPP_H
#define EXPORT_ODBPP_H

#include <functional>

#include <wx/string.h>

class BOARD;
class JOB_EXPORT_PCB_ODB;
class PROGRESS_REPORTER;
class REPORTER;


/**
 * Write @a aBoard as ODB++ (directory, zip or tgz archive) as specified by @a aJob.
 *
 * Errors go to @a aReporter.  When the output already exists, @a aConfirmOverwrite (if set) is
 * asked whether to replace it; returning false cancels the export.  Without it the export
 * writes over the existing output (kicad-cli behavior).
 */
void ExportODBPP( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                  PROGRESS_REPORTER* aProgressReporter = nullptr, REPORTER* aReporter = nullptr,
                  const std::function<bool( const wxString& aQuestion )>& aConfirmOverwrite = nullptr );

#endif // EXPORT_ODBPP_H
