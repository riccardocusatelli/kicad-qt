/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file eda_base_frame.h
 * @brief Base window classes and related definitions: the Qt version of
 *        include/eda_base_frame.h, on a QMainWindow.
 */

#ifndef  EDA_BASE_FRAME_H_
#define  EDA_BASE_FRAME_H_

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <kiqt.h>
#include <QMainWindow>

#include <wx/kiwx_window.h>     // the wxWidgets API of the windows (wxcompat)

#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <wx/toplevel.h>         // the styles of the frames (kiwx)
#include <layer_ids.h>
#include <frame_type.h>
#include <frame_panes.h>
#include <hotkeys_basic.h>
#include <kiway_holder.h>
#include <tool/action_manager.h>     // ACTION_CONDITIONS
#include <tool/tools_holder.h>
#include <widgets/ui_common.h>
#include <widgets/wx_infobar_message_type.h>
#include <undo_redo_container.h>
#include <units_provider.h>
#include <ui_events.h>
#include <origin_transforms.h>
#include <toolkit/close_request.h>
#include <toolkit/gui_timer.h>

#define VIEWER3D_FRAMENAME wxT( "Viewer3DFrameName" )
#define QUALIFIED_VIEWER3D_FRAMENAME( parent ) \
                    ( wxString( VIEWER3D_FRAMENAME ) + wxT( ":" ) + parent->GetName() )
#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS

#define KICAD_MANAGER_FRAME_NAME   wxT( "KicadFrame" )

class QCloseEvent;
class ACTION_TOOLBAR;
class ACTION_TOOLBAR_CONTROL;
class TOOLBAR_SETTINGS;
class QLabel;
class QDockWidget;
class KISTATUSBAR;
class wxFileName;
class EDA_ITEM;
enum class BITMAPS : unsigned int;
class EDA_DRAW_PANEL_GAL;
class BASE_SCREEN;
class PAGE_INFO;
class TITLE_BLOCK;
class MSG_PANEL_ITEM;
class TOOL_MANAGER;
class TOOL_DISPATCHER;
class ACTIONS;
class SETTINGS_MANAGER;
class SEARCH_STACK;
class APP_SETTINGS_BASE;
struct WINDOW_SETTINGS;
struct WINDOW_STATE;
class ACTION_MENU;
class WX_MENUBAR;
class TOOL_INTERACTIVE;
class WX_INFOBAR;
class APPEARANCE_CONTROLS_3D;


#define DEFAULT_MAX_UNDO_ITEMS 0
#define ABS_MAX_UNDO_ITEMS (INT_MAX / 2)


/**
 * The base frame for deriving all KiCad main window classes.
 *
 * This class is not intended to be used directly.  It provides support for automatic calls
 * to SaveSettings() function.  SaveSettings() for a derived class can choose to do nothing,
 * or rely on basic SaveSettings() support in this base class to do most of the work by
 * calling it from the derived class's SaveSettings().  This class is not a #KIWAY_PLAYER
 * because #KICAD_MANAGER_FRAME is derived from it and that class is not a player.
 *
 * In the Qt build the frame is a QMainWindow: the menu bar, the tool bars and the panes are
 * those of the main window; the tools find the same interface as in the wxWidgets build.
 */
class EDA_BASE_FRAME : public wxWindowCompat<QMainWindow>, public TOOLS_HOLDER,
                       public KIWAY_HOLDER, public UNITS_PROVIDER
{
public:
    /**
     * Specify whether we are interacting with the undo or redo stacks.
     */
    enum UNDO_REDO_LIST
    {
        UNDO_LIST,
        REDO_LIST
    };

    /**
     * @param aPos and @param aSize are the initial geometry of the frame (or wxDefaultPosition,
     *        wxDefaultSize).
     * @param aStyle are the wxWidgets styles of the frame (wxSTAY_ON_TOP,
     *        wxFRAME_FLOAT_ON_PARENT, wxFRAME_NO_TASKBAR, the buttons of the title bar).
     */
    EDA_BASE_FRAME( QWidget* aParent, FRAME_T aFrameType, const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize, long aStyle,
                    const wxString& aFrameName, KIWAY* aKiway, const EDA_IU_SCALE& aIuScale );

    ~EDA_BASE_FRAME();

    void ChangeUserUnits( EDA_UNITS aUnits );

    /**
     * Call a function each time the user units of the frame change (as the units changed
     * event of the wxWidgets build).
     *
     * @return the identifier of the function, to remove it.
     */
    int AddUnitsChangedListener( std::function<void( EDA_BASE_FRAME* )> aListener );

    void RemoveUnitsChangedListener( int aId );

    /**
     * Call a function each time the language of the GUI changes (as the language changed
     * event of the wxWidgets build), after the frame was updated, as long as @a aOwner lives.
     */
    void AddLanguageChangedListener( QObject* aOwner, std::function<void()> aListener );

    virtual void ToggleUserUnits() { }

    /**
     * Return a reference to the default ORIGIN_TRANSFORMS object
     */
    ORIGIN_TRANSFORMS& GetOriginTransforms() override
    {
        return m_originTransforms;
    }

    SETTINGS_MANAGER* GetSettingsManager() const { return m_settingsManager; }

    virtual SEVERITY GetSeverity( int aErrorCode ) const { return RPT_SEVERITY_UNDEFINED; }

    /**
     * Register a UI update handler for the control with ID @c aID.
     *
     * The conditions of the action are applied to the entries of the menus and the tool bars
     * of the frame that have this ID.
     *
     * @param aID is the control ID to register the handler for
     * @param aConditions are the UI conditions to use for the control states
     */
    virtual void RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions ) override;

    /**
     * Unregister a UI handler for a given ID that was previously registered.
     *
     * @param aID is the control ID to unregister the handler for
     */
    virtual void UnregisterUIUpdateHandler( int aID ) override;

    /**
     * Apply the registered UI conditions (enabled, checked, shown) to the entries of the menus
     * and the tool bars.
     */
    void UpdateUIConditions();

    /**
     * Select the given tool in the toolbars of the frame.
     */
    void SelectToolbarAction( const TOOL_ACTION& aAction );

    /**
     * Create (or fill again) the toolbars of the frame from its toolbar settings: the main and
     * auxiliary toolbars at the top, the options on the left and the drawing tools on the right.
     */
    virtual void RecreateToolbars();

    /**
     * Update toolbars if desired toolbar icon changed.
     */
    void OnToolbarSizeChanged();

    /**
     * Update the sizes of any controls in the toolbars of the frame.
     */
    virtual void UpdateToolbarControlSizes();

    /**
     * Register a creation factory for toolbar controls that are present in this frame.
     *
     * The factory function takes a single argument of type `ACTION_TOOLBAR*`, which is the
     * toolbar to add the controls to.
     *
     * @param aControlDesc is the control descriptor
     * @param aControlFactory A functor that creates the custom controls and then adds them to
     *                        the toolbar
     */
    void RegisterCustomToolbarControlFactory( const ACTION_TOOLBAR_CONTROL& aControlDesc,
                                              const std::function<void( ACTION_TOOLBAR* )>& aControlFactory );

    /**
     * The factory of a control of the toolbars, or nullptr.
     */
    std::function<void( ACTION_TOOLBAR* )>* GetCustomToolbarControlFactory( const std::string& aName );

    /**
     * Call @a aHandler when the menu item or the tool of id @a aId (an id, not an action) is
     * chosen.
     */
    void BindMenuCommand( int aId, std::function<void()> aHandler );

    /**
     * Run the handler of the menu item or tool of id @a aId (BindMenuCommand()).
     *
     * @return true if the frame has a handler for the id.
     */
    bool RunMenuCommand( int aId );

    /**
     * A control of the toolbars (with the id @a aId) is deleted: the frame forgets it.
     */
    virtual void ClearToolbarControl( int aId ) { }

    /**
     * @return the auto save interval in seconds, or 0 if it is disabled.
     */
    int GetAutoSaveInterval() const;

    bool IsType( FRAME_T aType ) const { return m_ident == aType; }
    FRAME_T GetFrameType() const { return m_ident; }

    /**
     * Return a #SEARCH_STACK pertaining to entire program.
     *
     * This is overloaded in #KICAD_MANAGER_FRAME
     */
    virtual const SEARCH_STACK& sys_search();

    virtual wxString help_name();

    /// The name of the frame (the prefix of its settings).
    wxString GetName() const;

    /// Set the title of the frame.
    void SetTitle( const wxString& aTitle );

    wxString GetTitle() const;

    /// Show a text in a field of the status bar.
    virtual void SetStatusText( const wxString& aText, int aField = 0 );

    /// Create the status bar of the frame with @a aNumber fields (OnCreateStatusBar()).
    KISTATUSBAR* CreateStatusBar( int aNumber );

    /// @return the status bar (nullptr if none).
    KISTATUSBAR* GetStatusBar() const { return m_statusBar; }

    /**
     * Set the widths of the fields of the status bar: a positive width is fixed, a negative one
     * is a stretch factor.
     */
    void SetStatusWidths( int aSize, const int* aWidths );

    /// Create the status bar of CreateStatusBar() (a status bar with the fields only).
    virtual KISTATUSBAR* OnCreateStatusBar( int aNumber );

    /**
     * Close the frame.
     *
     * @param aForce closes it even if the frame asks to keep it open.
     * @return true if it was closed.
     */
    bool Close( bool aForce = false );

    /// @return true if the frame is minimized.
    bool IsIconized() const { return isMinimized(); }

    /// Minimize the frame (or restore it, with @a aIconize false).
    void Iconize( bool aIconize = true );

    /// Bring the frame above the other windows, and activate it.
    void Raise();

    /// Show (or hide) the frame. @return true if its visibility changed.
    bool Show( bool aShow = true );

    /// Give the focus of the keyboard to the frame.
    void SetFocus() { setFocus(); }

    /// @return true if the frame accepts the input (it is not disabled by a modal dialog).
    bool IsEnabled() const { return isEnabled(); }

    /// Lay out again the panes of the frame (after they were shown, hidden or changed).
    void UpdatePanes();

    /**
     * The panes of the frame (the panels docked around the canvas), by name.
     *
     * Changing them (showing, hiding, docking) takes effect with UpdatePanes().
     */
    bool IsPaneShown( const wxString& aName ) const;
    void ShowPane( const wxString& aName, bool aShow = true );
    bool IsPaneFloating( const wxString& aName ) const;
    void SetPaneCaption( const wxString& aName, const wxString& aCaption );

    /// The side the pane is docked to (a #PANE_DOCK value, as stored in the settings).
    int  GetPaneDockDirection( const wxString& aName ) const;
    void SetPaneDockDirection( const wxString& aName, int aDirection );

    wxSize GetPaneFloatingSize( const wxString& aName ) const;
    void   SetPaneFloatingSize( const wxString& aName, const wxSize& aSize );

    /**
     * Give a docked pane a size (-1 keeps the size in that direction), and lay out the panes.
     */
    void SetPaneSize( const wxString& aName, int aWidth, int aHeight );

    /// The size of the panel of a pane.
    wxSize GetPaneSize( const wxString& aName ) const;

    /**
     * Give the frame its icons (in the sizes the system needs).
     *
     * @param aIcons are the bitmaps of the icons, with their height tag (-1 for the size of
     *               the bitmap).
     */
    void SetFrameIcons( const std::vector<std::pair<BITMAPS, int>>& aIcons );

    /**
     * Run a function later, from the event loop (after the event being handled).
     */
    void CallAfter( std::function<void()> aFunction );

    /**
     * Show the preferences of KiCad, at a page.
     *
     * @param aStartPage is the title of the page to show.
     * @param aStartParentPage is the title of the group of pages (the editor) of the page.
     */
    void ShowPreferences( wxString aStartPage, wxString aStartParentPage );

    /**
     * Display a bargraph (0 to 50 point length) for a PerCent value from 0 to 100.
     */
    void PrintMsg( const wxString& text );

    void CreateInfoBar();

    /**
     * @return the info bar of the frame (above the canvas).
     */
    WX_INFOBAR* GetInfoBar() { return m_infoBar; }

    /**
     * Show the #WX_INFOBAR displaying a message and an icon.
     *
     * @param aErrorMsg is the message to display.
     * @param aShowCloseButton true to show a close button on the right of the #WX_INFOBAR.
     * @param aType the type of message for the infobar
     */
    void ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton = false,
                           INFOBAR_MESSAGE_TYPE aType = INFOBAR_MESSAGE_TYPE::GENERIC );

    /**
     * Show the #WX_INFOBAR displaying a message and an icon, and run a function when it is
     * closed.
     */
    void ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                           std::function<void(void)> aCallback );

    void ShowInfoBarWarning( const wxString& aWarningMsg, bool aShowCloseButton = false );

    void ShowInfoBarMsg( const wxString& aMsg, bool aShowCloseButton = false );

    /**
     * Return the settings object used in SaveSettings(), and is overloaded in
     * #KICAD_MANAGER_FRAME.
     */
    virtual APP_SETTINGS_BASE* config() const;

    /**
     * Load window settings from the given settings object.
     *
     * Normally called by #LoadSettings() unless the window in question is a child window
     * that* stores its settings somewhere other than #APP_SETTINGS_BASE::m_Window.
     */
    void LoadWindowSettings( const WINDOW_SETTINGS* aCfg );

    /**
     * Save window settings to the given settings object.
     *
     * Normally called by #SaveSettings unless the window in question is a child window that
     * stores its settings somewhere other than #APP_SETTINGS_BASE::m_Window.
     */
    void SaveWindowSettings( WINDOW_SETTINGS* aCfg );

    /**
     * Load common frame parameters from a configuration file.
     *
     * Don't forget to call the base method or your frames won't remember their positions
     * and sizes.
     */
    virtual void LoadSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Save common frame parameters to a configuration data file.
     *
     * Don't forget to call the base class's SaveSettings() from your derived
     * #SaveSettings() otherwise the frames won't remember their positions and sizes.
     */
    virtual void SaveSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Return a pointer to the window settings for this frame.
     *
     * By default, points to aCfg->m_Window for top-level frames.
     *
     * @param aCfg is this frame's config object
     */
    virtual WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg );

    /**
     * Load frame state info from a configuration file
     */
    virtual void LoadWindowState( const WINDOW_STATE& aState );

    /**
     * @return a base name prefix used in Load/Save settings to build the full name of keys
     *         used in config.
     *
     * This is usually the name of the frame set by CTOR, except for frames shown in
     * multiple modes in which case the m_configName must be set to the base name so that
     * a single config can be used.
     */
    wxString ConfigBaseName() override
    {
        wxString baseCfgName = m_configName.IsEmpty() ? GetName() : m_configName;
        return baseCfgName;
    }

    /**
     * Save changes to the project local settings.
     *
     * These settings are not project based so they will not be saved by the project
     * settings.  This is called by #SaveProjectSettings.
     */
    virtual void SaveProjectLocalSettings() {};

    void SetMruPath( const wxString& aPath ) { m_mruPath = aPath; }

    wxString GetMruPath() const { return m_mruPath; }

    /**
     * Get the full filename + path of the currently opened file in the frame.
     *
     * If no file is open, an empty string is returned.
     *
     * @return the filename and full path to the open file
     */
    virtual wxString GetCurrentFileName() const { return wxEmptyString; }

    /**
     * Recreate the menu bar.
     *
     * Needed when the language or icons are changed
     */
    void ReCreateMenuBar();

    /// The frame is the active window.
    bool IsActive() const { return isActiveWindow(); }

    /**
     * Add the standard KiCad help menu to the menubar.
     */
    void AddStandardHelpMenu( WX_MENUBAR* aMenuBar );

    /**
     * Show @a aMenuBar in the frame, and delete its previous menu bar.
     */
    void InstallMenuBar( WX_MENUBAR* aMenuBar );

    /**
     * Create a menu list for language choice, and add it as submenu to \a MasterMenu.
     *
     * @param aMasterMenu is the main menu.
     * @param aControlTool is the tool to associate with the menu.
     */
    void AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool );

    /**
     * Check if \a aFileName can be written.
     *
     * The function performs a number of tests on \a aFileName to verify that it can be saved.
     * If \a aFileName defines a path with no file name, them the path is tested for user write
     * permission.  If \a aFileName defines a file name that does not exist in the path, the
     * path is tested for user write permission.  If \a aFileName defines a file that already
     * exits, the file name is tested for user write permissions.
     *
     * @note The file name path must be set or an assertion will be raised on debug builds and
     *       return false on release builds.
     * @param aFileName The full path and/or file name of the file to test.
     * @param aVerbose If true will show an error dialog if the file is not writable
     * @return False if \a aFileName cannot be written.
     */
    bool IsWritable( const wxFileName& aFileName, bool aVerbose = true );

    /**
     * Update the status bar information.
     *
     * The status bar can draw itself.  This is not a drawing function per se, but rather
     * updates lines of text held by the components within the status bar which is owned
     * by the wxFrame.
     */
    virtual void UpdateStatusBar() { }

    /**
     * Redraw the menus and what not in current language.
     */
    void ShowChangedLanguage() override;

    /**
     * Notification event that some of the common (suite-wide) settings have changed.
     * Update menus, toolbars, local variables, etc.
     */
    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Process light/dark theme change.
     */
    virtual void ThemeChanged();

    /**
     * Notification event that the project has changed.
     */
    virtual void ProjectChanged() {}

    wxString GetAboutTitle() const { return wxGetTranslation( m_aboutTitle ); }

    const wxString& GetUntranslatedAboutTitle() const { return m_aboutTitle; }

    /**
     * Get if the contents of the frame have been modified since the last save.
     *
     * @return true if the contents of the frame have not been saved
     */
    virtual bool IsContentModified() const;

    /**
     * Get the undecorated window size that can be used for restoring the window size.
     *
     * This is needed for GTK, since the normal wxWidgets GetSize() call will return
     * a window size that includes the window decorations added by the window manager.
     *
     * @return the undecorated window size
     */
    wxSize GetWindowSize();

    /**
     * Remove the \a aItemCount of old commands from \a aList and delete commands, pickers
     * and picked items if needed.
     *
     * Because picked items must be deleted only if they are not in use, this is a virtual
     * pure function that must be created for #SCH_SCREEN and #PCB_SCREEN.  Commands are
     * deleted from the older to the last.
     *
     * @param aList = the #UNDO_REDO_CONTAINER of commands.
     * @param aItemCount number of old commands to delete. -1 to remove all old commands
     *                   this will empty the list of commands.
     */
    virtual void ClearUndoORRedoList( UNDO_REDO_LIST aList, int aItemCount = -1 )
    { }

    /**
     * Clear the undo and redo list using #ClearUndoORRedoList()
     *
     * Picked items are deleted by ClearUndoORRedoList() according to their status.
     */
    virtual void ClearUndoRedoList();

    /**
     * Add a command to undo in the undo list.
     *
     * Delete the very old commands when the max count of undo commands is reached.
     */
    virtual void PushCommandToUndoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Add a command to redo in the redo list.
     *
     * Delete the very old commands when the max count of redo commands is reached.
     */
    virtual void PushCommandToRedoList( PICKED_ITEMS_LIST* aItem );

    /**
     * Return the last command to undo and remove it from list, nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromUndoList();

    /**
     * Return the last command to undo and remove it from list, nothing is deleted.
     */
    virtual PICKED_ITEMS_LIST* PopCommandFromRedoList();

    virtual int GetUndoCommandCount() const { return m_undoList.m_CommandsList.size(); }
    virtual int GetRedoCommandCount() const { return m_redoList.m_CommandsList.size(); }

    virtual wxString GetUndoActionDescription() const;
    virtual wxString GetRedoActionDescription() const;

    int GetMaxUndoItems() const { return m_undoRedoCountMax; }

    /**
     * Must be called after a model change in order to set the "modify" flag and
     * do other frame-specific processing.
     */
    virtual void OnModify();

    bool IsClosing() const { return m_isClosing; }

    bool NonUserClose( bool aForce )
    {
        m_isNonUserClose = true;
        return Close( aForce );
    }

    /**
     * Process a system color change.
     */
    virtual void HandleSystemColorChange();

    /**
     * Check if this frame is ready to accept API commands.
     *
     * A frame might not accept commands if a long-running process is underway, a dialog is
     * open, the user is interacting with a tool, etc.
     */
    virtual bool CanAcceptApiCommands() { return isEnabled(); }

protected:
    /**
     * Dock a panel to a side of the frame, as the pane @a aName (hidden when it is not a
     * pane shown by default).
     *
     * @return the dock widget of the pane.
     */
    QDockWidget* addPane( QWidget* aPanel, const wxString& aName, const wxString& aCaption,
                          PANE_DOCK aDock, bool aShow = true );

    /// @return the dock widget of the pane @a aName, or nullptr.
    QDockWidget* findPane( const wxString& aName ) const;

    /// Fill the menu bar (the frames of the editors make their menus).
    virtual void doReCreateMenuBar() {}

    /**
     * Set up the toolbar settings and the control factories of the frame (before
     * RecreateToolbars()).
     */
    virtual void configureToolbars();

    void onAutoSaveTimer();

    virtual bool isAutoSaveRequired() const { return m_autoSaveRequired; }

    /**
     * This should be overridden by the derived class to handle the auto save feature.
     *
     * @return true if the auto save was successful otherwise false.
     */
    virtual bool doAutoSave();

    virtual bool canRunAutoSave() const { return true; }

    /**
     * Ask the frame if it can close (for example after saving the changes).
     *
     * @param aRequest is the request; the frame can veto it or just return false.
     * @return true if the frame can close.
     */
    virtual bool canCloseWindow( CLOSE_REQUEST& aRequest ) { return true; }

    /**
     * Handle a window iconize event.
     *
     * @param aIconized is true when the window is iconized, false when it is restored.
     */
    virtual void handleIconizeEvent( bool aIconized ) {}

    /**
     * Handle a window activation event.
     *
     * @param aActive is true when the window is activated, false when it is deactivated.
     */
    virtual void handleActivateEvent( bool aActive ) {}

    /**
     * The key @a aSwitchKey (PRESET_SWITCH_KEY or VIEWPORT_SWITCH_KEY) is held down with Tab:
     * show the switcher of the layer presets or of the viewports (EDA_VIEW_SWITCHER).
     *
     * @return true if the key was used.
     */
    virtual bool handleViewSwitcherKey( int aSwitchKey ) { return false; }

    /// The frame is being closed: release what it uses.
    virtual void doCloseWindow() { }

    /**
     * Called when when the units setting has changed to allow for any derived classes
     * to handle refreshing and controls that have units based display.
     */
    virtual void unitsChangeRefresh() { }

    /**
     * Set up the UI conditions for the various actions and their controls in this frame.
     */
    virtual void setupUIConditions();

    /**
     * Make sure that the frame is on screen.
     */
    void ensureWindowIsOnScreen();

    /**
     * Save any design-related project settings associated with this frame.
     *
     * This method should only be called as the result of a user-initiated save.
     */
    virtual void saveProjectSettings() {}

    /// The frame is asked to close (by the user or by Close()).
    void closeEvent( QCloseEvent* aEvent ) override;

    /// The theme of the system or the state of the window (iconized) changed.
    void changeEvent( QEvent* aEvent ) override;

    /// Accept the drops of files (the files with an extension of m_acceptedExts).
    void DragAcceptFiles( bool aAccept ) { setAcceptDrops( aAccept ); }

    void dragEnterEvent( QDragEnterEvent* aEvent ) override;
    void dropEvent( QDropEvent* aEvent ) override;

    /**
     * Files were dropped on the window: store the paths of the accepted ones and call
     * DoWithAcceptedFiles().
     */
    void handleDroppedFiles( const std::vector<wxString>& aFiles );

    /**
     * Execute action on accepted dropped file.
     *
     * Called in handleDroppedFiles() and should be populated with the action to execute in
     * inherited classes.
     */
    virtual void            DoWithAcceptedFiles();
    std::vector<wxFileName> m_AcceptedFiles;

    /// Associate file extensions with action to execute.
    std::map<const wxString, TOOL_ACTION*> m_acceptedExts;

private:
    void commonInit( FRAME_T aFrameType );

    virtual bool IsModal() const { return false; }

protected:
    FRAME_T         m_ident;                // Id Type (pcb, schematic, library..)
    bool            m_maximizeByDefault;
    int             m_displayIndex;

    wxString                m_aboutTitle;        // Name of program displayed in About.

    WX_INFOBAR*                m_infoBar;           // Infobar for the frame
    KISTATUSBAR*               m_statusBar = nullptr;  // The status bar, if any
    APPEARANCE_CONTROLS_3D*    m_appearancePanel = nullptr;

    wxString                m_configName;        // Prefix used to identify some params (frame
                                                 // size...) and to name some config files
                                                 // (legacy hotkey files)

    SETTINGS_MANAGER*       m_settingsManager;

    wxSize                  m_frameSize;         // The size of the frame at its creation (the
                                                 // initial size of the canvases)

    bool                    m_supportsAutoSave;
    bool                    m_autoSavePending;
    bool                    m_autoSaveRequired;
    GUI_TIMER               m_autoSaveTimer;
    bool                    m_autoSavePermissionError;
    wxDateTime              m_autoSaveDeferredSince;

    int                     m_undoRedoCountMax;  // undo/Redo command Max depth
    UNDO_REDO_CONTAINER     m_undoList;          // Objects list for the undo command (old data)
    UNDO_REDO_CONTAINER     m_redoList;          // Objects list for the redo command (old data)

    wxString                m_mruPath;           // Most recently used path.

    ORIGIN_TRANSFORMS       m_originTransforms;  // Default display origin transforms object.

    ///< The UI conditions of the controls (menu entries, tool buttons) of an ID
    std::map<int, ACTION_CONDITIONS> m_uiConditions;

    // Toolbar Settings - this is not owned by the frame
    TOOLBAR_SETTINGS*    m_toolbarSettings;

    // Toolbar UI elements
    ACTION_TOOLBAR*      m_tbTopMain;
    ACTION_TOOLBAR*      m_tbTopAux;  // Additional tools under main toolbar
    ACTION_TOOLBAR*      m_tbRight;   // Drawing tools (typically on right edge of window)
    ACTION_TOOLBAR*      m_tbLeft;    // Options (typically on left edge of window)

    std::map<std::string, std::function<void( ACTION_TOOLBAR* )>> m_toolbarControlFactories;

    /// The handlers of the menu items and tools that are not actions (BindMenuCommand()).
    std::map<int, std::function<void()>> m_menuCommands;

    ///< The functions called when the user units change
    std::map<int, std::function<void( EDA_BASE_FRAME* )>> m_unitsChangedListeners;
    int                                                   m_nextUnitsChangedListener = 0;

    ///< The functions called when the language changes, by owner
    std::multimap<QObject*, std::function<void()>>        m_languageChangedListeners;

    ///< Set by #NonUserClose() to indicate that the user did not request the current close.
    bool            m_isClosing;
    bool            m_closeInProgress;
    bool            m_isNonUserClose;

    wxString        m_name;                     // The name of the frame
};

#endif  // EDA_BASE_FRAME_H_
