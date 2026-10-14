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
 * @file kiwx_validators.cpp
 * The validators of the wxWidgets compatibility layer of the Qt build.
 */

#include <wx/kiwx_gui.h>

#include <QMessageBox>
#include <QValidator>

#include <wx/intl.h>

#include <qt_strings.h>


bool wxWindowRefCompat::IsEnabled() const
{
    return m_window && m_window->isEnabled();
}


bool wxWindowRefCompat::IsShown() const
{
    return m_window && !m_window->isHidden();
}


void wxWindowRefCompat::SetFocus() const
{
    if( m_window )
        m_window->setFocus();
}


wxString wxWindowRefCompat::GetName() const
{
    return m_window ? ToWxString( m_window->objectName() ) : wxString();
}


void wxWindowRefCompat::Refresh() const
{
    if( m_window )
        m_window->update();
}


wxColour wxWindowRefCompat::GetBackgroundColour() const
{
    return m_window ? m_window->palette().color( m_window->backgroundRole() ) : QColor();
}


wxColour wxWindowRefCompat::GetForegroundColour() const
{
    return m_window ? m_window->palette().color( m_window->foregroundRole() ) : QColor();
}


int wxWindowRefCompat::GetId() const
{
    return m_window ? wxGetWindowIdCompat( m_window ) : wxID_ANY;
}


void wxWindowRefCompat::SetToolTip( const wxString& aTip ) const
{
    if( m_window )
        m_window->setToolTip( ToQString( aTip ) );
}


bool wxWindowRefCompat::ProcessEvent( wxEvent& aEvent ) const
{
    if( !m_window )
        return false;

    wxEvtHandler* handler = wxEvtHandlerCompat( m_window );
    return wxEvtHandler::ProcessChainCompat( handler->GetEventHandlerCompat(), aEvent );
}


wxEvtHandler* wxWindowRefCompat::GetEventHandler() const
{
    return m_window ? wxEvtHandlerCompat( m_window )->GetEventHandlerCompat() : nullptr;
}


void wxWindowRefCompat::PushEventHandler( wxEvtHandler* aHandler ) const
{
    if( m_window )
        wxEvtHandlerCompat( m_window )->PushEventHandlerCompat( aHandler );
}


wxEvtHandler* wxWindowRefCompat::PopEventHandler( bool aDeleteHandler ) const
{
    return m_window ? wxEvtHandlerCompat( m_window )->PopEventHandlerCompat( aDeleteHandler )
                    : nullptr;
}


wxWindowRefCompat wxWindowRefCompat::GetParent() const
{
    return wxWindowRefCompat( m_window ? m_window->parentWidget() : nullptr );
}


void wxWindowRefCompat::bindHandler( wxEventType aType, wxCompatEventSink::HANDLER aHandler,
                                     const void* aKey, int aId, int aLastId ) const
{
    if( !m_window )
        return;

    wxCompatEventSink* sink = wxEventSinkCompat( m_window );
    sink->Add( aType, std::move( aHandler ), aKey, aId, aLastId );
    wxConnectQtEventsCompat( m_window, aType, sink );
}


const wxValidator wxDefaultValidator;


// Text validators ---------------------------------------------------------------------------

wxTextValidator::wxTextValidator( long aStyle, wxString* aValue ) :
        m_stringValue( aValue ),
        m_validatorStyle( aStyle )
{
}


wxTextCtrl* wxTextValidator::GetTextEntry()
{
    return dynamic_cast<wxTextCtrl*>( static_cast<QWidget*>( m_validatorWindow ) );
}


static QString windowText( QWidget* aWindow )
{
    if( QLineEdit* edit = qobject_cast<QLineEdit*>( aWindow ) )
        return edit->text();

    if( QComboBox* combo = qobject_cast<QComboBox*>( aWindow ) )
        return combo->currentText();

    return QString();
}


static void setWindowText( QWidget* aWindow, const QString& aText )
{
    QSignalBlocker blocker( aWindow );

    if( QLineEdit* edit = qobject_cast<QLineEdit*>( aWindow ) )
        edit->setText( aText );
    else if( QComboBox* combo = qobject_cast<QComboBox*>( aWindow ); combo && combo->isEditable() )
        combo->setEditText( aText );
}


bool wxTextValidator::IsCharIncluded( wxUniChar aChar ) const
{
    return m_charIncludes.Find( aChar ) != wxNOT_FOUND;
}


bool wxTextValidator::IsCharExcluded( wxUniChar aChar ) const
{
    return m_charExcludes.Find( aChar ) != wxNOT_FOUND;
}


bool wxTextValidator::IsValidCharCompat( wxUniChar aChar ) const
{
    if( HasFlag( wxFILTER_EXCLUDE_CHAR_LIST ) && IsCharExcluded( aChar ) )
        return false;

    if( HasFlag( wxFILTER_INCLUDE_CHAR_LIST ) && !IsCharIncluded( aChar ) )
        return false;

    unsigned int c = aChar.GetValue();

    if( HasFlag( wxFILTER_ASCII ) && c > 127 )
        return false;

    if( HasFlag( wxFILTER_DIGITS ) && !( c >= '0' && c <= '9' ) )
        return false;

    if( HasFlag( wxFILTER_XDIGITS ) && !std::isxdigit( int( c ) ) )
        return false;

    if( HasFlag( wxFILTER_NUMERIC )
            && !( ( c >= '0' && c <= '9' ) || c == '.' || c == ',' || c == '-' || c == '+'
                  || c == 'e' || c == 'E' ) )
    {
        return false;
    }

    if( HasFlag( wxFILTER_ALPHA ) && !QChar( c ).isLetter() )
        return false;

    if( HasFlag( wxFILTER_ALPHANUMERIC ) && !QChar( c ).isLetterOrNumber() )
        return false;

    if( HasFlag( wxFILTER_SPACE ) && c == ' ' )
        return true;

    return true;
}


wxString wxTextValidator::IsValid( const wxString& aValue ) const
{
    if( HasFlag( wxFILTER_EMPTY ) && aValue.IsEmpty() )
        return _( "Required information entry is empty." );

    if( HasFlag( wxFILTER_INCLUDE_LIST ) && m_includes.Index( aValue ) == wxNOT_FOUND )
        return wxString::Format( _( "'%s' is not one of the valid strings" ), aValue );

    if( HasFlag( wxFILTER_EXCLUDE_LIST ) && m_excludes.Index( aValue ) != wxNOT_FOUND )
        return wxString::Format( _( "'%s' is one of the invalid strings" ), aValue );

    for( wxUniChar c : aValue )
    {
        if( !IsValidCharCompat( c ) )
            return wxString::Format( _( "'%s' contains invalid character(s)" ), aValue );
    }

    return wxString();
}


bool wxTextValidator::Validate( QWidget* aParent )
{
    // A disabled window is valid
    if( !m_validatorWindow || !m_validatorWindow->IsEnabled() )
        return true;

    wxString error = IsValid( ToWxString( windowText( m_validatorWindow ) ) );

    if( error.IsEmpty() )
        return true;

    m_validatorWindow->SetFocus();
    QMessageBox::warning( aParent, ToQString( _( "Validation conflict" ) ), ToQString( error ) );
    return false;
}


bool wxTextValidator::TransferToWindow()
{
    if( m_stringValue && m_validatorWindow )
        setWindowText( m_validatorWindow, ToQString( *m_stringValue ) );

    return true;
}


bool wxTextValidator::TransferFromWindow()
{
    if( m_stringValue && m_validatorWindow )
        *m_stringValue = ToWxString( windowText( m_validatorWindow ) );

    return true;
}


// Generic validators ------------------------------------------------------------------------

wxGenericValidator::wxGenericValidator( bool* aValue ) : m_bool( aValue ) {}
wxGenericValidator::wxGenericValidator( wxString* aValue ) : m_string( aValue ) {}
wxGenericValidator::wxGenericValidator( int* aValue ) : m_int( aValue ) {}


bool wxGenericValidator::TransferToWindow()
{
    QWidget* window = m_validatorWindow;

    if( !window )
        return false;

    QSignalBlocker blocker( window );

    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( window ); button && m_bool )
        button->setChecked( *m_bool );
    else if( QComboBox* combo = qobject_cast<QComboBox*>( window ); combo && m_int )
        combo->setCurrentIndex( *m_int );
    else if( QSpinBox* spin = qobject_cast<QSpinBox*>( window ); spin && m_int )
        spin->setValue( *m_int );
    else if( m_string )
        setWindowText( window, ToQString( *m_string ) );

    return true;
}


bool wxGenericValidator::TransferFromWindow()
{
    QWidget* window = m_validatorWindow;

    if( !window )
        return false;

    if( QAbstractButton* button = qobject_cast<QAbstractButton*>( window ); button && m_bool )
        *m_bool = button->isChecked();
    else if( QComboBox* combo = qobject_cast<QComboBox*>( window ); combo && m_int )
        *m_int = combo->currentIndex();
    else if( QSpinBox* spin = qobject_cast<QSpinBox*>( window ); spin && m_int )
        *m_int = spin->value();
    else if( m_string )
        *m_string = ToWxString( windowText( window ) );

    return true;
}


// The validators of the windows --------------------------------------------------------------

namespace
{

static const char* VALIDATOR_PROPERTY = "kiwxValidator";

/// The filter of the characters typed in a text control: the ones the validator accepts.
class CHAR_FILTER_COMPAT : public QValidator
{
public:
    CHAR_FILTER_COMPAT( const wxValidator* aValidator, QObject* aParent ) :
            QValidator( aParent ),
            m_validator( aValidator )
    {
    }

    State validate( QString& aInput, int& aPos ) const override
    {
        for( QChar c : aInput )
        {
            if( !m_validator->IsValidCharCompat( wxUniChar( c.unicode() ) ) )
                return Invalid;
        }

        return Acceptable;
    }

private:
    const wxValidator* m_validator;
};

} // namespace


void wxSetValidatorCompat( QWidget* aWindow, const wxValidator& aValidator )
{
    if( !aWindow )
        return;

    // The previous validator
    delete wxGetValidatorCompat( aWindow );
    aWindow->setProperty( VALIDATOR_PROPERTY, QVariant() );

    wxValidator* validator = static_cast<wxValidator*>( aValidator.Clone() );

    if( !validator )
        return;

    validator->SetWindow( aWindow );

    // The events of the window the validator handles
    for( wxEventType type : validator->GetEventTypesCompat() )
        wxConnectQtEventsCompat( aWindow, type, validator->GetEventSinkCompat() );
    aWindow->setProperty( VALIDATOR_PROPERTY,
                          QVariant::fromValue( reinterpret_cast<quintptr>( validator ) ) );

    // The validator goes with its window
    QObject::connect( aWindow, &QObject::destroyed,
                      [validator]() { delete validator; } );

    // The characters typed
    QLineEdit* edit = qobject_cast<QLineEdit*>( aWindow );

    if( !edit )
    {
        if( QComboBox* combo = qobject_cast<QComboBox*>( aWindow ) )
            edit = combo->lineEdit();
    }

    if( edit )
        edit->setValidator( new CHAR_FILTER_COMPAT( validator, edit ) );
}


wxValidator* wxGetValidatorCompat( const QWidget* aWindow )
{
    if( !aWindow )
        return nullptr;

    QVariant property = aWindow->property( VALIDATOR_PROPERTY );

    if( !property.isValid() )
        return nullptr;

    return reinterpret_cast<wxValidator*>( property.value<quintptr>() );
}
