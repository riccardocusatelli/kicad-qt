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

#include <tool/action_menu.h>

#include <QAction>
#include <QApplication>
#include <QCursor>

#include <bitmaps.h>
#include <eda_base_frame.h>
#include <functional>
#include <hotkeys_basic.h>
#include <id.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <tool/actions.h>
#include <tool/action_manager.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <wx/kiwx_menu.h>

using namespace std::placeholders;


/// The ID of an entry (the QAction data), or -1 (separators, titles, submenus).
static int itemId( const QAction* aAction )
{
    bool ok = false;
    int  id = aAction->data().toInt( &ok );

    return ok ? id : -1;
}


namespace
{
/// The wxMenuItem of an action of an ACTION_MENU: a child of the action, deleted with it.
class MENU_ITEM_HOLDER : public QObject
{
public:
    MENU_ITEM_HOLDER( QAction* aAction ) :
            QObject( aAction ),
            m_item( aAction, itemId( aAction ) >= 0 ? itemId( aAction ) : int( wxID_NONE ) )
    {
    }

    wxMenuItem* Item() { return &m_item; }

private:
    wxMenuItem m_item;
};
}


ACTION_MENU_ITEM::operator wxMenuItem*() const
{
    if( !m_action )
        return nullptr;

    MENU_ITEM_HOLDER* holder = m_action->findChild<MENU_ITEM_HOLDER*>( QString(),
                                                                       Qt::FindDirectChildrenOnly );

    if( !holder )
        holder = new MENU_ITEM_HOLDER( m_action );

    return holder->Item();
}


int ACTION_MENU_ITEM::GetId() const
{
    int id = m_action ? itemId( m_action ) : -1;
    return id >= 0 ? id : int( wxID_NONE );
}


void ACTION_MENU_ITEM::Enable( bool aEnable )
{
    if( m_action )
        m_action->setEnabled( aEnable );
}


bool ACTION_MENU_ITEM::IsEnabled() const
{
    return m_action && m_action->isEnabled();
}


void ACTION_MENU_ITEM::Check( bool aCheck )
{
    if( m_action )
        m_action->setChecked( aCheck );
}


bool ACTION_MENU_ITEM::IsChecked() const
{
    return m_action && m_action->isChecked();
}


void ACTION_MENU_ITEM::SetItemLabel( const wxString& aLabel )
{
    if( m_action )
        m_action->setText( ToQString( aLabel ) );
}


/// The icons are shown in the menus if the user wants it, except for check entries.
static bool useIcons()
{
    return Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Appearance.use_icons_in_menus;
}


ACTION_MENU::ACTION_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool ) :
    m_dirty( true ),
    m_titleDisplayed( false ),
    m_isContextMenu( isContextMenu ),
    m_icon( BITMAPS::INVALID_BITMAP ),
    m_selected( -1 ),
    m_tool( aTool ),
    m_parentMenu( nullptr )
{
    setToolTipsVisible( true );

    connect( this, &QMenu::aboutToShow, this, &ACTION_MENU::onAboutToShow );
    connect( this, &QMenu::hovered, this, &ACTION_MENU::onHovered );
}


ACTION_MENU::~ACTION_MENU()
{
    // The submenus are deleted with this menu (they are its children): they must not
    // unregister from it
    for( ACTION_MENU* menu : m_submenus )
        menu->m_parentMenu = nullptr;

    if( m_parentMenu )
        m_parentMenu->m_submenus.remove( this );
}


void ACTION_MENU::SetIcon( BITMAPS aIcon )
{
    m_icon = aIcon;
}


void ACTION_MENU::SetTitle( const wxString& aTitle )
{
    m_title = aTitle;

    // Update the menu title
    if( m_titleDisplayed )
        DisplayTitle( true );
}


void ACTION_MENU::DisplayTitle( bool aDisplay )
{
    if( ( !aDisplay || m_title.IsEmpty() ) && m_titleDisplayed )
    {
        // Remove the entry of the title and its separator
        QList<QAction*> entries = actions();

        for( int ii = 0; ii < 2 && ii < entries.size(); ++ii )
        {
            removeAction( entries[ii] );
            m_labels.erase( entries[ii] );
            delete entries[ii];
        }

        m_titleDisplayed = false;
    }
    else if( aDisplay && !m_title.IsEmpty() )
    {
        if( m_titleDisplayed )
        {
            // Simply update the title
            actions().first()->setText( ToQString( m_title ) );
        }
        else
        {
            // Add a separator and a menu entry to display the title
            QAction* title = new QAction( ToQString( m_title ), this );
            title->setEnabled( false );

            if( !!m_icon && useIcons() )
                title->setIcon( KiBitmapBundle( m_icon ) );

            QAction* first = actions().isEmpty() ? nullptr : actions().first();
            insertAction( first, title );
            insertSeparator( first );

            m_titleDisplayed = true;
        }
    }
}


QAction* ACTION_MENU::newItem( int aId, const wxString& aLabel, const wxString& aToolTip,
                               BITMAPS aIcon, bool aIsCheckmarkEntry )
{
    // A tab separates the label from its shortcut, which the menu shows in a column
    QString label = ToQString( aLabel );
    QAction* item = new QAction( label, this );

    item->setData( aId );
    item->setToolTip( ToQString( aToolTip ) );
    item->setStatusTip( ToQString( aToolTip ) );
    item->setCheckable( aIsCheckmarkEntry );

    if( !!aIcon && !aIsCheckmarkEntry && useIcons() )
        item->setIcon( KiBitmapBundle( aIcon ) );

    m_labels[item] = label.section( QLatin1Char( '\t' ), 0, 0 );

    connect( item, &QAction::triggered, this,
             [this, item]()
             {
                 onTriggered( item );
             } );

    return item;
}


ACTION_MENU_ITEM ACTION_MENU::Add( const wxString& aLabel, int aId, BITMAPS aIcon )
{
    KI_ASSERT_MSG( FindItem( aId ) == nullptr, wxS( "Duplicate menu IDs!" ) );

    QAction* item = newItem( aId, aLabel, wxEmptyString, aIcon, false );
    addAction( item );
    return item;
}


ACTION_MENU_ITEM ACTION_MENU::Add( const wxString& aLabel, const wxString& aTooltip, int aId,
                                   BITMAPS aIcon, bool aIsCheckmarkEntry )
{
    KI_ASSERT_MSG( FindItem( aId ) == nullptr, wxS( "Duplicate menu IDs!" ) );

    QAction* item = newItem( aId, aLabel, aTooltip, aIcon, aIsCheckmarkEntry );
    addAction( item );
    return item;
}


ACTION_MENU_ITEM ACTION_MENU::Add( const TOOL_ACTION& aAction, bool aIsCheckmarkEntry,
                                   const wxString& aOverrideLabel )
{
    // ID numbers for tool actions are assigned above ACTION_BASE_UI_ID inside TOOL_EVENT
    BITMAPS icon = aAction.GetIcon();

    // Allow the label to be overridden at point of use
    wxString menuLabel = aOverrideLabel.IsEmpty() ? aAction.GetMenuItem() : aOverrideLabel;

    QAction* item = newItem( aAction.GetUIId(), menuLabel, aAction.GetTooltip(), icon,
                             aIsCheckmarkEntry );

    m_toolActions[aAction.GetUIId()] = &aAction;

    addAction( item );
    return item;
}


ACTION_MENU_ITEM ACTION_MENU::Add( ACTION_MENU* aMenu )
{
    m_submenus.push_back( aMenu );
    aMenu->m_parentMenu = this;

    // The menu owns its submenus (a popup keeps its window flags)
    aMenu->setParent( this, aMenu->windowFlags() );

    KI_ASSERT_MSG( !aMenu->m_title.IsEmpty(), wxS( "Set a title for ACTION_MENU using SetTitle()" ) );

    aMenu->setTitle( ToQString( aMenu->m_title ) );

    if( !!aMenu->m_icon && useIcons() )
        aMenu->setIcon( KiBitmapBundle( aMenu->m_icon ) );

    return addMenu( aMenu );
}


void ACTION_MENU::AppendSeparator()
{
    addSeparator();
}


void ACTION_MENU::AddClose( const wxString& aAppname )
{
    Add( _( "Close" ) + wxS( "\tCtrl+W" ),
         wxString::Format( _( "Close %s" ), aAppname ),
         wxID_CLOSE,
         BITMAPS::exit );
}


void ACTION_MENU::AddQuitOrClose( KIFACE_BASE* aKiface, wxString aAppname )
{
    if( !aKiface || aKiface->IsSingle() ) // not when under a project mgr
        AddQuit( aAppname );
    else
        AddClose( aAppname );
}


void ACTION_MENU::AddQuit( const wxString& aAppname )
{
    Add( _( "Quit" ) + wxS( "\tCtrl+Q" ),
         wxString::Format( _( "Quit %s" ), aAppname ),
         wxID_EXIT,
         BITMAPS::exit );
}


void ACTION_MENU::Clear()
{
    m_titleDisplayed = false;

    const QList<QAction*> entries = actions();

    for( QAction* entry : entries )
    {
        removeAction( entry );

        // The entries of the submenus are their menu actions, owned by the submenus
        if( entry->parent() == this )
            delete entry;
    }

    std::list<ACTION_MENU*> submenus;
    submenus.swap( m_submenus );

    for( ACTION_MENU* menu : submenus )
    {
        menu->m_parentMenu = nullptr;
        delete menu;
    }

    m_labels.clear();
    m_toolActions.clear();

    KI_ASSERT( GetMenuItemCount() == 0 );
}


bool ACTION_MENU::HasEnabledItems() const
{
    for( const QAction* item : actions() )
    {
        if( item->isEnabled() && !item->isSeparator() )
            return true;
    }

    return false;
}


int ACTION_MENU::GetMenuItemCount() const
{
    return actions().size();
}


QAction* ACTION_MENU::FindItem( int aId ) const
{
    for( QAction* item : actions() )
    {
        if( item->menu() )
        {
            if( ACTION_MENU* submenu = dynamic_cast<ACTION_MENU*>( item->menu() ) )
            {
                if( QAction* found = submenu->FindItem( aId ) )
                    return found;
            }
        }
        else if( !item->isSeparator() && itemId( item ) == aId )
        {
            return item;
        }
    }

    return nullptr;
}


void ACTION_MENU::Check( int aId, bool aCheck )
{
    if( QAction* item = FindItem( aId ) )
        item->setChecked( aCheck );
}


void ACTION_MENU::Enable( int aId, bool aEnable )
{
    if( QAction* item = FindItem( aId ) )
        item->setEnabled( aEnable );
}


bool ACTION_MENU::IsChecked( int aId ) const
{
    QAction* item = FindItem( aId );
    return item && item->isChecked();
}


bool ACTION_MENU::IsEnabled( int aId ) const
{
    QAction* item = FindItem( aId );
    return item && item->isEnabled();
}


void ACTION_MENU::UpdateAll()
{
    try
    {
        update();
    }
    catch( std::exception& )
    {
    }

    if( m_tool )
        updateHotKeys();

    runOnSubmenus( std::bind( &ACTION_MENU::UpdateAll, _1 ) );
}


void ACTION_MENU::ClearDirty()
{
    m_dirty = false;
    runOnSubmenus( std::bind( &ACTION_MENU::ClearDirty, _1 ) );
}


void ACTION_MENU::SetDirty()
{
    m_dirty = true;
    runOnSubmenus( std::bind( &ACTION_MENU::SetDirty, _1 ) );
}


void ACTION_MENU::SetTool( TOOL_INTERACTIVE* aTool )
{
    m_tool = aTool;
    runOnSubmenus( std::bind( &ACTION_MENU::SetTool, _1, aTool ) );
}


ACTION_MENU* ACTION_MENU::Clone() const
{
    ACTION_MENU* clone = create();
    clone->Clear();
    clone->copyFrom( *this );
    return clone;
}


void ACTION_MENU::PopupAtMouse( TOOLS_HOLDER* aHolder )
{
    // The menu is modal: the tools see its choice when it closes
    exec( QCursor::pos() );
}


ACTION_MENU* ACTION_MENU::create() const
{
    ACTION_MENU* menu = new ACTION_MENU( false );

    KI_ASSERT_MSG( typeid( *this ) == typeid( *menu ),
                  wxString::Format( "You need to override create() method for class %s", typeid( *this ).name() ) );

    return menu;
}


TOOL_MANAGER* ACTION_MENU::getToolManager() const
{
    return m_tool ? m_tool->GetManager() : nullptr;
}


void ACTION_MENU::updateHotKeys()
{
    TOOL_MANAGER* toolMgr = getToolManager();

    KI_ASSERT( toolMgr );

    for( std::pair<const int, const TOOL_ACTION*>& ii : m_toolActions )
    {
        int                id = ii.first;
        const TOOL_ACTION& action = *ii.second;
        int                hotkey = toolMgr->GetHotKey( action );
        QAction*           item = FindItem( id );

        if( !item )
            continue;

        auto labelIt = m_labels.find( item );
        QString label = labelIt != m_labels.end() ? labelIt->second : item->text();

        // The menu shows the text after a tab as the shortcut of the entry
        if( ( hotkey & ~MD_MODIFIER_MASK ) > 0 )
            label += QLatin1Char( '\t' ) + ToQString( KeyNameFromKeyCode( hotkey ) );

        item->setText( label );
    }
}


// The position of the mouse when a context menu was opened, given to the command event of the
// entry chosen in it.
static VECTOR2D g_menu_open_position;


void ACTION_MENU::onAboutToShow()
{
    TOOL_MANAGER* toolMgr = getToolManager();

    if( m_dirty && toolMgr )
        toolMgr->RunAction<ACTION_MENU*>( ACTIONS::updateMenu, this );

    // Don't update the position if this menu is a submenu or a menubar menu
    if( !m_parentMenu && m_isContextMenu && toolMgr )
        g_menu_open_position = toolMgr->GetMousePosition();
}


void ACTION_MENU::onHovered( QAction* aAction )
{
    TOOL_MANAGER* toolMgr = getToolManager();
    int           id = itemId( aAction );

    // The hovered() signal of a menu is also emitted for the entries of its submenus: the entry
    // is handled by the menu it belongs to
    if( !toolMgr || id < 0 || !actions().contains( aAction ) )
        return;

    TOOL_EVENT evt( TC_COMMAND, TA_CHOICE_MENU_UPDATE, id );

    if( m_isContextMenu )
        evt.SetMousePosition( g_menu_open_position );
    else
        evt.SetHasPosition( false );

    toolMgr->ProcessEvent( evt );
}


void ACTION_MENU::onTriggered( QAction* aAction )
{
    OPT_TOOL_EVENT evt;
    wxString       menuText;
    TOOL_MANAGER*  toolMgr = getToolManager();

    // Store the selected position, so it can be checked by the tools
    m_selected = itemId( aAction );

    for( ACTION_MENU* parent = m_parentMenu; parent; parent = parent->m_parentMenu )
        parent->m_selected = m_selected;

    // Check if there is a TOOL_ACTION for the given UI ID
    if( toolMgr && toolMgr->GetActionManager()->IsActionUIId( m_selected ) )
        evt = findToolAction( m_selected );

    if( !evt )
    {
        runEventHandlers( m_selected, evt );

        // Handling non-ACTION menu entries.  Two ranges of ids are supported:
        //   between 0 and ID_CONTEXT_MENU_ID_MAX
        //   between ID_POPUP_MENU_START and ID_POPUP_MENU_END

        #define ID_CONTEXT_MENU_ID_MAX wxID_LOWEST  /* = 100 should be plenty */

        if( !evt && (   ( m_selected >= 0 && m_selected < ID_CONTEXT_MENU_ID_MAX )
                     || ( m_selected >= ID_POPUP_MENU_START && m_selected <= ID_POPUP_MENU_END ) ) )
        {
            if( PassHelpTextToHandler() || ( m_parentMenu && m_parentMenu->PassHelpTextToHandler() ) )
            {
                menuText = ToWxString( aAction->toolTip() );
            }
            else
            {
                auto labelIt = m_labels.find( aAction );
                menuText = ToWxString( labelIt != m_labels.end() ? labelIt->second
                                                                 : aAction->text() );
                menuText.Replace( wxS( "&" ), wxEmptyString );
            }

            evt = TOOL_EVENT( TC_COMMAND, TA_CHOICE_MENU_CHOICE, m_selected, AS_GLOBAL );
            evt->SetParameter( &menuText );
        }
    }

    // The other ids are commands of the frame (as the menu events of wxWidgets, which go to
    // the frame)
    if( !evt )
    {
        EDA_BASE_FRAME* frame = toolMgr ? dynamic_cast<EDA_BASE_FRAME*>( toolMgr->GetToolHolder() )
                                        : nullptr;

        if( !frame )
            frame = dynamic_cast<EDA_BASE_FRAME*>( QApplication::activeWindow() );

        if( frame )
            frame->RunMenuCommand( m_selected );

        return;
    }

    // forward the action/update event to the TOOL_MANAGER
    // clients that don't supply a tool will have to check GetSelected() themselves
    if( evt && toolMgr )
    {
        KI_LOG_TRACE( kicadTraceToolStack, wxS( "ACTION_MENU::onTriggered %s" ), evt->Format() );

        // A context menu gives the position where it was opened; an entry of a menu of the
        // menu bar has no position (the hotkeys don't go through the menus: the TOOL_DISPATCHER
        // gives them the position of the mouse).
        bool contextMenu = m_isContextMenu;

        for( ACTION_MENU* parent = m_parentMenu; parent; parent = parent->m_parentMenu )
            contextMenu |= parent->m_isContextMenu;

        if( contextMenu )
            evt->SetMousePosition( g_menu_open_position );
        else
            evt->SetHasPosition( false );

        toolMgr->ProcessEvent( *evt );
    }
}


void ACTION_MENU::runEventHandlers( int aId, OPT_TOOL_EVENT& aToolEvent )
{
    aToolEvent = eventHandler( aId );

    if( !aToolEvent )
        runOnSubmenus( std::bind( &ACTION_MENU::runEventHandlers, _1, aId, aToolEvent ) );
}


void ACTION_MENU::runOnSubmenus( std::function<void(ACTION_MENU*)> aFunction )
{
    try
    {
        std::for_each( m_submenus.begin(), m_submenus.end(),
                       [&]( ACTION_MENU* m )
                       {
                           aFunction( m );
                           m->runOnSubmenus( aFunction );
                       } );
    }
    catch( std::exception& )
    {
    }
}


OPT_TOOL_EVENT ACTION_MENU::findToolAction( int aId )
{
    OPT_TOOL_EVENT evt;

    auto findFunc =
            [&]( ACTION_MENU* m )
            {
                if( evt )
                    return;

                const auto it = m->m_toolActions.find( aId );

                if( it != m->m_toolActions.end() )
                    evt = it->second->MakeEvent();
            };

    findFunc( this );

    if( !evt )
        runOnSubmenus( findFunc );

    return evt;
}


void ACTION_MENU::copyFrom( const ACTION_MENU& aMenu )
{
    m_icon = aMenu.m_icon;
    m_title = aMenu.m_title;
    m_titleDisplayed = aMenu.m_titleDisplayed;
    m_selected = -1; // aMenu.m_selected;
    m_tool = aMenu.m_tool;
    m_toolActions = aMenu.m_toolActions;

    // Copy all menu entries
    for( const QAction* item : aMenu.actions() )
        appendCopy( aMenu, item );
}


QAction* ACTION_MENU::appendCopy( const ACTION_MENU& aMenu, const QAction* aSource )
{
    if( aSource->isSeparator() )
        return addSeparator();

    if( aSource->menu() )
    {
        ACTION_MENU* menu = dynamic_cast<ACTION_MENU*>( aSource->menu() );
        KI_ASSERT_MSG( menu, wxS( "Submenus are expected to be a ACTION_MENU" ) );

        if( !menu )
            return nullptr;

        return Add( menu->Clone() );
    }

    QAction* newItem = new QAction( aSource->icon(), aSource->text(), this );

    newItem->setData( aSource->data() );
    newItem->setToolTip( aSource->toolTip() );
    newItem->setStatusTip( aSource->statusTip() );
    newItem->setCheckable( aSource->isCheckable() );
    newItem->setChecked( aSource->isChecked() );
    newItem->setEnabled( aSource->isEnabled() );

    auto labelIt = aMenu.m_labels.find( aSource );

    if( labelIt != aMenu.m_labels.end() )
        m_labels[newItem] = labelIt->second;

    if( itemId( aSource ) >= 0 )
    {
        connect( newItem, &QAction::triggered, this,
                 [this, newItem]()
                 {
                     onTriggered( newItem );
                 } );
    }

    addAction( newItem );
    return newItem;
}


ACTION_MENU_ITEM KIUI::AddMenuItem( ACTION_MENU* aMenu, int aId, const wxString& aText,
                                    const wxString& aHelpText, const wxBitmapBundle& aImage,
                                    wxItemKind aType )
{
    int id = aId == wxID_ANY ? wxWindowIDNew() : aId;

    ACTION_MENU_ITEM item = aMenu->Add( aText, aHelpText, id, BITMAPS::INVALID_BITMAP,
                                        aType == wxITEM_CHECK || aType == wxITEM_RADIO );

    if( Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Appearance.use_icons_in_menus )
        static_cast<QAction*>( item )->setIcon( aImage );

    return item;
}
