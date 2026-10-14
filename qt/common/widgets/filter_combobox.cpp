
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

/**
 * @file filter_combobox.cpp
 * The combo box with a filtered popup list, with Qt (see common/widgets/filter_combobox.cpp).
 */

#include <widgets/filter_combobox.h>

#include <algorithm>

#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

#include <qt_strings.h>
#include <core/ki_assert.h>
#include <wx/intl.h>


#if defined( __APPLE__ )
    #define LIST_PADDING 7
#else
    #define LIST_PADDING 5
#endif


FILTER_COMBOPOPUP::FILTER_COMBOPOPUP() :
        QFrame( nullptr, Qt::Popup ),
        m_filterCtrl( nullptr ),
        m_listBox( nullptr ),
        m_combo( nullptr )
{
}


FILTER_COMBOPOPUP::~FILTER_COMBOPOPUP()
{
}


bool FILTER_COMBOPOPUP::Create( FILTER_COMBOBOX* aParent )
{
    m_combo = aParent;

    setFrameStyle( QFrame::Box | QFrame::Plain );

    QVBoxLayout* mainSizer = new QVBoxLayout( this );
    mainSizer->setContentsMargins( 1, 1, 1, 1 );
    mainSizer->setSpacing( 0 );

    QLabel* filterLabel = new QLabel( ToQString( _( "Filter:" ) ), this );
    mainSizer->addWidget( filterLabel );

    // The filter excludes the spaces (as the validator of the wxWidgets build)
    m_filterCtrl = new QLineEdit( this );
    m_filterCtrl->setValidator( new QRegularExpressionValidator(
            QRegularExpression( QStringLiteral( "[^ ]*" ) ), m_filterCtrl ) );
    mainSizer->addWidget( m_filterCtrl );

    m_listBox = new QListWidget( this );
    m_listBox->setSelectionMode( QAbstractItemView::SingleSelection );
    m_listBox->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    m_listBox->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    mainSizer->addSpacing( 2 );
    mainSizer->addWidget( m_listBox, 1 );

    // Hot-track the mouse (for focus and listbox selection)
    m_listBox->setMouseTracking( true );

    QObject::connect( m_listBox, &QListWidget::itemEntered, this,
                      [this]( QListWidgetItem* aItem )
                      {
                          m_listBox->setFocus();
                          m_listBox->setCurrentItem( aItem );
                      } );

    // Accept a click on an item
    QObject::connect( m_listBox, &QListWidget::itemClicked, this,
                      [this]( QListWidgetItem* aItem )
                      {
                          m_listBox->setCurrentItem( aItem );
                          Accept();
                      } );

    QObject::connect( m_filterCtrl, &QLineEdit::textChanged, this,
                      [this]()
                      {
                          rebuildList();
                          updateSize();

                          if( m_listBox->count() > 0 )
                              m_listBox->setCurrentRow( 0 );
                      } );

    m_filterCtrl->installEventFilter( this );
    m_listBox->installEventFilter( this );

    return true;
}


void FILTER_COMBOPOPUP::SetStringList( const wxArrayString& aStringList )
{
    m_stringList = aStringList;
    m_stringList.Sort();
    rebuildList();
}


wxString FILTER_COMBOPOPUP::GetStringValue() const
{
    return m_selectedString;
}


void FILTER_COMBOPOPUP::SetSelectedString( const wxString& aString )
{
    m_selectedString = aString;
    setComboValue( m_selectedString );
}


void FILTER_COMBOPOPUP::OnPopup()
{
    // While it can sometimes be useful to keep the filter, it's always unexpected.
    // Better to clear it.
    m_filterCtrl->clear();

    QList<QListWidgetItem*> current = m_listBox->findItems( ToQString( GetStringValue() ),
                                                            Qt::MatchExactly );

    if( !current.isEmpty() )
        m_listBox->setCurrentItem( current.front() );
    else
        m_listBox->setCurrentRow( -1 );

    m_filterCtrl->setFocus();

    updateSize();
}


void FILTER_COMBOPOPUP::OnStartingKey( QKeyEvent* aEvent )
{
    m_filterCtrl->setFocus();
    doStartingKey( aEvent );
}


void FILTER_COMBOPOPUP::Accept()
{
    wxString selectedString = getSelectedValue().value_or( wxEmptyString );

    Dismiss();

    // No update on empty
    if( !selectedString.IsEmpty() && selectedString != m_selectedString )
    {
        m_selectedString = selectedString;
        setComboValue( m_selectedString );

        notifySelected();
    }
}


void FILTER_COMBOPOPUP::Dismiss()
{
    hide();
}


void FILTER_COMBOPOPUP::getListContent( wxArrayString& aListContent )
{
    const wxString filterString = getFilterValue();

    // Simple substring, case-insensitive search
    for( const wxString& str : m_stringList )
    {
        if( filterString.IsEmpty() || str.Lower().Contains( filterString.Lower() ) )
            aListContent.push_back( str );
    }
}


void FILTER_COMBOPOPUP::rebuildList()
{
    wxArrayString newList;
    getListContent( newList );

    m_listBox->clear();

    for( const wxString& str : newList )
        m_listBox->addItem( ToQString( str ) );
}


void FILTER_COMBOPOPUP::setComboValue( const wxString& aValue )
{
    if( m_combo )
        m_combo->SetValue( aValue );
}


void FILTER_COMBOPOPUP::notifySelected()
{
    if( m_combo )
        m_combo->NotifyItemSelected();
}


std::optional<wxString> FILTER_COMBOPOPUP::getSelectedValue() const
{
    if( QListWidgetItem* item = m_listBox->currentItem() )
        return ToWxString( item->text() );

    return std::nullopt;
}


wxString FILTER_COMBOPOPUP::getFilterValue() const
{
    return ToWxString( m_filterCtrl->text().trimmed() );
}


void FILTER_COMBOPOPUP::updateSize()
{
    if( !m_combo )
        return;

    QFontMetrics metrics( m_listBox->font() );
    int          listWidth = m_combo->width();

    for( int i = 0; i < m_listBox->count(); ++i )
    {
        int itemWidth = metrics.horizontalAdvance( m_listBox->item( i )->text() );
        listWidth = std::max( listWidth, itemWidth + LIST_PADDING * 2
                                                 + m_listBox->verticalScrollBar()->sizeHint().width() );
    }

    int itemHeight = m_listBox->count() > 0 ? m_listBox->sizeHintForRow( 0 )
                                            : metrics.height() + 6;
    int listHeight = m_listBox->count() * itemHeight + LIST_PADDING * 3;
    int listTop = m_listBox->geometry().top() > 0 ? m_listBox->geometry().top()
                                                  : layout()->sizeHint().height()
                                                            - m_listBox->sizeHint().height();

    // The popup stays on the screen of the combo box, below it (or above it)
    QScreen* screen = m_combo->screen();
    QRect    available = screen ? screen->availableGeometry() : QRect( 0, 0, 1920, 1080 );
    QPoint   below = m_combo->mapToGlobal( QPoint( 0, m_combo->height() ) );
    int      maxHeight = std::max( available.bottom() - below.y(),
                                   m_combo->mapToGlobal( QPoint( 0, 0 ) ).y() - available.top() );

    if( listTop + listHeight >= maxHeight )
        listHeight = maxHeight - listTop - 1;

    resize( listWidth, listTop + listHeight );
}


void FILTER_COMBOPOPUP::hideEvent( QHideEvent* aEvent )
{
    QFrame::hideEvent( aEvent );

    if( m_combo )
        m_combo->update();
}


bool FILTER_COMBOPOPUP::eventFilter( QObject* aWatched, QEvent* aEvent )
{
    if( aEvent->type() == QEvent::KeyPress
        && ( aWatched == m_filterCtrl || aWatched == m_listBox ) )
    {
        return onKeyDown( static_cast<QKeyEvent*>( aEvent ) );
    }

    return QFrame::eventFilter( aWatched, aEvent );
}


bool FILTER_COMBOPOPUP::onKeyDown( QKeyEvent* aEvent )
{
    switch( aEvent->key() )
    {
    // Control keys go to the parent combobox
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    {
        Dismiss();

        if( m_combo )
        {
            bool forward = aEvent->key() == Qt::Key_Tab
                           && !( aEvent->modifiers() & Qt::ShiftModifier );

            QWidget* next = forward ? m_combo->nextInFocusChain()
                                    : m_combo->previousInFocusChain();

            if( next )
                next->setFocus( forward ? Qt::TabFocusReason : Qt::BacktabFocusReason );
        }

        return true;
    }

    case Qt::Key_Escape:
        Dismiss();
        return true;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        Accept();
        return true;

    // Arrows go to the list box
    case Qt::Key_Down:
        m_listBox->setFocus();
        m_listBox->setCurrentRow( std::min( m_listBox->currentRow() + 1,
                                            m_listBox->count() - 1 ) );
        return true;

    case Qt::Key_Up:
        m_listBox->setFocus();
        m_listBox->setCurrentRow( std::max( m_listBox->currentRow() - 1, 0 ) );
        return true;

    // Everything else goes to the filter textbox
    default:
        if( !m_filterCtrl->hasFocus() )
        {
            m_filterCtrl->setFocus();

            // Because we didn't have focus we missed our chance to have the native widget
            // handle the keystroke.  We'll have to do the first character ourselves.
            doStartingKey( aEvent );
            return true;
        }

        return false;
    }
}


void FILTER_COMBOPOPUP::doStartingKey( QKeyEvent* aEvent )
{
    if( aEvent->key() == Qt::Key_Backspace )
    {
        m_filterCtrl->backspace();
    }
    else
    {
        QString text = aEvent->text();

        if( !text.isEmpty() && text.at( 0 ).isPrint() && !text.at( 0 ).isSpace() )
        {
            m_filterCtrl->end( false );
            m_filterCtrl->insert( text );
        }
    }
}


wxDEFINE_EVENT( FILTERED_ITEM_SELECTED, wxCommandEvent );


FILTER_COMBOBOX::FILTER_COMBOBOX( wxWidgetArgCompat parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style ) :
        wxWindowCompat<QComboBox>( parent.get() ),
        m_filterPopup( nullptr ),
        m_style( style )
{
    SetId( id );

    // A read-only combo box shows the chosen string; the other ones can be typed in
    setEditable( !( style & wxCB_READONLY ) );

    if( size.x > 0 || size.y > 0 )
        resize( size.x > 0 ? size.x : width(), size.y > 0 ? size.y : height() );
}


FILTER_COMBOBOX::FILTER_COMBOBOX( wxWidgetArgCompat parent, wxWindowID id, const wxString& value,
                                  const wxPoint& pos, const wxSize& size,
                                  int count, wxString strings[], long style ) :
        FILTER_COMBOBOX( parent, id, pos, size, style )
{
    // These arguments are just to match wxFormBuilder's expected API; they are not supported
    KI_ASSERT( value.IsEmpty() && count == 0 && strings == nullptr );

    setFilterPopup( new FILTER_COMBOPOPUP() );
}


FILTER_COMBOBOX::~FILTER_COMBOBOX()
{
    delete m_filterPopup;
}


void FILTER_COMBOBOX::SetStringList( const wxArrayString& aList )
{
    m_filterPopup->SetStringList( aList );
}


void FILTER_COMBOBOX::SetSelectedString( const wxString& aString )
{
    m_filterPopup->SetSelectedString( aString );
}


void FILTER_COMBOBOX::SetValue( const wxString& aValue )
{
    QSignalBlocker blocker( this );

    if( isEditable() )
    {
        setEditText( ToQString( aValue ) );
    }
    else
    {
        clear();
        addItem( ToQString( aValue ) );
        setCurrentIndex( 0 );
    }
}


wxString FILTER_COMBOBOX::GetValue() const
{
    return ToWxString( currentText() );
}


void FILTER_COMBOBOX::OnFilteredItemSelected( std::function<void()> aHandler )
{
    m_selectedHandlers.push_back( std::move( aHandler ) );
}


void FILTER_COMBOBOX::NotifyItemSelected()
{
    // After the event, as the posted event of the wxWidgets build
    QTimer::singleShot( 0, this,
                        [this]()
                        {
                            // A copy: a handler may add handlers
                            std::vector<std::function<void()>> handlers = m_selectedHandlers;

                            for( const std::function<void()>& handler : handlers )
                                handler();

                            wxCommandEvent event( FILTERED_ITEM_SELECTED, GetId() );
                            event.SetEventObject( this );
                            ProcessEvent( event );
                        } );
}


void FILTER_COMBOBOX::showPopup()
{
    if( !m_filterPopup )
        return;

    m_filterPopup->OnPopup();

    // Below the combo box, or above it when there isn't room below
    QPoint   pos = mapToGlobal( QPoint( 0, height() ) );
    QScreen* scr = screen();

    if( scr )
    {
        QRect available = scr->availableGeometry();

        if( pos.y() + m_filterPopup->height() > available.bottom() )
            pos.setY( mapToGlobal( QPoint( 0, 0 ) ).y() - m_filterPopup->height() );

        if( pos.x() + m_filterPopup->width() > available.right() )
            pos.setX( std::max( available.left(), available.right() - m_filterPopup->width() ) );
    }

    m_filterPopup->move( pos );
    m_filterPopup->show();
    m_filterPopup->raise();
    m_filterPopup->activateWindow();

    // The size of the list is known once the popup is laid out
    m_filterPopup->OnPopup();
}


void FILTER_COMBOBOX::hidePopup()
{
    if( m_filterPopup )
        m_filterPopup->Dismiss();
}


bool FILTER_COMBOBOX::IsPopupShown() const
{
    return m_filterPopup && m_filterPopup->isVisible();
}


void FILTER_COMBOBOX::setFilterPopup( FILTER_COMBOPOPUP* aPopup )
{
    delete m_filterPopup;
    m_filterPopup = aPopup;
    m_filterPopup->Create( this );
}


void FILTER_COMBOBOX::keyPressEvent( QKeyEvent* aEvent )
{
    int key = aEvent->key();

    if( IsPopupShown() )
    {
        // The popup handles its keys
        QComboBox::keyPressEvent( aEvent );
    }
    // Shift-return accepts dialog
    else if( ( key == Qt::Key_Return || key == Qt::Key_Enter )
             && ( aEvent->modifiers() & Qt::ShiftModifier ) )
    {
        if( QDialog* dialog = qobject_cast<QDialog*>( window() ) )
        {
            for( QPushButton* button : dialog->findChildren<QPushButton*>() )
            {
                if( button->isDefault() && button->isEnabled() )
                {
                    button->click();
                    return;
                }
            }
        }
    }
    // Return, arrow-down and space-bar all open popup
    else if( key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Down
             || key == Qt::Key_Space )
    {
        showPopup();
    }
    // Non-control characters go to filterbox in popup for read-only controls
    else if( !isEditable() && !aEvent->text().isEmpty() && aEvent->text().at( 0 ).isPrint()
             && !aEvent->text().at( 0 ).isSpace() )
    {
        showPopup();
        m_filterPopup->OnStartingKey( aEvent );
    }
    else
    {
        QComboBox::keyPressEvent( aEvent );
    }
}
