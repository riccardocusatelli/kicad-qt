/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file unit_binder.cpp
 * UNIT_BINDER of the Qt build: the port of common/widgets/unit_binder.cpp to the widgets of Qt.
 */

#include <widgets/unit_binder.h>

#include <QComboBox>
#include <QEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QTimer>

#include <eda_units.h>
#include <eda_draw_frame.h>
#include <confirm.h>
#include <dialog_shim.h>
#include <core/ki_assert.h>

#include <qt_strings.h>

using namespace EDA_UNIT_UTILS::UI;


/// The line edit of a value control: a QLineEdit, or the one of an editable QComboBox.
static QLineEdit* lineEdit( QWidget* aCtrl )
{
    if( QLineEdit* edit = qobject_cast<QLineEdit*>( aCtrl ) )
        return edit;

    if( QComboBox* combo = qobject_cast<QComboBox*>( aCtrl ) )
        return combo->lineEdit();

    return nullptr;
}


/// Gives the focus and click events of the value control to the binder.
class UNIT_BINDER_EVENT_FILTER : public QObject
{
public:
    UNIT_BINDER_EVENT_FILTER( QObject* aParent, std::function<void()> aOnFocusIn,
                              std::function<void()> aOnFocusOut, std::function<void()> aOnClick ) :
            QObject( aParent ),
            m_onFocusIn( std::move( aOnFocusIn ) ),
            m_onFocusOut( std::move( aOnFocusOut ) ),
            m_onClick( std::move( aOnClick ) )
    {
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        switch( aEvent->type() )
        {
        case QEvent::FocusIn:            m_onFocusIn();  break;
        case QEvent::FocusOut:           m_onFocusOut(); break;
        case QEvent::MouseButtonRelease: m_onClick();    break;
        default:                                         break;
        }

        return false;
    }

private:
    std::function<void()> m_onFocusIn;
    std::function<void()> m_onFocusOut;
    std::function<void()> m_onClick;
};


UNIT_BINDER::UNIT_BINDER( EDA_DRAW_FRAME* aParent, QLabel* aLabel, QWidget* aValueCtrl,
                          QLabel* aUnitLabel, bool allowEval, bool aBindFrameEvents ) :
        UNIT_BINDER( aParent, aParent, aLabel, aValueCtrl, aUnitLabel, allowEval, aBindFrameEvents )
{
}


UNIT_BINDER::UNIT_BINDER( UNITS_PROVIDER* aUnitsProvider, wxWidgetArgCompat aEventSource,
                          QLabel* aLabel, QWidget* aValueCtrl, QLabel* aUnitLabel,
                          bool aAllowEval, bool aBindFocusEvent ) :
        m_bindFocusEvent( aBindFocusEvent ),
        m_label( aLabel ),
        m_valueCtrl( aValueCtrl ),
        m_eventSource( aEventSource.get() ),
        m_unitLabel( aUnitLabel ),
        m_iuScale( &aUnitsProvider->GetIuScale() ),
        m_units( aUnitsProvider->GetUserUnits() ),
        m_negativeZero( false ),
        m_dataType( EDA_DATA_TYPE::DISTANCE ),
        m_precision( 0 ),
        m_eval( aUnitsProvider->GetUserUnits() ),
        m_allowEval( aAllowEval && ( !m_valueCtrl || lineEdit( m_valueCtrl ) ) ),
        m_needsEval( false ),
        m_selStart( 0 ),
        m_selEnd( 0 ),
        m_unitsInValue( false ),
        m_originTransforms( aUnitsProvider->GetOriginTransforms() ),
        m_coordType( ORIGIN_TRANSFORMS::NOT_A_COORD ),
        m_dialogShim( nullptr ),
        m_unitsFrame( nullptr ),
        m_unitsListener( -1 ),
        m_eventFilter( nullptr )
{
    if( m_valueCtrl )
    {
        // Register the UNIT_BINDER for control state save/restore
        QWidget* parent = m_valueCtrl->parentWidget();

        while( parent && !dynamic_cast<DIALOG_SHIM*>( parent ) )
            parent = parent->parentWidget();

        if( parent )
        {
            m_dialogShim = static_cast<DIALOG_SHIM*>( parent );
            m_dialogShim->RegisterUnitBinder( this, m_valueCtrl );
        }
    }

    if( QLineEdit* edit = lineEdit( m_valueCtrl ) )
    {
        // Gives enough room to display a value in inches in textEntry
        // 3 digits + '.' + 10 digits
        int minWidth = edit->fontMetrics().horizontalAdvance( QStringLiteral( "XXX.XXXXXXXXXX" ) );

        if( m_valueCtrl->minimumWidth() < minWidth )
            m_valueCtrl->setMinimumWidth( minWidth );

        // Use ChangeValue() instead of SetValue() so we don't generate events.
        setText( m_negativeZero ? wxT( "-0" ) : wxT( "0" ), false );
    }

    if( m_unitLabel )
        m_unitLabel->setText( ToQString( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) ) );

    if( m_valueCtrl )
    {
        // The filter belongs to the binder (the control may be deleted first)
        m_eventFilter = new UNIT_BINDER_EVENT_FILTER( nullptr,
                                                      [this]() { onSetFocus(); },
                                                      [this]() { onKillFocus(); },
                                                      [this]() { onClick(); } );

        m_valueCtrl->installEventFilter( m_eventFilter );

        if( QLineEdit* edit = lineEdit( m_valueCtrl ); edit && edit != m_valueCtrl )
            edit->installEventFilter( m_eventFilter );

        // The value control may be deleted before the binder: forget it then
        QObject::connect( m_valueCtrl, &QObject::destroyed, m_eventFilter,
                          [this]()
                          {
                              if( m_dialogShim )
                              {
                                  m_dialogShim->UnregisterUnitBinder( this );
                                  m_dialogShim = nullptr;
                              }

                              m_valueCtrl = nullptr;
                          } );

        if( QComboBox* combo = qobject_cast<QComboBox*>( m_valueCtrl ) )
        {
            QObject::connect( combo, &QComboBox::activated, m_eventFilter,
                              [this]()
                              {
                                  onComboBox();
                              } );
        }
    }

    // The values follow the units of the frame
    if( EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_eventSource ) )
    {
        m_unitsFrame = frame;
        m_unitsListener = frame->AddUnitsChangedListener(
                [this]( EDA_BASE_FRAME* aProvider )
                {
                    onUnitsChanged( aProvider );
                } );
    }
}


UNIT_BINDER::~UNIT_BINDER()
{
    if( m_dialogShim )
        m_dialogShim->UnregisterUnitBinder( this );

    // The filter refers to this binder (deleting it removes it from the control)
    delete m_eventFilter;

    if( m_unitsFrame )
        m_unitsFrame->RemoveUnitsChangedListener( m_unitsListener );
}


wxString UNIT_BINDER::getText() const
{
    if( QLineEdit* edit = lineEdit( m_valueCtrl ) )
        return ToWxString( edit->text() );

    if( QLabel* label = qobject_cast<QLabel*>( m_valueCtrl ) )
        return ToWxString( label->text() );

    return wxEmptyString;
}


void UNIT_BINDER::setText( const wxString& aText, bool aNotify )
{
    if( QLineEdit* edit = lineEdit( m_valueCtrl ) )
    {
        // wxTextEntry::ChangeValue() doesn't send the change events
        bool blocked = !aNotify && edit->blockSignals( true );
        edit->setText( ToQString( aText ) );

        if( !aNotify )
            edit->blockSignals( blocked );
    }
    else if( QLabel* label = qobject_cast<QLabel*>( m_valueCtrl ) )
    {
        label->setText( ToQString( aText ) );
    }
}


void UNIT_BINDER::SetUnits( EDA_UNITS aUnits )
{
    m_units = aUnits;

    m_eval.SetDefaultUnits( m_units );
    m_eval.LocaleChanged();  // In case locale changed since last run

    if( m_unitLabel )
        m_unitLabel->setText( ToQString( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) ) );
}


void UNIT_BINDER::SetPrecision( int aLength )
{
    m_precision = std::min( aLength, 6 );
}


void UNIT_BINDER::SetDataType( EDA_DATA_TYPE aDataType )
{
    m_dataType = aDataType;

    if( m_unitLabel )
        m_unitLabel->setText( ToQString( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) ) );
}


void UNIT_BINDER::onUnitsChanged( EDA_BASE_FRAME* aProvider )
{
    if( !UnitsInvariant() )
    {
        int temp = GetIntValue();

        QComboBox* const           combo = qobject_cast<QComboBox*>( m_valueCtrl );
        std::vector<long long int> comboValues;

        // Read out the current values
        if( combo )
        {
            for( int i = 0; i < combo->count(); i++ )
            {
                const wxString value = ToWxString( combo->itemText( i ) );
                long long int  conv = ValueFromString( *m_iuScale, m_units, value, m_dataType );
                comboValues.push_back( conv );
            }
        }

        SetUnits( aProvider->GetUserUnits() );
        m_iuScale = &aProvider->GetIuScale();

        // Re-populate the combo box with updated values
        if( combo )
            SetOptionsList( comboValues );

        if( !IsIndeterminate() )
            SetValue( temp );
    }
}


void UNIT_BINDER::onClick()
{
    QLineEdit* edit = lineEdit( m_valueCtrl );

    if( edit && ( getText() == INDETERMINATE_ACTION || getText() == INDETERMINATE_STATE ) )
    {
        // These are tokens, not strings, so do a select all
        edit->selectAll();
    }
}


void UNIT_BINDER::onComboBox()
{
    QComboBox* combo = qobject_cast<QComboBox*>( m_valueCtrl );
    KI_CHECK( combo, /*void*/ );

    const wxString      value = ToWxString( combo->currentText() );
    const long long int conv = ValueFromString( *m_iuScale, m_units, value, m_dataType );

    QTimer::singleShot( 0, m_eventFilter,
                        [this, conv]
                        {
                            SetValue( conv );
                        } );
}


void UNIT_BINDER::onSetFocus()
{
    QLineEdit* edit = lineEdit( m_valueCtrl );

    if( edit )
    {
        if( m_allowEval )
        {
            wxString oldStr = m_eval.OriginalText();

            if( oldStr.length() && oldStr != getText() )
            {
                setText( oldStr, false );
                edit->setSelection( m_selStart, m_selEnd - m_selStart );
            }

            m_needsEval = true;
        }

        if( getText() == INDETERMINATE_ACTION || getText() == INDETERMINATE_STATE )
        {
            // These are tokens, not strings, so do a select all
            edit->selectAll();
        }
    }
}


void UNIT_BINDER::onKillFocus()
{
    QLineEdit* edit = lineEdit( m_valueCtrl );

    if( m_allowEval && edit )
    {
        wxString value = getText();
        bool     success = m_eval.Process( value );

        if( success && !value.IsEmpty() )
        {
            m_selStart = edit->selectionStart();
            m_selEnd = m_selStart >= 0 ? m_selStart + edit->selectedText().length() : 0;

            if( m_selStart < 0 )
                m_selStart = 0;

            value = m_eval.Result();

            if( m_unitsInValue && !value.IsEmpty() )
            {
                if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
                    value += wxT( " " );

                value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
            }

            setText( value, false );
        }

        m_needsEval = false;
    }
}


static wxString valueDescriptionFromLabel( QLabel* aLabel )
{
    wxString desc = aLabel ? ToWxString( aLabel->text() ) : wxString();

    desc.EndsWith( wxT( ":" ), &desc );
    return desc;
}


void UNIT_BINDER::delayedFocusHandler()
{
    if( !m_valueCtrl )
        return;

    if( !m_errorMessage.IsEmpty() )
        DisplayErrorMessage( m_valueCtrl->parentWidget(), m_errorMessage );

    m_errorMessage = wxEmptyString;
    m_valueCtrl->setFocus();
}


bool UNIT_BINDER::Validate( double aMin, double aMax, EDA_UNITS aUnits )
{
    QLineEdit* edit = lineEdit( m_valueCtrl );

    if( !edit || getText() == INDETERMINATE_ACTION || getText() == INDETERMINATE_STATE )
        return true;

    // TODO: Validate() does not currently support m_dataType being anything other than DISTANCE
    // Note: aMin and aMax are not always given in internal units
    if( GetValue() < FromUserUnit( *m_iuScale, aUnits, aMin ) )
    {
        double val_min_iu = FromUserUnit( *m_iuScale, aUnits, aMin );
        m_errorMessage = wxString::Format( _( "%s must be at least %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( *m_iuScale, m_units, val_min_iu, true ) );

        edit->selectAll();

        // Don't focus directly; we might be inside a focus out handler
        if( m_bindFocusEvent )
            QTimer::singleShot( 0, m_eventFilter, [this]() { delayedFocusHandler(); } );

        return false;
    }

    if( GetValue() > FromUserUnit( *m_iuScale, aUnits, aMax ) )
    {
        double val_max_iu = FromUserUnit( *m_iuScale, aUnits, aMax );
        m_errorMessage = wxString::Format( _( "%s must be less than %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( *m_iuScale, m_units, val_max_iu, true ) );

        edit->selectAll();

        // Don't focus directly; we might be inside a focus out handler
        if( m_bindFocusEvent )
            QTimer::singleShot( 0, m_eventFilter, [this]() { delayedFocusHandler(); } );

        return false;
    }

    return true;
}


void UNIT_BINDER::SetValue( long long int aValue )
{
    double   displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString textValue = StringFromValue( *m_iuScale, m_units, displayValue, false, m_dataType );

    if( displayValue == 0 && m_negativeZero )
        SetValue( wxT( "-" ) + textValue );
    else
        SetValue( textValue );
}


void UNIT_BINDER::SetDoubleValue( double aValue )
{
    double   displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString textValue = StringFromValue( *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                          m_dataType );

    if( displayValue == 0 && !std::signbit( displayValue ) && m_negativeZero )
        SetValue( wxT( "-" ) + textValue );
    else
        SetValue( textValue );
}


void UNIT_BINDER::SetAngleValue( const EDA_ANGLE& aValue )
{
    SetDoubleValue( m_originTransforms.ToDisplay( aValue, m_coordType ) );
}


void UNIT_BINDER::SetValue( const wxString& aValue )
{
    wxString value = aValue;

    if( m_unitsInValue && !value.IsEmpty() )
    {
        if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
            value += wxT( " " );

        value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
    }

    setText( value, true );

    if( m_allowEval )
        m_eval.Clear();

    if( m_unitLabel )
        m_unitLabel->setText( ToQString( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) ) );
}


wxString UNIT_BINDER::getTextForValue( long long int aValue ) const
{
    const double displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString     textValue = StringFromValue( *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                              m_dataType );

    if( displayValue == 0 && m_negativeZero )
        textValue = wxT( "-" ) + textValue;

    return textValue;
}


wxString UNIT_BINDER::getTextForDoubleValue( double aValue ) const
{
    const double displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString     textValue = StringFromValue(  *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                               m_dataType );

    if( displayValue == 0 && !std::signbit( displayValue ) && m_negativeZero )
        textValue = wxT( "-" ) + textValue;

    return textValue;
}


void UNIT_BINDER::ChangeValue( int aValue )
{
    ChangeValue( getTextForValue( aValue ) );
}


void UNIT_BINDER::ChangeDoubleValue( double aValue )
{
    ChangeValue( getTextForDoubleValue( aValue ) );
}


void UNIT_BINDER::ChangeAngleValue( const EDA_ANGLE& aValue )
{
    ChangeDoubleValue( m_originTransforms.ToDisplay( aValue, m_coordType ) );
}


void UNIT_BINDER::ChangeValue( const wxString& aValue )
{
    wxString value = aValue;

    if( m_unitsInValue && !value.IsEmpty() )
    {
        if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
            value += wxT( " " );

        value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
    }

    setText( value, false );

    if( m_allowEval )
        m_eval.Clear();

    if( m_unitLabel )
        m_unitLabel->setText( ToQString( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) ) );
}


long long int UNIT_BINDER::GetValue() const
{
    wxString value;

    if( lineEdit( m_valueCtrl ) )
    {
        value = getText();

        if( m_needsEval && !value.IsEmpty() && m_eval.Process( value ) )
            value = m_eval.Result();
        else
            value = getText();
    }
    else if( qobject_cast<QLabel*>( m_valueCtrl ) )
    {
        value = getText();
    }
    else
    {
        return 0;
    }

    long long int displayValue = ValueFromString( *m_iuScale, m_units, value, m_dataType );
    return m_originTransforms.FromDisplay( displayValue, m_coordType );
}


double UNIT_BINDER::setPrecision( double aValue, bool aValueUsesUserUnits ) const
{
    if( m_precision > 1 )
    {
        int scale = pow( 10, m_precision );
        int64_t tmp = aValue;

        if( !aValueUsesUserUnits )
            tmp = ToUserUnit( *m_iuScale, m_units, aValue ) * scale;

        aValue = static_cast<double>( tmp ) / scale;

        if( !aValueUsesUserUnits )
            aValue = FromUserUnit( *m_iuScale, m_units, aValue );
    }

    return aValue;
}


double UNIT_BINDER::GetDoubleValue() const
{
    wxString value;

    if( lineEdit( m_valueCtrl ) )
    {
        value = getText();

        if( m_needsEval && !value.IsEmpty() && m_eval.Process( value ) )
            value = m_eval.Result();
        else
            value = getText();
    }
    else if( qobject_cast<QLabel*>( m_valueCtrl ) )
    {
        value = getText();
    }
    else
    {
        return 0.0;
    }

    double displayValue = DoubleValueFromString( *m_iuScale, m_units, value, m_dataType );
    displayValue = setPrecision( displayValue, false );

    return m_originTransforms.FromDisplay( displayValue, m_coordType );
}


EDA_ANGLE UNIT_BINDER::GetAngleValue()
{
    return m_originTransforms.FromDisplay( EDA_ANGLE( GetDoubleValue(), DEGREES_T ), m_coordType );
}


void UNIT_BINDER::SetOptionsList( std::span<const long long int> aOptions )
{
    QComboBox* cb = qobject_cast<QComboBox*>( m_valueCtrl );
    KI_CHECK( cb, /* void */ );

    // Clearing the list doesn't change the value of the control
    QString text = cb->currentText();
    cb->clear();

    for( long long int value : aOptions )
        cb->addItem( ToQString( getTextForValue( value ) ) );

    cb->setEditText( text );
}


void UNIT_BINDER::SetDoubleOptionsList( std::span<const double> aOptions )
{
    QComboBox* cb = qobject_cast<QComboBox*>( m_valueCtrl );
    KI_CHECK( cb, /* void */ );

    QString text = cb->currentText();
    cb->clear();

    for( double value : aOptions )
        cb->addItem( ToQString( getTextForDoubleValue( value ) ) );

    cb->setEditText( text );
}


bool UNIT_BINDER::IsIndeterminate() const
{
    if( lineEdit( m_valueCtrl ) )
        return getText() == INDETERMINATE_STATE || getText() == INDETERMINATE_ACTION;

    return false;
}


bool UNIT_BINDER::IsNull() const
{
    if( lineEdit( m_valueCtrl ) )
        return getText().IsEmpty();

    return false;
}


void UNIT_BINDER::SetNull()
{
    if( lineEdit( m_valueCtrl ) )
        setText( wxEmptyString, true );
}


void UNIT_BINDER::SetLabel( const wxString& aLabel )
{
    m_label->setText( ToQString( aLabel ) );
}


void UNIT_BINDER::Enable( bool aEnable )
{
    if( m_label )
        m_label->setEnabled( aEnable );

    m_valueCtrl->setEnabled( aEnable );

    if( m_unitLabel )
        m_unitLabel->setEnabled( aEnable );
}


void UNIT_BINDER::Show( bool aShow, bool aResize )
{
    // The layouts of Qt give no room to the hidden widgets: no resize is needed
    m_label->setVisible( aShow );
    m_valueCtrl->setVisible( aShow );

    if( m_unitLabel )
        m_unitLabel->setVisible( aShow );
}


PROPERTY_EDITOR_UNIT_BINDER::PROPERTY_EDITOR_UNIT_BINDER( EDA_DRAW_FRAME* aParent ) :
        UNIT_BINDER( aParent, nullptr, nullptr, nullptr, true, false )
{
    m_unitsInValue = true;
}


PROPERTY_EDITOR_UNIT_BINDER::~PROPERTY_EDITOR_UNIT_BINDER()
{
}


void PROPERTY_EDITOR_UNIT_BINDER::SetControl( QWidget* aControl )
{
    delete m_eventFilter;
    m_eventFilter = nullptr;

    m_valueCtrl = aControl;

    if( m_valueCtrl )
    {
        m_eventFilter = new UNIT_BINDER_EVENT_FILTER( nullptr,
                                                      [this]() { onSetFocus(); },
                                                      [this]() { onKillFocus(); },
                                                      [this]() { onClick(); } );

        m_valueCtrl->installEventFilter( m_eventFilter );

        // The editor of a property is deleted when the edition ends
        QObject::connect( m_valueCtrl, &QObject::destroyed, m_eventFilter,
                          [this]()
                          {
                              m_valueCtrl = nullptr;
                          } );
    }
}
