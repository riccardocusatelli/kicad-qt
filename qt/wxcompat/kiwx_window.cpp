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

#include <wx/kiwx_window.h>

#include <algorithm>
#include <functional>

#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QScreen>
#include <QVariant>

#include <qt_strings.h>
#include <wx/kiwx_gui.h>
#include <wx/kiwx_menu.h>
#include <wx/kiwx_validators.h>


static const char* const WINDOW_PROPERTY = "kiwxWindow";


// The windows of the widgets ---------------------------------------------------------------

namespace
{
/// The window of a plain Qt widget (a child object of the widget, deleted with it).
class KIWX_WINDOW_HOLDER : public QObject, public wxWindow
{
public:
    explicit KIWX_WINDOW_HOLDER( QWidget* aWidget ) :
            QObject( aWidget ),
            m_widget( aWidget )
    {
        registerWindowCompat();
    }

    QWidget* GetHandle() const override { return m_widget; }

private:
    QWidget* m_widget;
};
} // namespace


wxWindow* wxWindowFromQt( QWidget* aWidget )
{
    if( !aWidget )
        return nullptr;

    QVariant window = aWidget->property( WINDOW_PROPERTY );

    if( window.isValid() )
        return static_cast<wxWindow*>( window.value<void*>() );

    return new KIWX_WINDOW_HOLDER( aWidget );
}


wxEvtHandler* wxEvtHandlerCompat( QWidget* aWidget )
{
    return wxWindowFromQt( aWidget );
}


void wxWindow::registerWindowCompat()
{
    GetHandle()->setProperty( WINDOW_PROPERTY, QVariant::fromValue( static_cast<void*>( this ) ) );

    if( QWidget* parent = GetHandle()->parentWidget() )
        m_parent = wxWindowFromQt( parent );
}


void wxWindow::unregisterWindowCompat()
{
    QWidget* widget = GetHandle();
    QVariant window = widget->property( WINDOW_PROPERTY );

    if( window.isValid() && window.value<void*>() == static_cast<void*>( this ) )
        widget->setProperty( WINDOW_PROPERTY, QVariant() );
}


// The list of the children -----------------------------------------------------------------

wxWindowList::wxWindowList( const QWidget* aParent )
{
    if( aParent )
    {
        // A copy: making the windows of the children adds objects to them, not to the parent
        const QObjectList children = aParent->children();

        for( QObject* child : children )
        {
            if( child->isWidgetType() )
                m_windows.push_back( wxWindowFromQt( static_cast<QWidget*>( child ) ) );
        }
    }

    makeNodes();
}


wxWindowList::wxWindowList( const std::vector<wxWindow*>& aWindows ) :
        m_windows( aWindows )
{
    makeNodes();
}


void wxWindowList::makeNodes()
{
    m_nodes.resize( m_windows.size() );

    for( size_t i = 0; i < m_nodes.size(); ++i )
    {
        m_nodes[i].m_data = m_windows[i];
        m_nodes[i].m_next = i + 1 < m_nodes.size() ? &m_nodes[i + 1] : nullptr;
    }
}


// The sizer of a window --------------------------------------------------------------------

// The objects of the events ----------------------------------------------------------------

wxObject* wxObjectOfQtCompat( QObject* aObject )
{
    if( !aObject )
        return nullptr;

    if( wxObject* object = dynamic_cast<wxObject*>( aObject ) )
        return object;

    if( QWidget* widget = qobject_cast<QWidget*>( aObject ) )
        return wxWindowFromQt( widget );

    return nullptr;
}


QObject* wxQtObjectOfCompat( wxObject* aObject )
{
    if( !aObject )
        return nullptr;

    if( QObject* object = dynamic_cast<QObject*>( aObject ) )
        return object;

    if( wxWindow* window = dynamic_cast<wxWindow*>( aObject ) )
        return window->GetHandle();

    return nullptr;
}


// wxSizer ----------------------------------------------------------------------------------

namespace
{
/// The layout of a sizer as a box layout (nullptr if it isn't one).
QBoxLayout* boxLayout( const wxSizer* aSizer )
{
    return qobject_cast<QBoxLayout*>( aSizer->GetLayoutCompat() );
}


bool isVertical( QBoxLayout* aBox )
{
    return aBox->direction() == QBoxLayout::TopToBottom
           || aBox->direction() == QBoxLayout::BottomToTop;
}


void showLayout( QLayout* aLayout, bool aShow )
{
    for( int i = 0; i < aLayout->count(); ++i )
    {
        QLayoutItem* item = aLayout->itemAt( i );

        if( item->widget() )
            item->widget()->setVisible( aShow );
        else if( item->layout() )
            showLayout( item->layout(), aShow );
    }
}
} // namespace


void wxSizer::Add( wxWidgetArgCompat aWindow, int aProportion, int aFlag, int aBorder,
                   void* aUserData )
{
    QLayout* layout = GetLayoutCompat();
    Insert( layout ? size_t( layout->count() ) : 0, aWindow, aProportion, aFlag, aBorder );
}


void wxSizer::Add( wxSizer* aSizer, int aProportion, int aFlag, int aBorder, void* aUserData )
{
    QLayout* layout = GetLayoutCompat();
    Insert( layout ? size_t( layout->count() ) : 0, aSizer, aProportion, aFlag, aBorder );
}


void wxSizer::Add( int aWidth, int aHeight, int aProportion, int aFlag, int aBorder,
                   void* aUserData )
{
    if( QBoxLayout* box = boxLayout( this ) )
    {
        if( aProportion > 0 )
            box->addStretch( aProportion );
        else
            box->addSpacing( isVertical( box ) ? aHeight : aWidth );
    }
    else if( QLayout* layout = GetLayoutCompat() )
    {
        layout->addItem( new QSpacerItem( std::max( aWidth, 0 ), std::max( aHeight, 0 ) ) );
    }
}


void wxSizer::Insert( size_t aIndex, wxWidgetArgCompat aWindow, int aProportion, int aFlag,
                      int aBorder, void* aUserData )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || !aWindow )
        return;

    if( QBoxLayout* box = boxLayout( this ) )
    {
        box->insertWidget( int( std::min<size_t>( aIndex, box->count() ) ), aWindow, aProportion,
                           wxSizerFlagsAlignment( aFlag, isVertical( box ) ) );
    }
    else
    {
        layout->addWidget( aWindow );
    }
}


void wxSizer::Insert( size_t aIndex, wxSizer* aSizer, int aProportion, int aFlag, int aBorder,
                      void* aUserData )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || !aSizer || !aSizer->GetLayoutCompat() )
        return;

    if( QBoxLayout* box = boxLayout( this ) )
    {
        box->insertLayout( int( std::min<size_t>( aIndex, box->count() ) ),
                           aSizer->GetLayoutCompat(), aProportion );
    }
    else
    {
        layout->addItem( aSizer->GetLayoutCompat() );
    }
}


void wxSizer::AddSpacer( int aSize )
{
    Add( aSize, aSize );
}


void wxSizer::AddStretchSpacer( int aProportion )
{
    Add( 0, 0, std::max( aProportion, 1 ) );
}


bool wxSizer::Detach( wxWidgetArgCompat aWindow )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || !aWindow || layout->indexOf( aWindow ) < 0 )
        return false;

    layout->removeWidget( aWindow );
    return true;
}


bool wxSizer::Detach( wxSizer* aSizer )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || !aSizer || !aSizer->GetLayoutCompat() )
        return false;

    QLayout* child = aSizer->GetLayoutCompat();

    for( int i = 0; i < layout->count(); ++i )
    {
        if( layout->itemAt( i )->layout() == child )
        {
            layout->takeAt( i );
            child->setParent( nullptr );
            return true;
        }
    }

    return false;
}


wxWindow* wxSizerItem::GetWindow() const
{
    return m_window ? wxWindowFromQt( m_window ) : nullptr;
}


wxSizer* wxSizerItem::GetSizer() const
{
    return dynamic_cast<wxSizer*>( m_layout );
}


void wxSizerItem::Show( bool aShow )
{
    if( m_window )
        m_window->setVisible( aShow );
    else if( m_layout )
        showLayout( m_layout, aShow );
}


bool wxSizerItem::IsShown() const
{
    return m_window ? !m_window->isHidden() : true;
}


wxSizerItem* wxSizer::itemCompat( QWidget* aWindow, QLayout* aLayout )
{
    for( const std::unique_ptr<wxSizerItem>& item : m_itemsCompat )
    {
        if( item->GetWidgetCompat() == aWindow && item->GetLayoutCompat() == aLayout )
            return item.get();
    }

    m_itemsCompat.push_back( std::make_unique<wxSizerItem>( aWindow, aLayout ) );
    return m_itemsCompat.back().get();
}


namespace
{
/// The layout holding a widget or a layout in @a aLayout (itself or a sub-layout).
QLayout* findHolder( QLayout* aLayout, QWidget* aWindow, QLayout* aChild, bool aRecursive )
{
    for( int i = 0; i < aLayout->count(); ++i )
    {
        QLayoutItem* item = aLayout->itemAt( i );

        if( ( aWindow && item->widget() == aWindow ) || ( aChild && item->layout() == aChild ) )
            return aLayout;

        if( aRecursive && item->layout() )
        {
            if( QLayout* holder = findHolder( item->layout(), aWindow, aChild, true ) )
                return holder;
        }
    }

    return nullptr;
}
} // namespace


wxSizerItem* wxSizer::FindItem( wxWidgetArgCompat aWindow, bool aRecursive )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || !aWindow || !findHolder( layout, aWindow, nullptr, aRecursive ) )
        return nullptr;

    return itemCompat( aWindow, nullptr );
}


wxSizerItem* wxSizer::FindItem( wxSizer* aSizer, bool aRecursive )
{
    QLayout* layout = GetLayoutCompat();
    QLayout* child = aSizer ? aSizer->GetLayoutCompat() : nullptr;

    if( !layout || !child || !findHolder( layout, nullptr, child, aRecursive ) )
        return nullptr;

    return itemCompat( nullptr, child );
}


wxSizerItem* wxSizer::GetItem( size_t aIndex )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || aIndex >= size_t( layout->count() ) )
        return nullptr;

    QLayoutItem* item = layout->itemAt( int( aIndex ) );
    return itemCompat( item->widget(), item->layout() );
}


bool wxSizer::Remove( int aIndex )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout || aIndex < 0 || aIndex >= layout->count() )
        return false;

    QLayoutItem* item = layout->takeAt( aIndex );

    // A sizer is deleted with its item; a window stays a child of its parent
    if( item->widget() )
        std::erase_if( m_itemsCompat, [&]( const auto& aItem ) { return aItem->GetWidgetCompat() == item->widget(); } );
    else if( item->layout() )
        std::erase_if( m_itemsCompat, [&]( const auto& aItem ) { return aItem->GetLayoutCompat() == item->layout(); } );

    delete item;
    return true;
}


bool wxSizer::Remove( wxSizer* aSizer )
{
    QLayout* layout = GetLayoutCompat();
    QLayout* child = aSizer ? aSizer->GetLayoutCompat() : nullptr;

    if( !layout || !child )
        return false;

    for( int i = 0; i < layout->count(); ++i )
    {
        if( layout->itemAt( i )->layout() == child )
            return Remove( i );
    }

    return false;
}


size_t wxSizer::GetItemCount() const
{
    QLayout* layout = GetLayoutCompat();
    return layout ? size_t( layout->count() ) : 0;
}


void wxSizer::Clear( bool aDeleteWindows )
{
    QLayout* layout = GetLayoutCompat();

    if( !layout )
        return;

    while( QLayoutItem* item = layout->takeAt( 0 ) )
    {
        if( aDeleteWindows && item->widget() )
            item->widget()->deleteLater();

        // An item of a layout is the layout itself: it is deleted with the item
        delete item;
    }
}


void wxSizer::Layout()
{
    if( QLayout* layout = GetLayoutCompat() )
    {
        layout->invalidate();
        layout->activate();
    }
}


wxSize wxSizer::Fit( wxWidgetArgCompat aWindow )
{
    Layout();

    if( !aWindow )
        return wxSize();

    aWindow->adjustSize();
    return wxSize( aWindow->width(), aWindow->height() );
}


void wxSizer::SetSizeHints( wxWidgetArgCompat aWindow )
{
    if( QLayout* layout = GetLayoutCompat(); layout && aWindow )
    {
        layout->activate();
        aWindow->setMinimumSize( layout->minimumSize() );
        aWindow->adjustSize();
    }
}


wxSize wxSizer::GetMinSize() const
{
    QLayout* layout = GetLayoutCompat();

    if( !layout )
        return wxSize();

    QSize size = layout->minimumSize();
    return wxSize( size.width(), size.height() );
}


wxSize wxSizer::GetSize() const
{
    QLayout* layout = GetLayoutCompat();

    if( !layout )
        return wxSize();

    QRect rect = layout->geometry();
    return wxSize( rect.width(), rect.height() );
}


wxWindow* wxSizer::GetContainingWindow() const
{
    QLayout* layout = GetLayoutCompat();
    QWidget* widget = layout ? layout->parentWidget() : nullptr;
    return widget ? wxWindowFromQt( widget ) : nullptr;
}


bool wxSizer::Show( wxWidgetArgCompat aWindow, bool aShow, bool aRecursive )
{
    QLayout* layout = GetLayoutCompat();

    if( !aWindow || ( layout && layout->indexOf( aWindow ) < 0 && !aRecursive ) )
        return false;

    aWindow->setVisible( aShow );
    return true;
}


bool wxSizer::Show( wxSizer* aSizer, bool aShow, bool aRecursive )
{
    if( !aSizer || !aSizer->GetLayoutCompat() )
        return false;

    showLayout( aSizer->GetLayoutCompat(), aShow );
    return true;
}


void wxSizer::ShowItems( bool aShow )
{
    if( QLayout* layout = GetLayoutCompat() )
        showLayout( layout, aShow );
}


bool wxSizer::IsShown( wxWidgetArgCompat aWindow ) const
{
    return aWindow && !aWindow->isHidden();
}


// wxWindow: the identity and the state -------------------------------------------------------

int wxWindow::GetId() const
{
    return wxGetWindowIdCompat( GetHandle() );
}


void wxWindow::SetId( int aId )
{
    wxSetWindowIdCompat( GetHandle(), aId );
}


wxString wxWindow::GetName() const
{
    return ToWxString( GetHandle()->objectName() );
}


void wxWindow::SetName( const wxString& aName )
{
    GetHandle()->setObjectName( ToQString( aName ) );
}


wxString wxWindow::GetLabel() const
{
    // The text of the widget (a label, a button...), or the title of a top level window
    QVariant text = GetHandle()->property( "text" );

    if( text.isValid() )
        return ToWxString( text.toString() );

    if( text = GetHandle()->property( "title" ); text.isValid() )
        return ToWxString( text.toString() );

    return ToWxString( GetHandle()->windowTitle() );
}


void wxWindow::SetLabel( const wxString& aLabel )
{
    QWidget* widget = GetHandle();

    if( widget->property( "text" ).isValid() )
        widget->setProperty( "text", ToQString( aLabel ) );
    else if( widget->property( "title" ).isValid() )
        widget->setProperty( "title", ToQString( aLabel ) );
    else
        widget->setWindowTitle( ToQString( aLabel ) );
}


bool wxWindow::Enable( bool aEnable )
{
    QWidget* widget = GetHandle();
    bool     changed = widget->isEnabled() != aEnable;

    widget->setEnabled( aEnable );
    return changed;
}


bool wxWindow::IsEnabled() const
{
    return GetHandle()->isEnabled();
}


bool wxWindow::IsThisEnabled() const
{
    QWidget* widget = GetHandle();
    return !widget->testAttribute( Qt::WA_ForceDisabled );
}


bool wxWindow::Show( bool aShow )
{
    QWidget* widget = GetHandle();
    bool     changed = widget->isHidden() == aShow;

    widget->setVisible( aShow );
    return changed;
}


bool wxWindow::IsShown() const
{
    return !GetHandle()->isHidden();
}


bool wxWindow::IsShownOnScreen() const
{
    return GetHandle()->isVisible();
}


void wxWindow::SetFocus()
{
    GetHandle()->setFocus( Qt::OtherFocusReason );
}


bool wxWindow::Navigate( int aFlags )
{
    // From this window to the next one of the focus chain of its top level window
    QWidget* start = GetHandle();
    bool     forward = aFlags & wxNavigationKeyEvent::IsForward;

    for( QWidget* test = forward ? start->nextInFocusChain() : start->previousInFocusChain();
         test && test != start; test = forward ? test->nextInFocusChain()
                                               : test->previousInFocusChain() )
    {
        if( test->window() == start->window() && test->isVisible() && test->isEnabled()
            && ( test->focusPolicy() & Qt::TabFocus ) && !start->isAncestorOf( test ) )
        {
            test->setFocus( forward ? Qt::TabFocusReason : Qt::BacktabFocusReason );
            return true;
        }
    }

    return false;
}


bool wxWindow::NavigateIn( int aFlags )
{
    // Within this window: from the focused child to the next one
    QWidget* focus = QApplication::focusWidget();
    QWidget* window = GetHandle();
    bool     forward = aFlags & wxNavigationKeyEvent::IsForward;

    if( !focus || !window->isAncestorOf( focus ) )
        focus = window;

    for( QWidget* test = forward ? focus->nextInFocusChain() : focus->previousInFocusChain();
         test && test != focus; test = forward ? test->nextInFocusChain()
                                               : test->previousInFocusChain() )
    {
        if( window->isAncestorOf( test ) && test->isVisible() && test->isEnabled()
            && ( test->focusPolicy() & Qt::TabFocus ) )
        {
            test->setFocus( forward ? Qt::TabFocusReason : Qt::BacktabFocusReason );
            return true;
        }
    }

    return false;
}


bool wxWindow::HasFocus() const
{
    return GetHandle()->hasFocus();
}


bool wxWindow::AcceptsFocus() const
{
    return GetHandle()->focusPolicy() != Qt::NoFocus;
}


wxWindow* wxWindow::FindFocus()
{
    return wxWindowFromQt( QApplication::focusWidget() );
}


// The appearance ---------------------------------------------------------------------------

bool wxWindow::SetFont( const wxFont& aFont )
{
    GetHandle()->setFont( aFont );
    return true;
}


wxFont wxWindow::GetFont() const
{
    return GetHandle()->font();
}


bool wxWindow::SetForegroundColour( const wxColour& aColour )
{
    QWidget* widget = GetHandle();
    QPalette palette = widget->palette();

    palette.setColor( widget->foregroundRole(), aColour );
    widget->setPalette( palette );
    return true;
}


bool wxWindow::SetBackgroundColour( const wxColour& aColour )
{
    QWidget* widget = GetHandle();
    QPalette palette = widget->palette();

    palette.setColor( widget->backgroundRole(), aColour );
    widget->setPalette( palette );
    widget->setAutoFillBackground( aColour.IsOk() );
    return true;
}


wxColour wxWindow::GetForegroundColour() const
{
    QWidget* widget = GetHandle();
    return widget->palette().color( widget->foregroundRole() );
}


wxColour wxWindow::GetBackgroundColour() const
{
    QWidget* widget = GetHandle();
    return widget->palette().color( widget->backgroundRole() );
}


void wxWindow::SetToolTip( const wxString& aTip )
{
    GetHandle()->setToolTip( ToQString( aTip ) );
}


wxString wxWindow::GetToolTipText() const
{
    return ToWxString( GetHandle()->toolTip() );
}


// The windows around -----------------------------------------------------------------------

wxWindow* wxWindow::GetParent() const
{
    return wxWindowFromQt( GetHandle()->parentWidget() );
}


wxWindow* wxWindow::GetGrandParent() const
{
    wxWindow* parent = GetParent();
    return parent ? parent->GetParent() : nullptr;
}


wxWindowList wxWindow::GetChildren() const
{
    return wxWindowList( GetHandle() );
}


static QWidget* siblingOf( QWidget* aWidget, int aDelta )
{
    QWidget* parent = aWidget->parentWidget();

    if( !parent )
        return nullptr;

    QList<QWidget*> siblings = parent->findChildren<QWidget*>( Qt::FindDirectChildrenOnly );
    qsizetype       index = siblings.indexOf( aWidget ) + aDelta;

    return index >= 0 && index < siblings.size() ? siblings[index] : nullptr;
}


wxWindow* wxWindow::GetPrevSibling() const
{
    return wxWindowFromQt( siblingOf( GetHandle(), -1 ) );
}


wxWindow* wxWindow::GetNextSibling() const
{
    return wxWindowFromQt( siblingOf( GetHandle(), 1 ) );
}


bool wxWindow::IsTopLevel() const
{
    return GetHandle()->isWindow();
}


bool wxWindow::IsDescendant( wxWindow* aWindow ) const
{
    return aWindow && GetHandle()->isAncestorOf( aWindow->GetHandle() );
}


bool wxWindow::Reparent( wxWindow* aParent )
{
    QWidget* widget = GetHandle();
    bool     visible = !widget->isHidden();

    widget->setParent( aParent ? aParent->GetHandle() : nullptr );
    widget->setVisible( visible );
    return true;
}


wxWindow* wxWindow::FindWindow( long aId ) const
{
    if( GetId() == aId )
        return const_cast<wxWindow*>( this );

    for( QWidget* child : GetHandle()->findChildren<QWidget*>() )
    {
        if( wxGetWindowIdCompat( child ) == aId )
            return wxWindowFromQt( child );
    }

    return nullptr;
}


wxWindow* wxWindow::FindWindow( const wxString& aName ) const
{
    if( GetName() == aName )
        return const_cast<wxWindow*>( this );

    return wxWindowFromQt( GetHandle()->findChild<QWidget*>( ToQString( aName ) ) );
}


namespace
{
/// The first widget of @a aParent (or of the top level windows) matching @a aMatch, as a window.
wxWindow* findWindowCompat( const wxWindow* aParent,
                            const std::function<bool( QWidget* )>& aMatch )
{
    QList<QWidget*> roots;

    if( aParent )
        roots << aParent->GetHandle();
    else
        roots = QApplication::topLevelWidgets();

    for( QWidget* root : roots )
    {
        if( aMatch( root ) )
            return wxWindowFromQt( root );

        for( QWidget* child : root->findChildren<QWidget*>() )
        {
            if( aMatch( child ) )
                return wxWindowFromQt( child );
        }
    }

    return nullptr;
}
} // namespace


wxWindow* wxWindow::FindWindowByName( const wxString& aName, const wxWindow* aParent )
{
    QString name = ToQString( aName );
    return findWindowCompat( aParent,
                             [&]( QWidget* aWidget ) { return aWidget->objectName() == name; } );
}


wxWindow* wxWindow::FindWindowById( long aId, const wxWindow* aParent )
{
    return findWindowCompat( aParent,
                             [&]( QWidget* aWidget ) { return wxGetWindowIdCompat( aWidget ) == aId; } );
}


wxWindow* wxWindow::FindWindowByLabel( const wxString& aLabel, const wxWindow* aParent )
{
    QString label = ToQString( aLabel );

    return findWindowCompat( aParent,
                             [&]( QWidget* aWidget )
                             {
                                 QVariant text = aWidget->property( "text" );
                                 return text.isValid() ? text.toString() == label
                                                       : aWidget->windowTitle() == label;
                             } );
}


// The geometry -----------------------------------------------------------------------------

void wxWindow::SetMinSize( const wxSize& aSize )
{
    GetHandle()->setMinimumSize( std::max( aSize.x, 0 ), std::max( aSize.y, 0 ) );
}


void wxWindow::SetMaxSize( const wxSize& aSize )
{
    GetHandle()->setMaximumSize( aSize.x < 0 ? QWIDGETSIZE_MAX : aSize.x,
                                 aSize.y < 0 ? QWIDGETSIZE_MAX : aSize.y );
}


void wxWindow::SetInitialSize( const wxSize& aSize )
{
    SetMinSize( aSize );

    QWidget* widget = GetHandle();
    QSize    hint = widget->sizeHint();

    widget->resize( aSize.x > 0 ? aSize.x : hint.width(), aSize.y > 0 ? aSize.y : hint.height() );
}


void wxWindow::SetSizeHints( const wxSize& aMinSize, const wxSize& aMaxSize, const wxSize& aIncSize )
{
    SetMinSize( aMinSize );
    SetMaxSize( aMaxSize );
}


void wxWindow::SetSizeHints( int aMinW, int aMinH, int aMaxW, int aMaxH, int aIncW, int aIncH )
{
    SetSizeHints( wxSize( aMinW, aMinH ), wxSize( aMaxW, aMaxH ) );
}


wxSize wxWindow::GetMinSize() const
{
    QWidget* widget = GetHandle();
    return wxSize( widget->minimumWidth() > 0 ? widget->minimumWidth() : -1,
                   widget->minimumHeight() > 0 ? widget->minimumHeight() : -1 );
}


wxSize wxWindow::GetMaxSize() const
{
    QWidget* widget = GetHandle();
    return wxSize( widget->maximumWidth() < QWIDGETSIZE_MAX ? widget->maximumWidth() : -1,
                   widget->maximumHeight() < QWIDGETSIZE_MAX ? widget->maximumHeight() : -1 );
}


wxSize wxWindow::GetEffectiveMinSize() const
{
    QSize size = GetHandle()->minimumSizeHint().expandedTo( GetHandle()->minimumSize() );
    return wxSize( size.width(), size.height() );
}


wxSize wxWindow::GetSize() const
{
    QWidget* widget = GetHandle();
    return wxSize( widget->width(), widget->height() );
}


void wxWindow::GetSize( int* aWidth, int* aHeight ) const
{
    wxSize size = GetSize();

    if( aWidth )
        *aWidth = size.x;

    if( aHeight )
        *aHeight = size.y;
}


wxSize wxWindow::GetClientSize() const
{
    QRect rect = GetHandle()->contentsRect();
    return wxSize( rect.width(), rect.height() );
}


void wxWindow::GetClientSize( int* aWidth, int* aHeight ) const
{
    wxSize size = GetClientSize();

    if( aWidth )
        *aWidth = size.x;

    if( aHeight )
        *aHeight = size.y;
}


wxRect wxWindow::GetClientRect() const
{
    QRect rect = GetHandle()->contentsRect();
    return wxRect( 0, 0, rect.width(), rect.height() );
}


wxSize wxWindow::GetBestSize() const
{
    QSize size = GetHandle()->sizeHint();
    return wxSize( size.width(), size.height() );
}


wxPoint wxWindow::GetPosition() const
{
    QPoint pos = GetHandle()->pos();
    return wxPoint( pos.x(), pos.y() );
}


void wxWindow::GetPosition( int* aX, int* aY ) const
{
    wxPoint pos = GetPosition();

    if( aX )
        *aX = pos.x;

    if( aY )
        *aY = pos.y;
}


wxPoint wxWindow::GetScreenPosition() const
{
    QPoint pos = GetHandle()->mapToGlobal( QPoint( 0, 0 ) );
    return wxPoint( pos.x(), pos.y() );
}


wxRect wxWindow::GetRect() const
{
    QRect rect = GetHandle()->geometry();
    return wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
}


wxRect wxWindow::GetScreenRect() const
{
    wxPoint pos = GetScreenPosition();
    wxSize  size = GetSize();
    return wxRect( pos.x, pos.y, size.x, size.y );
}


void wxWindow::SetSize( const wxSize& aSize )
{
    QWidget* widget = GetHandle();
    widget->resize( aSize.x >= 0 ? aSize.x : widget->width(),
                    aSize.y >= 0 ? aSize.y : widget->height() );
}


void wxWindow::SetSize( const wxRect& aRect, int aFlags )
{
    SetSize( aRect.x, aRect.y, aRect.width, aRect.height, aFlags );
}


void wxWindow::SetSize( int aX, int aY, int aWidth, int aHeight, int aFlags )
{
    QWidget* widget = GetHandle();
    QRect    rect = widget->geometry();

    // -1 keeps the current value (unless wxSIZE_ALLOW_MINUS_ONE)
    bool allowMinusOne = aFlags & wxSIZE_ALLOW_MINUS_ONE;

    if( aX != -1 || allowMinusOne )
        rect.moveLeft( aX );

    if( aY != -1 || allowMinusOne )
        rect.moveTop( aY );

    if( aWidth >= 0 )
        rect.setWidth( aWidth );

    if( aHeight >= 0 )
        rect.setHeight( aHeight );

    widget->setGeometry( rect );
}


void wxWindow::Move( const wxPoint& aPos, int aFlags )
{
    GetHandle()->move( aPos.x, aPos.y );
}


wxPoint wxWindow::ClientToScreen( const wxPoint& aPos ) const
{
    QPoint pos = GetHandle()->mapToGlobal( QPoint( aPos.x, aPos.y ) );
    return wxPoint( pos.x(), pos.y() );
}


void wxWindow::ClientToScreen( int* aX, int* aY ) const
{
    wxPoint pos = ClientToScreen( wxPoint( aX ? *aX : 0, aY ? *aY : 0 ) );

    if( aX )
        *aX = pos.x;

    if( aY )
        *aY = pos.y;
}


wxPoint wxWindow::ScreenToClient( const wxPoint& aPos ) const
{
    QPoint pos = GetHandle()->mapFromGlobal( QPoint( aPos.x, aPos.y ) );
    return wxPoint( pos.x(), pos.y() );
}


void wxWindow::ScreenToClient( int* aX, int* aY ) const
{
    wxPoint pos = ScreenToClient( wxPoint( aX ? *aX : 0, aY ? *aY : 0 ) );

    if( aX )
        *aX = pos.x;

    if( aY )
        *aY = pos.y;
}


wxSize wxWindow::GetTextExtent( const wxString& aText ) const
{
    QSize size = GetHandle()->fontMetrics().size( 0, ToQString( aText ) );
    return wxSize( size.width(), size.height() );
}


void wxWindow::GetTextExtent( const wxString& aText, int* aWidth, int* aHeight, int* aDescent,
                              int* aExternalLeading, const wxFont* aFont ) const
{
    QFontMetrics metrics = aFont ? QFontMetrics( *aFont ) : GetHandle()->fontMetrics();
    QSize        size = metrics.size( 0, ToQString( aText ) );

    if( aWidth )
        *aWidth = size.width();

    if( aHeight )
        *aHeight = size.height();

    if( aDescent )
        *aDescent = metrics.descent();

    if( aExternalLeading )
        *aExternalLeading = metrics.leading();
}


int wxWindow::GetCharWidth() const
{
    return GetHandle()->fontMetrics().averageCharWidth();
}


int wxWindow::GetCharHeight() const
{
    return GetHandle()->fontMetrics().height();
}


double wxWindow::GetContentScaleFactor() const
{
    return GetHandle()->devicePixelRatioF();
}


wxSize wxWindow::FromPhys( const wxSize& aSize ) const
{
    return wxSize( FromPhys( aSize.x ), FromPhys( aSize.y ) );
}


int wxWindow::FromPhys( int aValue ) const
{
    return aValue < 0 ? aValue : int( aValue / GetContentScaleFactor() + 0.5 );
}


wxSize wxWindow::ToPhys( const wxSize& aSize ) const
{
    return wxSize( ToPhys( aSize.x ), ToPhys( aSize.y ) );
}


int wxWindow::ToPhys( int aValue ) const
{
    return aValue < 0 ? aValue : int( aValue * GetContentScaleFactor() + 0.5 );
}


/// The size of the dialog units of wxWidgets in the font of a widget.
static QSizeF dialogUnits( const QWidget* aWidget )
{
    QFontMetricsF metrics = aWidget->fontMetrics();

    // wxWidgets uses the average width of the letters of the alphabet
    double width = metrics.horizontalAdvance(
                           QStringLiteral( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" ) )
                   / 52.0;

    return QSizeF( width / 4.0, metrics.height() / 8.0 );
}


wxPoint wxWindow::ConvertDialogToPixels( const wxPoint& aPoint ) const
{
    QSizeF unit = dialogUnits( GetHandle() );
    return wxPoint( aPoint.x < 0 ? aPoint.x : int( aPoint.x * unit.width() + 0.5 ),
                    aPoint.y < 0 ? aPoint.y : int( aPoint.y * unit.height() + 0.5 ) );
}


wxSize wxWindow::ConvertDialogToPixels( const wxSize& aSize ) const
{
    wxPoint point = ConvertDialogToPixels( wxPoint( aSize.x, aSize.y ) );
    return wxSize( point.x, point.y );
}


wxPoint wxWindow::ConvertPixelsToDialog( const wxPoint& aPoint ) const
{
    QSizeF unit = dialogUnits( GetHandle() );
    return wxPoint( aPoint.x < 0 ? aPoint.x : int( aPoint.x / unit.width() + 0.5 ),
                    aPoint.y < 0 ? aPoint.y : int( aPoint.y / unit.height() + 0.5 ) );
}


wxSize wxWindow::ConvertPixelsToDialog( const wxSize& aSize ) const
{
    wxPoint point = ConvertPixelsToDialog( wxPoint( aSize.x, aSize.y ) );
    return wxSize( point.x, point.y );
}


// The painting and the layout --------------------------------------------------------------

void wxWindow::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    if( aRect )
        GetHandle()->update( aRect->x, aRect->y, aRect->width, aRect->height );
    else
        GetHandle()->update();
}


void wxWindow::Update()
{
    GetHandle()->repaint();
}


void wxWindow::Freeze()
{
    if( m_freezeCount++ == 0 )
        GetHandle()->setUpdatesEnabled( false );
}


void wxWindow::Thaw()
{
    if( m_freezeCount > 0 && --m_freezeCount == 0 )
        GetHandle()->setUpdatesEnabled( true );
}


bool wxWindow::Layout()
{
    if( QLayout* layout = GetHandle()->layout() )
    {
        layout->invalidate();
        layout->activate();
    }

    return true;
}


void wxWindow::Fit()
{
    GetHandle()->adjustSize();
}


void wxWindow::PostSizeEvent()
{
    QWidget* widget = GetHandle();
    QApplication::postEvent( widget, new QResizeEvent( widget->size(), widget->size() ) );
}


wxSizer* wxWindow::GetSizer()
{
    QWidget* client = GetClientWidgetCompat();
    QLayout* layout = client ? client->layout() : nullptr;

    if( !layout )
        return nullptr;

    if( wxSizer* sizer = dynamic_cast<wxSizer*>( layout ) )
        return sizer;

    if( !m_sizerProxy )
        m_sizerProxy = std::make_unique<wxSizerProxy>( client );

    return m_sizerProxy.get();
}


void wxWindow::SetSizer( wxSizer* aSizer, bool aDeleteOld )
{
    QWidget* widget = GetClientWidgetCompat();
    QLayout* layout = aSizer ? aSizer->GetLayoutCompat() : nullptr;

    // A widget has one layout: the old one goes first
    if( QLayout* old = widget->layout(); old && old != layout )
        delete old;

    if( layout )
        widget->setLayout( layout );
}


void wxWindow::SetSizerAndFit( wxSizer* aSizer, bool aDeleteOld )
{
    SetSizer( aSizer, aDeleteOld );
    Fit();
}


void wxWindow::Centre( int aDirection )
{
    wxCentreWindow( GetHandle(), aDirection );
}


void wxWindow::Raise()
{
    GetHandle()->raise();
}


void wxWindow::Lower()
{
    GetHandle()->lower();
}


void wxWindow::CaptureMouse()
{
    GetHandle()->grabMouse();
}


void wxWindow::ReleaseMouse()
{
    GetHandle()->releaseMouse();
}


bool wxWindow::HasCapture() const
{
    return QWidget::mouseGrabber() == GetHandle();
}


bool wxWindow::Close( bool aForce )
{
    return GetHandle()->close();
}


bool wxWindow::Destroy()
{
    GetHandle()->deleteLater();
    return true;
}


// The data of the window -------------------------------------------------------------------

void wxWindow::SetValidator( const wxValidator& aValidator )
{
    wxSetValidatorCompat( GetHandle(), aValidator );
}


wxValidator* wxWindow::GetValidator() const
{
    return wxGetValidatorCompat( GetHandle() );
}


bool wxWindow::TransferDataToWindow()
{
    return wxTransferDataToChildrenCompat( GetHandle() );
}


bool wxWindow::TransferDataFromWindow()
{
    return wxTransferDataFromChildrenCompat( GetHandle() );
}


bool wxWindow::Validate()
{
    return wxValidateChildrenCompat( GetHandle() );
}


// The handlers of the events ---------------------------------------------------------------

bool wxWindow::ProcessEvent( wxEvent& aEvent )
{
    return ProcessChainCompat( GetEventHandlerCompat(), aEvent );
}


void wxWindow::SetEvtHandlerEnabled( bool aEnable )
{
    GetHandle()->blockSignals( !aEnable );
    wxEvtHandler::SetEvtHandlerEnabled( aEnable );
}


void wxWindow::bindHandlerCompat( wxEventType aType, wxCompatEventSink::HANDLER aHandler,
                                  const void* aKey, int aId, int aLastId )
{
    m_eventsCompat.Add( aType, std::move( aHandler ), aKey, aId, aLastId );
    wxConnectQtEventsCompat( GetHandle(), aType, &m_eventsCompat );
}


bool wxWindow::PopupMenu( wxMenu* aMenu, const wxPoint& aPos )
{
    wxCommandEvent event;

    if( wxPopupMenuCompat( GetHandle(), aMenu, aPos, event ) )
        ProcessEvent( event );

    return true;
}


int wxWindow::GetPopupMenuSelectionFromUser( wxMenu& aMenu, const wxPoint& aPos )
{
    return wxPopupMenuSelectionCompat( GetHandle(), &aMenu, aPos );
}


// The data transfers -----------------------------------------------------------------------

namespace
{

enum class TRANSFER
{
    TO_WINDOW,
    FROM_WINDOW,
    VALIDATE
};


/// A window of the layer (not the window made for a plain widget).
wxWindow* compatWindow( QWidget* aWidget )
{
    QVariant window = aWidget->property( WINDOW_PROPERTY );

    if( !window.isValid() )
        return nullptr;

    wxWindow* result = static_cast<wxWindow*>( window.value<void*>() );
    return dynamic_cast<QObject*>( result ) == aWidget ? result : nullptr;
}


/**
 * A transfer of the children of @a aWindow: the ones of their validators, then the ones of the
 * windows of the layer (their own transfers go to their children) or of the children of the
 * plain widgets.
 */
bool transferChildren( QWidget* aWindow, TRANSFER aTransfer )
{
    bool result = true;

    for( QWidget* child : aWindow->findChildren<QWidget*>( Qt::FindDirectChildrenOnly ) )
    {
        // A dialog of its own is not part of the transfers of its parent
        if( child->isWindow() )
            continue;

        if( wxValidator* validator = wxGetValidatorCompat( child ) )
        {
            switch( aTransfer )
            {
            case TRANSFER::TO_WINDOW:   result &= validator->TransferToWindow();                  break;
            case TRANSFER::FROM_WINDOW: result &= validator->TransferFromWindow();                break;
            case TRANSFER::VALIDATE:    result &= validator->Validate( wxWindowFromQt( aWindow ) ); break;
            }
        }

        if( wxWindow* window = compatWindow( child ) )
        {
            switch( aTransfer )
            {
            case TRANSFER::TO_WINDOW:   result &= window->TransferDataToWindow();   break;
            case TRANSFER::FROM_WINDOW: result &= window->TransferDataFromWindow(); break;
            case TRANSFER::VALIDATE:    result &= window->Validate();               break;
            }
        }
        else
        {
            result &= transferChildren( child, aTransfer );
        }
    }

    return result;
}

} // namespace


bool wxTransferDataToChildrenCompat( wxWidgetArgCompat aWindow )
{
    return aWindow ? transferChildren( aWindow, TRANSFER::TO_WINDOW ) : true;
}


bool wxTransferDataFromChildrenCompat( wxWidgetArgCompat aWindow )
{
    return aWindow ? transferChildren( aWindow, TRANSFER::FROM_WINDOW ) : true;
}


bool wxValidateChildrenCompat( wxWidgetArgCompat aWindow )
{
    return aWindow ? transferChildren( aWindow, TRANSFER::VALIDATE ) : true;
}


bool wxTransferDataToWindowCompat( wxWidgetArgCompat aWindow )
{
    if( !aWindow )
        return true;

    if( wxWindow* window = compatWindow( aWindow ) )
        return window->TransferDataToWindow();

    return wxTransferDataToChildrenCompat( aWindow );
}


bool wxTransferDataFromWindowCompat( wxWidgetArgCompat aWindow )
{
    if( !aWindow )
        return true;

    if( wxWindow* window = compatWindow( aWindow ) )
        return window->TransferDataFromWindow();

    return wxTransferDataFromChildrenCompat( aWindow );
}


bool wxValidateCompat( wxWidgetArgCompat aWindow )
{
    if( !aWindow )
        return true;

    if( wxWindow* window = compatWindow( aWindow ) )
        return window->Validate();

    return wxValidateChildrenCompat( aWindow );
}


wxWindow* wxGetTopLevelParent( wxWidgetArgCompat aWindow )
{
    return aWindow ? wxWindowFromQt( aWindow->window() ) : nullptr;
}


void wxCentreWindow( wxWidgetArgCompat aWindow, int aDirection )
{
    if( !aWindow )
        return;

    QWidget* parent = aWindow->parentWidget() ? aWindow->parentWidget()->window() : nullptr;
    QRect    area;

    if( parent && parent != aWindow.get() )
        area = parent->frameGeometry();
    else if( QScreen* screen = aWindow->screen() )
        area = screen->availableGeometry();
    else
        return;

    QPoint pos = aWindow->pos();
    QSize  size = aWindow->frameGeometry().size();

    if( aDirection & wxHORIZONTAL )
        pos.setX( area.x() + ( area.width() - size.width() ) / 2 );

    if( aDirection & wxVERTICAL )
        pos.setY( area.y() + ( area.height() - size.height() ) / 2 );

    aWindow->move( pos );
}
