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
 * @file action_toolbar.cpp
 * ACTION_TOOLBAR of the Qt build (see common/tool/action_toolbar.cpp for the wxWidgets one).
 */

#include <tool/action_toolbar.h>

#include <algorithm>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QToolButton>

#include <bitmaps.h>
#include <eda_base_frame.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/conditional_menu.h>
#include <tool/selection.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/ui/toolbar_configuration.h>
#include <tool/ui/toolbar_context_menu_registry.h>
#include <trace_helpers.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <ui_hooks.h>
#include <wx/kiwx_gdi.h>


ACTION_GROUP::ACTION_GROUP( const std::string_view& aName )
{
    m_name = aName;
    m_id   = ACTION_MANAGER::MakeActionId( m_name );
    m_defaultAction = nullptr;
}


ACTION_GROUP::ACTION_GROUP( const std::string_view& aName,
                            const std::vector<const TOOL_ACTION*>& aActions )
{
    m_name = aName;
    m_id   = ACTION_MANAGER::MakeActionId( m_name );

    SetActions( aActions );
}


void ACTION_GROUP::SetActions( const std::vector<const TOOL_ACTION*>& aActions )
{
    KI_ASSERT_MSG( aActions.size() > 0, wxS( "Action groups must have at least one action" ) );

    // The default action is just the first action in the vector
    m_actions       = aActions;
    m_defaultAction = m_actions[0];
}


int ACTION_GROUP::GetUIId() const
{
    return m_id + TOOL_ACTION::GetBaseUIId();
}


void ACTION_GROUP::SetDefaultAction( const TOOL_ACTION& aDefault )
{
    bool valid = std::any_of( m_actions.begin(), m_actions.end(),
                              [&]( const TOOL_ACTION* aAction ) -> bool
                              {
                                  // For some reason, we can't compare the actions directly
                                  return aAction->GetId() == aDefault.GetId();
                              } );

    KI_ASSERT_MSG( valid, wxS( "Action must be present in a group to be the default" ) );

    m_defaultAction = &aDefault;
}


/// We use \t in short description to align accelerators in menus, but not in tooltips.
static QString tooltipText( const wxString& aTip )
{
    wxString tip = aTip;
    tip.Replace( "\t", "  " );
    return ToQString( tip );
}


ACTION_TOOLBAR::ACTION_TOOLBAR( EDA_BASE_FRAME* aParent, bool aVertical ) :
        QToolBar( aParent ),
        m_parent( aParent ),
        m_toolManager( aParent->GetToolManager() )
{
    setOrientation( aVertical ? Qt::Vertical : Qt::Horizontal );
    setMovable( false );
    setFloatable( false );
    setIconSize( QSize( iconSize(), iconSize() ) );
}


ACTION_TOOLBAR::~ACTION_TOOLBAR()
{
    // Clear all the maps keeping track of our items on the toolbar
    m_toolMenus.clear();
    m_actionGroups.clear();
    m_toolCancellable.clear();
    m_toolKinds.clear();
    m_toolActions.clear();
    m_auiItems.clear();
}


int ACTION_TOOLBAR::iconSize() const
{
    if( COMMON_SETTINGS* settings = Pgm().GetCommonSettings() )
        return settings->m_Appearance.toolbar_icon_size;

    return 24;
}


std::list<ACTION_TOOLBAR_CONTROL*> ACTION_TOOLBAR::GetCustomControlList( FRAME_T aContext )
{
    std::list<ACTION_TOOLBAR_CONTROL*> controls;

    for( ACTION_TOOLBAR_CONTROL* control : GetAllCustomControls() )
    {
        if( control->SupportedFor( aContext ) )
            controls.push_back( control );
    }

    return controls;
}


void ACTION_TOOLBAR::ApplyConfiguration( const TOOLBAR_CONFIGURATION& aConfig )
{
    // Keep each group's selection across the rebuild.
    std::map<std::string, std::string> currentGroupItems = m_groupSelections;

    // Remove existing tools
    ClearToolbar();

    std::vector<TOOLBAR_ITEM> items = aConfig.GetToolbarItems();

    // Add all the items to the toolbar
    for( auto& item : items )
    {
        switch( item.m_Type )
        {
        case TOOLBAR_ITEM_TYPE::SEPARATOR:
            AddScaledSeparator( m_parent );
            break;

        case TOOLBAR_ITEM_TYPE::SPACER:
            AddSpacer( item.m_Size );
            break;

        case TOOLBAR_ITEM_TYPE::TB_GROUP:
        {
            // Add a group of items to the toolbar
            std::string                     groupName = item.m_GroupName.ToStdString();
            std::vector<const TOOL_ACTION*> tools;
            const TOOL_ACTION*              defaultTool = nullptr;

            for( TOOLBAR_ITEM& groupItem : item.m_GroupItems )
            {
                switch( groupItem.m_Type )
                {
                case TOOLBAR_ITEM_TYPE::SEPARATOR:
                case TOOLBAR_ITEM_TYPE::SPACER:
                case TOOLBAR_ITEM_TYPE::TB_GROUP:
                case TOOLBAR_ITEM_TYPE::CONTROL:
                    KI_FAIL_MSG( wxT( "Unsupported group item type" ) );
                    continue;

                case TOOLBAR_ITEM_TYPE::TOOL:
                    TOOL_ACTION* grpAction =
                            m_toolManager->GetActionManager()->FindAction( groupItem.m_ActionName );

                    if( !grpAction )
                    {
                        KI_FAIL_MSG( wxString::Format( wxT( "Unable to find group tool %s" ),
                                                       groupItem.m_ActionName ) );
                        continue;
                    }

                    tools.push_back( grpAction );

                    if( currentGroupItems[groupName] == groupItem.m_ActionName )
                        defaultTool = grpAction;
                }
            }

            // A group needs at least one action
            if( tools.empty() )
                continue;

            std::unique_ptr<ACTION_GROUP> group = std::make_unique<ACTION_GROUP>( groupName, tools );

            if( defaultTool )
                group->SetDefaultAction( *defaultTool );

            AddGroup( std::move( group ) );

            // Look up and attach context menu if one is registered for this group
            auto menuFactory = TOOLBAR_CONTEXT_MENU_REGISTRY::GetGroupMenuFactory( groupName );

            if( menuFactory && m_toolManager )
            {
                // Register the menu for each action in the group
                for( const TOOL_ACTION* grpAction : tools )
                    AddToolContextMenu( *grpAction, menuFactory( m_toolManager ) );
            }

            break;
        }

        case TOOLBAR_ITEM_TYPE::CONTROL:
        {
            // Add a custom control to the toolbar
            ACTION_TOOLBAR_CONTROL_FACTORY* factory =
                    m_parent->GetCustomToolbarControlFactory( item.m_ControlName );

            if( !factory )
            {
                KI_FAIL_MSG( wxString::Format( wxT( "Unable to find control factory for %s" ),
                                               item.m_ControlName ) );
                continue;
            }

            // The factory functions are responsible for adding the controls to the toolbar
            (*factory)( this );
            break;
        }

        case TOOLBAR_ITEM_TYPE::TOOL:
        {
            TOOL_ACTION* action = m_toolManager->GetActionManager()->FindAction( item.m_ActionName );

            if( !action )
            {
                KI_FAIL_MSG( wxString::Format( wxT( "Unable to find toolbar tool %s" ),
                                               item.m_ActionName ) );
                continue;
            }

            Add( *action );

            // Look up and attach context menu if one is registered for this action
            auto factory = TOOLBAR_CONTEXT_MENU_REGISTRY::GetMenuFactory( item.m_ActionName );

            if( factory && m_toolManager )
                AddToolContextMenu( *action, factory( m_toolManager ) );

            break;
        }
        }
    }

    KiRealize();
}


void ACTION_TOOLBAR::Add( const TOOL_ACTION& aAction )
{
    KI_ASSERT_MSG( !aAction.CheckToolbarState( TOOLBAR_STATE::HIDDEN ),
                   wxString::Format( "Attempting to add hidden action %s to the toolbar",
                                     aAction.GetName() ) );

    bool isToggleEntry = aAction.CheckToolbarState( TOOLBAR_STATE::TOGGLE );
    bool isCancellable = aAction.CheckToolbarState( TOOLBAR_STATE::CANCEL );

    Add( aAction, isToggleEntry, isCancellable );
}


void ACTION_TOOLBAR::Add( const TOOL_ACTION& aAction, bool aIsToggleEntry, bool aIsCancellable )
{
    KI_ASSERT_MSG( !( aIsCancellable && !aIsToggleEntry ),
                   wxS( "aIsCancellable requires aIsToggleEntry" ) );

    int toolId = aAction.GetUIId();

    QAction* item = new QAction( KiBitmapBundleDef( aAction.GetIcon(), iconSize() ),
                                 tooltipText( aAction.GetButtonTooltip() ), this );
    item->setData( toolId );
    item->setCheckable( aIsToggleEntry );

    QObject::connect( item, &QAction::triggered, this,
                      [this, toolId]( bool aChecked )
                      {
                          onTriggered( toolId, aChecked );
                      } );

    addAction( item );

    m_items[ toolId ]           = item;
    m_toolKinds[ toolId ]       = aIsToggleEntry;
    m_toolActions[ toolId ]     = &aAction;
    m_toolCancellable[ toolId ] = aIsCancellable;
}


void ACTION_TOOLBAR::AddButton( const TOOL_ACTION& aAction )
{
    Add( aAction, false, false );

    // A large button, with its text
    if( QToolButton* button = qobject_cast<QToolButton*>( widgetForAction( m_items[aAction.GetUIId()] ) ) )
        button->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
}


wxAuiToolBarItem* ACTION_TOOLBAR::AddTool( int aId, const wxString& aLabel,
                                           const wxBitmapBundle& aBitmap,
                                           const wxString& aShortHelp, wxItemKind aKind )
{
    int toolId = aId == wxID_ANY ? wxWindowIDNew() : aId;

    QAction* item = new QAction( aBitmap, ToQString( aLabel ), this );
    item->setData( toolId );
    item->setToolTip( ToQString( aShortHelp.IsEmpty() ? aLabel : aShortHelp ) );
    item->setCheckable( aKind == wxITEM_CHECK || aKind == wxITEM_RADIO );

    // The command of the tool is run by the frame (the menu events of wxWidgets)
    QObject::connect( item, &QAction::triggered, this,
                      [this, toolId]()
                      {
                          m_parent->RunMenuCommand( toolId );
                      } );

    addAction( item );
    m_items[ toolId ] = item;

    std::unique_ptr<wxAuiToolBarItem>& tool = m_auiItems[ toolId ];
    tool = std::make_unique<wxAuiToolBarItem>( toolId, item );
    return tool.get();
}


void ACTION_TOOLBAR::AddScaledSeparator( QWidget* aWindow )
{
    addSeparator();
}


void ACTION_TOOLBAR::AddSpacer( int aSize )
{
    QWidget* spacer = new QWidget( this );

    if( orientation() == Qt::Horizontal )
        spacer->setFixedWidth( aSize );
    else
        spacer->setFixedHeight( aSize );

    addWidget( spacer );
}


void ACTION_TOOLBAR::Add( QWidget* aControl, const wxString& aLabel )
{
    KI_ASSERT( aControl );

    int id = KIQT::GetWindowId( aControl );

    QAction* item = addWidget( aControl );

    if( !aLabel.IsEmpty() )
        item->setText( ToQString( aLabel ) );

    m_controlIDs.push_back( id );
    m_items[ id ] = item;
}


void ACTION_TOOLBAR::AddToolContextMenu( const TOOL_ACTION& aAction,
                                         std::unique_ptr<ACTION_MENU> aMenu )
{
    int toolId = aAction.GetUIId();

    m_toolMenus[toolId] = std::move( aMenu );
}


void ACTION_TOOLBAR::AddGroup( std::unique_ptr<ACTION_GROUP> aGroup )
{
    int                groupId = aGroup->GetUIId();
    const TOOL_ACTION* defaultAction = aGroup->GetDefaultAction();
    ACTION_GROUP*      group = aGroup.get();

    KI_ASSERT( defaultAction );

    // Turn this into a toggle entry if any one of the actions is a toggle entry
    bool isToggleEntry = false;

    for( const auto& act : aGroup->GetActions() )
        isToggleEntry |= act->CheckToolbarState( TOOLBAR_STATE::TOGGLE );

    m_toolKinds[ groupId ]    = isToggleEntry;
    m_toolActions[ groupId ]  = defaultAction;
    m_actionGroups[ groupId ] = std::move( aGroup );

    // The main toolbar item representing the group
    QAction* item = new QAction( KiBitmapBundleDef( defaultAction->GetIcon(), iconSize() ),
                                 QString(), this );
    item->setData( groupId );
    item->setCheckable( isToggleEntry );

    QObject::connect( item, &QAction::triggered, this,
                      [this, groupId]( bool aChecked )
                      {
                          onTriggered( groupId, aChecked );
                      } );

    // The palette: the actions of the group, after a long press on the button
    QMenu* palette = new QMenu( this );

    for( const TOOL_ACTION* action : group->GetActions() )
    {
        QAction* entry = palette->addAction( KiBitmapBundleDef( action->GetIcon(), iconSize() ),
                                             tooltipText( action->GetButtonTooltip() ) );

        QObject::connect( entry, &QAction::triggered, this,
                          [this, group, action]()
                          {
                              onPaletteAction( group, *action );
                          } );
    }

    item->setMenu( palette );
    addAction( item );

    if( QToolButton* button = qobject_cast<QToolButton*>( widgetForAction( item ) ) )
        button->setPopupMode( QToolButton::DelayedPopup );

    m_items[ groupId ] = item;

    // Select the default action
    doSelectAction( group, *defaultAction );
}


void ACTION_TOOLBAR::SelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction )
{
    bool valid = std::any_of( aGroup->m_actions.begin(), aGroup->m_actions.end(),
                              [&]( const TOOL_ACTION* action2 ) -> bool
                              {
                                  // For some reason, we can't compare the actions directly
                                  return aAction.GetId() == action2->GetId();
                              } );

    if( valid )
        doSelectAction( aGroup, aAction );
}


void ACTION_TOOLBAR::SelectAction( const TOOL_ACTION& aAction )
{
    // Find the group that contains this action and select it
    for( auto& [id, groupPtr] : m_actionGroups )
    {
        ACTION_GROUP* group = groupPtr.get();

        bool inGroup = std::any_of( group->m_actions.begin(), group->m_actions.end(),
                                    [&]( const TOOL_ACTION* action2 )
                                    {
                                        return aAction.GetId() == action2->GetId();
                                    } );

        if( inGroup )
        {
            doSelectAction( group, aAction );
            break;
        }
    }
}


void ACTION_TOOLBAR::doSelectAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction )
{
    int      groupId = aGroup->GetUIId();
    QAction* item = findItem( groupId );

    if( !item )
        return;

    // Update the item information
    item->setToolTip( tooltipText( aAction.GetButtonTooltip() ) );
    item->setIcon( KiBitmapBundleDef( aAction.GetIcon(), iconSize() ) );

    // Register a new handler with the new UI conditions
    if( m_toolManager )
    {
        m_toolManager->GetToolHolder()->UnregisterUIUpdateHandler( groupId );

        const ACTION_CONDITIONS* cond = m_toolManager->GetActionManager()->GetCondition( aAction );

        // Register the new UI condition to control this entry
        if( cond )
        {
            m_toolManager->GetToolHolder()->RegisterUIUpdateHandler( groupId, *cond );
        }
        else
        {
            KI_LOG_TRACE( kicadTraceToolStack, wxString::Format( "No UI condition for action %s",
                                                                 aAction.GetName() ) );
        }
    }

    // Update the currently selected action
    m_toolActions[ groupId ] = &aAction;
    m_groupSelections[aGroup->GetName()] = aAction.GetName();
}


QAction* ACTION_TOOLBAR::findItem( int aId ) const
{
    auto it = m_items.find( aId );
    return it == m_items.end() ? nullptr : it->second;
}


void ACTION_TOOLBAR::UpdateControlWidths()
{
    for( int id : m_controlIDs )
        UpdateControlWidth( id );
}


void ACTION_TOOLBAR::UpdateControlWidth( int aID )
{
    // The layout of the toolbar follows the size hints of the controls
    if( QAction* item = findItem( aID ) )
    {
        if( QWidget* control = widgetForAction( item ) )
            control->updateGeometry();
    }
}


void ACTION_TOOLBAR::ClearToolbar()
{
    // Clear all the maps keeping track of our items on the toolbar
    m_toolMenus.clear();
    m_actionGroups.clear();
    m_toolCancellable.clear();
    m_toolKinds.clear();
    m_toolActions.clear();

    for( int id : m_controlIDs )
        m_parent->ClearToolbarControl( id );

    m_controlIDs.clear();
    m_items.clear();

    // Remove the actual tools (and the controls) from the toolbar
    for( QAction* action : actions() )
    {
        removeAction( action );

        if( action->parent() == this )
            action->deleteLater();
    }

    clear();
}


void ACTION_TOOLBAR::SetToolBitmap( const TOOL_ACTION& aAction, const QIcon& aBitmap )
{
    if( QAction* item = findItem( aAction.GetUIId() ) )
        item->setIcon( aBitmap );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aState )
{
    int      toolId = aAction.GetUIId();
    QAction* item = findItem( toolId );

    if( !item )
        return;

    if( m_toolKinds[ toolId ] )
        item->setChecked( aState );
    else
        item->setEnabled( aState );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked )
{
    if( QAction* item = findItem( aAction.GetUIId() ) )
    {
        item->setEnabled( aEnabled );
        item->setChecked( aEnabled && aChecked );
    }
}


void ACTION_TOOLBAR::onTriggered( int aId, bool aChecked )
{
    if( !m_toolManager )
        return;

    OPT_TOOL_EVENT evt;

    const auto actionIt = m_toolActions.find( aId );
    const auto cancelIt = m_toolCancellable.find( aId );
    const auto groupIt  = m_actionGroups.find( aId );

    // Determine if the tool is actually cancellable
    bool isCancellable = ( cancelIt != m_toolCancellable.end() ) ? cancelIt->second : false;

    // The selection tool is a special case because it is the "default" tool and does not show
    // up on the tool stack. We want to toggle through selection modes only when the tool is
    // already active.
    bool selectionSpecialCase = false;

    if( actionIt != m_toolActions.end() )
    {
        selectionSpecialCase = m_parent->ToolStackIsEmpty()
                               && ( actionIt->second->GetId() == ACTIONS::selectSetRect.GetId()
                                    || actionIt->second->GetId() == ACTIONS::selectSetLasso.GetId() );
    }

    // The toolbar item is toggled before it is triggered, so it is not checked anymore when it
    // was checked: the tool is cancelled
    if( isCancellable && !aChecked )
    {
        m_toolManager->CancelTool();
    }
    else if( groupIt != m_actionGroups.end()
             && ( selectionSpecialCase
                  || std::none_of( groupIt->second->GetActions().begin(),
                                   groupIt->second->GetActions().end(),
                                   []( const TOOL_ACTION* a )
                                   {
                                       return a->IsActivation();
                                   } ) ) )
    {
        // For non-tool toggle groups (units, crosshair, line modes), cycle to the next
        // action on click. Tool groups (route track, etc.) fall through and just dispatch
        // the currently displayed action.
        ACTION_GROUP*                          group   = groupIt->second.get();
        const std::vector<const TOOL_ACTION*>& actions = group->GetActions();
        const TOOL_ACTION*                     current = actionIt->second;

        const TOOL_ACTION* next = actions[0];

        for( size_t i = 0; i < actions.size(); ++i )
        {
            if( actions[i]->GetId() == current->GetId() )
            {
                next = actions[( i + 1 ) % actions.size()];
                break;
            }
        }

        evt = next->MakeEvent();
        evt->SetHasPosition( false );
        m_toolManager->ProcessEvent( *evt );
        m_toolManager->GetToolHolder()->RefreshCanvas();

        doSelectAction( group, *next );
    }
    else if( actionIt != m_toolActions.end() )
    {
        // Dispatch a tool event
        evt = actionIt->second->MakeEvent();
        evt->SetHasPosition( false );
        m_toolManager->ProcessEvent( *evt );
        m_toolManager->GetToolHolder()->RefreshCanvas();
    }

    // The state of the buttons follows the UI conditions
    m_parent->UpdateUIConditions();
}


void ACTION_TOOLBAR::onPaletteAction( ACTION_GROUP* aGroup, const TOOL_ACTION& aAction )
{
    if( !m_toolManager )
        return;

    // Dispatch a tool event
    OPT_TOOL_EVENT evt = aAction.MakeEvent();
    evt->SetHasPosition( false );
    m_toolManager->ProcessEvent( *evt );
    m_toolManager->GetToolHolder()->RefreshCanvas();

    // Update the main toolbar item with the selected action
    doSelectAction( aGroup, aAction );

    m_parent->UpdateUIConditions();
}


void ACTION_TOOLBAR::contextMenuEvent( QContextMenuEvent* aEvent )
{
    QAction* item = actionAt( aEvent->pos() );

    if( !item || !item->data().isValid() )
    {
        QToolBar::contextMenuEvent( aEvent );
        return;
    }

    showContextMenu( item->data().toInt() );
    aEvent->accept();
}


void ACTION_TOOLBAR::showContextMenu( int aToolId )
{
    // Ensure that the ID maps to a proper tool ID. If right-clicked on a group item, this is
    // needed to get the ID of the currently selected action, since the ID is that of the group.
    const auto actionIt = m_toolActions.find( aToolId );

    if( actionIt != m_toolActions.end() )
        aToolId = actionIt->second->GetUIId();

    // Find the menu for the action
    const auto menuIt = m_toolMenus.find( aToolId );

    if( menuIt == m_toolMenus.end() )
        return;

    // Update and show the menu
    ACTION_MENU* menu = menuIt->second.get();
    SELECTION    dummySel;

    if( CONDITIONAL_MENU* condMenu = dynamic_cast<CONDITIONAL_MENU*>( menu ) )
        condMenu->Evaluate( dummySel );

    menu->UpdateAll();
    menu->exec( QCursor::pos() );
}


bool ACTION_TOOLBAR::KiRealize()
{
    return true;
}


void ACTION_TOOLBAR::RefreshBitmaps()
{
    setIconSize( QSize( iconSize(), iconSize() ) );

    for( const auto& [toolId, action] : m_toolActions )
    {
        if( QAction* item = findItem( toolId ) )
            item->setIcon( KiBitmapBundleDef( action->GetIcon(), iconSize() ) );
    }

    for( const auto& [groupId, group] : m_actionGroups )
    {
        QAction* item = findItem( groupId );

        if( !item || !item->menu() )
            continue;

        QList<QAction*> entries = item->menu()->actions();

        for( size_t ii = 0; ii < group->GetActions().size() && ii < (size_t) entries.size(); ++ii )
            entries[ii]->setIcon( KiBitmapBundleDef( group->GetActions()[ii]->GetIcon(), iconSize() ) );
    }
}


void ACTION_TOOLBAR::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::PaletteChange || aEvent->type() == QEvent::ThemeChange )
        RefreshBitmaps();

    QToolBar::changeEvent( aEvent );
}


wxFont ACTION_TOOLBAR::GetFont() const
{
    return wxFont( font() );
}
