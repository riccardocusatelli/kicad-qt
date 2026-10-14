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
#include <QAction>
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
#include <ui_hooks.h>


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
    aEvent.m_uniChar = key->text().isEmpty() ? 0 : wxChar( key->text().front().unicode() );

    // The letters are the upper case ones, as the codes of the keys of wxWidgets
    if( aEvent.m_uniChar >= 'a' && aEvent.m_uniChar <= 'z' )
        aEvent.m_uniChar -= 'a' - 'A';

    if( key->modifiers() & Qt::ShiftModifier )
        aEvent.m_modifiers |= wxMOD_SHIFT;

    if( key->modifiers() & Qt::ControlModifier )
        aEvent.m_modifiers |= wxMOD_CONTROL;

    if( key->modifiers() & Qt::AltModifier )
        aEvent.m_modifiers |= wxMOD_ALT;

    if( key->modifiers() & Qt::MetaModifier )
        aEvent.m_modifiers |= wxMOD_META;
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


void wxUpdateUIEvent::Enable( bool aEnable )
{
    if( QWidget* widget = qobject_cast<QWidget*>( GetEventObject() ) )
        widget->setEnabled( aEnable );
    else if( QAction* action = qobject_cast<QAction*>( GetEventObject() ) )
        action->setEnabled( aEnable );
}


void wxUpdateUIEvent::Check( bool aCheck )
{
    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( GetEventObject() ) )
    {
        QSignalBlocker blocker( button );
        button->setChecked( aCheck );
    }
    else if( QAction* action = qobject_cast<QAction*>( GetEventObject() ) )
    {
        QSignalBlocker blocker( action );
        action->setChecked( aCheck );
    }
}


void wxUpdateUIEvent::Show( bool aShow )
{
    if( QWidget* widget = qobject_cast<QWidget*>( GetEventObject() ) )
        widget->setVisible( aShow );
    else if( QAction* action = qobject_cast<QAction*>( GetEventObject() ) )
        action->setVisible( aShow );
}


void wxUpdateUIEvent::SetText( const wxString& aText )
{
    QObject* object = GetEventObject();

    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( object ) )
        button->setText( ToQString( aText ) );
    else if( QLabel* label = qobject_cast<QLabel*>( object ) )
        label->setText( ToQString( aText ) );
    else if( QAction* action = qobject_cast<QAction*>( object ) )
        action->setText( ToQString( aText ).section( '\t', 0, 0 ) );
}


// The handlers of the events -------------------------------------------------------------

void wxCompatEventSink::Add( wxEventType aType, HANDLER aHandler, const void* aKey, int aId,
                             int aLastId )
{
    m_entries.push_back( { aType, std::move( aHandler ), aKey, aId, aLastId } );
}


bool wxCompatEventSink::Remove( wxEventType aType, const void* aKey )
{
    size_t count = m_entries.size();

    std::erase_if( m_entries,
                   [&]( const ENTRY& aEntry )
                   {
                       return aEntry.m_type == aType && ( !aKey || aEntry.m_key == aKey );
                   } );

    return m_entries.size() != count;
}


bool wxCompatEventSink::Has( wxEventType aType ) const
{
    return std::any_of( m_entries.begin(), m_entries.end(),
                        [&]( const ENTRY& aEntry ) { return aEntry.m_type == aType; } );
}


bool wxCompatEventSink::Process( wxEvent& aEvent )
{
    // A copy: a handler can bind or unbind handlers
    std::vector<ENTRY> entries = m_entries;

    // The handlers bound last are called first, as with wxWidgets
    for( auto it = entries.rbegin(); it != entries.rend(); ++it )
    {
        if( it->m_type != aEvent.GetEventType() )
            continue;

        if( it->m_id != wxID_ANY )
        {
            int last = it->m_lastId == wxID_ANY ? it->m_id : it->m_lastId;

            if( aEvent.GetId() < it->m_id || aEvent.GetId() > last )
                continue;
        }

        aEvent.Skip( false );
        it->m_handler( aEvent );

        if( !aEvent.GetSkipped() )
            return true;
    }

    // Not processed: the default processing happens
    aEvent.Skip( true );
    return false;
}


namespace
{

/// Give a command event of a widget to its sink.
template <typename EVENT = wxCommandEvent>
void sendCompat( QWidget* aWidget, wxCompatEventSink* aSink, wxEventType aType,
                 const std::function<void( EVENT& )>& aFill = nullptr )
{
    EVENT event( aType, wxGetWindowIdCompat( aWidget ) );
    event.SetEventObject( aWidget );

    if( aFill )
        aFill( event );

    aSink->Process( event );
}

} // namespace


// The handlers of events ---------------------------------------------------------------------

void wxCallAfterCompat( std::function<void()> aFunction, std::weak_ptr<bool> aAlive )
{
    QMetaObject::invokeMethod( qApp,
                               [function = std::move( aFunction ), alive = std::move( aAlive )]()
                               {
                                   // Not after the handler is deleted
                                   if( !alive.expired() )
                                       function();
                               },
                               Qt::QueuedConnection );
}


wxEvtHandler::~wxEvtHandler()
{
    // The handlers pushed on this window go with it, as with wxWidgets they should be popped
    while( m_topCompat && m_topCompat != this )
        PopEventHandlerCompat( false );

    Unlink();
}


void wxEvtHandler::Unlink()
{
    if( m_previousCompat )
        m_previousCompat->m_nextCompat = m_nextCompat;

    if( m_nextCompat )
        m_nextCompat->m_previousCompat = m_previousCompat;

    m_nextCompat = nullptr;
    m_previousCompat = nullptr;
}


bool wxEvtHandler::ProcessChainCompat( wxEvtHandler* aFirst, wxEvent& aEvent )
{
    for( wxEvtHandler* handler = aFirst; handler; handler = handler->m_nextCompat )
    {
        if( handler->TryHereCompat( aEvent ) )
            return true;
    }

    return false;
}


void wxEvtHandler::PushEventHandlerCompat( wxEvtHandler* aHandler )
{
    if( !aHandler )
        return;

    wxEvtHandler* top = GetEventHandlerCompat();

    aHandler->m_nextCompat = top;
    aHandler->m_previousCompat = nullptr;
    top->m_previousCompat = aHandler;
    m_topCompat = aHandler;
}


wxEvtHandler* wxEvtHandler::PopEventHandlerCompat( bool aDeleteHandler )
{
    wxEvtHandler* top = m_topCompat;

    if( !top || top == this )
        return nullptr;

    wxEvtHandler* next = top->m_nextCompat;
    m_topCompat = ( next == this ) ? nullptr : next;

    if( next )
        next->m_previousCompat = nullptr;

    top->m_nextCompat = nullptr;
    top->m_previousCompat = nullptr;

    if( aDeleteHandler )
    {
        delete top;
        return nullptr;
    }

    return top;
}


bool wxEvtHandler::RemoveEventHandlerCompat( wxEvtHandler* aHandler )
{
    if( !aHandler || aHandler == this )
        return false;

    for( wxEvtHandler* handler = m_topCompat; handler && handler != this;
         handler = handler->m_nextCompat )
    {
        if( handler != aHandler )
            continue;

        if( m_topCompat == aHandler )
            m_topCompat = ( aHandler->m_nextCompat == this ) ? nullptr : aHandler->m_nextCompat;

        aHandler->Unlink();
        return true;
    }

    return false;
}


void wxConnectQtEventsCompat( QWidget* aWidget, wxEventType aType, wxCompatEventSink* aSink )
{
    // Connected once per type
    QByteArray property = "kiwxEvents" + QByteArray::number( aType );

    if( aWidget->property( property.constData() ).isValid() )
        return;

    aWidget->setProperty( property.constData(), true );

    QWidget* w = aWidget;

    switch( aType )
    {
    case wxEVT_BUTTON:
    case wxEVT_CHECKBOX:
    case wxEVT_RADIOBUTTON:
    case wxEVT_TOGGLEBUTTON:
        if( QAbstractButton* button = qobject_cast<QAbstractButton*>( w ) )
        {
            QObject::connect( button, &QAbstractButton::clicked, button,
                              [w, aSink, aType, button]()
                              {
                                  sendCompat( w, aSink, aType,
                                              std::function<void( wxCommandEvent& )>(
                                                      [button]( wxCommandEvent& e )
                                                      {
                                                          e.SetInt( button->isChecked() ? 1 : 0 );
                                                      } ) );
                              } );
        }

        break;

    case wxEVT_TEXT:
        if( QLineEdit* edit = qobject_cast<QLineEdit*>( w ) )
        {
            QObject::connect( edit, &QLineEdit::textChanged, edit,
                              [w, aSink]( const QString& aText )
                              {
                                  sendCompat( w, aSink, wxEVT_TEXT,
                                              std::function<void( wxCommandEvent& )>(
                                                      [&]( wxCommandEvent& e ) { e.SetString( ToWxString( aText ) ); } ) );
                              } );
        }
        else if( QPlainTextEdit* text = qobject_cast<QPlainTextEdit*>( w ) )
        {
            QObject::connect( text, &QPlainTextEdit::textChanged, text,
                              [w, aSink]() { sendCompat( w, aSink, wxEVT_TEXT ); } );
        }
        else if( QComboBox* combo = qobject_cast<QComboBox*>( w ) )
        {
            QObject::connect( combo, &QComboBox::editTextChanged, combo,
                              [w, aSink]( const QString& aText )
                              {
                                  sendCompat( w, aSink, wxEVT_TEXT,
                                              std::function<void( wxCommandEvent& )>(
                                                      [&]( wxCommandEvent& e ) { e.SetString( ToWxString( aText ) ); } ) );
                              } );
        }
        else if( QAbstractSpinBox* spin = qobject_cast<QAbstractSpinBox*>( w ) )
        {
            QObject::connect( spin, &QAbstractSpinBox::editingFinished, spin,
                              [w, aSink]() { sendCompat( w, aSink, wxEVT_TEXT ); } );
        }

        break;

    case wxEVT_TEXT_ENTER:
        if( QLineEdit* edit = qobject_cast<QLineEdit*>( w ) )
        {
            QObject::connect( edit, &QLineEdit::returnPressed, edit,
                              [w, aSink]() { sendCompat( w, aSink, wxEVT_TEXT_ENTER ); } );
        }
        else if( QComboBox* combo = qobject_cast<QComboBox*>( w ); combo && combo->lineEdit() )
        {
            QObject::connect( combo->lineEdit(), &QLineEdit::returnPressed, combo,
                              [w, aSink]() { sendCompat( w, aSink, wxEVT_TEXT_ENTER ); } );
        }

        break;

    case wxEVT_CHOICE:
    case wxEVT_COMBOBOX:
        if( QComboBox* combo = qobject_cast<QComboBox*>( w ) )
        {
            QObject::connect( combo, &QComboBox::activated, combo,
                              [w, aSink, aType, combo]( int aIndex )
                              {
                                  sendCompat( w, aSink, aType,
                                              std::function<void( wxCommandEvent& )>(
                                                      [&]( wxCommandEvent& e )
                                                      {
                                                          e.SetInt( aIndex );
                                                          e.SetString( ToWxString( combo->itemText( aIndex ) ) );
                                                      } ) );
                              } );
        }

        break;

    case wxEVT_LISTBOX:
    case wxEVT_LISTBOX_DCLICK:
    case wxEVT_CHECKLISTBOX:
        if( QListWidget* list = qobject_cast<QListWidget*>( w ) )
        {
            auto send = [w, aSink, aType]( int aRow )
                        {
                            sendCompat( w, aSink, aType,
                                        std::function<void( wxCommandEvent& )>(
                                                [&]( wxCommandEvent& e ) { e.SetInt( aRow ); } ) );
                        };

            if( aType == wxEVT_LISTBOX )
                QObject::connect( list, &QListWidget::currentRowChanged, list, send );
            else if( aType == wxEVT_LISTBOX_DCLICK )
                QObject::connect( list, &QListWidget::itemDoubleClicked, list,
                                  [send, list]( QListWidgetItem* aItem ) { send( list->row( aItem ) ); } );
            else
                QObject::connect( list, &QListWidget::itemChanged, list,
                                  [send, list]( QListWidgetItem* aItem ) { send( list->row( aItem ) ); } );
        }

        break;

    case wxEVT_SPINCTRL:
        if( QSpinBox* spin = qobject_cast<QSpinBox*>( w ) )
        {
            QObject::connect( spin, &QSpinBox::valueChanged, spin,
                              [w, aSink]( int aValue )
                              {
                                  sendCompat<wxSpinEvent>( w, aSink, wxEVT_SPINCTRL,
                                                           [&]( wxSpinEvent& e ) { e.SetPosition( aValue ); } );
                              } );
        }

        break;

    case wxEVT_SPINCTRLDOUBLE:
        if( QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>( w ) )
        {
            QObject::connect( spin, &QDoubleSpinBox::valueChanged, spin,
                              [w, aSink]( double aValue )
                              {
                                  sendCompat<wxSpinDoubleEvent>( w, aSink, wxEVT_SPINCTRLDOUBLE,
                                                                 [&]( wxSpinDoubleEvent& e ) { e.SetValue( aValue ); } );
                              } );
        }

        break;

    case wxEVT_SLIDER:
        if( QAbstractSlider* slider = qobject_cast<QAbstractSlider*>( w ) )
        {
            QObject::connect( slider, &QAbstractSlider::valueChanged, slider,
                              [w, aSink]( int aValue )
                              {
                                  sendCompat( w, aSink, wxEVT_SLIDER,
                                              std::function<void( wxCommandEvent& )>(
                                                      [&]( wxCommandEvent& e ) { e.SetInt( aValue ); } ) );
                              } );
        }

        break;

    case wxEVT_NOTEBOOK_PAGE_CHANGED:
        if( QTabWidget* tabs = qobject_cast<QTabWidget*>( w ) )
        {
            QObject::connect( tabs, &QTabWidget::currentChanged, tabs,
                              [w, aSink]( int aPage )
                              {
                                  sendCompat<wxBookCtrlEvent>( w, aSink, wxEVT_NOTEBOOK_PAGE_CHANGED,
                                                               [&]( wxBookCtrlEvent& e ) { e.SetSelection( aPage ); } );
                              } );
        }

        break;

    case wxEVT_RADIOBOX:
        if( wxRadioBox* box = dynamic_cast<wxRadioBox*>( w ) )
        {
            box->OnSelected(
                    [w, aSink, box]( int aIndex )
                    {
                        sendCompat( w, aSink, wxEVT_RADIOBOX,
                                    std::function<void( wxCommandEvent& )>(
                                            [&]( wxCommandEvent& e )
                                            {
                                                e.SetInt( aIndex );
                                                e.SetString( box->GetString( aIndex ) );
                                            } ) );
                    } );
        }

        break;

    case wxEVT_UPDATE_UI:
        KIQT::RegisterUpdateUI( w,
                                [w, aSink]()
                                {
                                    wxUpdateUIEvent event( wxGetWindowIdCompat( w ) );
                                    event.SetEventObject( w );
                                    aSink->Process( event );
                                } );
        break;

    case wxEVT_IDLE:
        KIQT::RegisterUpdateUI( w,
                                [w, aSink]()
                                {
                                    wxIdleEvent event;
                                    event.SetEventObject( w );
                                    aSink->Process( event );
                                } );
        break;

    case wxEVT_INIT_DIALOG:
        KIQT::OnEvent( w, QEvent::Show,
                       [w, aSink, done = false]( QEvent* ) mutable
                       {
                           if( done )
                               return;

                           done = true;
                           wxInitDialogEvent event( wxGetWindowIdCompat( w ) );
                           event.SetEventObject( w );
                           aSink->Process( event );
                       } );
        break;

    default:
        // The events of the widget (the input events, the focus, the size...)
        KIQT::FilterEvents( w,
                [w, aSink, aType]( QEvent* aQtEvent ) -> bool
                {
                    QEvent::Type qtType = aQtEvent->type();
                    int          id = wxGetWindowIdCompat( w );

                    auto mouse =
                            [&]( wxEventType aMouseType ) -> bool
                            {
                                wxMouseEvent event( aMouseType );
                                event.SetId( id );
                                event.SetEventObject( w );
                                wxFillMouseEventCompat( event, aQtEvent );
                                return aSink->Process( event );
                            };

                    auto mouseButton = [&]( Qt::MouseButton aButton )
                                       {
                                           QMouseEvent* m = dynamic_cast<QMouseEvent*>( aQtEvent );
                                           return m && m->button() == aButton;
                                       };

                    switch( aType )
                    {
                    case wxEVT_SET_FOCUS:
                    case wxEVT_KILL_FOCUS:
                        if( qtType == ( aType == wxEVT_SET_FOCUS ? QEvent::FocusIn : QEvent::FocusOut ) )
                        {
                            wxFocusEvent event( aType, id );
                            event.SetEventObject( w );
                            aSink->Process( event );
                        }

                        return false;

                    case wxEVT_SIZE:
                        if( qtType == QEvent::Resize )
                        {
                            wxSizeEvent event( wxSizeOfEventCompat( aQtEvent ), id );
                            event.SetEventObject( w );
                            aSink->Process( event );
                        }

                        return false;

                    case wxEVT_SHOW:
                        return false;

                    case wxEVT_LEFT_DOWN:
                        return qtType == QEvent::MouseButtonPress && mouseButton( Qt::LeftButton ) && mouse( aType );

                    case wxEVT_LEFT_UP:
                        return qtType == QEvent::MouseButtonRelease && mouseButton( Qt::LeftButton ) && mouse( aType );

                    case wxEVT_LEFT_DCLICK:
                        return qtType == QEvent::MouseButtonDblClick && mouseButton( Qt::LeftButton ) && mouse( aType );

                    case wxEVT_RIGHT_DOWN:
                        return qtType == QEvent::MouseButtonPress && mouseButton( Qt::RightButton ) && mouse( aType );

                    case wxEVT_RIGHT_UP:
                        return qtType == QEvent::MouseButtonRelease && mouseButton( Qt::RightButton ) && mouse( aType );

                    case wxEVT_MOTION:
                        return qtType == QEvent::MouseMove && mouse( aType );

                    case wxEVT_MOUSEWHEEL:
                        return qtType == QEvent::Wheel && mouse( aType );

                    case wxEVT_LEAVE_WINDOW:
                        return qtType == QEvent::Leave && mouse( aType );

                    case wxEVT_ENTER_WINDOW:
                        return qtType == QEvent::Enter && mouse( aType );

                    case wxEVT_KEY_UP:
                        if( qtType == QEvent::KeyRelease )
                        {
                            wxKeyEvent event( aType );
                            event.SetId( id );
                            event.SetEventObject( w );
                            wxFillKeyEventCompat( event, aQtEvent );
                            return aSink->Process( event );
                        }

                        return false;

                    case wxEVT_CHAR_HOOK:
                    case wxEVT_CHAR:
                    case wxEVT_KEY_DOWN:
                        if( qtType == QEvent::KeyPress || qtType == QEvent::ShortcutOverride )
                        {
                            // The key is not processed by the widget if a handler processes it
                            if( qtType == QEvent::ShortcutOverride )
                                return false;

                            wxKeyEvent event( aType );
                            event.SetId( id );
                            event.SetEventObject( w );
                            wxFillKeyEventCompat( event, aQtEvent );
                            return aSink->Process( event );
                        }

                        return false;

                    case wxEVT_CLOSE_WINDOW:
                        if( qtType == QEvent::Close )
                        {
                            wxCloseEvent event( wxEVT_CLOSE_WINDOW, id );
                            event.SetEventObject( w );

                            // The window stays open if a handler processes the event
                            if( aSink->Process( event ) )
                            {
                                aQtEvent->ignore();
                                return true;
                            }
                        }

                        return false;

                    default:
                        return false;
                    }
                } );
        break;
    }
}


// Controls ---------------------------------------------------------------------------------

wxPanel::wxPanel( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                  long aStyle, const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent.get() )
{
    SetWindowStyleFlag( aStyle );

    SetId( aId );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 || aSize.y > 0 )
        SetMinSize( aSize );
}


wxStaticText::wxStaticText( wxWidgetArgCompat aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
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


wxStaticLine::wxStaticLine( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                            long aStyle ) :
        wxWindowCompat<QFrame>( aParent )
{
    SetId( aId );
    setFrameShape( ( aStyle & wxLI_VERTICAL ) ? QFrame::VLine : QFrame::HLine );
    setFrameShadow( QFrame::Sunken );
}


wxStaticBitmap::wxStaticBitmap( wxWidgetArgCompat aParent, int aId ) :
        wxWindowCompat<QLabel>( aParent )
{
    SetId( aId );
}


wxStaticBitmap::wxStaticBitmap( wxWidgetArgCompat aParent, int aId, const wxBitmapBundle& aBitmap,
                                const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                const wxString& aName ) :
        wxWindowCompat<QLabel>( aParent )
{
    SetId( aId );
    SetBitmap( aBitmap );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 && aSize.y > 0 )
        setFixedSize( aSize.x, aSize.y );
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


wxTextCtrl::wxTextCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aValue, const wxPoint& aPos,
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


wxTextCtrlMultiLine::wxTextCtrlMultiLine( wxWidgetArgCompat aParent ) :
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


void wxTextCtrlMultiLine::Replace( long aFrom, long aTo, const wxString& aText )
{
    SetSelection( aFrom, aTo );

    QTextCursor cursor = textCursor();
    cursor.insertText( ToQString( aText ) );
    setTextCursor( cursor );
}


void wxTextCtrlMultiLine::SetSelection( long aFrom, long aTo )
{
    long        length = GetLastPosition();
    QTextCursor cursor = textCursor();

    if( aFrom == -1 && aTo == -1 )
    {
        aFrom = 0;
        aTo = length;
    }

    long to = aTo < 0 ? length : std::min( aTo, length );
    long from = std::clamp( aFrom, 0L, to );

    cursor.setPosition( int( from ) );
    cursor.setPosition( int( to ), QTextCursor::KeepAnchor );
    setTextCursor( cursor );
}


void wxTextCtrlMultiLine::GetSelection( long* aFrom, long* aTo ) const
{
    QTextCursor cursor = textCursor();

    if( aFrom )
        *aFrom = cursor.selectionStart();

    if( aTo )
        *aTo = cursor.selectionEnd();
}


void wxTextCtrlMultiLine::SetInsertionPoint( long aPos )
{
    QTextCursor cursor = textCursor();
    cursor.setPosition( int( std::clamp( aPos, 0L, GetLastPosition() ) ) );
    setTextCursor( cursor );
}


wxString wxTextCtrlMultiLine::GetLineText( long aLine ) const
{
    return ToWxString( document()->findBlockByNumber( int( aLine ) ).text() );
}


wxButton::wxButton( wxWidgetArgCompat aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
                    const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QPushButton>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );
    setAutoDefault( false );
}


wxToggleButton::wxToggleButton( wxWidgetArgCompat aParent, int aId, const wxString& aLabel ) :
        wxButton( aParent, aId, aLabel )
{
    setCheckable( true );
}


void wxToggleButton::SetValue( bool aValue )
{
    QSignalBlocker blocker( this );
    setChecked( aValue );
}


wxBitmapButton::wxBitmapButton( wxWidgetArgCompat aParent, int aId, const QIcon& aBitmap ) :
        wxButton( aParent, aId )
{
    if( !aBitmap.isNull() )
        setIcon( aBitmap );
}


wxCheckBox::wxCheckBox( wxWidgetArgCompat aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
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


wxRadioButton::wxRadioButton( wxWidgetArgCompat aParent, int aId, const wxString& aLabel,
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


wxChoice::wxChoice( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
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


wxComboBox::wxComboBox( wxWidgetArgCompat aParent, int aId, const wxString& aValue, const wxPoint& aPos,
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


void wxComboBox::Clear()
{
    wxChoice::Clear();

    if( lineEdit() )
        lineEdit()->clear();
}


void wxComboBox::showPopup()
{
    wxCommandEvent event( wxEVT_COMBOBOX_DROPDOWN, GetId() );
    event.SetEventObject( this );
    ProcessEvent( event );

    QComboBox::showPopup();
}


void wxComboBox::hidePopup()
{
    QComboBox::hidePopup();

    wxCommandEvent event( wxEVT_COMBOBOX_CLOSEUP, GetId() );
    event.SetEventObject( this );
    ProcessEvent( event );
}


wxBitmapComboBox::wxBitmapComboBox( wxWidgetArgCompat aParent, int aId, const wxString& aValue,
                                    const wxPoint& aPos, const wxSize& aSize,
                                    const wxArrayString& aChoices, long aStyle ) :
        wxComboBox( aParent, aId, aValue, aPos, aSize, aChoices, aStyle )
{
}


wxBitmapComboBox::wxBitmapComboBox( wxWidgetArgCompat aParent, int aId, const wxString& aValue,
                                    const wxPoint& aPos, const wxSize& aSize, int aCount,
                                    const wxString aChoices[], long aStyle ) :
        wxComboBox( aParent, aId, aValue, aPos, aSize,
                    aChoices ? wxArrayString( size_t( std::max( aCount, 0 ) ), aChoices )
                             : wxArrayString(),
                    aStyle )
{
}


int wxBitmapComboBox::Append( const wxString& aItem, const wxBitmapBundle& aBitmap )
{
    return Insert( aItem, aBitmap, GetCount() );
}


int wxBitmapComboBox::Append( const wxString& aItem, const wxBitmapBundle& aBitmap,
                              void* aClientData )
{
    int index = Append( aItem, aBitmap );
    SetClientData( index, aClientData );
    return index;
}


int wxBitmapComboBox::Insert( const wxString& aItem, const wxBitmapBundle& aBitmap,
                              unsigned int aPos )
{
    int index = wxComboBox::Insert( aItem, aPos );
    SetItemBitmap( index, aBitmap );
    return index;
}


void wxBitmapComboBox::SetItemBitmap( unsigned int aIndex, const wxBitmapBundle& aBitmap )
{
    setItemIcon( int( aIndex ), aBitmap );
}


wxBitmap wxBitmapComboBox::GetItemBitmap( unsigned int aIndex ) const
{
    return wxBitmap( itemIcon( int( aIndex ) ).pixmap( iconSize() ) );
}


wxListBox::wxListBox( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
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


wxCheckListBox::wxCheckListBox( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
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


wxSpinCtrl::wxSpinCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aValue, const wxPoint& aPos,
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


wxSpinCtrlDouble::wxSpinCtrlDouble( wxWidgetArgCompat aParent, int aId ) :
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


wxSlider::wxSlider( wxWidgetArgCompat aParent, int aId, int aValue, int aMin, int aMax ) :
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


wxGauge::wxGauge( wxWidgetArgCompat aParent, int aId, int aRange ) :
        wxWindowCompat<QProgressBar>( aParent )
{
    SetId( aId );
    setRange( 0, aRange );
    setValue( 0 );
}


wxStaticBox::wxStaticBox( wxWidgetArgCompat aParent, int aId, const wxString& aLabel ) :
        wxWindowCompat<QGroupBox>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );
}


wxStaticBox::wxStaticBox( wxWidgetArgCompat aParent, int aId, const QString& aLabel ) :
        wxWindowCompat<QGroupBox>( aParent )
{
    SetId( aId );
    setTitle( aLabel );
}


wxRadioBox::wxRadioBox( wxWidgetArgCompat aParent, int aId, const wxString& aLabel, const wxPoint& aPos,
                        const wxSize& aSize, const wxArrayString& aChoices, int aMajorDimension,
                        long aStyle ) :
        wxWindowCompat<QGroupBox>( aParent )
{
    SetId( aId );
    SetLabel( aLabel );

    // wxRA_SPECIFY_COLS: aMajorDimension columns, wxRA_SPECIFY_ROWS: aMajorDimension rows
    int  major = std::max( aMajorDimension, 1 );
    bool byRows = aStyle & wxRA_SPECIFY_ROWS;

    QGridLayout* layout = new QGridLayout( this );

    for( size_t i = 0; i < aChoices.size(); ++i )
    {
        QRadioButton* button = new QRadioButton( ToQString( aChoices[i] ), this );
        int           n = int( i );

        if( byRows )
            layout->addWidget( button, n % major, n / major );
        else
            layout->addWidget( button, n / major, n % major );

        m_buttons.push_back( button );
    }

    if( !m_buttons.empty() )
        m_buttons.front()->setChecked( true );
}


int wxRadioBox::GetSelection() const
{
    for( size_t i = 0; i < m_buttons.size(); ++i )
    {
        if( m_buttons[i]->isChecked() )
            return int( i );
    }

    return wxNOT_FOUND;
}


void wxRadioBox::SetSelection( int aIndex )
{
    if( aIndex < 0 || aIndex >= int( m_buttons.size() ) )
        return;

    QSignalBlocker blocker( m_buttons[aIndex] );
    m_buttons[aIndex]->setChecked( true );
}


wxString wxRadioBox::GetString( unsigned int aIndex ) const
{
    return aIndex < m_buttons.size() ? ToWxString( m_buttons[aIndex]->text() ) : wxString();
}


void wxRadioBox::SetString( unsigned int aIndex, const wxString& aLabel )
{
    if( aIndex < m_buttons.size() )
        m_buttons[aIndex]->setText( ToQString( aLabel ) );
}


wxString wxRadioBox::GetStringSelection() const
{
    int sel = GetSelection();
    return sel >= 0 ? GetString( sel ) : wxString();
}


bool wxRadioBox::SetStringSelection( const wxString& aString )
{
    int index = FindString( aString );

    if( index == wxNOT_FOUND )
        return false;

    SetSelection( index );
    return true;
}


int wxRadioBox::FindString( const wxString& aString, bool aCaseSensitive ) const
{
    for( size_t i = 0; i < m_buttons.size(); ++i )
    {
        if( GetString( i ).IsSameAs( aString, aCaseSensitive ) )
            return int( i );
    }

    return wxNOT_FOUND;
}


bool wxRadioBox::Enable( unsigned int aIndex, bool aEnable )
{
    if( aIndex >= m_buttons.size() )
        return false;

    m_buttons[aIndex]->setEnabled( aEnable );
    return true;
}


bool wxRadioBox::Show( unsigned int aIndex, bool aShow )
{
    if( aIndex >= m_buttons.size() )
        return false;

    m_buttons[aIndex]->setVisible( aShow );
    return true;
}


bool wxRadioBox::IsItemEnabled( unsigned int aIndex ) const
{
    return aIndex < m_buttons.size() && m_buttons[aIndex]->isEnabled();
}


bool wxRadioBox::IsItemShown( unsigned int aIndex ) const
{
    return aIndex < m_buttons.size() && !m_buttons[aIndex]->isHidden();
}


void wxRadioBox::SetItemToolTip( unsigned int aIndex, const wxString& aTip )
{
    if( aIndex < m_buttons.size() )
        m_buttons[aIndex]->setToolTip( ToQString( aTip ) );
}


void wxRadioBox::OnSelected( std::function<void( int aIndex )> aHandler )
{
    for( size_t i = 0; i < m_buttons.size(); ++i )
    {
        QObject::connect( m_buttons[i], &QAbstractButton::clicked, this,
                          [aHandler, i]() { aHandler( int( i ) ); } );
    }
}


wxTreebook::wxTreebook( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                        long aStyle, const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent ),
        m_selection( wxNOT_FOUND ),
        m_changing( false )
{
    SetId( aId );

    m_tree = new QTreeWidget( this );
    m_tree->setHeaderHidden( true );
    m_tree->setColumnCount( 1 );
    m_tree->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

    m_stack = new QStackedWidget( this );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_tree, 0 );
    layout->addWidget( m_stack, 1 );

    QObject::connect( m_tree, &QTreeWidget::currentItemChanged, this,
                      [this]( QTreeWidgetItem* aItem, QTreeWidgetItem* ) { onItemSelected( aItem ); } );
}


bool wxTreebook::insertPage( wxWidgetArgCompat aPage, const wxString& aText, int aParent, bool aSelect )
{
    if( !aPage )
        return false;

    QTreeWidgetItem* item = nullptr;

    {
        QSignalBlocker blocker( m_tree );

        if( aParent >= 0 )
            item = new QTreeWidgetItem( m_pages[aParent].m_item, { ToQString( aText ) } );
        else
            item = new QTreeWidgetItem( m_tree, { ToQString( aText ) } );

        // The pages are expanded, as with wxWidgets
        if( aParent >= 0 )
            m_pages[aParent].m_item->setExpanded( true );
    }

    m_stack->addWidget( aPage );
    m_pages.push_back( { aPage, item, aParent } );

    if( aSelect || m_selection == wxNOT_FOUND )
        ChangeSelection( m_pages.size() - 1 );

    m_tree->resizeColumnToContents( 0 );
    return true;
}


bool wxTreebook::AddPage( wxWidgetArgCompat aPage, const wxString& aText, bool aSelect, int aImage )
{
    return insertPage( aPage, aText, -1, aSelect );
}


bool wxTreebook::AddSubPage( wxWidgetArgCompat aPage, const wxString& aText, bool aSelect, int aImage )
{
    // The last top level page
    int parent = wxNOT_FOUND;

    for( int i = int( m_pages.size() ) - 1; i >= 0; --i )
    {
        if( m_pages[i].m_parent < 0 )
        {
            parent = i;
            break;
        }
    }

    return insertPage( aPage, aText, parent, aSelect );
}


wxWindow* wxTreebook::GetPage( size_t aPage ) const
{
    return aPage < m_pages.size() ? wxWindowFromQt( m_pages[aPage].m_window ) : nullptr;
}


wxWindow* wxTreebook::GetCurrentPage() const
{
    return m_selection >= 0 ? GetPage( m_selection ) : nullptr;
}


int wxTreebook::ChangeSelection( size_t aPage )
{
    int old = m_selection;

    if( aPage >= m_pages.size() )
        return old;

    m_selection = int( aPage );
    m_stack->setCurrentWidget( m_pages[aPage].m_window );

    QSignalBlocker blocker( m_tree );
    m_tree->setCurrentItem( m_pages[aPage].m_item );
    return old;
}


int wxTreebook::SetSelection( size_t aPage )
{
    int old = m_selection;

    if( aPage >= m_pages.size() || int( aPage ) == m_selection )
        return old;

    wxBookCtrlEvent changing( wxEVT_TREEBOOK_PAGE_CHANGING, GetId(), int( aPage ), old );
    changing.SetEventObject( this );
    ProcessEvent( changing );

    if( !changing.IsAllowed() )
    {
        // The current page stays selected in the tree
        QSignalBlocker blocker( m_tree );

        if( old >= 0 )
            m_tree->setCurrentItem( m_pages[old].m_item );

        return old;
    }

    ChangeSelection( aPage );

    wxBookCtrlEvent changed( wxEVT_TREEBOOK_PAGE_CHANGED, GetId(), int( aPage ), old );
    changed.SetEventObject( this );
    ProcessEvent( changed );

    return old;
}


void wxTreebook::onItemSelected( QTreeWidgetItem* aItem )
{
    int page = pageOf( aItem );

    if( page >= 0 )
        SetSelection( page );
}


int wxTreebook::pageOf( QTreeWidgetItem* aItem ) const
{
    for( size_t i = 0; i < m_pages.size(); ++i )
    {
        if( m_pages[i].m_item == aItem )
            return int( i );
    }

    return wxNOT_FOUND;
}


wxString wxTreebook::GetPageText( size_t aPage ) const
{
    return aPage < m_pages.size() ? ToWxString( m_pages[aPage].m_item->text( 0 ) ) : wxString();
}


bool wxTreebook::SetPageText( size_t aPage, const wxString& aText )
{
    if( aPage >= m_pages.size() )
        return false;

    m_pages[aPage].m_item->setText( 0, ToQString( aText ) );
    return true;
}


int wxTreebook::GetPageParent( size_t aPage ) const
{
    return aPage < m_pages.size() ? m_pages[aPage].m_parent : wxNOT_FOUND;
}


bool wxTreebook::IsNodeExpanded( size_t aPage ) const
{
    return aPage < m_pages.size() && m_pages[aPage].m_item->isExpanded();
}


bool wxTreebook::ExpandNode( size_t aPage, bool aExpand )
{
    if( aPage >= m_pages.size() )
        return false;

    m_pages[aPage].m_item->setExpanded( aExpand );
    return true;
}


bool wxTreebook::DeletePage( size_t aPage )
{
    if( aPage >= m_pages.size() )
        return false;

    // The page and its sub pages (they follow it)
    size_t last = aPage + 1;

    while( last < m_pages.size() && m_pages[last].m_parent >= int( aPage ) )
        last++;

    QWidget* current = m_selection >= 0 && size_t( m_selection ) < m_pages.size()
                               ? m_pages[m_selection].m_window
                               : nullptr;

    {
        QSignalBlocker blocker( m_tree );

        for( size_t i = aPage; i < last; ++i )
        {
            m_stack->removeWidget( m_pages[i].m_window );
            m_pages[i].m_window->deleteLater();
        }

        // Deleting the item of the page deletes the items of its sub pages
        delete m_pages[aPage].m_item;
    }

    size_t count = last - aPage;
    m_pages.erase( m_pages.begin() + aPage, m_pages.begin() + last );

    for( PAGE& page : m_pages )
    {
        if( page.m_parent >= int( last ) )
            page.m_parent -= int( count );
    }

    // The selection follows its page, or goes to the first page
    m_selection = wxNOT_FOUND;

    for( size_t i = 0; i < m_pages.size(); ++i )
    {
        if( m_pages[i].m_window == current )
            m_selection = int( i );
    }

    if( m_selection == wxNOT_FOUND && !m_pages.empty() )
        ChangeSelection( 0 );

    return true;
}


bool wxTreebook::DeleteAllPages()
{
    {
        QSignalBlocker blocker( m_tree );
        m_tree->clear();
    }

    for( PAGE& page : m_pages )
    {
        m_stack->removeWidget( page.m_window );
        page.m_window->deleteLater();
    }

    m_pages.clear();
    m_selection = wxNOT_FOUND;
    return true;
}


wxNotebook::wxNotebook( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
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


bool wxNotebook::AddPage( wxWidgetArgCompat aPage, const wxString& aText, bool aSelect, int aImage )
{
    return InsertPage( count(), aPage, aText, aSelect, aImage );
}


bool wxNotebook::InsertPage( size_t aIndex, wxWidgetArgCompat aPage, const wxString& aText, bool aSelect,
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


wxStdDialogButtonSizer::wxStdDialogButtonSizer( wxWidgetArgCompat aParent ) :
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


void wxBoxSizer::Add( wxWidgetArgCompat aWindow, int aProportion, int aFlag, int aBorder, void* aUserData )
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


void wxBoxSizer::Insert( size_t aIndex, wxWidgetArgCompat aWindow, int aProportion, int aFlag, int aBorder )
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


wxStaticBoxSizer::wxStaticBoxSizer( int aOrient, wxWidgetArgCompat aParent, const wxString& aLabel ) :
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


void wxFlexGridSizer::Add( wxWidgetArgCompat aWindow, int aProportion, int aFlag, int aBorder )
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
                  wxWidgetArgCompat aParent, int aX, int aY )
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
