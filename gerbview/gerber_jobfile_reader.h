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

#ifndef GERBER_JOBFILE_READER_H
#define GERBER_JOBFILE_READER_H

#include <string>

#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>

class REPORTER;


/**
 * this class read and parse a Gerber job file to extract useful info
 * for GerbView
 *
 * In a gerber job file, old (deprecated) format, data lines start by
 * %TF.     (usual Gerber X2 info)
 * %TJ.B.   (board info)
 * %TJ.D.   (design info)
 * %TJ.L.   (layers info)
 * some others are not yet handled by Kicad
 * M02*     is the last line

 * In a gerber job file, JSON format, first lines are
 *   {
 *    "Header":
 * and the block ( a JSON array) containing the filename of files to load is
 *    "FilesAttributes":
 *    [
 *      {
 *        "Path":  "interf_u-Composant.gbr",
 *        "FileFunction":  "Copper,L1,Top",
 *        "FilePolarity":  "Positive"
 *      },
 *      {
 *        "Path":  "interf_u-In1.Cu.gbr",
 *        "FileFunction":  "Copper,L2,Inr",
 *        "FilePolarity":  "Positive"
 *      },
 *    ],
 */

class GERBER_JOBFILE_READER
{
public:
    GERBER_JOBFILE_READER( const wxString& aFileName, REPORTER* aReporter )
    {
        m_filename = aFileName;
        m_reporter = aReporter;
    }

    ~GERBER_JOBFILE_READER() {}

    bool ReadGerberJobFile();       /// read a .gbrjob file
    wxArrayString& GetGerberFiles() { return m_GerberFiles; }

private:
    REPORTER* m_reporter;
    wxFileName m_filename;
    wxArrayString m_GerberFiles;    // List of gerber files in job

    // Convert a JSON string, that uses escaped sequence of 4 hexadecimal digits
    // to encode unicode chars when not ASCII7 codes
    // json11 converts this sequence to UTF8 string
    wxString formatStringFromJSON( const std::string& name );
};

#endif // GERBER_JOBFILE_READER_H
