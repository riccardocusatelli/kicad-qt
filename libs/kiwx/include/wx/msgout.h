/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/msgout.h, wxWindows Library Licence 3.1),
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
#pragma GCC visibility push( default )

#include <cstdio>

#include <wx/string.h>

/// Where the messages that are not log messages go (the usage of a program, for instance).
class wxMessageOutput
{
public:
    virtual ~wxMessageOutput() {}

    /// The current output; without one set, the standard error (a console program).
    static wxMessageOutput* Get();

    /// Sets the output, returns the previous one.
    static wxMessageOutput* Set( wxMessageOutput* aOutput );

    virtual void Output( const wxString& aStr ) = 0;

    template <typename... ARGS>
    void Printf( const wxString& aFormat, const ARGS&... aArgs )
    {
        Output( wxString::Format( aFormat, aArgs... ) );
    }

private:
    static wxMessageOutput* ms_msgOut;
};


/// Writes the messages to a stream, each one ending with a newline.
class wxMessageOutputStderr : public wxMessageOutput
{
public:
    explicit wxMessageOutputStderr( FILE* aStream = stderr ) : m_fp( aStream ) {}

    void Output( const wxString& aStr ) override;

protected:
    FILE* m_fp;
};


enum wxMessageOutputFlags
{
    wxMSGOUT_PREFER_STDERR = 0,
    wxMSGOUT_PREFER_MSGBOX = 1
};


/// Without user interface, the standard error.
class wxMessageOutputBest : public wxMessageOutputStderr
{
public:
    explicit wxMessageOutputBest( wxMessageOutputFlags = wxMSGOUT_PREFER_STDERR ) {}
};

#pragma GCC visibility pop
