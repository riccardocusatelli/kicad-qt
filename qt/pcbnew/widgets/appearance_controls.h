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
 * @file appearance_controls.h
 * APPEARANCE_CONTROLS of the Qt build: the appearance panel of the board editors (the layers,
 * the objects, the nets and net classes, the presets and viewports).  What the controls do is
 * shared with the wxWidgets build (pcbnew/widgets/appearance_controls.cpp); the widgets are in
 * appearance_controls_qt.cpp.
 */

#ifndef APPEARANCE_CONTROLS_H
#define APPEARANCE_CONTROLS_H

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <board.h>
#include <gal/color4d.h>
#include <layer_ids.h>
#include <project/board_project_settings.h>
#include <widgets/appearance_controls_base.h>

#include <wx/arrstr.h>
#include <wx/gdicmn.h>

class BITMAP_TOGGLE;
class COLOR_SWATCH;
class PCB_BASE_FRAME;
class QCheckBox;
class QLabel;
class QMenu;
class QRadioButton;
class QSlider;
class QToolButton;
class QVBoxLayout;

using KIGFX::COLOR4D;


/// A net of the nets grid.
struct NET_GRID_ENTRY
{
    NET_GRID_ENTRY( int aCode, const wxString& aName, const COLOR4D& aColor, bool aVisible ) :
            code( aCode ),
            name( aName ),
            color( aColor ),
            visible( aVisible )
    {
    }

    int      code;
    wxString name;
    COLOR4D  color;
    bool     visible;
};


class APPEARANCE_CONTROLS : public APPEARANCE_CONTROLS_BASE, public BOARD_LISTENER
{
public:
    /// The settings of a row of the panel: a layer, an object or a net class.
    struct APPEARANCE_SETTING
    {
        int      id;
        wxString label;
        wxString tooltip;
        bool     visible;
        bool     can_control_opacity;
        bool     can_control_visibility;
        bool     spacer;

        QWidget*       ctl_panel;
        QLabel*        ctl_indicator;
        BITMAP_TOGGLE* ctl_visibility;
        COLOR_SWATCH*  ctl_color;
        QLabel*        ctl_text;
        QSlider*       ctl_opacity;

        APPEARANCE_SETTING( const wxString& aLabel, int aId,
                            const wxString& aTooltip = wxEmptyString,
                            bool aCanControlOpacity = false,
                            bool aCanControlVisibility = true ) :
                id( aId ),
                label( aLabel ),
                tooltip( aTooltip ),
                visible( true ),
                can_control_opacity( aCanControlOpacity ),
                can_control_visibility( aCanControlVisibility ),
                spacer( false ),
                ctl_panel( nullptr ),
                ctl_indicator( nullptr ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr ),
                ctl_text( nullptr ),
                ctl_opacity( nullptr )
        {
        }

        APPEARANCE_SETTING() :
                id( -1 ),
                visible( false ),
                can_control_opacity( false ),
                can_control_visibility( true ),
                spacer( true ),
                ctl_panel( nullptr ),
                ctl_indicator( nullptr ),
                ctl_visibility( nullptr ),
                ctl_color( nullptr ),
                ctl_text( nullptr ),
                ctl_opacity( nullptr )
        {
        }
    };

    APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, QWidget* aFocusOwner, bool aFpEditor = false );
    ~APPEARANCE_CONTROLS() override;

    APPEARANCE_CONTROLS( const APPEARANCE_CONTROLS& ) = delete;
    APPEARANCE_CONTROLS& operator=( const APPEARANCE_CONTROLS& ) = delete;

    wxSize GetBestSize() const;

    /// The size of the panel (its width is the one of the pane, saved in the settings).
    wxSize GetSize() const { return wxSize( width(), height() ); }

    ///< Update the panel contents from the application and board models.
    void OnBoardChanged();

    void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                 std::vector<BOARD_ITEM*>& aRemovedItems,
                                 std::vector<BOARD_ITEM*>& aChangedItems ) override;

    ///< Update the colors on all the widgets from the new chosen color theme.
    void OnColorThemeChanged();

    ///< Respond to change in OS's DarkMode
    void OnDarkModeToggle();

    ///< Update the widget when the active board layer is changed.
    void OnLayerChanged();

    /// Notifies the panel when a net has been hidden or shown via the external tool.
    void OnNetVisibilityChanged( int aNetCode, bool aVisibility );

    ///< Manually update visibility for a given layer
    void SetLayerVisible( int aLayer, bool isVisible );

    void SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible = true );

    void UpdateDisplayOptions();

    ///< Return a list of the layer presets created by the user.
    std::vector<LAYER_PRESET> GetUserLayerPresets() const;

    ///< Update the current layer presets from those saved in the project file.
    void SetUserLayerPresets( std::vector<LAYER_PRESET>& aPresetList );

    void ApplyLayerPreset( const wxString& aPresetName );
    void ApplyLayerPreset( const LAYER_PRESET& aPreset );

    wxString GetActiveLayerPreset() const
    {
        if( m_currentPreset )
            return m_currentPreset->name;
        else
            return wxEmptyString;
    }

    const wxArrayString& GetLayerPresetsMRU() { return m_presetMRU; }

    ///< Return a list of viewports created by the user.
    std::vector<VIEWPORT> GetUserViewports() const;

    ///< Update the current viewports from those saved in the project file.
    void SetUserViewports( std::vector<VIEWPORT>& aPresetList );

    void ApplyViewport( const wxString& aPresetName );
    void ApplyViewport( const VIEWPORT& aPreset );

    const wxArrayString& GetViewportsMRU() { return m_viewportMRU; }

    int  GetTabIndex() const;
    void SetTabIndex( int aTab );

    /// Function to force a redraw of the collapsible panes in this control.
    void RefreshCollapsiblePanes();

    bool IsLayerOptionsExpanded();
    bool IsNetOptionsExpanded();
    bool IsTogglingNetclassRatsnestVisibility();

    void CommonSettingsChanged( int aFlag );

    /// The language changed: the texts of the panel are translated again.
    void OnLanguageChanged();

protected:
    void OnNotebookPageChanged( int aPage ) override;
    void OnSetFocus( QWidget* aSource ) override;
    void OnNetGridClick( int aRow, int aCol ) override;
    void OnNetGridDoubleClick( int aRow, int aCol ) override;
    void OnNetGridRightClick( int aRow, int aCol ) override;
    void onLayerPresetChanged( QWidget* aSource ) override;
    void onViewportChanged( QWidget* aSource ) override;

private:
    void createControls();

    void rebuildLayers();

    void rebuildLayerContextMenu();

    void syncColorsAndVisibility();

    void rebuildObjects();

    void syncObjectSettings();

    /// The context menu of a net class.
    void showNetclassMenu( bool isDefaultClass, const wxString& aName );

    void rebuildNets();

    /// The nets of the board in the nets grid (filtered by the filter text).
    void rebuildNetsGrid();

    /// Show the state of a row of the nets grid.
    void updateNetRow( int aRow );

    /// Choose the color of the net of a row of the nets grid.
    void editNetColor( int aRow );

    void setNetColor( NET_GRID_ENTRY& aNet, const COLOR4D& aColor );
    void setNetVisible( NET_GRID_ENTRY& aNet, bool aVisible );

    void loadDefaultLayerPresets();

    void rebuildLayerPresetsWidget( bool aReset );

    void syncLayerPresetSelection();

    void rebuildViewportsWidget();

    void onLayerLeftClick( int aLayer );

    void showLayerContextMenu();

    void onLayerVisibilityToggled( PCB_LAYER_ID aLayer );

    void onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible, bool isFinal );

    void setVisibleLayers( const LSET& aLayers );

    bool isLayerEnabled( PCB_LAYER_ID aLayer ) const;

    void setVisibleObjects( GAL_SET aObjects );

    LSET getVisibleLayers();

    GAL_SET getVisibleObjects();

    void onObjectOpacitySlider( int aLayer, float aOpacity );

    void updateLayerPresetSelection( const wxString& aName );

    void doApplyLayerPreset( const LAYER_PRESET& aPreset );

    void updateViewportSelection( const wxString& aName );

    void doApplyViewport( const VIEWPORT& aViewport );

    void showNetclass( const wxString& aClassName, bool aShow = true );

    void onNetContextMenu( int aId, int aRow );

    void handleBoardItemsChanged();

    void passOnFocus();

    void onReadOnlySwatch();

    bool doesBoardItemNeedRebuild( BOARD_ITEM* aBoardItem );

    bool doesBoardItemNeedRebuild( std::vector<BOARD_ITEM*>& aBoardItems );

    /// The preset or viewport of an entry of a combo box (nullptr for the other entries).
    LAYER_PRESET* presetAt( int aIndex ) const;
    VIEWPORT*     viewportAt( int aIndex ) const;

    // What the controls do, without the GUI toolkit (pcbnew/widgets/appearance_controls.cpp)

    /// The command @a aId of the layers context menu (ID_SHOW_ALL_COPPER_LAYERS...).
    void doLayerContextMenu( int aId );

    /// The color of a layer (or of an object) was changed in its swatch.
    void onLayerColorChanged( int aLayer, const COLOR4D& aColor );

    void setNetclassColor( const wxString& aNetclassName, const COLOR4D& aColor );
    void setNetColorMode( NET_COLOR_MODE aMode );
    void setRatsnestDisplay( bool aShow, RATSNEST_MODE aMode );

    /// The command @a aId of the netclass context menu, for m_contextMenuNetclass.
    void doNetclassContextMenu( int aId );

    /// The GAL layers of the Objects tab of the footprint editor.
    static bool isAllowedInFpEditor( int aLayer );

    // The nets grid of the GUI toolkit (appearance_controls_qt.cpp)
    void netGridSetVisibility( int aNetCode, bool aVisible );
    void netGridRefresh();

    PCB_BASE_FRAME* m_frame;

    QWidget* m_focusOwner;

    static const std::vector<APPEARANCE_SETTING> s_objectSettings;

    BOARD* m_board;

    bool m_isFpEditor;

    std::vector<NET_GRID_ENTRY> m_nets;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_layerSettings;
    std::map<PCB_LAYER_ID, APPEARANCE_SETTING*>      m_layerSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_objectSettings;
    std::map<GAL_LAYER_ID, APPEARANCE_SETTING*>      m_objectSettingsMap;

    std::vector<std::unique_ptr<APPEARANCE_SETTING>> m_netclassSettings;
    std::map<wxString, APPEARANCE_SETTING*>          m_netclassSettingsMap;

    // TODO(JE) Move preset storage to the PCB_CONTROL tool

    std::map<wxString, LAYER_PRESET> m_layerPresets;
    LAYER_PRESET*                    m_currentPreset;
    LAYER_PRESET*                    m_lastSelectedUserPreset;
    wxArrayString                    m_presetMRU;

    std::map<wxString, VIEWPORT>     m_viewports;
    VIEWPORT*                        m_lastSelectedViewport;
    wxArrayString                    m_viewportMRU;

    QMenu*                           m_layerContextMenu;

    /// The net class of the context menu being shown.
    wxString                         m_contextMenuNetclass;

    QWidget*     m_layersWidget;
    QVBoxLayout* m_layersOuterSizer;
    QWidget*     m_objectsWidget;
    QVBoxLayout* m_objectsOuterSizer;
    QWidget*     m_netclassWidget;

    // The built-in layer presets

    static LAYER_PRESET presetNoLayers;
    static LAYER_PRESET presetAllLayers;
    static LAYER_PRESET presetAllCopper;
    static LAYER_PRESET presetInnerCopper;
    static LAYER_PRESET presetFront;
    static LAYER_PRESET presetFrontAssembly;
    static LAYER_PRESET presetBack;
    static LAYER_PRESET presetBackAssembly;

    // a LAYER_PRESET used only to store the objects visibility of the
    // last selected built-in LAYER_PRESET preset
    static LAYER_PRESET m_lastBuiltinPreset;

    int m_pointSize;

    QColor m_layerPanelColour;

    // Layer display options controls
    QToolButton*  m_paneLayerDisplayOptions;
    QWidget*      m_layerDisplayPane;
    QLabel*       m_inactiveLayersLabel;
    QRadioButton* m_rbHighContrastNormal;
    QRadioButton* m_rbHighContrastDim;
    QRadioButton* m_rbHighContrastOff;
    QCheckBox*    m_cbFlipBoard;

    // Net display options controls
    QToolButton*  m_paneNetDisplayOptions;
    QWidget*      m_netDisplayPane;
    QLabel*       m_txtNetDisplayTitle;
    QRadioButton* m_rbNetColorAll;
    QRadioButton* m_rbNetColorRatsnest;
    QRadioButton* m_rbNetColorOff;
    QLabel*       m_txtRatsnestVisibility;
    QRadioButton* m_rbRatsnestAllLayers;
    QRadioButton* m_rbRatsnestVisLayers;
    QRadioButton* m_rbRatsnestNone;

    enum POPUP_ID
    {
        ID_CHANGE_COLOR = 7000,
        ID_SET_NET_COLOR,
        ID_CLEAR_NET_COLOR,
        ID_USE_SCHEMATIC_NET_COLOR,
        ID_SHOW_ALL_NETS,
        ID_HIDE_OTHER_NETS,
        ID_HIGHLIGHT_NET,
        ID_SELECT_NET,
        ID_DESELECT_NET,
        ID_SHOW_ALL_COPPER_LAYERS,
        ID_HIDE_ALL_COPPER_LAYERS,
        ID_HIDE_ALL_BUT_ACTIVE,
        ID_PRESET_NO_LAYERS,
        ID_PRESET_ALL_LAYERS,
        ID_PRESET_FRONT,
        ID_PRESET_FRONT_ASSEMBLY,
        ID_PRESET_INNER_COPPER,
        ID_PRESET_BACK,
        ID_PRESET_BACK_ASSEMBLY,
        ID_HIDE_ALL_NON_COPPER,
        ID_SHOW_ALL_NON_COPPER,
        ID_LAST_VALUE
    };

    bool m_togglingNetclassRatsnestVisibility;
};

#endif
