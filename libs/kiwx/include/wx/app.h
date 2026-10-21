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

#include <wx/arrstr.h>
#include <wx/string.h>

#ifdef Yield
#undef Yield    // <windows.h> defines Yield() as a macro
#endif

class wxWindow;

/// The command line arguments of the application (wxCmdLineArgsArray).
class wxCmdLineArgsArray
{
public:
    void Init( int aArgc, char** aArgv );

    const wxArrayString& GetArguments() const { return m_args; }
    size_t               size() const { return m_args.size(); }
    const wxString&      operator[]( size_t aIndex ) const { return m_args[aIndex]; }
    wxString&            operator[]( size_t aIndex ) { return m_args[aIndex]; }
    explicit operator bool() const { return !m_args.empty(); }

    /// As wchar_t** (wxWidgets builds it on demand too).
    operator wchar_t**() const;

private:
    wxArrayString                      m_args;
    mutable std::vector<std::wstring>  m_wide;
    mutable std::vector<wchar_t*>      m_widePtrs;
};


/**
 * The application object of a program without user interface: the names and the command
 * line, and the OnInit() / OnRun() / OnExit() sequence of wxEntry().
 */
class WXDLLIMPEXP_BASE wxAppConsole
{
public:
    wxAppConsole();
    virtual ~wxAppConsole();

    static wxAppConsole* GetInstance() { return ms_instance; }
    static void          SetInstance( wxAppConsole* aApp ) { ms_instance = aApp; }

    virtual bool OnInit() { return true; }
    virtual int  OnRun() { return 0; }
    virtual int  OnExit();
    virtual void OnUnhandledException() {}
    virtual void OnAssertFailure( const wxChar* aFile, int aLine, const wxChar* aFunc,
                                  const wxChar* aCond, const wxChar* aMsg );

    virtual bool IsGUI() const { return false; }
    bool         IsMainLoopRunning() const { return false; }

    wxString GetAppName() const;
    void     SetAppName( const wxString& aName ) { m_appName = aName; }
    wxString GetAppDisplayName() const;
    void     SetAppDisplayName( const wxString& aName ) { m_appDisplayName = aName; }
    wxString GetClassName() const { return m_className; }
    void     SetClassName( const wxString& aName ) { m_className = aName; }
    wxString GetVendorName() const { return m_vendorName; }
    void     SetVendorName( const wxString& aName ) { m_vendorName = aName; }
    wxString GetVendorDisplayName() const
    {
        return m_vendorDisplayName.empty() ? m_vendorName : m_vendorDisplayName;
    }
    void SetVendorDisplayName( const wxString& aName ) { m_vendorDisplayName = aName; }

    void ExitMainLoop() {}

    // The event loop of the user interface (none here: the application of a user interface
    // overrides them)

    /// Process the pending events.
    virtual bool Yield( bool aOnlyIfNeeded = false ) { return true; }

    /// Process the pending events without the input of the user (wxSafeYield()).
    virtual bool SafeYield( wxWindow* aWindow, bool aOnlyIfNeeded ) { return true; }

    /// Process the pending events of the categories @a aEventsToProcess (wxEventCategory).
    virtual bool SafeYieldFor( wxWindow* aWindow, long aEventsToProcess ) { return true; }

    virtual void      SetTopWindow( wxWindow* aWindow ) {}
    virtual wxWindow* GetTopWindow() const { return nullptr; }

    int                argc = 0;
    wxCmdLineArgsArray argv;

private:
    static wxAppConsole* ms_instance;

    wxString m_appName;
    wxString m_appDisplayName;
    wxString m_className;
    wxString m_vendorName;
    wxString m_vendorDisplayName;
};

/// The application class without user interface.
typedef wxAppConsole wxApp;

#define wxTheApp wxAppConsole::GetInstance()

/// The equivalent of wxEntry(): runs the application, returns the process exit code.
int wxEntry( int& aArgc, char** aArgv );

#define wxIMPLEMENT_APP_CONSOLE( appname )                                                       \
    int main( int argc, char** argv )                                                            \
    {                                                                                            \
        appname app;                                                                             \
        return wxEntry( argc, argv );                                                            \
    }

#define IMPLEMENT_APP_CONSOLE( appname ) wxIMPLEMENT_APP_CONSOLE( appname )

#ifndef _WIN32
#pragma GCC visibility pop
#endif
