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

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/crt.h>   // as wxWidgets, through wx/hashmap.h
#include <wx/math.h>
#include <wx/arrstr.h>
#include <wx/filefn.h>
#include <wx/string.h>

#include <map>

class wxProcess;

typedef std::map<wxString, wxString> wxEnvVariableHashMap;

struct wxExecuteEnv
{
    wxString             cwd;
    wxEnvVariableHashMap env;
};

enum
{
    wxEXEC_ASYNC = 0,
    wxEXEC_SYNC = 1,
    wxEXEC_SHOW_CONSOLE = 2,
    wxEXEC_MAKE_GROUP_LEADER = 4,
    wxEXEC_NODISABLE = 8,
    wxEXEC_NOEVENTS = 16,
    wxEXEC_HIDE_CONSOLE = 32,
    wxEXEC_BLOCK = wxEXEC_SYNC | wxEXEC_NOEVENTS
};

bool     wxGetEnv( const wxString& aVar, wxString* aValue );
bool     wxSetEnv( const wxString& aVar, const wxString& aValue );
bool     wxUnsetEnv( const wxString& aVar );
bool     wxGetEnvMap( wxEnvVariableHashMap* aMap );
wxString wxExpandEnvVars( const wxString& aStr );

wxString wxGetHomeDir();
wxString wxGetUserHome( const wxString& aUser = wxString() );
wxString wxGetUserId();
bool     wxGetUserId( wxChar* aBuf, int aSize );
wxString wxGetUserName();
wxString wxGetHostName();
wxString wxGetFullHostName();
unsigned long wxGetProcessId();
wxString wxGetOsDescription();
int      wxGetOsVersion( int* aMajor = nullptr, int* aMinor = nullptr, int* aMicro = nullptr );
wxString wxGetCpuArchitectureName();
wxString wxGetNativeCpuArchitectureName();
bool     wxIsPlatform64Bit();

struct wxLinuxDistributionInfo
{
    wxString Id;
    wxString Release;
    wxString CodeName;
    wxString Description;

    bool operator==( const wxLinuxDistributionInfo& o ) const
    {
        return Id == o.Id && Release == o.Release && CodeName == o.CodeName && Description == o.Description;
    }
};

wxLinuxDistributionInfo wxGetLinuxDistributionInfo();

void wxMilliSleep( unsigned long aMs );
void wxMicroSleep( unsigned long aUs );
void wxSleep( int aSeconds );

/// Run a command; with wxEXEC_SYNC @return its exit code, else the process id (0 on failure).
long wxExecute( const wxString& aCommand, int aFlags = wxEXEC_ASYNC, wxProcess* aProcess = nullptr,
                const wxExecuteEnv* aEnv = nullptr );
long wxExecute( const wxString& aCommand, wxArrayString& aOutput, int aFlags = 0,
                const wxExecuteEnv* aEnv = nullptr );
long wxExecute( const wxString& aCommand, wxArrayString& aOutput, wxArrayString& aErrors,
                int aFlags = 0, const wxExecuteEnv* aEnv = nullptr );
long wxExecute( const char* const* aArgv, int aFlags = wxEXEC_ASYNC, wxProcess* aProcess = nullptr,
                const wxExecuteEnv* aEnv = nullptr );
long wxExecute( const wchar_t* const* aArgv, int aFlags = wxEXEC_ASYNC, wxProcess* aProcess = nullptr,
                const wxExecuteEnv* aEnv = nullptr );
bool wxShell( const wxString& aCommand = wxString() );

/// Yielding to the GUI: nothing to do without a GUI.
inline bool wxYield() { return true; }
inline bool wxSafeYield( void* = nullptr, bool = false ) { return true; }


wxString wxNow();

#pragma GCC visibility pop
