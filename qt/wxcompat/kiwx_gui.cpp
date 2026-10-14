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
 * @file kiwx_gui.cpp
 * The controls and the events of the wxWidgets compatibility layer of the Qt build.
 */

#include <wx/kiwx_gui.h>

#include <atomic>

#include <QAbstractButton>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QMessageBox>
#include <QScreen>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QVariant>

#include <toolkit/qt_keys.h>


// Ids and events ---------------------------------------------------------------------------

int wxWindowIDNew()
{
    // The automatic ids of wxWidgets are negative, below wxID_AUTO_LOWEST
    static std::atomic<int> s_next( -32000 );
    return s_next--;
}


static const char* WINDOW_ID_PROPERTY = "kiwxWindowId";


int wxGetWindowIdCompat( const QObject* aWindow )
{
    if( !aWindow )
        return wxID_ANY;

    QVariant id = aWindow->property( WINDOW_ID_PROPERTY );

    if( !id.isValid() )
    {
        // A new id, as wxWidgets gives to the windows created with wxID_ANY
        id = wxWindowIDNew();
        const_cast<QObject*>( aWindow )->setProperty( WINDOW_ID_PROPERTY, id );
    }

    return id.toInt();
}


void wxSetWindowIdCompat( QObject* aWindow, int aId )
{
    if( aWindow )
        aWindow->setProperty( WINDOW_ID_PROPERTY, aId == wxID_ANY ? wxWindowIDNew() : aId );
}


wxEventType wxNewEventType()
{
    static std::atomic<int> s_next( wxEVT_USER_FIRST );
    return s_next++;
}


void wxFillMouseEventCompat( wxMouseEvent& aEvent, QEvent* aQtEvent )
{
    Qt::KeyboardModifiers modifiers;

    if( QMouseEvent* mouse = dynamic_cast<QMouseEvent*>( aQtEvent ) )
    {
        aEvent.m_x = int( mouse->position().x() );
        aEvent.m_y = int( mouse->position().y() );
        modifiers = mouse->modifiers();
    }
    else if( QWheelEvent* wheel = dynamic_cast<QWheelEvent*>( aQtEvent ) )
    {
        aEvent.m_x = int( wheel->position().x() );
        aEvent.m_y = int( wheel->position().y() );
        aEvent.m_wheelRotation = wheel->angleDelta().y();
        modifiers = wheel->modifiers();
    }

    aEvent.m_shift = modifiers & Qt::ShiftModifier;
    aEvent.m_control = modifiers & Qt::ControlModifier;
    aEvent.m_alt = modifiers & Qt::AltModifier;
}


void wxFillKeyEventCompat( wxKeyEvent& aEvent, QEvent* aQtEvent )
{
    QKeyEvent* key = dynamic_cast<QKeyEvent*>( aQtEvent );

    if( !key )
        return;

    aEvent.m_keyCode = TOOLKIT::KeyCodeFromQt( *key );
    aEvent.m_modifiers = 0;

    if( key->modifiers() & Qt::ShiftModifier )
        aEvent.m_modifiers |= wxMOD_SHIFT;

    if( key->modifiers() & Qt::ControlModifier )
        aEvent.m_modifiers |= wxMOD_CONTROL;

    if( key->modifiers() & Qt::AltModifier )
        aEvent.m_modifiers |= wxMOD_ALT;
}


wxSize wxSizeOfEventCompat( QEvent* aQtEvent )
{
    if( QResizeEvent* resize = dynamic_cast<QResizeEvent*>( aQtEvent ) )
        return wxSize( resize->size().width(), resize->size().height() );

    return wxSize();
}


wxString wxStripMenuCodes( const wxString& aLabel )
{
    wxString result;

    for( size_t i = 0; i < aLabel.length(); ++i )
    {
        wxUniChar c = aLabel[i];

        if( c == '&' )
        {
            // "&&" is a '&'
            if( i + 1 < aLabel.length() && aLabel[i + 1] == '&' )
            {
                result += '&';
                ++i;
            }

            continue;
        }

        if( c == '\t' )
            break;

        result += c;
    }

    return result;
}


void wxCentreWindow( QWidget* aWindow, int aDirection )
{
    QWidget* parent = aWindow->parentWidget() ? aWindow->parentWidget()->window() : nullptr;
    QRect    area;

    if( parent && parent != aWindow )
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


// The sizer of a window --------------------------------------------------------------------

QLayout* wxSizerProxy::GetLayout() const
{
    return m_window ? m_window->layout() : nullptr;
}


void wxSizerProxy::Layout()
{
    if( QLayout* layout = GetLayout() )
    {
        layout->invalidate();
        layout->activate();
    }
}


wxSize wxSizerProxy::Fit( QWidget* aWindow )
{
    Layout();

    if( !aWindow )
        return wxSize();

    aWindow->adjustSize();
    return wxSize( aWindow->width(), aWindow->height() );
}


void wxSizerProxy::SetSizeHints( QWidget* aWindow )
{
    if( QLayout* layout = GetLayout(); layout && aWindow )
    {
        layout->activate();
        aWindow->setMinimumSize( layout->minimumSize() );
        aWindow->adjustSize();
    }
}


wxSize wxSizerProxy::GetMinSize() const
{
    QLayout* layout = GetLayout();

    if( !layout )
        return wxSize();

    QSize size = layout->minimumSize();
    return wxSize( size.width(), size.height() );
}


bool wxSizerProxy::Show( QWidget* aWindow, bool aShow, bool aRecursive )
{
    if( !aWindow )
        return false;

    aWindow->setVisible( aShow );
    return true;
}


bool wxSizerProxy::IsShown( QWidget* aWindow ) const
{
    return aWindow && !aWindow->isHidden();
}


void wxUpdateUIEvent::Enable( bool aEnable )
{
    if( QWidget* widget = qobject_cast<QWidget*>( GetEventObject() ) )
        widget->setEnabled( aEnable );
}


void wxUpdateUIEvent::Check( bool aCheck )
{
    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( GetEventObject() ) )
    {
        QSignalBlocker blocker( button );
        button->setChecked( aCheck );
    }
}


void wxUpdateUIEvent::Show( bool aShow )
{
    if( QWidget* widget = qobject_cast<QWidget*>( GetEventObject() ) )
        widget->setVisible( aShow );
}


void wxUpdateUIEvent::SetText( const wxString& aText )
{
    QObject* object = GetEventObject();

    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( object ) )
        button->setText( ToQString( aText ) );
    else if( QLabel* label = qobject_cast<QLabel*>( object ) )
        label->setText( ToQString( aText ) );
}


// Controls ---------------------------------------------------------------------------------

wxPanel::wxPanel( QWidget* aParent, int aId, const wxPoint& aPos, const wxSize& aSize, long aStyle,
                  const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent )
{
    SetId( aId );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 || aSize.y > 0 )
        SetMinSize( aSize );
}


wxStaticText::wxStaticText( QWidget* aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
                            const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QLabel>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );

    if( aStyle & wxALIGN_RIGHT )
        setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    else if( aStyle & wxALIGN_CENTER_HORIZONTAL )
        setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
}


void wxStaticText::SetLabel( const wxString& aLabel )
{
    // The mnemonics of a static text are not shown
    setText( ToQString( wxStripMenuCodes( aLabel ) ) );
}


wxString wxStaticText::GetLabel() const
{
    return ToWxString( text() );
}


void wxStaticText::Wrap( int aWidth )
{
    setWordWrap( aWidth > 0 );

    if( aWidth > 0 )
        setMaximumWidth( std::max( aWidth, minimumWidth() ) );
}


wxStaticLine::wxStaticLine( QWidget* aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                            long aStyle ) :
        wxWindowCompat<QFrame>( aParent )
{
    SetId( aId );
    setFrameShape( ( aStyle & wxLI_VERTICAL ) ? QFrame::VLine : QFrame::HLine );
    setFrameShadow( QFrame::Sunken );
}


wxStaticBitmap::wxStaticBitmap( QWidget* aParent, int aId ) :
        wxWindowCompat<QLabel>( aParent )
{
    SetId( aId );
}


void wxStaticBitmap::SetBitmap( const QIcon& aBitmap )
{
    QList<QSize> sizes = aBitmap.availableSizes();
    QSize        size = sizes.isEmpty() ? QSize( 16, 16 ) : sizes.first();
    setPixmap( aBitmap.pixmap( size ) );
}


void wxStaticBitmap::SetBitmap( const QPixmap& aBitmap )
{
    setPixmap( aBitmap );
}


wxTextCtrl::wxTextCtrl( QWidget* aParent, int aId, const wxString& aValue, const wxPoint& aPos,
                        const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QLineEdit>( aParent )
{
    SetId( aId );
    setText( ToQString( aValue ) );

    if( aStyle & wxTE_READONLY )
        setReadOnly( true );

    if( aStyle & wxTE_PASSWORD )
        setEchoMode( QLineEdit::Password );

    if( aSize.x > 0 )
        setMinimumWidth( aSize.x );
}


void wxTextCtrl::SetValue( const wxString& aValue )
{
    setText( ToQString( aValue ) );
    setModified( false );
}


void wxTextCtrl::ChangeValue( const wxString& aValue )
{
    QSignalBlocker blocker( this );
    SetValue( aValue );
}


void wxTextCtrl::SetSelection( long aFrom, long aTo )
{
    // (-1, -1) selects all the text
    if( aFrom == -1 && aTo == -1 )
    {
        selectAll();
        return;
    }

    long length = text().size();
    long to = aTo < 0 ? length : std::min( aTo, length );
    long from = std::clamp( aFrom, 0L, to );
    setSelection( int( from ), int( to - from ) );
}


void wxTextCtrl::GetSelection( long* aFrom, long* aTo ) const
{
    long start = hasSelectedText() ? selectionStart() : cursorPosition();
    long length = hasSelectedText() ? selectedText().size() : 0;

    if( aFrom )
        *aFrom = start;

    if( aTo )
        *aTo = start + length;
}


void wxTextCtrl::Replace( long aFrom, long aTo, const wxString& aText )
{
    QString value = text();
    long    to = std::min<long>( aTo, value.size() );
    long    from = std::clamp( aFrom, 0L, to );
    value.replace( int( from ), int( to - from ), ToQString( aText ) );
    setText( value );
}


wxString wxTextCtrl::GetRange( long aFrom, long aTo ) const
{
    QString value = text();
    long    to = std::min<long>( aTo, value.size() );
    long    from = std::clamp( aFrom, 0L, to );
    return ToWxString( value.mid( int( from ), int( to - from ) ) );
}


wxTextCtrlMultiLine::wxTextCtrlMultiLine( QWidget* aParent ) :
        wxWindowCompat<QPlainTextEdit>( aParent )
{
}


void wxTextCtrlMultiLine::SetValue( const wxString& aValue )
{
    setPlainText( ToQString( aValue ) );
    document()->setModified( false );
}


void wxTextCtrlMultiLine::ChangeValue( const wxString& aValue )
{
    QSignalBlocker blocker( this );
    SetValue( aValue );
}


void wxTextCtrlMultiLine::AppendText( const wxString& aText )
{
    QTextCursor cursor = textCursor();
    cursor.movePosition( QTextCursor::End );
    cursor.insertText( ToQString( aText ) );
    setTextCursor( cursor );
}


void wxTextCtrlMultiLine::SetInsertionPointEnd()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition( QTextCursor::End );
    setTextCursor( cursor );
}


wxString wxTextCtrlMultiLine::GetLineText( long aLine ) const
{
    return ToWxString( document()->findBlockByNumber( int( aLine ) ).text() );
}


wxButton::wxButton( QWidget* aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
                    const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QPushButton>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );
    setAutoDefault( false );
}


wxToggleButton::wxToggleButton( QWidget* aParent, int aId, const wxString& aLabel ) :
        wxButton( aParent, aId, aLabel )
{
    setCheckable( true );
}


void wxToggleButton::SetValue( bool aValue )
{
    QSignalBlocker blocker( this );
    setChecked( aValue );
}


wxBitmapButton::wxBitmapButton( QWidget* aParent, int aId, const QIcon& aBitmap ) :
        wxButton( aParent, aId )
{
    if( !aBitmap.isNull() )
        setIcon( aBitmap );
}


wxCheckBox::wxCheckBox( QWidget* aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
                        const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QCheckBox>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );

    if( aStyle & wxCHK_3STATE )
        setTristate( true );
}


void wxCheckBox::SetValue( bool aValue )
{
    QSignalBlocker blocker( this );
    setChecked( aValue );
}


wxCheckBoxState wxCheckBox::Get3StateValue() const
{
    switch( checkState() )
    {
    case Qt::Checked:          return wxCHK_CHECKED;
    case Qt::PartiallyChecked: return wxCHK_UNDETERMINED;
    default:                   return wxCHK_UNCHECKED;
    }
}


void wxCheckBox::Set3StateValue( wxCheckBoxState aState )
{
    QSignalBlocker blocker( this );

    switch( aState )
    {
    case wxCHK_CHECKED:
        setCheckState( Qt::Checked );
        break;

    case wxCHK_UNDETERMINED:
        setTristate( true );
        setCheckState( Qt::PartiallyChecked );
        break;

    default:
        setCheckState( Qt::Unchecked );
        break;
    }
}


wxRadioButton::wxRadioButton( QWidget* aParent, int aId, const wxString& aLabel,
                              const wxPoint& aPos, const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QRadioButton>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );
}


void wxRadioButton::SetValue( bool aValue )
{
    QSignalBlocker blocker( this );
    setChecked( aValue );
}


// The client data of the items of the lists of Qt
static const int CLIENT_DATA_ROLE = Qt::UserRole + 100;


wxChoice::wxChoice( QWidget* aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                    const wxArrayString& aChoices, long aStyle ) :
        wxItemContainerCompat<QComboBox>( aParent )
{
    SetId( aId );

    for( const wxString& choice : aChoices )
        addItem( ToQString( choice ) );

    setCurrentIndex( -1 );
}


wxString wxChoice::GetString( unsigned int aIndex ) const
{
    return ToWxString( itemText( int( aIndex ) ) );
}


void wxChoice::SetString( unsigned int aIndex, const wxString& aString )
{
    setItemText( int( aIndex ), ToQString( aString ) );
}


void wxChoice::SetSelection( int aIndex )
{
    QSignalBlocker blocker( this );
    setCurrentIndex( aIndex );
}


int wxChoice::Insert( const wxString& aItem, unsigned int aPos, void* aClientData )
{
    QSignalBlocker blocker( this );
    int            index = std::min( int( aPos ), count() );

    insertItem( index, ToQString( aItem ),
                QVariant::fromValue( reinterpret_cast<quintptr>( aClientData ) ) );

    // A wxChoice has no selection until one is set, a QComboBox selects its first item
    if( count() == 1 && !isEditable() )
        setCurrentIndex( -1 );

    return index;
}


void wxChoice::Clear()
{
    QSignalBlocker blocker( this );
    clear();
}


void* wxChoice::GetClientData( unsigned int aIndex ) const
{
    return reinterpret_cast<void*>( itemData( int( aIndex ) ).value<quintptr>() );
}


void wxChoice::SetClientData( unsigned int aIndex, void* aData )
{
    setItemData( int( aIndex ), QVariant::fromValue( reinterpret_cast<quintptr>( aData ) ) );
}


wxComboBox::wxComboBox( QWidget* aParent, int aId, const wxString& aValue, const wxPoint& aPos,
                        const wxSize& aSize, const wxArrayString& aChoices, long aStyle ) :
        wxChoice( aParent, aId, aPos, aSize, aChoices, aStyle )
{
    if( !( aStyle & wxCB_READONLY ) )
    {
        setEditable( true );
        setEditText( ToQString( aValue ) );
    }
}


void wxComboBox::SetValue( const wxString& aValue )
{
    if( isEditable() )
    {
        setEditText( ToQString( aValue ) );
    }
    else
    {
        int index = findText( ToQString( aValue ) );

        if( index >= 0 )
            setCurrentIndex( index );
    }
}


void wxComboBox::ChangeValue( const wxString& aValue )
{
    QSignalBlocker blocker( this );
    SetValue( aValue );
}


void wxComboBox::SelectAll()
{
    if( lineEdit() )
        lineEdit()->selectAll();
}


void wxComboBox::SetInsertionPointEnd()
{
    if( lineEdit() )
        lineEdit()->end( false );
}


void wxComboBox::SetSelection( long aFrom, long aTo )
{
    if( !lineEdit() )
        return;

    if( aFrom == -1 && aTo == -1 )
        lineEdit()->selectAll();
    else
        lineEdit()->setSelection( int( aFrom ), int( aTo - aFrom ) );
}


wxListBox::wxListBox( QWidget* aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                      const wxArrayString& aChoices, long aStyle ) :
        wxItemContainerCompat<QListWidget>( aParent )
{
    SetId( aId );

    for( const wxString& choice : aChoices )
        addItem( ToQString( choice ) );

    if( aStyle & ( wxLB_MULTIPLE | wxLB_EXTENDED ) )
        setSelectionMode( QAbstractItemView::ExtendedSelection );
}


wxString wxListBox::GetString( unsigned int aIndex ) const
{
    QListWidgetItem* it = item( int( aIndex ) );
    return it ? ToWxString( it->text() ) : wxString();
}


void wxListBox::SetString( unsigned int aIndex, const wxString& aString )
{
    if( QListWidgetItem* it = item( int( aIndex ) ) )
        it->setText( ToQString( aString ) );
}


int wxListBox::GetSelection() const
{
    QList<QListWidgetItem*> selected = selectedItems();

    if( selected.isEmpty() )
        return wxNOT_FOUND;

    return row( selected.first() );
}


void wxListBox::SetSelection( int aIndex )
{
    QSignalBlocker blocker( this );

    if( aIndex < 0 )
    {
        clearSelection();
        return;
    }

    if( selectionMode() == QAbstractItemView::SingleSelection )
        setCurrentRow( aIndex );
    else if( QListWidgetItem* it = item( aIndex ) )
        it->setSelected( true );
}


void wxListBox::SetSelection( int aIndex, bool aSelect )
{
    QSignalBlocker blocker( this );

    if( QListWidgetItem* it = item( aIndex ) )
        it->setSelected( aSelect );
}


int wxListBox::Insert( const wxString& aItem, unsigned int aPos, void* aClientData )
{
    int index = std::min( int( aPos ), count() );
    auto* it = new QListWidgetItem( ToQString( aItem ) );
    it->setData( CLIENT_DATA_ROLE, QVariant::fromValue( reinterpret_cast<quintptr>( aClientData ) ) );
    insertItem( index, it );
    return index;
}


void wxListBox::Delete( unsigned int aIndex )
{
    delete takeItem( int( aIndex ) );
}


void wxListBox::Clear()
{
    QSignalBlocker blocker( this );
    clear();
}


void* wxListBox::GetClientData( unsigned int aIndex ) const
{
    QListWidgetItem* it = item( int( aIndex ) );
    return it ? reinterpret_cast<void*>( it->data( CLIENT_DATA_ROLE ).value<quintptr>() ) : nullptr;
}


void wxListBox::SetClientData( unsigned int aIndex, void* aData )
{
    if( QListWidgetItem* it = item( int( aIndex ) ) )
        it->setData( CLIENT_DATA_ROLE, QVariant::fromValue( reinterpret_cast<quintptr>( aData ) ) );
}


int wxListBox::GetSelections( std::vector<int>& aSelections ) const
{
    aSelections.clear();

    for( int i = 0; i < count(); ++i )
    {
        if( item( i )->isSelected() )
            aSelections.push_back( i );
    }

    return int( aSelections.size() );
}


bool wxListBox::IsSelected( int aIndex ) const
{
    QListWidgetItem* it = item( aIndex );
    return it && it->isSelected();
}


void wxListBox::Deselect( int aIndex )
{
    SetSelection( aIndex, false );
}


void wxListBox::DeselectAll()
{
    QSignalBlocker blocker( this );
    clearSelection();
}


void wxListBox::EnsureVisible( int aIndex )
{
    if( QListWidgetItem* it = item( aIndex ) )
        scrollToItem( it );
}


wxCheckListBox::wxCheckListBox( QWidget* aParent, int aId, const wxPoint& aPos,
                                const wxSize& aSize, const wxArrayString& aChoices, long aStyle ) :
        wxListBox( aParent, aId, aPos, aSize, wxArrayString(), aStyle )
{
    for( const wxString& choice : aChoices )
        Append( choice );
}


bool wxCheckListBox::IsChecked( unsigned int aIndex ) const
{
    QListWidgetItem* it = item( int( aIndex ) );
    return it && it->checkState() == Qt::Checked;
}


void wxCheckListBox::Check( unsigned int aIndex, bool aCheck )
{
    QSignalBlocker blocker( this );

    if( QListWidgetItem* it = item( int( aIndex ) ) )
        it->setCheckState( aCheck ? Qt::Checked : Qt::Unchecked );
}


int wxCheckListBox::Insert( const wxString& aItem, unsigned int aPos, void* aClientData )
{
    QSignalBlocker blocker( this );
    int            index = wxListBox::Insert( aItem, aPos, aClientData );
    QListWidgetItem* it = item( index );
    it->setFlags( it->flags() | Qt::ItemIsUserCheckable );
    it->setCheckState( Qt::Unchecked );
    return index;
}


wxSpinCtrl::wxSpinCtrl( QWidget* aParent, int aId, const wxString& aValue, const wxPoint& aPos,
                        const wxSize& aSize, long aStyle, int aMin, int aMax, int aInitial ) :
        wxWindowCompat<QSpinBox>( aParent )
{
    SetId( aId );
    setRange( aMin, aMax );
    setValue( aInitial );

    if( !aValue.IsEmpty() )
        SetValue( aValue );
}


void wxSpinCtrl::SetValue( int aValue )
{
    QSignalBlocker blocker( this );
    setValue( aValue );
}


void wxSpinCtrl::SetValue( const wxString& aValue )
{
    long value = 0;

    if( aValue.ToLong( &value ) )
        SetValue( int( value ) );
}


wxSpinCtrlDouble::wxSpinCtrlDouble( QWidget* aParent, int aId ) :
        wxWindowCompat<QDoubleSpinBox>( aParent )
{
    SetId( aId );
}


void wxSpinCtrlDouble::SetValue( double aValue )
{
    QSignalBlocker blocker( this );
    setValue( aValue );
}


void wxSpinCtrlDouble::SetValue( const wxString& aValue )
{
    double value = 0;

    if( aValue.ToDouble( &value ) )
        SetValue( value );
}


wxSlider::wxSlider( QWidget* aParent, int aId, int aValue, int aMin, int aMax ) :
        wxWindowCompat<QSlider>( Qt::Horizontal, aParent )
{
    SetId( aId );
    setRange( aMin, aMax );
    setValue( aValue );
}


void wxSlider::SetValue( int aValue )
{
    QSignalBlocker blocker( this );
    setValue( aValue );
}


wxGauge::wxGauge( QWidget* aParent, int aId, int aRange ) :
        wxWindowCompat<QProgressBar>( aParent )
{
    SetId( aId );
    setRange( 0, aRange );
    setValue( 0 );
}


wxStaticBox::wxStaticBox( QWidget* aParent, int aId, const wxString& aLabel ) :
        wxWindowCompat<QGroupBox>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );
}


wxNotebook::wxNotebook( QWidget* aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                        long aStyle ) :
        wxWindowCompat<QTabWidget>( aParent )
{
    SetId( aId );
}


int wxNotebook::SetSelection( size_t aPage )
{
    int old = currentIndex();
    setCurrentIndex( int( aPage ) );
    return old;
}


int wxNotebook::ChangeSelection( size_t aPage )
{
    QSignalBlocker blocker( this );
    return SetSelection( aPage );
}


bool wxNotebook::SetPageText( size_t aPage, const wxString& aText )
{
    if( int( aPage ) >= count() )
        return false;

    setTabText( int( aPage ), ToQString( aText ) );
    return true;
}


bool wxNotebook::AddPage( QWidget* aPage, const wxString& aText, bool aSelect, int aImage )
{
    return InsertPage( count(), aPage, aText, aSelect, aImage );
}


bool wxNotebook::InsertPage( size_t aIndex, QWidget* aPage, const wxString& aText, bool aSelect,
                             int aImage )
{
    int index = insertTab( int( aIndex ), aPage, ToQString( aText ) );

    if( aSelect )
        setCurrentIndex( index );

    return index >= 0;
}


bool wxNotebook::RemovePage( size_t aPage )
{
    if( int( aPage ) >= count() )
        return false;

    removeTab( int( aPage ) );
    return true;
}


bool wxNotebook::DeletePage( size_t aPage )
{
    QWidget* page = widget( int( aPage ) );

    if( !RemovePage( aPage ) )
        return false;

    delete page;
    return true;
}


bool wxNotebook::DeleteAllPages()
{
    while( count() > 0 )
        DeletePage( 0 );

    return true;
}


// The buttons of a dialog ------------------------------------------------------------------

static const char* STANDARD_BUTTON_PROPERTY = "kiqtStandardButton";


wxStdDialogButtonSizer::wxStdDialogButtonSizer( QWidget* aParent ) :
        QDialogButtonBox( aParent )
{
}


wxButton* wxStdDialogButtonSizer::AddStandardButton( QDialogButtonBox::StandardButton aButton,
                                                      int aId )
{
    // The role and the text of a standard button of Qt
    QPushButton* standard = addButton( aButton );
    ButtonRole   role = buttonRole( standard );
    QString      text = standard->text();
    removeButton( standard );
    delete standard;

    wxButton* button = new wxButton( this, aId, ToWxString( text ) );
    button->setProperty( STANDARD_BUTTON_PROPERTY, int( aButton ) );
    addButton( button, role );
    return button;
}


wxButton* wxStdDialogButtonSizer::StandardButtonCompat( QDialogButtonBox::StandardButton aButton ) const
{
    for( QAbstractButton* button : buttons() )
    {
        if( button->property( STANDARD_BUTTON_PROPERTY ).toInt() == int( aButton ) )
            return dynamic_cast<wxButton*>( button );
    }

    return nullptr;
}


wxButton* wxStdDialogButtonSizer::GetAffirmativeButton() const
{
    if( wxButton* button = StandardButtonCompat( Ok ) )
        return button;

    if( wxButton* button = StandardButtonCompat( Yes ) )
        return button;

    return StandardButtonCompat( Save );
}


wxButton* wxStdDialogButtonSizer::GetApplyButton() const
{
    return StandardButtonCompat( Apply );
}


wxButton* wxStdDialogButtonSizer::GetNegativeButton() const
{
    return StandardButtonCompat( No );
}


wxButton* wxStdDialogButtonSizer::GetCancelButton() const
{
    return StandardButtonCompat( Cancel );
}


wxButton* wxStdDialogButtonSizer::GetHelpButton() const
{
    return StandardButtonCompat( Help );
}


// Sizers -----------------------------------------------------------------------------------

Qt::Alignment wxSizerFlagsAlignment( int aFlags, bool aVertical )
{
    if( aFlags & wxEXPAND )
        return Qt::Alignment();

    Qt::Alignment alignment;

    if( aVertical )
    {
        if( aFlags & wxALIGN_RIGHT )
            alignment = Qt::AlignRight;
        else if( aFlags & wxALIGN_CENTER_HORIZONTAL )
            alignment = Qt::AlignHCenter;
        else
            alignment = Qt::AlignLeft;
    }
    else
    {
        if( aFlags & wxALIGN_BOTTOM )
            alignment = Qt::AlignBottom;
        else if( aFlags & wxALIGN_CENTER_VERTICAL )
            alignment = Qt::AlignVCenter;
        else
            alignment = Qt::AlignTop;
    }

    return alignment;
}


wxBoxSizer::wxBoxSizer( int aOrient ) :
        wxSizerCompat<QBoxLayout>( aOrient == wxHORIZONTAL ? QBoxLayout::LeftToRight
                                                            : QBoxLayout::TopToBottom )
{
}


int wxBoxSizer::GetOrientation() const
{
    return ( direction() == LeftToRight || direction() == RightToLeft ) ? wxHORIZONTAL
                                                                        : wxVERTICAL;
}


void wxBoxSizer::Add( QWidget* aWindow, int aProportion, int aFlag, int aBorder, void* aUserData )
{
    Insert( count(), aWindow, aProportion, aFlag, aBorder );
}


void wxBoxSizer::Add( QLayout* aSizer, int aProportion, int aFlag, int aBorder, void* aUserData )
{
    Insert( count(), aSizer, aProportion, aFlag, aBorder );
}


void wxBoxSizer::Add( int aWidth, int aHeight, int aProportion, int aFlag, int aBorder )
{
    if( aProportion > 0 )
        addStretch( aProportion );
    else
        addSpacing( GetOrientation() == wxVERTICAL ? aHeight : aWidth );
}


void wxBoxSizer::Insert( size_t aIndex, QWidget* aWindow, int aProportion, int aFlag, int aBorder )
{
    if( !aWindow )
        return;

    insertWidget( int( std::min<size_t>( aIndex, count() ) ), aWindow, aProportion,
                  wxSizerFlagsAlignment( aFlag, GetOrientation() == wxVERTICAL ) );
}


void wxBoxSizer::Insert( size_t aIndex, QLayout* aSizer, int aProportion, int aFlag, int aBorder )
{
    if( !aSizer )
        return;

    insertLayout( int( std::min<size_t>( aIndex, count() ) ), aSizer, aProportion );
}


wxStaticBoxSizer::wxStaticBoxSizer( wxStaticBox* aBox, int aOrient ) :
        wxBoxSizer( aOrient ),
        m_box( aBox )
{
    if( m_box )
        m_box->setLayout( this );
}


wxStaticBoxSizer::wxStaticBoxSizer( int aOrient, QWidget* aParent, const wxString& aLabel ) :
        wxStaticBoxSizer( new wxStaticBox( aParent, wxID_ANY, aLabel ), aOrient )
{
}


wxGridBagSizer::wxGridBagSizer( int aVGap, int aHGap )
{
    setVerticalSpacing( std::max( aVGap, 4 ) );
    setHorizontalSpacing( std::max( aHGap, 4 ) );
}


wxFlexGridSizer::wxFlexGridSizer( int aCols, int aVGap, int aHGap ) :
        wxFlexGridSizer( 0, aCols, aVGap, aHGap )
{
}


wxFlexGridSizer::wxFlexGridSizer( int aRows, int aCols, int aVGap, int aHGap ) :
        m_cols( aCols ),
        m_next( 0 )
{
    setVerticalSpacing( std::max( aVGap, 4 ) );
    setHorizontalSpacing( std::max( aHGap, 4 ) );
}


void wxFlexGridSizer::nextCell( int& aRow, int& aCol )
{
    int cols = std::max( m_cols, 1 );
    aRow = m_next / cols;
    aCol = m_next % cols;
    m_next++;
}


void wxFlexGridSizer::Add( QWidget* aWindow, int aProportion, int aFlag, int aBorder )
{
    int row, col;
    nextCell( row, col );

    if( !aWindow )
        return;

    Qt::Alignment alignment;

    if( !( aFlag & wxEXPAND ) )
    {
        alignment = ( aFlag & wxALIGN_RIGHT ) ? Qt::AlignRight
                    : ( aFlag & wxALIGN_CENTER_HORIZONTAL ) ? Qt::AlignHCenter
                                                             : Qt::AlignLeft;
        alignment |= ( aFlag & wxALIGN_BOTTOM ) ? Qt::AlignBottom
                     : ( aFlag & wxALIGN_CENTER_VERTICAL ) ? Qt::AlignVCenter
                                                            : Qt::AlignTop;
    }

    addWidget( aWindow, row, col, alignment );
}


void wxFlexGridSizer::Add( QLayout* aSizer, int aProportion, int aFlag, int aBorder )
{
    int row, col;
    nextCell( row, col );

    if( aSizer )
        addLayout( aSizer, row, col );
}


void wxFlexGridSizer::Add( int aWidth, int aHeight, int aProportion, int aFlag, int aBorder )
{
    int row, col;
    nextCell( row, col );
    addItem( new QSpacerItem( std::max( aWidth, 0 ), std::max( aHeight, 0 ) ), row, col );
}


// Dialogs and messages ---------------------------------------------------------------------

int wxMessageBox( const wxString& aMessage, const wxString& aCaption, long aStyle,
                  QWidget* aParent, int aX, int aY )
{
    QMessageBox::Icon icon = QMessageBox::Information;

    if( aStyle & wxICON_ERROR )
        icon = QMessageBox::Critical;
    else if( aStyle & wxICON_WARNING )
        icon = QMessageBox::Warning;
    else if( aStyle & wxICON_QUESTION )
        icon = QMessageBox::Question;

    QMessageBox::StandardButtons buttons;

    if( aStyle & wxYES )
        buttons |= QMessageBox::Yes;

    if( aStyle & wxNO )
        buttons |= QMessageBox::No;

    if( aStyle & wxCANCEL )
        buttons |= QMessageBox::Cancel;

    if( ( aStyle & wxOK ) || !buttons )
        buttons |= QMessageBox::Ok;

    QMessageBox box( icon, ToQString( aCaption ), ToQString( aMessage ), buttons, aParent );

    switch( box.exec() )
    {
    case QMessageBox::Yes:    return wxYES;
    case QMessageBox::No:     return wxNO;
    case QMessageBox::Cancel: return wxCANCEL;
    default:                  return wxOK;
    }
}


QWidget* wxFindFocusCompat()
{
    return QApplication::focusWidget();
}


void wxBell()
{
    QApplication::beep();
}
