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
 * @file printing.cpp
 * The printing of the platform with Qt (see libs/kiplatform/os/<platform>/printing.cpp): the
 * printers of Qt print the printouts; the PDF files are not printed by the platform.
 */

#include <printing.h>

#include <wx/kiwx_print.h>


// Exported from kiqt as the other functions of KIPLATFORM (their header has no export macro)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( const std::string& aFile )
    {
        return PRINT_RESULT::UNSUPPORTED;
    }


    void ResetPrintToFilePath( wxPrintData& aData )
    {
        // The printers of Qt don't keep the files of their own spooling in the settings
    }
}
} // namespace KIPLATFORM

#ifndef _WIN32
#pragma GCC visibility pop
#endif
