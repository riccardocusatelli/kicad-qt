/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

/**
 * @file action_toolbar.h
 * ACTION_TOOLBAR of the Qt build: the interface of include/tool/action_toolbar.h, on a QToolBar.
 */

#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <kiqt.h>
#include <QToolBar>

#include <tool/action_manager.h>
#include <frame_type.h>
#include <tool/action_toolbar_control.h>
#include <core/ki_assert.h>
#include <wx/aui/auibar.h>
#include <wx/kiwx_gdi.h>        // wxBitmapBundle

class QAction;
class QToolButton;
class ACTION_MENU;
class wxFont;
class EDA_BASE_FRAME;
class TOOL_ACTION;
class TOOL_MANAGER;
class TOOLBAR_CONFIGURATION;


/**
 * A group of actions that will be displayed together on a toolbar palette.
 */
class ACTION_GROUP
{
public:
    // Make the toolbar a friend so it can easily access everything inside here
    friend class ACTION_TOOLBAR;

    ACTION_GROUP( const std::string_view& aName );

    ACTION_GROUP( const std::string_view& aName, const std::vector<const TOOL_ACTION*>& aActions );

    /**
     * Set the default action to use when first creating the toolbar palette icon.
     *
     * If no default action is provided, the default will be the first action in the
     * vector.
     *
     * @param aDefault is the default action.
     */
    void SetDefaultAction( const TOOL_ACTION& aDefault );

    /**
     * Get the default action to use when first creating this group's toolbar palette icon.
     */
    const TOOL_ACTION* GetDefaultAction() const { return m_defaultAction; }

    /**
     * Get the name of the group.
     */
    std::string GetName() const { return m_name; }

    /**
     * Get the ID used in the UI to reference this group
     */
    int GetUIId() const;

    /**
     * Set the actions contained in this group.
     *
     * The first action in the list will be the new default action.
     *
     * @param aActions is the new set of actions.
     */
    void SetActions( const std::vector<const TOOL_ACTION*>& aActions );

    /**
     * Get a vector of all the actions contained inside this group.
     */
    const std::vector<const TOOL_ACTION*>& GetActions() const { return m_actions; }

protected:
    ///< The action ID for this action group
    int m_id;

    ///< The name of this action group
    std::string m_name;

    ///< The default action to display on the toolbar item
    const TOOL_ACTION* m_defaultAction;

    ///< The actions that compose the group.  Non-owning.
    std::vector<const TOOL_ACTION*> m_actions;
};


/**
 * Define the structure of a toolbar with buttons that invoke ACTIONs.
 *
 * The buttons are QActions whose data is the UI id of their TOOL_ACTION (or of their group), so
 * that the frame updates their state from the UI conditions of the actions.  The actions of a
 * group are in the palette of the button of the group, which opens after a long press.
 */
class ACTION_TOOLBAR : public QToolBar
{
public:
    /**
     * @param aVertical is true for the toolbars on the sides of the frame.
     */
    ACTION_TOOLBAR( EDA_BASE_FRAME* aParent, bool aVertical = false );

    /**
     * The constructor of the wxWidgets build (an AUI toolbar): vertical with wxAUI_TB_VERTICAL.
     */
    ACTION_TOOLBAR( EDA_BASE_FRAME* aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                    long aStyle ) :
            ACTION_TOOLBAR( aParent, ( aStyle & wxAUI_TB_VERTICAL ) != 0 )
    {
    }

    ~ACTION_TOOLBAR() override;

    void SetToolManager( TOOL_MANAGER* aManager ) { m_toolManager = aManager; }

    /**
     * Add a TOOL_ACTION-based button to the toolbar.
     *
     * The toggle/cancel attributes are set using the attributes in the action.
     *
     * After selecting the entry, a #TOOL_EVENT command containing name of the action is sent.
     *
     * @param aAction is the action to add.
     */
    void Add( const TOOL_ACTION& aAction );

    /**
     * Add a TOOL_ACTION-based button to the toolbar.
     *
     * After selecting the entry, a #TOOL_EVENT command containing name of the action is sent.
     *
     * @param aAction is the action to add.
     * @param aIsToggleEntry makes the toolbar item a toggle entry when true.
     * @param aIsCancellable when true, cancels the tool if clicked when tool is active.
     */
    void Add( const TOOL_ACTION& aAction, bool aIsToggleEntry, bool aIsCancellable = false );

    /**
     * Add a large button such as used in the KiCad Manager Frame's launch bar.
     *
     * @param aAction
     */
    void AddButton( const TOOL_ACTION& aAction );

    /**
     * Add a separator.
     */
    void AddScaledSeparator( QWidget* aWindow );

    /// Repaint the toolbar (wxWindow::Refresh()).
    void Refresh() { update(); }

    /// Give the focus to the toolbar (wxWindow::SetFocus()).
    void SetFocus() { setFocus(); }

    /// The window of the toolbar (wxWindow::GetParent()).
    QWidget* GetParent() const { return parentWidget(); }

    /**
     * Add a tool that is not an action (wxAuiToolBar::AddTool()): a click runs the command of
     * its id in the frame (EDA_BASE_FRAME::RunMenuCommand()).
     *
     * @param aId is the id of the tool, wxID_ANY for a new one.
     * @return the item of the tool (owned by the toolbar).
     */
    wxAuiToolBarItem* AddTool( int aId, const wxString& aLabel, const wxBitmapBundle& aBitmap,
                               const wxString& aShortHelp = wxEmptyString,
                               wxItemKind aKind = wxITEM_NORMAL );

    /// Add an empty space of @a aSize pixels.
    void AddSpacer( int aSize );

    /**
     * Add a custom control to the toolbar.
     *
     * @param aControl is the control to add.  Its id (KIQT::SetWindowId()) is given to the
     *                 frame when the toolbar is cleared (EDA_BASE_FRAME::ClearToolbarControl()).
     * @param aLabel is the label of the control.
     */
    void Add( QWidget* aControl, const wxString& aLabel = wxEmptyString );

    /// Add a custom control to the toolbar (wxAuiToolBar::AddControl()).
    void AddControl( QWidget* aControl, const wxString& aLabel = wxEmptyString )
    {
        Add( aControl, aLabel );
    }

    /// The font of the toolbar (wxWindow::GetFont()).
    wxFont GetFont() const;

    /**
     * Add a context menu to a specific tool item on the toolbar.
     *
     * This toolbar gets ownership of the menu object passed in.
     *
     * @param aAction is the action to get the menu
     * @param aMenu is the context menu
     */
    void AddToolContextMenu( const TOOL_ACTION& aAction, std::unique_ptr<ACTION_MENU> aMenu );

    /**
     * Add a set of actions to a toolbar as a group. One action from the group will be displayed
     * at a time.
     *
     * @param aGroup is the group to add. The first action in the group will be the first shown
     *               on the toolbar.
     */
    void AddGroup( std::unique_ptr<ACTION_GROUP> aGroup );

    /**
     * Select an action inside a group
     *
     * @param aGroup is the group that contains the action
     * @param aAction is the action to select
     */
    void SelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction );

    /**
     * Select an action inside any group that contains it.
     *
     * @param aAction is the action to select
     */
    void SelectAction( const TOOL_ACTION& aAction );

    /**
     * Replace the contents of this toolbar with the configuration given in
     * @c aConfig.
     *
     * @param aConfig is the configuration to apply to the toolbar
     */
    void ApplyConfiguration( const TOOLBAR_CONFIGURATION& aConfig );

    /**
     * Update the width of all wxControl tools on thsi toolbar
     */
    void UpdateControlWidths();

    /**
     * Update the toolbar item width of a control using its best size.
     *
     * @param aID is the ID of the toolbar item to update the width for
     */
    void UpdateControlWidth( int aID );

    /**
     * Clear the toolbar and remove all associated menus.
     */
    void ClearToolbar();

    /**
     * Updates the bitmap of a particular tool.
     *
     * @param aAction is the action to update
     * @param aBitmap is the new bitmap
     */
    void SetToolBitmap( const TOOL_ACTION& aAction, const QIcon& aBitmap );

    /**
     * Apply the default toggle action.
     *
     * For checkable items, this sets the checked state; for normal items, the enabled state.
     */
    void Toggle( const TOOL_ACTION& aAction, bool aState );

    void Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked );

    /**
     * Make the toolbar take its final size (nothing to do with Qt; kept for the frames).
     */
    bool KiRealize();
    bool Realize() { return KiRealize(); }

    /**
     * Reload all the bitmaps for the tools (e.g. when switching icon themes)
     */
    void RefreshBitmaps();

    /**
     * Get the list of custom controls that could be used on toolbars.
     */
    static std::list<ACTION_TOOLBAR_CONTROL*>& GetAllCustomControls()
    {
        return ACTION_TOOLBAR_CONTROL::GetAll();
    }

    /**
     * Get the list of custom controls that could be used on a particular frame type.
     */
    static std::list<ACTION_TOOLBAR_CONTROL*> GetCustomControlList( FRAME_T aContext );

    static constexpr bool TOGGLE = true;
    static constexpr bool CANCEL = true;

protected:
    /**
     * Update a group toolbar item to look like a specific action.
     *
     * Note: This function does not verify that the action is inside the group.
     */
    void doSelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction );

    /// A toolbar item was triggered.
    void onTriggered( int aId, bool aChecked );

    /// An action of the palette of a group was chosen.
    void onPaletteAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction );

    ///< Show the context menu registered for the given tool ID (handles group remapping).
    void showContextMenu( int aToolId );

    /// The context menu of the toolbar (on a button: the menu of its action).
    void contextMenuEvent( QContextMenuEvent* aEvent ) override;

    /// The new icons of the theme.
    void changeEvent( QEvent* aEvent ) override;

    /// The item of an id (button, group or control), or nullptr.
    QAction* findItem( int aId ) const;

    /// The size of the icons of the toolbars (in the settings).
    int iconSize() const;

protected:
    EDA_BASE_FRAME*         m_parent;
    TOOL_MANAGER*           m_toolManager;

    std::map<int, bool>                m_toolKinds;
    std::map<int, bool>                m_toolCancellable;
    std::map<int, const TOOL_ACTION*>  m_toolActions;
    std::map<int, QAction*>            m_items;

    /// IDs for all the control items in this toolbar
    std::vector<int> m_controlIDs;

    std::map<int, std::unique_ptr<ACTION_GROUP>> m_actionGroups;
    std::map<int, std::unique_ptr<ACTION_MENU>>  m_toolMenus;

    /// The tools that are not actions (AddTool())
    std::map<int, std::unique_ptr<wxAuiToolBarItem>> m_auiItems;

    /// Selected action per group name
    std::map<std::string, std::string> m_groupSelections;
};

/**
 * Type for the function signature that is used to add custom controls to the toolbar.
 *
 * Note, these functions SHOULD NOT use the Qt-provided `addWidget` function to add the controls
 * to the toolbar, instead they should use the `ACTION_TOOLBAR::Add` functions to ensure proper
 * registration of the control.
 */
typedef std::function<void ( ACTION_TOOLBAR* )> ACTION_TOOLBAR_CONTROL_FACTORY;
