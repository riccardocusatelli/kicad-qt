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
 * @file pgm_gui.h
 * @brief see class PGM_GUI
 */

#ifndef PGM_GUI_H
#define PGM_GUI_H

#include <atomic>
#include <future>
#include <mutex>

#include <pgm_base.h>

class wxApp;
class wxSingleInstanceChecker;
class wxSplashScreen;

class BACKGROUND_JOBS_MONITOR;
class GL_CONTEXT_MANAGER;
class KISTATUSBAR;
class KIWAY;
class NOTIFICATIONS_MANAGER;
class SCRIPTING;
struct BACKGROUND_JOB;
struct LOAD_MESSAGE;

#ifdef KICAD_IPC_API
class API_PLUGIN_MANAGER;
class KICAD_API_SERVER;
#endif


/**
 * The program services of the graphical KiCad applications: PGM_BASE plus the wxApp, the
 * splash screen, the single instance checker, background job and notification UI, the
 * IPC API plugins, Python scripting and the GL context manager.
 *
 * The editors and the project manager derive their program class from PGM_GUI; GUI code
 * reaches it with PgmGui().
 */
class KICOMMON_API PGM_GUI : public PGM_BASE
{
public:
    PGM_GUI();
    ~PGM_GUI() override;

    GL_CONTEXT_MANAGER* GetGLContextManager() { return m_glContextManager; }

    /**
     * Specific to MacOSX (not used under Linux or Windows).
     *
     * MacOSX requires it for file association.
     * @see http://wiki.wxwidgets.org/WxMac-specific_topics
     */
    virtual void MacOpenFile( const wxString& aFileName ) = 0;

    virtual BACKGROUND_JOBS_MONITOR& GetBackgroundJobMonitor() const
    {
        return *m_background_jobs_monitor;
    }

    virtual NOTIFICATIONS_MANAGER& GetNotificationsManager() const
    {
        return *m_notifications_manager;
    }

#ifdef KICAD_IPC_API
    virtual API_PLUGIN_MANAGER& GetPluginManager() const { return *m_plugin_manager; }

    KICAD_API_SERVER& GetApiServer() { return *m_api_server; }
#endif

    /**
     * Return the path to the preferred text editor application.
     *
     * @param   aCanShowFileChooser If no editor is currently set and this argument is
     *          'true' then this method will show a file chooser dialog asking for the
     *          editor's executable.
     * @return  Returns the full path of the editor, or an empty string if no editor has
     *          been set.
     */
    virtual const wxString& GetTextEditor( bool aCanShowFileChooser = true );

    /**
     * Show a dialog that instructs the user to select a new preferred editor.
     *
     * @param   aDefaultEditor Default full path for the default editor this dialog should
     *          show by default.
     * @return  the full path of the editor, or an empty string if no editor was chosen.
     */
    virtual const wxString AskUserForPreferredEditor(
            const wxString& aDefaultEditor = wxEmptyString );

    /**
     * Return a bare naked wxApp which may come from wxPython, SINGLE_TOP, or kicad.exe.
     *
     * This should return what wxGetApp() returns.
     */
    virtual wxApp& App();

    bool InitPgm( bool aHeadless = false, bool aSkipPyInit = false,
                  bool aIsUnitTest = false ) override;

    void Destroy() override;

    bool ConfirmAction( const wxString& aTitle, const wxString& aMessage ) override;

    void ShowSplash();
    void HideSplash();

    /**
     * Allow access to the wxSingleInstanceChecker to test for other running KiCads.
     */
    std::unique_ptr<wxSingleInstanceChecker>& SingleInstance()
    {
        return m_pgm_checker;
    }

    /**
     * Starts a background job to preload the global and project design block libraries.
     * Design block handling code is not associated with a particular KIFACE so this is
     * handled here unlike symbol/footprint loading which are taken care of by the KIFACEs.
     */
    void PreloadDesignBlockLibraries( KIWAY* aKiway );

    /**
     * Register a status bar to receive library load warning messages.
     * Multiple status bars can be registered (one per open frame).
     */
    void RegisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Unregister a status bar from receiving library load warning messages.
     */
    void UnregisterLibraryLoadStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Add library load messages to all registered status bars.
     * Thread-safe - can be called from background threads.
     */
    void AddLibraryLoadMessages( const std::vector<LOAD_MESSAGE>& aMessages );

    /**
     * Clear library load messages from all registered status bars.
     */
    void ClearLibraryLoadMessages();

    /**
     * wxWidgets on MSW tends to crash if you spool up more than one print job at a time.
     */
    bool m_Printing;

    std::vector<void*> m_ModalDialogs;

    bool m_PropertyGridInitialized;

protected:
    void reportInitError( const wxString& aMessage ) override;

protected:
    std::unique_ptr<BACKGROUND_JOBS_MONITOR> m_background_jobs_monitor;
    std::unique_ptr<NOTIFICATIONS_MANAGER> m_notifications_manager;

    std::unique_ptr<SCRIPTING> m_python_scripting;

    /// Check if there is another copy of Kicad running at the same time.
    std::unique_ptr<wxSingleInstanceChecker> m_pgm_checker;

#ifdef KICAD_IPC_API
    std::unique_ptr<API_PLUGIN_MANAGER> m_plugin_manager;
    std::unique_ptr<KICAD_API_SERVER> m_api_server;
#endif

    GL_CONTEXT_MANAGER* m_glContextManager;

    wxSplashScreen* m_splash;

    std::shared_ptr<BACKGROUND_JOB> m_libraryPreloadBackgroundJob;
    std::future<void>               m_libraryPreloadReturn;
    std::atomic_bool                m_libraryPreloadInProgress;
    std::atomic_bool                m_libraryPreloadAbort;

    std::vector<KISTATUSBAR*>       m_libraryLoadStatusBars;
    mutable std::mutex              m_libraryLoadStatusBarsMutex;
};


/**
 * The program of a graphical KiCad application.  Only valid in the editors and the project
 * manager (not in kicad-cli), where the program derives from PGM_GUI.
 */
KICOMMON_API extern PGM_GUI& PgmGui();

#endif // PGM_GUI_H
