/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file action_menu.h
 * ACTION_MENU of the Qt build: the interface of include/tool/action_menu.h, on a QMenu.
 */

#pragma once

#include <map>
#include <list>
#include <functional>

#include <kiqt.h>
#include <QMenu>

#include <wx/intl.h>          // wxGetTranslation (kiwx)

#include <tool/tool_event.h>

class QAction;
class KIFACE_BASE;
class TOOL_INTERACTIVE;
class TOOLS_HOLDER;
class TOOL_MANAGER;

enum class BITMAPS : unsigned int;

/**
 * Define the structure of a menu based on ACTIONs.
 *
 * The entries are QActions with an ID: the UI ID of their TOOL_ACTION, or the ID given to the
 * menu.  The hotkeys of the tool actions are shown in the menu; they are handled by the
 * TOOL_DISPATCHER, not by shortcuts of the menu.
 */
class ACTION_MENU : public QMenu
{
public:
    /// Default constructor
    ACTION_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool = nullptr );

    ~ACTION_MENU() override;

    ACTION_MENU( const ACTION_MENU& aMenu ) = delete;
    ACTION_MENU& operator=( const ACTION_MENU& aMenu ) = delete;

    /**
     * Set title for the menu. The title is shown as a text label shown on the top of
     * the menu.
     *
     * @param aTitle is the new title.
     */
    void SetTitle( const wxString& aTitle );
    void SetUntranslatedTitle( const wxString& aTitle ) { m_untranslatedTitle = aTitle; }

    wxString GetTitle() const { return m_title; }

    /**
     * Decide whether a title for a pop up menu should be displayed.
     */
    void DisplayTitle( bool aDisplay = true );

    /**
     * Assign an icon for the entry.
     *
     * @param aIcon is the icon to be assigned. NULL is used to remove icon.
     */
    void SetIcon( BITMAPS aIcon );

    /**
     * Add an entry with an ID to the menu.
     *
     * After highlighting/selecting the entry, a #TOOL_EVENT will be generated with the
     * #TA_CHOICE_MENU_UPDATE / #TA_CHOICE_MENU_CHOICE action and the ID as the parameter.
     * The text after a tab in the label is shown as the shortcut of the entry.
     */
    QAction* Add( const wxString& aLabel, int aId, BITMAPS aIcon );
    QAction* Add( const wxString& aLabel, const wxString& aToolTip, int aId, BITMAPS aIcon,
                  bool aIsCheckmarkEntry = false );

    /**
     * Add an entry to the menu based on the #TOOL_ACTION object.
     *
     * After selecting the entry, a #TOOL_EVENT will be generated.
     *
     * @param aAction is the action to be added to menu entry.
     * @param aIsCheckmarkEntry is true to indicate a check menu entry, false for normal menu
     *                          entry.
     * @param aOverrideLabel is the label to show in the menu (overriding the action's menu text)
     *                       when non-empty.
     */
    QAction* Add( const TOOL_ACTION& aAction, bool aIsCheckmarkEntry = false,
                  const wxString& aOverrideLabel = wxEmptyString );

    /**
     * Add an action menu as a submenu.
     *
     * The menu takes the ownership of the submenu.
     *
     * @param aMenu is the submenu to be added. This should be a new instance (use Clone()) if
     *              required.
     */
    QAction* Add( ACTION_MENU* aMenu );

    /// Add a separator.
    void AppendSeparator();

    /**
     * Add a standard close item to the menu with the accelerator key CTRL-W.
     *
     * Emits the wxID_CLOSE event.
     *
     * @param aAppname is the application name to append to the tooltip.
     */
    void AddClose( const wxString& aAppname = "" );

    /**
     * Add either a standard Quit or Close item to the menu.
     *
     * If \a aKiface is NULL or in single-instance then quit (wxID_QUIT) is used, otherwise
     * close (wxID_CLOSE) is used.
     *
     * @param aAppname is the application name to append to the tooltip.
     */
    void AddQuitOrClose( KIFACE_BASE* aKiface, wxString aAppname = "" );

    /**
     * Add a standard Quit item to the menu.
     *
     * Emits the wxID_EXIT event.
     *
     * @param aAppname is the application name to append to the tooltip.
     */
    void AddQuit( const wxString& aAppname = "" );

    /**
     * Remove all the entries from the menu (as well as its title).
     *
     * It leaves the menu in the initial state.
     */
    void Clear();

    /**
     * Return true if the menu has any enabled items.
     */
    bool HasEnabledItems() const;

    /// The number of entries (separators included).
    int GetMenuItemCount() const;

    /// The entry of an ID (in this menu or its submenus), or nullptr.
    QAction* FindItem( int aId ) const;

    /// Check (or uncheck) the entry of an ID.
    void Check( int aId, bool aCheck = true );

    /// Enable (or disable) the entry of an ID.
    void Enable( int aId, bool aEnable = true );

    bool IsChecked( int aId ) const;

    bool IsEnabled( int aId ) const;

    /**
     * Return the position of selected item.
     *
     * If the returned value is negative, that means that menu was dismissed.
     *
     * @return The position of selected item in the action menu.
     */
    inline int GetSelected() const
    {
        return m_selected;
    }

    /**
     * Run update handlers for the menu and its submenus.
     */
    void UpdateAll();

    /**
     * Used by some menus to just-in-time translate their titles.
     */
    virtual void UpdateTitle()
    {
        if( !m_untranslatedTitle.IsEmpty() )
            m_title = wxGetTranslation( m_untranslatedTitle );
    }

    /**
     * Clear the dirty flag on the menu and all descendants.
     */
    void ClearDirty();
    void SetDirty();

    /**
     * Set a tool that is the creator of the menu.
     *
     * @param aTool is the tool that created the menu.
     */
    void SetTool( TOOL_INTERACTIVE* aTool );

    /**
     * Create a deep, recursive copy of this ACTION_MENU.
     */
    ACTION_MENU* Clone() const;

    /**
     * Show the menu as a popup menu at the mouse cursor, and wait until it is closed.
     *
     * @param aHolder is the window (frame) that the menu belongs to.
     */
    void PopupAtMouse( TOOLS_HOLDER* aHolder );

    virtual bool PassHelpTextToHandler() { return false; }

    static constexpr bool NORMAL = false;
    static constexpr bool CHECK  = true;

protected:
    /// Return an instance of this class. It has to be overridden in inheriting classes.
    virtual ACTION_MENU* create() const;

    /// Return an instance of TOOL_MANAGER class.
    TOOL_MANAGER* getToolManager() const;

    /**
     * Update menu state stub.
     *
     * It is called before a menu is shown, in order to update its state.  Here you can tick
     * current settings, enable/disable entries, etc.
     */
    virtual void update()
    {
    }

    /**
     * Event handler stub.
     *
     * It should be used if you want to generate a #TOOL_EVENT from a menu entry.  It will be
     * called with the ID of the menu entry that is clicked.
     */
    virtual OPT_TOOL_EVENT eventHandler( int aId )
    {
        return OPT_TOOL_EVENT();
    }

    /**
     * Copy another menus data to this instance.
     *
     * Old entries are preserved and ones form aMenu are copied.
     */
    void copyFrom( const ACTION_MENU& aMenu );

protected:
    /// Append a copy of an entry (and of its submenu) of another menu.
    QAction* appendCopy( const ACTION_MENU& aMenu, const QAction* aSource );

    /// A new entry of the menu with an ID (not appended to the menu).
    QAction* newItem( int aId, const wxString& aLabel, const wxString& aToolTip, BITMAPS aIcon,
                      bool aIsCheckmarkEntry );

    /// The menu is about to be shown.
    void onAboutToShow();

    /// An entry of the menu was highlighted.
    void onHovered( QAction* aAction );

    /// An entry of the menu (not of a submenu) was chosen.
    void onTriggered( QAction* aAction );

    /// Update hot key settings for TOOL_ACTIONs in this menu.
    void updateHotKeys();

    /// Traverse the submenus tree looking for a submenu capable of handling a particular menu
    /// entry. In case it is handled, it is returned the aToolEvent parameter.
    void runEventHandlers( int aId, OPT_TOOL_EVENT& aToolEvent );

    /// Run a function on the menu and all its submenus.
    void runOnSubmenus( std::function<void(ACTION_MENU*)> aFunction );

    /// Check if any of submenus contains a TOOL_ACTION with a specific ID.
    OPT_TOOL_EVENT findToolAction( int aId );

    bool m_dirty;               // Menu requires update before display

    bool m_titleDisplayed;
    bool m_isContextMenu;

    /// Menu title.
    wxString m_title;
    wxString m_untranslatedTitle;

    /// Optional icon.
    BITMAPS m_icon;

    /// Store the id number of selected item.
    int m_selected;

    /// Creator of the menu.
    TOOL_INTERACTIVE* m_tool;

    /// The menu this menu is a submenu of, if any.
    ACTION_MENU* m_parentMenu;

    /// Associates tool actions with menu item IDs. Non-owning.
    std::map<int, const TOOL_ACTION*> m_toolActions;

    /// The labels of the entries, without the hotkeys shown after them.
    std::map<const QAction*, QString> m_labels;

    /// List of submenus.
    std::list<ACTION_MENU*> m_submenus;

    friend class TOOL_INTERACTIVE;
};
