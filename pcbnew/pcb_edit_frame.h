/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras@wanadoo.fr
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef  __PCB_EDIT_FRAME_H__
#define  __PCB_EDIT_FRAME_H__

#include "pcb_base_edit_frame.h"
#include "zones.h"
#include <board.h>
#include <mail_type.h>
#include <settings/app_settings.h>
#include <toolkit/gui_timer.h>
#include <toolkit/toolkit_types.h>
#include <variant>

class PCB_SCREEN;
class BOARD;
class BOARD_COMMIT;
class BOARD_ITEM_CONTAINER;
class DESIGN_BLOCK;
class DIALOG_BOOK_REPORTER;
class FOOTPRINT;
class PCB_TRACK;
class PCB_VIA;
class PAD;
class PCB_SELECTION;
class PCB_TARGET;
class PCB_GROUP;
class PCB_DIMENSION_BASE;
class DRC;
class DIALOG_FIND;
class DIALOG_FIND_BY_PROPERTIES;
class DIALOG_PLOT;
class ZONE;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class SELECTION;
class PCB_MARKER;
class BOARD_ITEM;
class NETLIST;
class REPORTER;
struct PARSE_ERROR;
class IO_ERROR;
class FP_LIB_TABLE;
class BOARD_NETLIST_UPDATER;
class ACTION_MENU;
class TOOL_ACTION;
class DIALOG_BOARD_SETUP;
class DIALOG_FOOTPRINT_FIELDS_TABLE;
class PCB_DESIGN_BLOCK_PANE;
class PANEL_CONSTRAINTS;
class WX_INFOBAR;
struct PLUGIN_ACTION;

class KICAD_API_SERVER;
class API_HANDLER_PCB;
class API_HANDLER_COMMON;
class API_HANDLER_LIBRARIES;

enum LAST_PATH_TYPE : unsigned int;

namespace PCB { struct IFACE; }     // KIFACE is in pcbnew.cpp

wxDECLARE_EVENT( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED, wxCommandEvent );

/**
 * The main frame for Pcbnew.
 *
 * See also class PCB_BASE_FRAME(): Basic class for Pcbnew and GerbView.
 */
class PCB_EDIT_FRAME : public PCB_BASE_EDIT_FRAME, public BOARD_LISTENER
{
public:
    virtual ~PCB_EDIT_FRAME();

<<<<<<< b2e17e0a
    /**
     * Load the footprints for each #SCH_COMPONENT in \a aNetlist from the list of libraries.
     *
     * @param aNetlist is the netlist of components to load the footprints into.
     * @param aReporter is the #REPORTER object to report to.
     * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
     *           occurs while reading footprint library files.
     */
    void LoadFootprints( NETLIST& aNetlist, REPORTER& aReporter );

    /// Close the frame (@a aCommandId wxID_CLOSE) or quit KiCad (wxID_EXIT).
    void OnQuit( int aCommandId );
||||||| 1304b76aad
    /**
     * Load the footprints for each #SCH_COMPONENT in \a aNetlist from the list of libraries.
     *
     * @param aNetlist is the netlist of components to load the footprints into.
     * @param aReporter is the #REPORTER object to report to.
     * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
     *           occurs while reading footprint library files.
     */
    void LoadFootprints( NETLIST& aNetlist, REPORTER& aReporter );

    void OnQuit( wxCommandEvent& event );
=======
    void OnQuit( wxCommandEvent& event );
>>>>>>> ed8c16703e

    /**
     * Get if the current board has been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Update the layer manager and other widgets from the board setup
     * (layer and items visibility, colors ...)
     */
    void UpdateUserInterface();

    void HardRedraw() override;

    /**
     * Execute a remote command send by Eeschema via a socket, port KICAD_PCB_PORT_SERVICE_NUMBER
     * (currently 4242).
     *
     * This is a virtual function called by EDA_DRAW_FRAME::OnSockRequest().
     *
     * @param cmdline is the received command from socket.
     */
    void ExecuteRemoteCommand( const char* cmdline ) override;

    void HandleRemoteNetHighlight( const std::vector<wxString>& aNetNames );

    void KiwayMailIn( KIWAY_MAIL_EVENT& aEvent ) override;

    void      SetLastSchematicSheetPath( const KIID_PATH& aPath );
    KIID_PATH GetLastSchematicSheetPath() const { return m_lastSchematicSheetPath; }

    /**
     * @return the name of the wxAuiPaneInfo managing the Search panel
     */
    static const wxString SearchPaneName() { return wxT( "Search" ); }
    static const wxString ConstraintsPaneName() { return wxT( "Constraints" ); }

    /// Show/hide the dockable geometric-constraint list pane, refreshing it when shown (#2329).
    void ToggleConstraintsPanel();

    /// The dockable geometric-constraint list pane (#2329), or nullptr.
    PANEL_CONSTRAINTS* GetConstraintsPanel() const { return m_constraintsPanel; }

    /**
     * Show the Find dialog.
     */
    void ShowFindDialog();

    /**
     * Show the Find by Properties dialog.
     */
    void ShowFindByPropertiesDialog();

    /**
     * Notify the Find by Properties dialog that the selection has changed.
     */
    void NotifyFindByPropertiesDialog();

    /**
     * Find the next item using our existing search parameters.
     */
    void FindNext( bool reverse = false );

    bool LayerManagerShown();
    bool PropertiesShown();
    bool NetInspectorShown();

    /// Show the current track width and via size in the choice boxes of the toolbar.
    void OnUpdateSelectViaSize();
    void OnUpdateSelectTrackWidth();

    void UpdateTrackWidthSelectBox( TOOLKIT_CHOICE* aTrackWidthSelectBox, bool aShowNetclass,
                                    bool aShowEdit );
    void UpdateViaSizeSelectBox( TOOLKIT_CHOICE* aViaSizeSelectBox, bool aShowNetclass,
                                 bool aShowEdit );

    /**
     * Fill the via stack preset selector with the board's defined presets plus an edit entry.
     */
    void UpdateViaStackSelectBox( wxChoice* aViaStackSelectBox );

    /**
     * Update the variant selection dropdown with the current board's variant names.
     *
     * If the currently selected variant is no longer available, the default (no variant)
     * will be selected.
     */
    void UpdateVariantSelectionCtrl();

    /**
     * Set the current variant on the board and update the drawing sheet's cached
     * variant name and description accordingly.
     */
    void SetCurrentVariant( const wxString& aVariantName );

    /**
     * Event handler for variant selection changes in the toolbar.
     */
    void onVariantSelected();

    /**
     * Return the angle used for rotate operations.
     */
    EDA_ANGLE GetRotationAngle() const override;

    /**
     * @return the color of the grid
     */
    COLOR4D GetGridColor() override;

    /**
     * @param[in] aColor the new color of the grid.
     */
    void SetGridColor( const COLOR4D& aColor ) override;

    /**
     * Return true if button visibility action plugin setting was set to true
     * or it is unset and plugin defaults to true.
     */
    static bool GetPluginActionButtonVisible( const wxString& aPluginPath, bool aPluginDefault );

    /**
     * Return ordered list of plugins in sequence in which they should appear on toolbar or
     * in settings.
     */
    static std::vector<const PLUGIN_ACTION*> GetOrderedPluginActions();

    void SaveProjectLocalSettings() override;

    /**
     * Load the current project's file configuration settings which are pertinent
     * to this PCB_EDIT_FRAME instance.
     *
     * @return always returns true.
     */
    bool LoadProjectSettings();

    /// Apply what the project file sets and this frame caches, leaving the local settings alone.
    void ApplyProjectFileSettings();

    /**
     * Bring the frame in line with a project file that was reread from disk.
     *
     * Every value this frame copies out of the project file and writes back on save or close must be
     * refreshed here; a copy left stale silently undoes the new file on the next save.
     */
    void ReloadProjectFileSettings();

    /// Update what depends on the board's project settings after they changed in memory.
    void RefreshProjectSettings();

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Load the drawing sheet file.
     */
    void LoadDrawingSheet();

    /**
     * Get the last path for a particular type.
     *
     * @return the absolute path and file name of the last file successfully read.
     */
    wxString GetLastPath( LAST_PATH_TYPE aType );

    /**
     * Set the path of the last file successfully read.
     *
     * @note the file path is converted to a path relative to the project file path.  If
     *       the path cannot be made relative, than m_lastNetListRead is set to and empty
     *       string.  This could happen when the net list file is on a different drive than
     *       the project file.  The advantage of relative paths is that is more likely to
     *       work when opening the same project from both Windows and Linux.
     *
     * @param aType
     * @param aLastPath is the last file with full path successfully read.
     */
    void SetLastPath( LAST_PATH_TYPE aType, const wxString& aLastPath );

    /**
     * If aCreateMarkers then create DRC exclusion markers from the serialized data.  If false,
     * then use the serialized data to set exclusion flags on existing markers.
     */
    void ResolveDRCExclusions( bool aCreateMarkers );

    void Process_Special_Functions( int aCommandId );

    /// Select a track width or via size (the command @a aCommandId of a menu or of a choice
    /// box of the toolbar).
    void Tracks_and_Vias_Size_Event( int aCommandId );

    /**
     * Handle the via stack preset selector on the auxiliary toolbar.
     */
    void SelectViaStack_Event( wxCommandEvent& event );


    /**
     * Recreate the layer box by clearing the old list and building a new one from the new
     * layer names and colors.
     *
     * @param aForceResizeToolbar true to resize the parent toolbar false if not needed (mainly
     *                            in parent toolbar creation, or when the layers names are not
     *                            modified)
     */
    void ReCreateLayerBox( bool aForceResizeToolbar = true );


    /**
     * Must be called after a board change to set the modified flag.
     *
     * Reload the 3D view if required and calls the base PCB_BASE_FRAME::OnModify function
     * to update auxiliary information.
     */
    void OnModify() override;

    /**
     * Keep the selection and view in step with generated tables rebuilt outside a commit.
     */
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;

    /**
     * Change the currently active layer to \a aLayer and also update the #APPEARANCE_CONTROLS.
     */
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override
    {
        SetActiveLayer( aLayer, false );
    }

    /**
     * @param aLayer is the layer to set active
     * @param aForceRedraw will repaint things that depend on layer switch even if the new active
     *                     layer is the same as the previous one
     */
    void SetActiveLayer( PCB_LAYER_ID aLayer, bool aForceRedraw );

    void OnDisplayOptionsChanged() override;

    /**
     * Test whether a given element category is visible. Keep this as an inline function.
     *
     * @param aElement is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum GAL_LAYER_ID
     */
    bool IsElementVisible( GAL_LAYER_ID aElement ) const;

    /**
     * Change the visibility of an element category.
     *
     * @param aElement is from the enum by the same name.
     * @param aNewState The new visibility state of the element category.
     * @see enum PCB_LAYER_ID.
     */
    void SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState );

    /// @copydoc EDA_DRAW_FRAME::ActivateGalCanvas()
    void ActivateGalCanvas() override;

    /**
     * Tell every hole to redraw its drill symbol.
     *
     * A cached GAL group replays until its own item is updated, so changing a map or a
     * symbol assignment is invisible until the holes themselves are refreshed.
     */
    void RefreshDrillSymbols( int aUpdateFlags ) override;

    void ShowBoardSetupDialog( const wxString& aInitialPage = wxEmptyString, wxWindow* aParent = nullptr );

    void PrepareLayerIndicator( bool aForceRebuild = false );

    void ToggleLayersManager();

    void ToggleNetInspector();

    void ToggleSearch();

    bool IsSearchPaneShown() { return IsPaneShown( SearchPaneName() ); }
    void FocusSearch();

    void ToggleLibraryTree() override;

    /**
     * Create an ASCII footprint position file.
     *
     * @param aFullFileName the full file name of the file to create.
     * @param aUnitsMM false to use inches, true to use mm in coordinates.
     * @param aOnlySMD true to force only footprints flagged smd to be in the list
     * @param aNoTHItems true to include only footprints with no TH pads no matter
     *                   the footprint flag
     * @param aExcludeDNP true to exclude footprints flagged DNP
     * @param aExcludeBOM true to exclude footprints flagged exclude from BOM
     * @param aTopSide true to list footprints on front (top) side.
     * @param aBottomSide true to list footprints on back (bottom) side, if \a aTopSide and
     *                    \a aTopSide are true, list footprints on both sides.
     * @param aFormatCSV true to use a comma separated file (CSV) format; default = false
     * @param aUseAuxOrigin true to use auxiliary axis as an origin for the position data
     * @param aNegateBottomX true to negate X coordinates for bottom side of the placement file
     * @return the number of footprints found on aSide side or -1 if the file could not be created.
     */
    int DoGenFootprintsPositionFile( const wxString& aFullFileName, bool aUnitsMM, bool aOnlySMD,
                                     bool aNoTHItems, bool aExcludeDNP, bool aExcludeBOM, bool aTopSide,
                                     bool aBottomSide, bool aFormatCSV, bool aUseAuxOrigin,
                                     bool aNegateBottomX );

    /// Open the file of the file history menu item @a aCommandId.
    void OnFileHistory( int aCommandId );
    void OnClearFileHistory();

    bool SaveBoard( bool aSaveAs = false, bool aSaveCopy = false );

    /**
     * Load a KiCad board (.kicad_pcb) from \a aFileName.
     *
     * @param aFileSet hold the BOARD file to load, a vector of one element.
     * @param aCtl KICTL_ bits, one to indicate that an append of the board file
     *             \a aFileName to the currently loaded file is desired.
     *             @see #KIWAY_PLAYER for bit defines.
     *
     * @return false if file load fails, otherwise true.
    bool LoadOnePcbFile( const wxString& aFileName, bool aAppend = false,
                         bool aForceFileDialog = false );
     */
    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Reload the board from disk.
     * The caller handles confirmation before unsaved edits are discarded.
     *
     * @return false when there is no filename or the reload fails.
     */
    bool ReloadFromDisk();

    /**
     * Write the board data structures to \a a aFileName.
     *
     * Create a backup when requested and update flags (modified and saved flags).
     *
     * @param aFileName The file name to write or wxEmptyString to prompt user for
     *                  file name.
     * @param addToHistory controls whether or not to add the saved file to the recent file list
     * @param aChangeProject is true if the project should be changed to the new board filename
     * @return True if file was saved successfully.
     */
    bool SavePcbFile( const wxString& aFileName, bool addToHistory = true,
                      bool aChangeProject = true );

    /**
     * Write the board data structures to \a aFileName.
     *
     * Unlike SavePcbFile, does not make anything else (no backup, broad filename change, no
     * flag changes ...).  Used under a project mgr to save under a new name the current board.
     * When not under a project mgr, the full SavePcbFile is used.
     *
     * @param aFileName The file name to write.
     * @param aCreateProject will create an empty project alongside the board file
     * @param aHeadless will suppress informational output (e.g. to be used from the API)
     * @return True if file was saved successfully.
     */
    bool SavePcbCopy( const wxString& aFileName, bool aCreateProject = false,
                      bool aHeadless = false );

    /**
     * Delete all and reinitialize the current board.
     *
     * @param doAskAboutUnsavedChanges true to prompt user if existing board contains unsaved
     *                                 changes, false to re-initialize silently.
     * @param aFinal if true, we are clearing the board to exit, so don't run more events.
     */
    bool Clear_Pcb( bool doAskAboutUnsavedChanges, bool aFinal = false );

    /// @copydoc PCB_BASE_FRAME::SetBoard()
    void SetBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr ) override
    {
        SetBoard( aBoard, true, aReporter );
    }

    void SetBoard( BOARD* aBoard, bool aBuildConnectivity, PROGRESS_REPORTER* aReporter = nullptr );

    /// @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    std::unique_ptr<GRID_HELPER> MakeGridHelper() override;

    /// @copydoc PCB_BASE_FRAME::SetPageSettings()
    void SetPageSettings( const PAGE_INFO& aPageSettings ) override;

    bool SaveBoardAsDesignBlock( const wxString& aLibraryName );

    bool SaveSelectionAsDesignBlock( const wxString& aLibraryName );

    bool UpdateDesignBlockFromBoard( const LIB_ID& aLibId );

    bool UpdateDesignBlockFromSelection( const LIB_ID& aLibId );

    PCB_DESIGN_BLOCK_PANE* GetDesignBlockPane() const { return m_designBlocksPane; }

    /**
     * Save footprints in a library:
     *
     * @param aStoreInNewLib true to save footprints in a existing library. Existing footprints
     *                       will be kept or updated.  This library should be in fp lib table,
     *                       and is type is .pretty. False to save footprints in a new library.
     *                       If it is an existing lib, previous footprints will be removed.
     * @param aLibName optional library name to create, stops dialog call. Must be called with
     *                 \a aStoreInNewLib as true.
     * @param aLibPath is the path to export the library to.
     */
    void ExportFootprintsToLibrary( bool aStoreInNewLib );

    /**
     * Create the file(s) exporting current BOARD to a VRML file.
     *
     * @note When copying 3D shapes files, the new filename is build from the full path
     *       name, changing the separators by underscore.  This is needed because files
     *       with the same shortname can exist in different directories
     * @note ExportVRML_File generates coordinates in board units (BIU) inside the file.
     * @todo Use mm inside the file.  A general scale transform is applied to the whole
     *       file (1.0 to have the actual WRML unit im mm, 0.001 to have the actual WRML
     *       unit in meters.
     * @note For 3D models built by a 3D modeler, the unit is 0,1 inches.  A specific scale
     *       is applied to 3D models to convert them to internal units.
     *
     * @param aFullFileName the full filename of the file to create
     * @param aMMtoWRMLunit the VRML scaling factor: 1.0 to export in mm. 0.001 for meters
     * @param aIncludeUnspecified
     * @param aIncludeDNP include DNP footprint models on export.
     * @param aExport3DFiles true to copy 3D shapes in the subir a3D_Subdir
     * @param aUseRelativePaths set to true to use relative paths instead of absolute paths
     *                          in the board VRML file URLs.
     * @param a3D_Subdir sub directory where 3D shapes files are copied.  This is only used
     *                   when aExport3DFiles == true.
     * @param aXRef X value of PCB (0,0) reference point.
     * @param aYRef Y value of PCB (0,0) reference point.
     * @return true if Ok.
     */
    bool ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                          bool aIncludeUnspecified, bool aIncludeDNP,
                          bool aExport3DFiles, bool aUseRelativePaths,
                          const wxString& a3D_Subdir, double aXRef, double aYRef );

    /**
     * Export the current BOARD to a specctra dsn file.
     *
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
     *
     * @return true if OK
     */
    bool ExportSpecctraFile( const wxString& aFullFilename );

    /**
     * Import a specctra *.ses file and use it to relocate footprints and to replace all vias and
     * tracks in an existing and loaded #BOARD.
     *
     * Changes are committed through #BOARD_COMMIT so they participate in undo/redo and refresh
     * the canvas view (including moved footprints).
     *
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
     */
    bool ImportSpecctraSession( const wxString& aFullFilename );

    // Footprint editing (see also PCB_BASE_FRAME)
    void ShowFootprintPropertiesDialog( FOOTPRINT* aFootprint );

    int ShowExchangeFootprintsDialog( FOOTPRINT* aFootprint, bool aUpdateMode, bool aSelectedMode );

    /**
     * Install the corresponding dialog editor for the given item.
     *
     * @param aItem a pointer to the BOARD_ITEM to edit.
     */
    void OnEditItemRequest( BOARD_ITEM* aItem ) override;

    /**
     * Change the active layer in the editor
     *
     * @param layer New layer to make active
     */
    void SwitchLayer( PCB_LAYER_ID layer ) override;

    /**
     * Modify one track segment width or one via diameter (using DRC control).
     *
     * Basic routine used by other routines when editing tracks or vias.
     * Note that casting this to boolean will allow you to determine whether any action
     * happened.
     *
     * @param aItem the track segment or via to modify.
     * @param aItemsListPicker the list picker to use for an undo command (can be NULL).
     * @param aUseDesignRules true to use design rules value, false to use current designSettings
     *                        value.
     */
    void SetTrackSegmentWidth( PCB_TRACK* aItem, PICKED_ITEMS_LIST* aItemsListPicker,
                               bool aUseDesignRules );


    /**
     * Edit params (layer, clearance, ...) for a zone outline.
     */
    void Edit_Zone_Params( ZONE* zone_container );

    // Properties dialogs
    void ShowTargetOptionsDialog( PCB_TARGET* aTarget );
    void InstallNetlistFrame();

    /**
     * @param aNetlist a #NETLIST owned by the caller.  This function fills it in.
     * @param aAnnotateMessage a message to be shown if annotation must be performed.  If empty,
     *                         annotation will be skipped.
     * @return true if a netlist was fetched.
     */
    bool FetchNetlistFromSchematic( NETLIST& aNetlist, const wxString& aAnnotateMessage );

    /**
     * Test if standalone mode.
     *
     * @return 0 if in standalone, -1 if Eeschema cannot be opened,
     * -2 if the schematic cannot be opened and 1 if OK.
     * If OK, opens Eeschema, and opens the schematic for this project
     */
    int TestStandalone();

    /**
     * Read a netlist from a file into a #NETLIST object.
     *
     * @param aFilename is the netlist to load.
     * @param aNetlist is the object to populate with data.
     * @param aReporter is a #REPORTER object to display messages.
     * @return true if the netlist was read successfully.
     */
    bool ReadNetlistFromFile( const wxString& aFilename, NETLIST& aNetlist, REPORTER& aReporter );

    /**
     * Called after netlist is updated.
     *
     * @param aUpdater is the updater object that was run.
     * @param aRunDragCommand [out] is set to true if a drag command should be run following this call.
     */
    void OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater, bool* aRunDragCommand );

    /**
     * Send a message to the schematic editor to try to find schematic counterparts
     * of specified PCB items and select them.
     *
     * @param aItems are the items to try to select on schematic.
     * @param aFocusItem set to item to select and focus on even if selection can't be
     *                   represented in Schematic editor fully.
     * @param aForce select elements in Schematic editor whether or not the user has
     *               the select option chosen.
     */
    void SendSelectItemsToSch( const std::deque<EDA_ITEM*>& aItems, EDA_ITEM* aFocusItem,
                               bool aForce );

    /**
     * Send a message to the schematic editor so that it may move its cursor
     * to an item with the same reference as the \a aSyncItem and highlight it.
     *
     * @param aSyncItem The object whose reference is used to highlight in Eeschema.
     */
    void SendCrossProbeItem( BOARD_ITEM* aSyncItem );

    /**
     * Send a net name to Eeschema for highlighting.
     *
     * @param aNetName is the name of a net, or empty string to clear highlight.
     */
    void SendCrossProbeNetName( const wxString& aNetName );

    void ShowChangedLanguage() override;

    /**
     * Update the state of the GUI after a new board is loaded or created.
     */
    void OnBoardLoaded();

    /**
     * Set the main window title bar text.
     *
     * If file name defined by PCB_SCREEN::m_FileName is not set, the title is set to the
     * application name appended with no file.  Otherwise, the title is set to the full path
     * and file name and read only is appended to the title if the user does not have write
     * access to the file.
     */
    void UpdateTitle();

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Called when light/dark theme is changed.
     */
    void ThemeChanged() override;

    void ProjectChanged() override;

    bool CanAcceptApiCommands() override;

    wxString GetCurrentFileName() const override;

    SELECTION& GetCurrentSelection() override;

    TOOL_ACTION* GetExportNetlistAction() { return m_exportNetlistAction; }

    DIALOG_BOOK_REPORTER* GetInspectDrcErrorDialog();

    DIALOG_BOOK_REPORTER* GetInspectConstraintsDialog();

    DIALOG_BOOK_REPORTER* GetInspectClearanceDialog();

    DIALOG_BOOK_REPORTER* GetFootprintDiffDialog();

    DIALOG_FOOTPRINT_FIELDS_TABLE* GetFootprintFieldsTableDialog();
    bool                           CloseFootprintFieldsTableDialog();

    /**
     * Perform auto save when the board has been modified and not saved within the
     * auto save interval.
     *
     * @return true if the auto save was successful.
     */
    bool DoAutoSave();

    void ClearToolbarControl( int aId ) override;

protected:
    /**
     * Store the previous layer toolbar icon state information
     */
    struct LAYER_TOOLBAR_ICON_VALUES
    {
        int     previous_icon_size;
        COLOR4D previous_Route_Layer_TOP_color;
        COLOR4D previous_Route_Layer_BOTTOM_color;
        COLOR4D previous_background_color;

        LAYER_TOOLBAR_ICON_VALUES()
                : previous_icon_size( 0 ),
                  previous_Route_Layer_TOP_color( COLOR4D::UNSPECIFIED ),
                  previous_Route_Layer_BOTTOM_color( COLOR4D::UNSPECIFIED ),
                  previous_background_color( COLOR4D::UNSPECIFIED )
        {
        }
    };

    LAYER_TOOLBAR_ICON_VALUES m_prevIconVal;

    void doReCreateMenuBar() override;

    void configureToolbars() override;

    void ReCreateAuxiliaryToolbar() override;

    // The Tool Framework initialization
    void setupTools();
    void setupUIConditions() override;

    /**
     * Switch currently used canvas (Cairo / OpenGL).
     *
     * It also reinit the layers manager that slightly changes with canvases
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

<<<<<<< b2e17e0a
    /**
     * Fill action menu with all registered action plugins
     */
    void buildActionPluginMenus( ACTION_MENU* aActionMenu );

    /**
     * Append action plugin buttons to given toolbar
     */
    void addActionPluginTools( ACTION_TOOLBAR* aToolbar );

    /**
     * Execute action plugin's Run() method and updates undo buffer.
     *
     * @param aActionPlugin action plugin
     */
    void RunActionPlugin( ACTION_PLUGIN* aActionPlugin );

    /**
     * Launched by the menu when an action is called.
     *
     * @param aId is the id of the menu item.
     */
    void OnActionPluginMenu( int aId );

    /**
     * Launched by the button when an action is called.
     *
     * @param aId is the id of the button.
     */
    void OnActionPluginButton( int aId );

||||||| 1304b76aad
    /**
     * Fill action menu with all registered action plugins
     */
    void buildActionPluginMenus( ACTION_MENU* aActionMenu );

    /**
     * Append action plugin buttons to given toolbar
     */
    void addActionPluginTools( ACTION_TOOLBAR* aToolbar );

    /**
     * Execute action plugin's Run() method and updates undo buffer.
     *
     * @param aActionPlugin action plugin
     */
    void RunActionPlugin( ACTION_PLUGIN* aActionPlugin );

    /**
     * Launched by the menu when an action is called.
     *
     * @param aEvent sent by wx
     */
    void OnActionPluginMenu( wxCommandEvent& aEvent);

    /**
     * Launched by the button when an action is called.
     *
     * @param aEvent sent by wx
     */
    void OnActionPluginButton( wxCommandEvent& aEvent );

=======
>>>>>>> ed8c16703e
    PLUGIN_ACTION_SCOPE PluginActionScope() const override { return PLUGIN_ACTION_SCOPE::PCB; }

    /**
     * Perform auto save when the board has been modified and not saved within the
     * auto save interval.
     *
     * @return true if the auto save was successful.
     */
    bool doAutoSave() override { return DoAutoSave(); }

    bool canRunAutoSave() const override;

    /**
     * Return true when an interactive tool operation (routing, dragging, point editing,
     * zone filling, or a blocked undo/redo) is currently in progress.  Used to gate both
     * API command acceptance and autosave so neither stomps on a live edit.
     */
    bool interactiveOperationInProgress() const;

    /**
     * Load the given filename but sets the path to the current project path.
     *
     * @param aFileName full file path of file to be imported.
     * @param aFileType PCB_FILE_T value for file type.
     * @param aProperties is a list of import properties.
     */
    bool importFile( const wxString& aFileName, int aFileType,
                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Reconcile the footprint-library references of a freshly imported non-KiCad board so that
     * every board footprint FPID resolves to a registered project library.  Reads the generated
     * cache nickname and provenance source libraries from m_importProperties, falling back to a
     * nickname derived from the board filename for a standalone import.
     *
     * @param aDefinitions are the importer's caller-owned cached library footprints, captured
     *                     during load before the plugin was destroyed.
     * @param aBoardPath
     */
    void reconcileImportedFootprintLibraries(
            std::vector<std::unique_ptr<FOOTPRINT>> aDefinitions, const wxString& aBoardPath );

    /**
     * @brief Save a board object to a file
     *
     * @param aBoard The board object to save
     * @param aFileName The file name to save the board to
     * @param aHeadless If true, suppresses informational output (e.g. to be used from the API)
     *
     * @return
     */
    bool saveBoardAsFile( BOARD& aBoard, const wxString& aFileName, bool aHeadless = false );

    bool saveSelectionToDesignBlock( const wxString& aNickname, PCB_SELECTION& aSelection, DESIGN_BLOCK& aBlock );


    bool canCloseWindow( CLOSE_REQUEST& aRequest ) override;
    void doCloseWindow() override;

    // protected so that PCB::IFACE::CreateWindow() is the only factory.
    PCB_EDIT_FRAME( KIWAY* aKiway, TOOLKIT_WINDOW* aParent );

<<<<<<< b2e17e0a
    /// Zoom to fit the board once the frame has its final size (shown on screen).
    void onSize();

||||||| 1304b76aad
    void onSize( wxSizeEvent& aEvent );

=======
>>>>>>> ed8c16703e
    int inferLegacyEdgeClearance( BOARD* aBoard, bool aShowUserMsg = true );

    void redrawNetnames();

    /// Replace the WRL 3D models by their STEP siblings, and offer the options for the others
    /// (pcb_edit_frame_dialogs.cpp).
    void checkWrlModels();

    /// Destroy the modeless dialogs (find, reports) of the frame (pcb_edit_frame_dialogs.cpp).
    void destroyModelessDialogs();

    void saveProjectSettings() override;

<<<<<<< b2e17e0a
    /**
     * A modeless report dialog was closed (with the button @a aId).
     *
     * @param aDialogName is the name of the dialog.
     */
    void onCloseModelessBookReporterDialogs( const wxString& aDialogName, int aId );
||||||| 1304b76aad
    void onCloseModelessBookReporterDialogs( wxCommandEvent& aEvent );
=======
    void onCloseModelessBookReporterDialogs( wxCommandEvent& aEvent );
    void onCloseFootprintFieldsTableDialog( wxCommandEvent& aEvent );
>>>>>>> ed8c16703e

<<<<<<< b2e17e0a
#ifdef KICAD_IPC_API
    void onPluginAvailabilityChanged();
#endif
||||||| 1304b76aad
#ifdef KICAD_IPC_API
    void onPluginAvailabilityChanged( wxCommandEvent& aEvt );
#endif
=======
    void onPluginAvailabilityChanged( wxCommandEvent& aEvt );
>>>>>>> ed8c16703e

private:
    /// Dock the panels of the frame around the canvas (shown as in the settings).
    void setupLayout();

    /// Bind the events of the GUI toolkit (menu commands, choice boxes, size, idle).
    void setupToolkitEvents();
    void releaseToolkitEvents();

    /// Show the event counters of the canvas every second (an advanced option).
    void showEventCounters();

    /// The state of the frame with the GUI toolkit (its event handlers), defined with the
    /// toolkit parts of the frame.
    struct TOOLKIT_STATE;

    struct TOOLKIT_STATE_DELETER
    {
        void operator()( TOOLKIT_STATE* aState ) const;
    };

    std::unique_ptr<TOOLKIT_STATE, TOOLKIT_STATE_DELETER> m_toolkitState;

    /// A track width or via size was chosen in the choice box of the toolbar.
    void onTrackWidthChoice();
    void onViaSizeChoice();

public:
<<<<<<< b2e17e0a
    TOOLKIT_CHOICE* m_SelTrackWidthBox;    // a choice box to display and select current track width
    TOOLKIT_CHOICE* m_SelViaSizeBox;       // a choice box to display and select current via diameter
    TOOLKIT_CHOICE* m_CurrentVariantCtrl;  // a choice box to display and select current variant
||||||| 1304b76aad
    wxChoice* m_SelTrackWidthBox;        // a choice box to display and select current track width
    wxChoice* m_SelViaSizeBox;           // a choice box to display and select current via diameter
    wxChoice* m_CurrentVariantCtrl;      // a choice box to display and select current variant
=======
    wxChoice* m_SelTrackWidthBox;        // a choice box to display and select current track width
    wxChoice* m_SelViaSizeBox;           // a choice box to display and select current via diameter
    wxChoice* m_SelViaStackBox;          // a choice box to display and select current via stack preset
    wxChoice* m_CurrentVariantCtrl;      // a choice box to display and select current variant
>>>>>>> ed8c16703e

    bool      m_ShowLayerManagerTools;
    bool      m_ShowSearch;
    bool      m_ShowNetInspector;

    bool      m_ZoneFillsDirty;          // Board has been modified since last zone fill.

    bool      m_ProbingSchToPcb;         // Recursion guard when synchronizing selection from schematic

    /// Reactive text-var invalidation listener state. The handle alone is
    /// ambiguous across board/project swaps — the tracker it belongs to must
    /// be remembered so the destructor (and tracker-change detection)
    /// removes from the correct tracker, not whatever GetBoard() points at
    /// post-swap. Handle == 0 means not installed.
    std::size_t             m_textVarListenerHandle = 0;
    class TEXT_VAR_TRACKER* m_textVarListenerTracker = nullptr;

    /**
     * Drop every cached reference into the current BOARD's text-var tracker.
     * Must run before the BOARD is freed (SetBoard replacement or frame
     * teardown).
     */
    void detachTextVarTracker();

    void StartCrossProbeFlash( const std::vector<BOARD_ITEM*>& aItems );
<<<<<<< b2e17e0a
    void OnCrossProbeFlashTimer();
||||||| 1304b76aad
    void OnCrossProbeFlashTimer( wxTimerEvent& aEvent );
=======
    void OnCrossProbeFlashTimer( wxTimerEvent& aEvent );
    void UpdateProperties() override;
>>>>>>> ed8c16703e

private:
    /// Recompile the design rules, reporting a parse error in the infobar.
    void recompileDesignRules();

    /// Add the markers a resolve created for exclusions and redraw every excluded marker.
    void addExclusionMarkers( const std::vector<PCB_MARKER*>& aMarkers );

    friend struct PCB::IFACE;
    friend class APPEARANCE_CONTROLS;

    /**
     * The export board netlist tool action object.
     *
     * This is created at runtime rather than declared statically so it doesn't show up in
     * the list of assignable hot keys since it's only available as an advanced configuration
     * option.
     */
    TOOL_ACTION* m_exportNetlistAction;

    DIALOG_FIND*               m_findDialog;
    DIALOG_FIND_BY_PROPERTIES* m_findByPropertiesDialog;
    DIALOG_BOOK_REPORTER*      m_inspectDrcErrorDlg;
    DIALOG_BOOK_REPORTER*      m_inspectClearanceDlg;
    DIALOG_BOOK_REPORTER*      m_inspectConstraintsDlg;
    DIALOG_BOOK_REPORTER*      m_footprintDiffDlg;
    DIALOG_BOARD_SETUP*        m_boardSetupDlg;
    DIALOG_FOOTPRINT_FIELDS_TABLE* m_footprintFieldsTableDialog;

    std::vector<LIB_ID>    m_designBlockHistoryList;
    PCB_DESIGN_BLOCK_PANE* m_designBlocksPane;
    // Tool Reset() reads this before the ctor creates the panel.
    PANEL_CONSTRAINTS* m_constraintsPanel = nullptr; ///< Dockable geometric-constraint list (#2329).

    /// Secondary infobar that stacks above the main one; reserved for load-time
    /// notices (currently the WRL -> STEP migration prompt) that must not be
    /// stomped by later infobar messages such as read-only warnings or DRC
    /// rule errors.
    WX_INFOBAR*            m_loadNoticeInfoBar = nullptr;

    const std::map<std::string, UTF8>* m_importProperties; // Properties used for non-KiCad import.

    /**
     * Keep track of viewport so that track net labels can be adjusted when it changes.
     */
    BOX2D             m_lastNetnamesViewport;
    double            m_lastNetnamesScale = 0.0;

    std::unique_ptr<GUI_TIMER> m_eventCounterTimer;

    // Cross-probe flashing support
    GUI_TIMER         m_crossProbeFlashTimer;          ///< Timer to toggle selection visibility for flash
    int               m_crossProbeFlashPhase = 0;      ///< Phase counter
    std::vector<KIID> m_crossProbeFlashItems;          ///< Items to flash (by UUID)
    bool              m_crossProbeFlashing = false;    ///< Currently flashing guard

    // Most recent schematic path (default to root), used in the footprint fields
    // table to enable the same schematic scope limiting as the symbol fields table
    KIID_PATH         m_lastSchematicSheetPath;

    std::unique_ptr<API_HANDLER_PCB>    m_apiHandler;
    std::unique_ptr<API_HANDLER_COMMON> m_apiHandlerCommon;
<<<<<<< b2e17e0a

    /// The id of the listener of the plugin availability (-1 for none)
    int m_pluginAvailabilityListener = -1;
#endif
||||||| 1304b76aad
#endif
=======
    std::unique_ptr<API_HANDLER_LIBRARIES> m_apiLibrariesHandler;
>>>>>>> ed8c16703e
};

#endif  // __PCB_EDIT_FRAME_H__
