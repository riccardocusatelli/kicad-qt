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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/mstream.h>
#include <wx/utils.h>

#include <memory>
#include <string>

enum wxSignal
{
    wxSIGNONE = 0,
    wxSIGHUP,
    wxSIGINT,
    wxSIGQUIT,
    wxSIGILL,
    wxSIGTRAP,
    wxSIGABRT,
    wxSIGEMT,
    wxSIGFPE,
    wxSIGKILL,
    wxSIGBUS,
    wxSIGSEGV,
    wxSIGSYS,
    wxSIGPIPE,
    wxSIGALRM,
    wxSIGTERM
};

enum wxKillError
{
    wxKILL_OK,
    wxKILL_BAD_SIGNAL,
    wxKILL_ACCESS_DENIED,
    wxKILL_NO_PROCESS,
    wxKILL_ERROR
};

enum wxKillFlags
{
    wxKILL_NOCHILDREN = 0,
    wxKILL_CHILDREN = 1
};

int  wxKill( long aPid, wxSignal aSig = wxSIGTERM, wxKillError* aRc = nullptr,
             int aFlags = wxKILL_NOCHILDREN );


/**
 * A child process of wxExecute().  Without event loop a redirected process is run until its
 * end, and its output and errors are then available as streams.
 */
class wxProcess
{
public:
    explicit wxProcess( int aFlags = 0 ) : m_redirect( aFlags & 1 ) {}
    virtual ~wxProcess() = default;

    virtual void OnTerminate( int, int ) {}

    void Redirect() { m_redirect = true; }
    bool IsRedirected() const { return m_redirect; }
    void Detach() {}

    long GetPid() const { return m_pid; }
    void SetPid( long aPid ) { m_pid = aPid; }

    wxInputStream*  GetInputStream() const { return m_input.get(); }
    wxInputStream*  GetErrorStream() const { return m_error.get(); }
    wxOutputStream* GetOutputStream() const { return nullptr; }
    bool            IsInputAvailable() const { return m_input && m_input->CanRead(); }
    bool            IsErrorAvailable() const { return m_error && m_error->CanRead(); }
    bool            IsInputOpened() const { return m_input != nullptr; }
    void            CloseOutput() {}

    static wxKillError Kill( int aPid, wxSignal aSig = wxSIGTERM, int aFlags = wxKILL_NOCHILDREN );
    static bool        Exists( int aPid );

    /// Used by wxExecute(): the captured standard output and error.
    void SetCapturedOutput( const std::string& aOutput, const std::string& aErrors )
    {
        m_input = std::make_unique<wxMemoryInputStream>( aOutput.data(), aOutput.size() );
        m_error = std::make_unique<wxMemoryInputStream>( aErrors.data(), aErrors.size() );
    }

private:
    bool                                 m_redirect = false;
    long                                 m_pid = 0;
    std::unique_ptr<wxMemoryInputStream> m_input;
    std::unique_ptr<wxMemoryInputStream> m_error;
};

#define wxPROCESS_DEFAULT 0
#define wxPROCESS_REDIRECT 1

#ifndef _WIN32
#pragma GCC visibility pop
#endif
