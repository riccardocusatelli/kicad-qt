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

#include <wx/kiwx_menu.h>

#include <algorithm>

#include <QAction>
#include <QActionGroup>
#include <QCursor>
#include <QKeySequence>
#include <QRegularExpression>
#include <QWidget>

#include <qt_strings.h>
#include <wx/kiwx_gui.h>


static const char* const ITEM_PROPERTY = "kiwxMenuItem";


/// The label of an item without its accelerator ("Copy\tCtrl+C") and the accelerator.
static void splitLabel( const wxString& aText, QString* aLabel, QString* aAccel )
{
    QString text = ToQString( aText );
    int     tab = text.indexOf( '\t' );

    *aLabel = tab >= 0 ? text.left( tab ) : text;
    *aAccel = tab >= 0 ? text.mid( tab + 1 ) : QString();
}


// wxMenuItem --------------------------------------------------------------------------------

wxMenuItem::wxMenuItem( wxMenu* aParentMenu, int aId, const wxString& aText,
                        const wxString& aHelp, wxItemKind aKind, wxMenu* aSubMenu ) :
        m_parentMenu( aParentMenu ),
        m_id( aId == wxID_ANY ? wxWindowIDNew() : aId ),
        m_text( aText ),
        m_help( aHelp ),
        m_kind( aId == wxID_SEPARATOR ? wxITEM_SEPARATOR : aKind ),
        m_subMenu( aSubMenu ),
        m_action( nullptr )
{
}


wxMenuItem::~wxMenuItem()
{
    if( m_action )
    {
        m_action->setProperty( ITEM_PROPERTY, QVariant() );

        // The actions of the sub-menus belong to their menus
        if( !m_subMenu )
            delete m_action;
    }

    delete m_subMenu;
}


void wxMenuItem::attach( wxMenu* aMenu, QAction* aBefore )
{
    m_parentMenu = aMenu;

    if( m_kind == wxITEM_SEPARATOR )
    {
        m_action = new QAction( aMenu );
        m_action->setSeparator( true );
    }
    else if( m_subMenu )
    {
        m_subMenu->SetParent( aMenu );
        m_subMenu->setTitle( ToQString( m_text ) );
        m_action = m_subMenu->menuAction();
    }
    else
    {
        m_action = new QAction( aMenu );
        m_action->setCheckable( IsCheckable() );
    }

    m_action->setProperty( ITEM_PROPERTY, QVariant::fromValue( static_cast<void*>( this ) ) );

    if( !m_subMenu )
        SetItemLabel( m_text );

    SetHelp( m_help );
    aMenu->insertAction( aBefore, m_action );

    // The radio items next to each other are exclusive
    if( m_kind == wxITEM_RADIO )
    {
        QList<QAction*> actions = aMenu->actions();
        int             index = actions.indexOf( m_action );
        QActionGroup*   group = nullptr;

        if( index > 0 )
        {
            wxMenuItem* previous = wxMenu::ItemOfActionCompat( actions[index - 1] );

            if( previous && previous->m_kind == wxITEM_RADIO )
                group = actions[index - 1]->actionGroup();
        }

        if( !group )
        {
            group = new QActionGroup( aMenu );
            m_action->setChecked( true );
        }

        group->addAction( m_action );
    }
}


void wxMenuItem::SetItemLabel( const wxString& aText )
{
    m_text = aText;

    if( !m_action )
        return;

    QString label;
    QString accel;
    splitLabel( aText, &label, &accel );

    if( m_subMenu )
    {
        m_subMenu->setTitle( label );
        return;
    }

    m_action->setText( label );

    // The accelerator is shown in the menu: the actions of the tools handle the keys
    m_action->setShortcut( QKeySequence( accel ) );
    m_action->setShortcutContext( Qt::WidgetShortcut );
}


wxString wxMenuItem::GetItemLabel() const
{
    return m_text;
}


wxString wxMenuItem::GetItemLabelText() const
{
    QString label;
    QString accel;
    splitLabel( m_text, &label, &accel );
    label.remove( QRegularExpression( QStringLiteral( "&(?!&)" ) ) );
    label.replace( QStringLiteral( "&&" ), QStringLiteral( "&" ) );
    return ToWxString( label );
}


void wxMenuItem::SetHelp( const wxString& aHelp )
{
    m_help = aHelp;

    if( m_action )
    {
        m_action->setStatusTip( ToQString( aHelp ) );
        m_action->setToolTip( ToQString( aHelp ) );
    }
}


wxString wxMenuItem::GetHelp() const
{
    return m_help;
}


void wxMenuItem::Enable( bool aEnable )
{
    if( m_action )
        m_action->setEnabled( aEnable );
}


bool wxMenuItem::IsEnabled() const
{
    return !m_action || m_action->isEnabled();
}


void wxMenuItem::Check( bool aCheck )
{
    if( m_action && m_action->isCheckable() )
        m_action->setChecked( aCheck );
}


bool wxMenuItem::IsChecked() const
{
    return m_action && m_action->isChecked();
}


void wxMenuItem::SetBitmap( const wxBitmapBundle& aBitmap )
{
    if( m_action )
        m_action->setIcon( aBitmap );
}


// wxMenu ------------------------------------------------------------------------------------

wxMenu::wxMenu( const wxString& aTitle, long aStyle ) :
        QMenu( ToQString( aTitle ) ),
        m_parentMenu( nullptr )
{
}


wxMenu::~wxMenu()
{
    // The items go before the actions of the menu
    m_items.clear();
}


wxMenuItem* wxMenu::ItemOfActionCompat( QAction* aAction )
{
    if( !aAction )
        return nullptr;

    return static_cast<wxMenuItem*>( aAction->property( ITEM_PROPERTY ).value<void*>() );
}


void wxMenu::insertItem( size_t aPos, wxMenuItem* aItem )
{
    aPos = std::min( aPos, m_items.size() );

    QAction* before = aPos < m_items.size() ? m_items[aPos]->m_action : nullptr;
    aItem->attach( this, before );
    m_items.insert( m_items.begin() + aPos, std::unique_ptr<wxMenuItem>( aItem ) );
}


wxMenuItem* wxMenu::Append( int aId, const wxString& aText, const wxString& aHelp,
                            wxItemKind aKind )
{
    return Append( new wxMenuItem( this, aId, aText, aHelp, aKind ) );
}


wxMenuItem* wxMenu::Append( int aId, const wxString& aText, wxMenu* aSubMenu,
                            const wxString& aHelp )
{
    return Append( new wxMenuItem( this, aId, aText, aHelp, wxITEM_NORMAL, aSubMenu ) );
}


wxMenuItem* wxMenu::Append( wxMenuItem* aItem )
{
    insertItem( m_items.size(), aItem );
    return aItem;
}


wxMenuItem* wxMenu::AppendCheckItem( int aId, const wxString& aText, const wxString& aHelp )
{
    return Append( aId, aText, aHelp, wxITEM_CHECK );
}


wxMenuItem* wxMenu::AppendRadioItem( int aId, const wxString& aText, const wxString& aHelp )
{
    return Append( aId, aText, aHelp, wxITEM_RADIO );
}


wxMenuItem* wxMenu::AppendSeparator()
{
    return Append( wxID_SEPARATOR );
}


wxMenuItem* wxMenu::AppendSubMenu( wxMenu* aSubMenu, const wxString& aText, const wxString& aHelp )
{
    return Append( wxID_ANY, aText, aSubMenu, aHelp );
}


wxMenuItem* wxMenu::Insert( size_t aPos, wxMenuItem* aItem )
{
    insertItem( aPos, aItem );
    return aItem;
}


wxMenuItem* wxMenu::Insert( size_t aPos, int aId, const wxString& aText, const wxString& aHelp,
                            wxItemKind aKind )
{
    return Insert( aPos, new wxMenuItem( this, aId, aText, aHelp, aKind ) );
}


wxMenuItem* wxMenu::InsertSeparator( size_t aPos )
{
    return Insert( aPos, wxID_SEPARATOR );
}


wxMenuItem* wxMenu::Remove( wxMenuItem* aItem )
{
    auto it = std::find_if( m_items.begin(), m_items.end(),
                            [&]( const std::unique_ptr<wxMenuItem>& aEntry )
                            {
                                return aEntry.get() == aItem;
                            } );

    if( it == m_items.end() )
        return nullptr;

    wxMenuItem* item = it->release();
    m_items.erase( it );

    if( item->m_action )
    {
        removeAction( item->m_action );
        item->m_action->setProperty( ITEM_PROPERTY, QVariant() );

        if( !item->m_subMenu )
            delete item->m_action;

        item->m_action = nullptr;
    }

    item->m_parentMenu = nullptr;
    return item;
}


wxMenuItem* wxMenu::Remove( int aId )
{
    return Remove( FindChildItem( aId ) );
}


bool wxMenu::Delete( wxMenuItem* aItem )
{
    wxMenuItem* item = Remove( aItem );

    if( !item )
        return false;

    // Delete() keeps the sub-menu
    item->m_subMenu = nullptr;
    delete item;
    return true;
}


bool wxMenu::Delete( int aId )
{
    return Delete( FindChildItem( aId ) );
}


bool wxMenu::Destroy( wxMenuItem* aItem )
{
    wxMenuItem* item = Remove( aItem );
    delete item;
    return item != nullptr;
}


bool wxMenu::Destroy( int aId )
{
    return Destroy( FindChildItem( aId ) );
}


wxMenuItem* wxMenu::FindItem( int aId, wxMenu** aMenu ) const
{
    for( const std::unique_ptr<wxMenuItem>& item : m_items )
    {
        if( item->GetId() == aId )
        {
            if( aMenu )
                *aMenu = const_cast<wxMenu*>( this );

            return item.get();
        }

        if( item->GetSubMenu() )
        {
            if( wxMenuItem* found = item->GetSubMenu()->FindItem( aId, aMenu ) )
                return found;
        }
    }

    return nullptr;
}


int wxMenu::FindItem( const wxString& aText ) const
{
    for( const std::unique_ptr<wxMenuItem>& item : m_items )
    {
        if( item->GetItemLabelText() == aText )
            return item->GetId();

        if( item->GetSubMenu() )
        {
            int id = item->GetSubMenu()->FindItem( aText );

            if( id != wxNOT_FOUND )
                return id;
        }
    }

    return wxNOT_FOUND;
}


wxMenuItem* wxMenu::FindItemByPosition( size_t aPos ) const
{
    return aPos < m_items.size() ? m_items[aPos].get() : nullptr;
}


wxMenuItem* wxMenu::FindChildItem( int aId, size_t* aPos ) const
{
    for( size_t i = 0; i < m_items.size(); ++i )
    {
        if( m_items[i]->GetId() == aId )
        {
            if( aPos )
                *aPos = i;

            return m_items[i].get();
        }
    }

    return nullptr;
}


std::vector<wxMenuItem*> wxMenu::GetMenuItems() const
{
    std::vector<wxMenuItem*> items;

    for( const std::unique_ptr<wxMenuItem>& item : m_items )
        items.push_back( item.get() );

    return items;
}


void wxMenu::Enable( int aId, bool aEnable )
{
    if( wxMenuItem* item = FindItem( aId ) )
        item->Enable( aEnable );
}


bool wxMenu::IsEnabled( int aId ) const
{
    wxMenuItem* item = FindItem( aId );
    return item && item->IsEnabled();
}


void wxMenu::Check( int aId, bool aCheck )
{
    if( wxMenuItem* item = FindItem( aId ) )
        item->Check( aCheck );
}


bool wxMenu::IsChecked( int aId ) const
{
    wxMenuItem* item = FindItem( aId );
    return item && item->IsChecked();
}


void wxMenu::SetLabel( int aId, const wxString& aText )
{
    if( wxMenuItem* item = FindItem( aId ) )
        item->SetItemLabel( aText );
}


wxString wxMenu::GetLabel( int aId ) const
{
    wxMenuItem* item = FindItem( aId );
    return item ? item->GetItemLabel() : wxString();
}


wxString wxMenu::GetLabelText( int aId ) const
{
    wxMenuItem* item = FindItem( aId );
    return item ? item->GetItemLabelText() : wxString();
}


void wxMenu::SetHelpString( int aId, const wxString& aHelp )
{
    if( wxMenuItem* item = FindItem( aId ) )
        item->SetHelp( aHelp );
}


void wxMenu::SetTitle( const wxString& aTitle )
{
    setTitle( ToQString( aTitle ) );
}


wxString wxMenu::GetTitle() const
{
    return ToWxString( title() );
}


void wxMenu::UpdateUI( void* aSource )
{
    for( const std::unique_ptr<wxMenuItem>& item : m_items )
    {
        if( item->IsSeparator() )
            continue;

        if( item->GetSubMenu() )
        {
            item->GetSubMenu()->UpdateUI( aSource );
            continue;
        }

        // The handlers change the action of the item
        wxUpdateUIEvent event( item->GetId() );
        event.SetEventObject( item->GetActionCompat() );
        m_events.Process( event );
    }
}


bool wxPopupMenuCompat( QWidget* aWindow, wxMenu* aMenu, const wxPoint& aPos,
                        wxCommandEvent& aEvent )
{
    if( !aMenu )
        return false;

    aMenu->UpdateUI();

    QPoint pos = aPos == wxDefaultPosition ? QCursor::pos()
                                           : aWindow->mapToGlobal( QPoint( aPos.x, aPos.y ) );

    wxMenuItem* item = wxMenu::ItemOfActionCompat( aMenu->exec( pos ) );

    if( !item || item->IsSeparator() || item->IsSubMenu() )
        return false;

    aEvent = wxCommandEvent( wxEVT_MENU, item->GetId() );
    aEvent.SetInt( item->IsChecked() ? 1 : 0 );

    // The handlers of the menu (and of its parents) first
    wxMenu* menu = item->GetMenu();

    while( menu )
    {
        aEvent.SetEventObject( menu );

        if( menu->ProcessEvent( aEvent ) )
            return false;

        menu = menu->GetParent();
    }

    aEvent.SetEventObject( aWindow );
    return true;
}
