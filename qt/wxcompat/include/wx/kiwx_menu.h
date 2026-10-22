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

/**
 * @file kiwx_menu.h
 * The menus of the wxWidgets compatibility layer of the Qt build (wxcompat): wxMenu on QMenu,
 * wxMenuItem on QAction.  wxWindow::PopupMenu() shows a menu and gives the wxEVT_MENU event of
 * the chosen item to the handlers of the menu, then to the ones of the window (as wxWidgets).
 */

#ifndef KIWX_MENU_H
#define KIWX_MENU_H

#include <kiqt.h>

#include <memory>
#include <vector>

#include <QMenu>
#include <QMenuBar>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_events.h>
#include <wx/kiwx_gdi.h>
#include <wx/kiwx_window.h>

class QAction;
class wxMenu;


/// An item of a menu (wxMenuItem): a QAction.
class KIQT_API wxMenuItem
{
public:
    wxMenuItem( wxMenu* aParentMenu = nullptr, int aId = wxID_SEPARATOR,
                const wxString& aText = wxEmptyString, const wxString& aHelp = wxEmptyString,
                wxItemKind aKind = wxITEM_NORMAL, wxMenu* aSubMenu = nullptr );
    /// The item of an action of a menu of Qt that is not a wxMenu (an ACTION_MENU): the item
    /// doesn't own the action.
    wxMenuItem( QAction* aAction, int aId );

    ~wxMenuItem();

    wxMenuItem( const wxMenuItem& ) = delete;
    wxMenuItem& operator=( const wxMenuItem& ) = delete;

    int GetId() const { return m_id; }
    wxItemKind GetKind() const { return m_kind; }
    bool IsSeparator() const { return m_kind == wxITEM_SEPARATOR; }
    bool IsCheckable() const { return m_kind == wxITEM_CHECK || m_kind == wxITEM_RADIO; }
    bool IsSubMenu() const { return m_subMenu != nullptr; }

    wxMenu* GetMenu() const { return m_parentMenu; }
    wxMenu* GetSubMenu() const { return m_subMenu; }

    void SetItemLabel( const wxString& aText );
    wxString GetItemLabel() const;
    wxString GetItemLabelText() const;
    void SetHelp( const wxString& aHelp );
    wxString GetHelp() const;

    void Enable( bool aEnable = true );
    bool IsEnabled() const;
    void Check( bool aCheck = true );
    bool IsChecked() const;

    void SetBitmap( const wxBitmapBundle& aBitmap );

    /// The action of the item (null before the item is in a menu).
    QAction* GetActionCompat() const { return m_action; }

private:
    friend class wxMenu;

    /// Make the action of the item in @a aMenu (Append(), Insert()).
    void attach( wxMenu* aMenu, QAction* aBefore );

    wxMenu*    m_parentMenu;
    int        m_id;
    wxString   m_text;
    wxString   m_help;
    wxItemKind m_kind;
    wxMenu*    m_subMenu;
    QAction*   m_action;
    bool       m_ownsAction = true;
};


/// A menu (wxMenu): a QMenu whose items have the ids of wxWidgets.
class KIQT_API wxMenu : public QMenu, public wxEvtHandler
{
public:
    explicit wxMenu( const wxString& aTitle = wxEmptyString, long aStyle = 0 );
    explicit wxMenu( long aStyle ) : wxMenu( wxEmptyString, aStyle ) {}
    ~wxMenu() override;

    wxMenuItem* Append( int aId, const wxString& aText = wxEmptyString,
                        const wxString& aHelp = wxEmptyString, wxItemKind aKind = wxITEM_NORMAL );
    wxMenuItem* Append( int aId, const wxString& aText, wxMenu* aSubMenu,
                        const wxString& aHelp = wxEmptyString );
    wxMenuItem* Append( wxMenuItem* aItem );
    wxMenuItem* AppendCheckItem( int aId, const wxString& aText,
                                 const wxString& aHelp = wxEmptyString );
    wxMenuItem* AppendRadioItem( int aId, const wxString& aText,
                                 const wxString& aHelp = wxEmptyString );
    wxMenuItem* AppendSeparator();
    wxMenuItem* AppendSubMenu( wxMenu* aSubMenu, const wxString& aText,
                               const wxString& aHelp = wxEmptyString );

    wxMenuItem* Insert( size_t aPos, wxMenuItem* aItem );
    wxMenuItem* Insert( size_t aPos, int aId, const wxString& aText = wxEmptyString,
                        const wxString& aHelp = wxEmptyString, wxItemKind aKind = wxITEM_NORMAL );
    wxMenuItem* InsertSeparator( size_t aPos );
    wxMenuItem* Prepend( wxMenuItem* aItem ) { return Insert( 0, aItem ); }
    wxMenuItem* PrependSeparator() { return InsertSeparator( 0 ); }

    /// Remove an item from the menu and give it to the caller.
    wxMenuItem* Remove( int aId );
    wxMenuItem* Remove( wxMenuItem* aItem );

    /// Remove and delete an item (Delete() keeps a sub-menu, Destroy() deletes it).
    bool Delete( int aId );
    bool Delete( wxMenuItem* aItem );
    bool Destroy( int aId );
    bool Destroy( wxMenuItem* aItem );

    wxMenuItem* FindItem( int aId, wxMenu** aMenu = nullptr ) const;
    int FindItem( const wxString& aText ) const;
    wxMenuItem* FindItemByPosition( size_t aPos ) const;
    wxMenuItem* FindChildItem( int aId, size_t* aPos = nullptr ) const;

    size_t GetMenuItemCount() const { return m_items.size(); }
    std::vector<wxMenuItem*> GetMenuItems() const;

    void Enable( int aId, bool aEnable );
    bool IsEnabled( int aId ) const;
    void Check( int aId, bool aCheck );
    bool IsChecked( int aId ) const;
    void SetLabel( int aId, const wxString& aText );
    wxString GetLabel( int aId ) const;
    wxString GetLabelText( int aId ) const;
    void SetHelpString( int aId, const wxString& aHelp );

    void SetTitle( const wxString& aTitle );
    wxString GetTitle() const;

    wxMenu* GetParent() const { return m_parentMenu; }
    void SetParent( wxMenu* aParent ) { m_parentMenu = aParent; }

    /// The wxEVT_UPDATE_UI events of the items (the handlers of the menu).
    void UpdateUI( void* aSource = nullptr );

    /// The menus of an action (the menu of the item of an action).
    static wxMenuItem* ItemOfActionCompat( QAction* aAction );

private:
    friend class wxMenuItem;

    void insertItem( size_t aPos, wxMenuItem* aItem );

    wxMenu*                                  m_parentMenu;
    std::vector<std::unique_ptr<wxMenuItem>> m_items;
};


/// The menu bar of a frame (wxMenuBar).
class KIQT_API wxMenuBar : public wxWindowCompat<QMenuBar>
{
public:
    explicit wxMenuBar( long aStyle = 0 ) {}

    /// Append a menu, with its title (a & marks the mnemonic).
    bool Append( wxMenu* aMenu, const wxString& aTitle );

    size_t GetMenuCount() const { return actions().size(); }
};


/**
 * Show the menu @a aMenu on @a aWindow at @a aPos (wxDefaultPosition: at the mouse) and give
 * the wxEVT_MENU event of the chosen item to the handlers of the menu.
 *
 * @param aEvent is the event of the chosen item, for the handlers of the window.
 * @return true if an item was chosen and the handlers of the menu didn't process its event.
 */
KIQT_API bool wxPopupMenuCompat( QWidget* aWindow, wxMenu* aMenu, const wxPoint& aPos,
                                 wxCommandEvent& aEvent );

/**
 * Show the menu @a aMenu on @a aWindow at @a aPos (wxDefaultPosition: at the mouse) and return
 * the id of the chosen item (wxID_NONE if none), without its event.
 */
KIQT_API int wxPopupMenuSelectionCompat( QWidget* aWindow, wxMenu* aMenu, const wxPoint& aPos );

#endif // KIWX_MENU_H
