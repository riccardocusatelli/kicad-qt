// The wxWidgets compatibility layer of the Qt build: <wx/valnum.h>, the integer validator
// (wxIntegerValidator) on the text validator of the layer.
#pragma once

#include <wx/kiwx_gui.h>
#include <wx/kiwx_validators.h>

#include <limits>

#include <wx/intl.h>


/// The styles of the numeric validators (wxNumValidatorStyle).
enum wxNumValidatorStyle
{
    wxNUM_VAL_DEFAULT = 0x0,
    wxNUM_VAL_THOUSANDS_SEPARATOR = 0x1,
    wxNUM_VAL_ZERO_AS_BLANK = 0x2,
    wxNUM_VAL_NO_TRAILING_ZEROES = 0x4
};


/// A validator of an integer between a minimum and a maximum (wxIntegerValidator).
template <typename T>
class wxIntegerValidator : public wxTextValidator
{
public:
    explicit wxIntegerValidator( T* aValue = nullptr, int aStyle = wxNUM_VAL_DEFAULT ) :
            wxTextValidator( wxFILTER_INCLUDE_CHAR_LIST ),
            m_value( aValue )
    {
        SetCharIncludes( std::numeric_limits<T>::is_signed ? wxS( "-0123456789" )
                                                           : wxS( "0123456789" ) );
    }

    wxIntegerValidator( const wxIntegerValidator& aOther ) = default;

    wxObject* Clone() const override { return new wxIntegerValidator( *this ); }

    void SetMin( T aMin ) { m_min = aMin; }
    void SetMax( T aMax ) { m_max = aMax; }
    T GetMin() const { return m_min; }
    T GetMax() const { return m_max; }

    void SetRange( T aMin, T aMax )
    {
        m_min = aMin;
        m_max = aMax;
    }

    wxString IsValid( const wxString& aValue ) const override
    {
        if( aValue.IsEmpty() )
            return wxString();

        long long value = 0;

        if( !aValue.ToLongLong( &value ) )
            return _( "Invalid integer value" );

        if( value < static_cast<long long>( m_min ) || value > static_cast<long long>( m_max ) )
            return wxString::Format( _( "Value must be between %lld and %lld" ),
                                     static_cast<long long>( m_min ),
                                     static_cast<long long>( m_max ) );

        return wxString();
    }

    bool TransferToWindow() override
    {
        if( m_value && GetTextEntry() )
            GetTextEntry()->SetValue( wxString::Format( wxS( "%lld" ),
                                                        static_cast<long long>( *m_value ) ) );

        return true;
    }

    bool TransferFromWindow() override
    {
        long long value = 0;

        if( m_value && GetTextEntry() && GetTextEntry()->GetValue().ToLongLong( &value ) )
            *m_value = static_cast<T>( value );

        return true;
    }

private:
    T* m_value;
    T  m_min = std::numeric_limits<T>::lowest();
    T  m_max = std::numeric_limits<T>::max();
};
