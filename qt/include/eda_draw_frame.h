/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file eda_draw_frame.h
 * EDA_DRAW_FRAME of the Qt build: the interface of include/eda_draw_frame.h, on the Qt
 * KIWAY_PLAYER (a QMainWindow).
 */

#ifndef DRAW_FRAME_H_
#define DRAW_FRAME_H_

#include <api/plugin_action_scope.h>
#include <eda_base_frame.h>
#include <msg_panel_context.h>
#include <kiway_player.h>
#include <gal/gal_display_options.h>
#include <gal_display_options_common.h>
#include <gal/color4d.h>
#include <class_draw_panel_gal.h>
#include <kiid.h>
#include <hotkeys_basic.h>
#include <lib_id.h>

class QLabel;
class wxFileName;
class EDA_ITEM;
class EDA_MSG_PANEL;
class GRID_HELPER;
class COLOR_SETTINGS;
class LOCKFILE;
class TOOL_MENU;
class APP_SETTINGS_BASE;
class LIB_TREE;
class PROPERTIES_PANEL;
class PAGE_INFO;
class TITLE_BLOCK;
struct EDA_SEARCH_DATA;
enum class BITMAP_TYPE;

namespace KIGFX
{
    class GAL_DISPLAY_OPTIONS;
    class RENDER_SETTINGS;
}

using KIGFX::COLOR4D;
using KIGFX::RENDER_SETTINGS;

#define LIB_EDIT_FRAME_NAME           wxT( "LibeditFrame" )
#define LIB_VIEW_FRAME_NAME           wxT( "ViewlibFrame" )
#define SCH_EDIT_FRAME_NAME           wxT( "SchematicFrame" )
#define SYMBOL_CHOOSER_FRAME_NAME     wxT( "SymbolChooserFrame" )
#define PL_EDITOR_FRAME_NAME          wxT( "PlEditorFrame" )
#define FOOTPRINT_WIZARD_FRAME_NAME   wxT( "FootprintWizard" )
#define FOOTPRINT_CHOOSER_FRAME_NAME  wxT( "FootprintChooserFrame" )
#define FOOTPRINT_EDIT_FRAME_NAME     wxT( "ModEditFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME   wxT( "ModViewFrame" )
#define PCB_EDIT_FRAME_NAME           wxT( "PcbFrame" )


/**
 * The message panel context of a frame: its units, its origin and its items.
 */
class FRAME_MSG_PANEL_CONTEXT : public MSG_PANEL_CONTEXT
{
public:
    explicit FRAME_MSG_PANEL_CONTEXT( EDA_DRAW_FRAME* aFrame );

    void               GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits ) override;
    ORIGIN_TRANSFORMS& GetOriginTransforms() override;
    FRAME_T            GetFrameType() const override;
    wxString           EllipsizeStatusText( const wxString& aText ) const override;
    EDA_ITEM*          ResolveItem( const KIID& aId, bool aAllowNullptrReturn = false ) const override;

    EDA_DRAW_FRAME* GetFrame() const { return m_frame; }

private:
    EDA_DRAW_FRAME* m_frame;
};


/**
 * The base class for create windows for drawing purpose.  The Eeschema, Pcbnew and
 * GerbView main windows are just a few examples of classes derived from EDA_DRAW_FRAME.
 */
class EDA_DRAW_FRAME : public KIWAY_PLAYER
{
public:
    EDA_DRAW_FRAME( KIWAY* aKiway, QWidget* aParent, FRAME_T aFrameType, const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize, long aStyle,
                    const wxString& aFrameName, const EDA_IU_SCALE& aIuScale );

    ~EDA_DRAW_FRAME();

    /**
     * Mark a schematic file as being in use.
     *
     * Use ReleaseFile() to undo this.
     *
     * @param aFileName full path to the file.
     * @return false if the file was already locked, true otherwise.
     */
    bool LockFile( const wxString& aFileName );

    /**
     * Release the current file marked in use.
     */
    void ReleaseFile();

    /**
     * Toggle the scripting console visibility.
     */
    void ScriptingConsoleEnableDisable();

    /**
     * Get the current visibility of the scripting console window.
     */
    bool IsScriptingConsoleVisible();

    EDA_SEARCH_DATA& GetFindReplaceData();
    wxArrayString& GetFindHistoryList() { return m_findStringHistoryList; }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) = 0;
    virtual const PAGE_INFO& GetPageSettings() const = 0;

    /**
     * Works off of GetPageSettings() to return the size of the paper page in
     * the internal units of this particular view.
     */
    virtual const VECTOR2I GetPageSizeIU() const = 0;

    /**
     * For those frames that support polar coordinates.
     */
    bool GetShowPolarCoords() const { return m_polarCoords; }
    void SetShowPolarCoords( bool aShow ) { m_polarCoords = aShow; }

    void ToggleUserUnits() override;

    /**
     * Get the pair or units in current use.
     *
     * The primary unit is the main unit of the frame, and the secondary unit is the unit
     * of the other system that was used most recently.
     */
    void GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits ) override;

    /**
     * Return the absolute coordinates of the origin of the snap grid.
     *
     * This is treated as a relative offset and snapping will occur at multiples of the grid
     * size relative to this point.
     */
    virtual const VECTOR2I& GetGridOrigin() const = 0;
    virtual void            SetGridOrigin( const VECTOR2I& aPosition ) = 0;

    /**
     * @return a grid helper of the frame (snapping the points to the grid).
     */
    virtual std::unique_ptr<GRID_HELPER> MakeGridHelper();

    /**
     * Return the nearest \a aGridSize location to \a aPosition.
     *
     * @param aPosition The position to check.
     * @return The nearest grid position.
     */
    VECTOR2I GetNearestGridPosition( const VECTOR2I& aPosition ) const;

    /**
     * Return the nearest \a aGridSize / 2 location to \a aPosition.
     *
     * This is useful when attempting for keep outer points on grid but
     * not the middle point.
     *
     * @param aPosition The position to check.
     * @return The nearest half-grid position.
     */
    VECTOR2I GetNearestHalfGridPosition( const VECTOR2I& aPosition ) const;

    virtual const TITLE_BLOCK& GetTitleBlock() const = 0;
    virtual void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) = 0;

    /**
     * @return the color that should be used to draw the drawing area background.
     */
    virtual COLOR4D GetDrawBgColor() const { return m_drawBgColor; }

    virtual void SetDrawBgColor( const COLOR4D& aColor) { m_drawBgColor= aColor ; }

    /**
     * Returns a pointer to the active color theme settings.
     *
     * @param aForceRefresh will force a reload of the theme from the settings manager.
     */
    virtual COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const;

    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Return a human readable value for display in dialogs.
     */
    virtual wxString GetScreenDesc() const;
    virtual wxString GetFullScreenDesc() const;

    /**
     * Return a pointer to a BASE_SCREEN or one of its derivatives.
     *
     * It is overloaded by derived classes to return #SCH_SCREEN or #PCB_SCREEN.
     */
    virtual BASE_SCREEN* GetScreen() const  { return m_currentScreen; }

    /**
     * Clear the message panel.
     */
    void EraseMsgBox();

    /**
     * Indicates whether the grid is visible.
     */
    bool IsGridVisible();
    virtual void SetGridVisibility( bool aVisible );

    bool IsGridOverridden();
    virtual void SetGridOverrides( bool aOverride );

    // Toolbar-related functions
    virtual void ReCreateHToolbar() { };
    virtual void ReCreateVToolbar() { };
    virtual void ReCreateLeftToolbar() { };
    virtual void ReCreateAuxiliaryToolbar() { }

    /**
     * @return the color of the grid
     */
    virtual COLOR4D GetGridColor() { return m_gridColor; }

    /**
     * @param aColor is the color of the grid
     */
    virtual void SetGridColor( const COLOR4D& aColor ) { m_gridColor = aColor; }

    /**
     * Update the grid selection box of the tool bar (the list of the grids, the current one).
     */
    void UpdateGridSelectBox();

    /**
     * Select the current grid in the grid selection box of the tool bar.
     */
    void SyncGridSelectBox();

    /**
     * Update the zoom selection box of the tool bar.
     */
    void UpdateZoomSelectBox();

    /**
     * Return a human readable value which can be displayed as zoom level indicator in dialogs.
     */
    const wxString GetZoomLevelIndicator() const;

    /**
     * Rebuild the GAL and redraws the screen.
     *
     * Call when something went wrong.
     */
    virtual void HardRedraw();

    /**
     * Redraw the screen with best zoom level and the best centering that shows all the
     * page or the board.
     */
    virtual void Zoom_Automatique( bool aWarpPointer );

    /**
     * Useful to focus on a particular location, in find functions.
     *
     * Move the graphic cursor (crosshair cursor) at a given coordinate and reframes the
     * drawing if the requested point is out of view or if center on location is requested.
     *
     * @param aPos is the point to go to.
     */
    void FocusOnLocation( const VECTOR2I& aPos, bool aAllowScroll = true );

    /**
     * Focus on a particular canvas item.
     *
     * @param aItem is the item to focus on.  nullptr clears the focus.
     * @param aAllowScroll is true if the canvas can be scrolled to show the item.
     */
    virtual void FocusOnItem( EDA_ITEM* aItem, bool aAllowScroll = true ) {}

    virtual void ClearFocus() { FocusOnItem( nullptr ); }

    /**
     * Construct a "basic" menu for a tool, containing only items that apply to all tools
     * (e.g. zoom and grid).
     */
    void AddStandardSubMenus( TOOL_MENU& aMenu );

    /**
     * Send a message to the toolbar or status bar.
     */
    void DisplayToolMsg( const wxString& msg ) override;

    void DisplayConstraintsMsg( const wxString& msg );

    /// Show a text in a field of the status bar (0: message; 1: zoom; 2: cursor; 3: delta;
    /// 4: grid; 5: units; 6: tool; 7: constraint).

    /**
     * Called when modifying the page settings.
     *
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    virtual void OnPageSettingsChange() {}

    /**
     * Update the status bar information.
     */
    void UpdateStatusBar() override;

    /**
     * Display current unit pane in the status bar.
     */
    void DisplayUnitsMsg();

    /**
     * Display current grid size in the status bar.
     */
    virtual void DisplayGridMsg();

    /**
     * @return true if locks should be overridden (the option of the status bar).
     */
    bool GetOverrideLocks() const;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Append a message to the message panel.
     *
     * This helper method checks to make sure the message panel exists in the frame and
     * appends a message to it using the message panel AppendMessage() method.
     *
     * @param aTextUpper The message upper text.
     * @param aTextLower The message lower text.
     * @param aPadding Number of spaces to pad between messages.
     */
    void AppendMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding = 6 );

    /**
     * Clear all messages from the message panel.
     */
    virtual void ClearMsgPanel();

    /**
     * Clear the message panel and populates it with the contents of \a aList.
     *
     * @param aList is the list of #MSG_PANEL_ITEM objects to fill the message panel.
     */
    void SetMsgPanel( const std::vector< MSG_PANEL_ITEM >& aList );
    void SetMsgPanel( EDA_ITEM* aItem );

    void SetMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding = 6 );

    /**
     * Redraw the message panel.
     */
    virtual void UpdateMsgPanel();

    /**
     * Fetch an item by KIID.  Frame-type-specific implementation.
     */
    virtual EDA_ITEM* ResolveItem( const KIID& aId, bool aAllowNullptrReturn = false ) const
    {
        return nullptr;
    }

    virtual LIB_TREE* GetLibTree() const { return nullptr; }
    virtual LIB_ID GetTargetLibId() const { return LIB_ID(); }

    virtual bool IsLibraryTreeShown() const { return false; }
    virtual void ToggleLibraryTree() {};
    virtual void FocusLibraryTreeInput() {};

    PROPERTIES_PANEL* GetPropertiesPanel() { return m_propertiesPanel; }

    void UpdateProperties();

    virtual void ToggleProperties() {}

    static const wxString PropertiesPaneName() { return wxS( "PropertiesManager" ); }
    static const wxString NetInspectorPanelName() { return wxS( "NetInspector" ); }
    static const wxString DesignBlocksPaneName() { return wxS( "DesignBlocks" ); }
    static const wxString RemoteSymbolPaneName() { return wxS( "RemoteSymbol" ); }
    static const wxString AppearancePanelName() { return wxS( "LayersManager" ); }

    /**
     * Get the context of the message panel (units, origin) of the frame.
     */
    MSG_PANEL_CONTEXT* GetMsgPanelContext();

    /**
     * Use to start up the GAL drawing canvas.
     */
    virtual void ActivateGalCanvas();

    /**
     * Change the current rendering backend.
     */
    virtual void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    /**
     * Return a pointer to GAL-based canvas of given EDA draw frame.
     *
     * @return Pointer to GAL-based canvas.
     */
    virtual EDA_DRAW_PANEL_GAL* GetCanvas() const { return m_canvas; }
    void SetCanvas( EDA_DRAW_PANEL_GAL* aPanel ) { m_canvas = aPanel; }

    TOOLKIT_WINDOW* GetToolCanvas() const override { return GetCanvas(); }

    /**
     * Return a reference to the gal rendering options used by GAL for rendering.
     */
    GAL_DISPLAY_OPTIONS_IMPL& GetGalDisplayOptions() { return m_galDisplayOptions; }

    void RefreshCanvas() override
    {
        GetCanvas()->Refresh();
    }

    /**
     * Return bounding box of document with option to not include some items.
     *
     * Used most commonly by "Zoom to Fit" and "Zoom to Objects".  In Eeschema for "Zoom to Fit"
     * it's passed "true" to include drawing sheet border, and "false" by "Zoom To Objects" to
     * ignore drawing sheet border.  In Pcbnew, false makes it ignore any items outside the PCB
     * edge such as fabrication notes.
     *
     * @param aIncludeAllVisible True to include everything visible in bbox calculations,
     *                           false to ignore some visible items (program dependent).
     * @return Bounding box of the document (ignoring some items as requested).
     */
    virtual const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const;

    /**
     * Redraw the menus and what not in current language.
     */
    void ShowChangedLanguage() override;

    /**
     * Save the current view as an image file.
     *
     * @param aFileName The full path of the file to save.
     * @param aBitmapType The type of the image file.
     * @return True if the file was successfully saved or false if the file failed to be saved.
     */
    bool SaveCanvasImageToFile( const wxString& aFileName, BITMAP_TYPE aBitmapType );

    virtual PLUGIN_ACTION_SCOPE PluginActionScope() const { return PLUGIN_ACTION_SCOPE::INVALID; }

protected:
    virtual std::unique_ptr<MSG_PANEL_CONTEXT> createMsgPanelContext();

    virtual void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    void unitsChangeRefresh() override;

    void setupUIConditions() override;

    /**
     * Set the units of the frame (and the last ones of the other system) from its settings.
     */
    void setupUnits( APP_SETTINGS_BASE* aCfg );

    /**
     * The rectangles of the dialogs open over the frame, in the client coordinates of the
     * canvas (the views avoid putting the items under them).
     */
    std::vector<BOX2D> findDialogScreenRects();

    /**
     * Make the main area of the frame: the info bar above the canvas.  The frame must have
     * its canvas.
     */
    void setupCentralArea();

    /**
     * Make the status bar of the frame, with the fields of the editors (zoom, coordinates,
     * grid, units...).
     */
    void createStatusBar();

    /// The status bar of the draw frames: with the messages of the loads.
    KISTATUSBAR* OnCreateStatusBar( int aNumber ) override;

    /// Set the widths of the fields of the status bar for the current font.
    void updateStatusBarWidths();

    /**
     * Determines the Canvas type to load (with prompt if required) and initializes m_canvasType
     */
    virtual void resolveCanvasType();

    /**
     * Returns the canvas type stored in the application settings.
     */
    EDA_DRAW_PANEL_GAL::GAL_TYPE loadCanvasTypeSetting();

    /**
     * Store the canvas type in the application settings.
     */
    bool saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    void handleActivateEvent( bool aActive ) override;

    /// The keys pressed in the frame (outside its canvas) that its widgets don't use are
    /// hotkeys of the tools.
    void keyPressEvent( QKeyEvent* aEvent ) override;

    std::unique_ptr<LOCKFILE> m_file_checker;

    COLOR4D              m_gridColor;         // Grid color
    COLOR4D              m_drawBgColor;       // The background color of the draw canvas; BLACK for
                                              // Pcbnew, BLACK or WHITE for Eeschema
    int                  m_undoRedoCountMax;  // Default Undo/Redo command Max depth, to be handed
                                              // to screens
    bool                 m_polarCoords;       // For those frames that support polar coordinates

    bool                 m_showBorderAndTitleBlock;  // Show the drawing sheet (border & title block).

    std::unique_ptr<MSG_PANEL_CONTEXT> m_msgPanelContext;

    std::unique_ptr<EDA_SEARCH_DATA> m_findReplaceData;
    wxArrayString        m_findStringHistoryList;
    wxArrayString        m_replaceStringHistoryList;

    EDA_MSG_PANEL*       m_messagePanel;
    int                  m_lastToolbarIconSize;

    COLOR_SETTINGS*      m_colorSettings;

    PROPERTIES_PANEL*    m_propertiesPanel;

    /// The current canvas type.
    EDA_DRAW_PANEL_GAL::GAL_TYPE    m_canvasType;

private:
    BASE_SCREEN*                m_currentScreen;      ///< current used SCREEN
    EDA_DRAW_PANEL_GAL*         m_canvas;

    ///< This the frame's interface to setting GAL display options.
    GAL_DISPLAY_OPTIONS_IMPL  m_galDisplayOptions;
};

#endif  // DRAW_FRAME_H_
