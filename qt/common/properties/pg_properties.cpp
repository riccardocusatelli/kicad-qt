/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.

/**
 * @file pg_properties.cpp
 * The properties of the property grid with Qt, and the functions making them for the
 * toolkit-independent code of the properties panels (properties/pg_factory.h).
 */

#include <properties/pg_properties.h>

#include <eda_draw_frame.h>
#include <properties/color4d_variant.h>
#include <properties/property.h>
#include <properties/std_optional_variants.h>
#include <core/ki_assert.h>

#include <wx/intl.h>


PG_PROPERTY::PG_PROPERTY( EDITOR aEditor ) :
        m_editor( aEditor ),
        m_clientData( nullptr ),
        m_readOnly( false ),
        m_editorFrame( nullptr )
{
}


wxString PG_PROPERTY::ValueToString( const wxVariant& aValue ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    return aValue.MakeString();
}


wxString PG_PROPERTY::GetValueAsString() const
{
    if( IsValueUnspecified() )
        return wxEmptyString;

    return ValueToString( m_value );
}


bool PG_PROPERTY::StringToValue( wxVariant& aVariant, const wxString& aText ) const
{
    aVariant = aText;
    return true;
}


wxString PGPROPERTY_INT::ValueToString( const wxVariant& aValue ) const
{
    if( aValue.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* data = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aValue.GetData() );

        return data->Value().has_value() ? wxString::Format( wxS( "%d" ), data->Value().value() )
                                         : wxString();
    }

    if( aValue.GetType() == wxS( "longlong" ) )
        return wxString::Format( wxS( "%lld" ), aValue.GetLongLong().GetValue() );

    if( aValue.GetType() == wxS( "long" ) )
        return wxString::Format( wxS( "%ld" ), aValue.GetLong() );

    return PG_PROPERTY::ValueToString( aValue );
}


bool PGPROPERTY_INT::StringToValue( wxVariant& aVariant, const wxString& aText ) const
{
    long long value;

    if( !aText.Strip( wxString::both ).ToLongLong( &value ) )
        return false;

    if( value < std::numeric_limits<long>::min() || value > std::numeric_limits<long>::max() )
        return false;

    aVariant = static_cast<long>( value );
    return true;
}


wxString PGPROPERTY_UINT::ValueToString( const wxVariant& aValue ) const
{
    if( aValue.GetType() == wxS( "ulonglong" ) )
        return wxString::Format( wxS( "%llu" ), aValue.GetULongLong().GetValue() );

    if( aValue.GetType() == wxS( "long" ) )
        return wxString::Format( wxS( "%lu" ), static_cast<unsigned long>( aValue.GetLong() ) );

    return PG_PROPERTY::ValueToString( aValue );
}


bool PGPROPERTY_UINT::StringToValue( wxVariant& aVariant, const wxString& aText ) const
{
    unsigned long long value;
    wxString           text = aText.Strip( wxString::both );

    if( text.StartsWith( wxS( "-" ) ) || !text.ToULongLong( &value ) )
        return false;

    if( value > static_cast<unsigned long long>( std::numeric_limits<long>::max() ) )
        return false;

    aVariant = static_cast<long>( value );
    return true;
}


wxString PGPROPERTY_FLOAT::ValueToString( const wxVariant& aValue ) const
{
    double value;

    if( aValue.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* data = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aValue.GetData() );

        if( !data->Value().has_value() )
            return wxEmptyString;

        value = data->Value().value();
    }
    else if( aValue.GetType() == wxS( "double" ) || aValue.GetType() == wxS( "long" ) )
    {
        value = aValue.GetDouble();
    }
    else
    {
        return PG_PROPERTY::ValueToString( aValue );
    }

    // As the float properties of wxPropertyGrid: the decimals, without the trailing zeros
    wxString text = wxString::Format( wxS( "%f" ), value );

    if( text.Contains( wxS( "." ) ) )
    {
        while( text.EndsWith( wxS( "0" ) ) )
            text.RemoveLast();

        if( text.EndsWith( wxS( "." ) ) )
            text.RemoveLast();
    }

    return text;
}


bool PGPROPERTY_FLOAT::StringToValue( wxVariant& aVariant, const wxString& aText ) const
{
    double value;

    if( !aText.Strip( wxString::both ).ToDouble( &value ) )
        return false;

    aVariant = value;
    return true;
}


wxString PGPROPERTY_ENUM::ValueToString( const wxVariant& aValue ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    int index = m_choices.Index( static_cast<int>( aValue.GetLong() ) );

    if( index < 0 )
        return wxEmptyString;

    return m_choices.GetLabel( index );
}


wxString PGPROPERTY_BOOL::ValueToString( const wxVariant& aValue ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    return aValue.GetBool() ? _( "True" ) : _( "False" );
}


KIGFX::COLOR4D PGPROPERTY_COLOR4D::GetColor() const
{
    if( m_value.IsType( wxS( "COLOR4D" ) ) )
        return static_cast<COLOR4D_VARIANT_DATA*>( m_value.GetData() )->Color();

    wxAny av = m_value.GetAny();

    return av.IsNull() ? KIGFX::COLOR4D::UNSPECIFIED : av.As<KIGFX::COLOR4D>();
}


/// Give the property the label, the name and the help of @a aProperty.
static void initProperty( PG_PROPERTY* aRet, const PROPERTY_BASE* aProperty )
{
    aRet->SetLabel( wxGetTranslation( aProperty->Name() ) );
    aRet->SetName( aProperty->Name() );
    aRet->SetHelpString( wxGetTranslation( aProperty->Name() ) );
    aRet->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
}


TOOLKIT_PG_PROPERTY* PGPropertyFactory( const PROPERTY_BASE* aProperty, EDA_DRAW_FRAME* aFrame )
{
    PG_PROPERTY*     ret = nullptr;
    PROPERTY_DISPLAY display = aProperty->Display();

    switch( display )
    {
    case PROPERTY_DISPLAY::PT_TIME:
        ret = new PGPROPERTY_TIME( aFrame );
        break;

    case PROPERTY_DISPLAY::PT_SIZE:
        ret = new PGPROPERTY_SIZE( aFrame );
        break;

    case PROPERTY_DISPLAY::PT_AREA:
        ret = new PGPROPERTY_AREA( aFrame );
        break;

    case PROPERTY_DISPLAY::PT_COORD:
        ret = new PGPROPERTY_COORD( aFrame, aProperty->CoordType() );
        break;

    case PROPERTY_DISPLAY::PT_DECIDEGREE:
    case PROPERTY_DISPLAY::PT_DEGREE:
    {
        PGPROPERTY_ANGLE* prop = new PGPROPERTY_ANGLE();

        if( display == PROPERTY_DISPLAY::PT_DECIDEGREE )
            prop->SetScale( 10.0 );

        ret = prop;
        break;
    }

    case PROPERTY_DISPLAY::PT_RATIO:
        ret = new PGPROPERTY_RATIO();
        break;

    case PROPERTY_DISPLAY::PT_NET:
        ret = new PGPROPERTY_NET( aProperty->Choices() );
        break;

    default:
        KI_FAIL();
        KI_FALLTHROUGH;
        /* fall through */
    case PROPERTY_DISPLAY::PT_DEFAULT:
    {
        size_t typeId = aProperty->TypeHash();

        // Enum property
        if( aProperty->HasChoices() )
        {
            const_cast<PROPERTY_BASE*>( aProperty )->TranslateChoices();

            ret = new PGPROPERTY_ENUM( aProperty->Choices() );
        }
        else if( typeId == TYPE_HASH( int ) || typeId == TYPE_HASH( long ) )
        {
            ret = new PGPROPERTY_INT();
        }
        else if( typeId == TYPE_HASH( unsigned int ) || typeId == TYPE_HASH( unsigned long ) )
        {
            ret = new PGPROPERTY_UINT();
        }
        else if( typeId == TYPE_HASH( float ) || typeId == TYPE_HASH( double ) )
        {
            ret = new PGPROPERTY_FLOAT();
        }
        else if( typeId == TYPE_HASH( bool ) )
        {
            ret = new PGPROPERTY_BOOL();
        }
        else if( typeId == TYPE_HASH( wxString ) )
        {
            ret = new PGPROPERTY_STRING();
        }
        else if( typeId == TYPE_HASH( KIGFX::COLOR4D ) )
        {
            ret = new PGPROPERTY_COLOR4D();
        }
        else
        {
            KI_FAIL_MSG( wxString::Format( wxS( "Property %s not supported by PGPropertyFactory" ),
                                          aProperty->Name() ) );

            // A disabled row
            ret = new PGPROPERTY_STRING();
            ret->SetReadOnly( true );
        }

        break;
    }
    }

    if( ret )
        initProperty( ret, aProperty );

    return ret;
}


TOOLKIT_PG_PROPERTY* PGColorEnumPropertyFactory( const PROPERTY_BASE* aProperty,
                                                 const std::vector<std::pair<wxString, int>>& aChoices,
                                                 std::function<KIGFX::COLOR4D( int aValue )> aColorFunc )
{
    PROPERTY_CHOICES choices;

    for( const auto& [name, value] : aChoices )
        choices.Add( name, value );

    PGPROPERTY_COLORENUM* ret = new PGPROPERTY_COLORENUM( choices, std::move( aColorFunc ) );

    initProperty( ret, aProperty );

    return ret;
}


void PGSetEditor( TOOLKIT_PG_PROPERTY* aProperty, PG_EDITOR aEditor, EDA_DRAW_FRAME* aFrame )
{
    switch( aEditor )
    {
    case PG_EDITOR::FPID: aProperty->SetEditor( PG_PROPERTY::EDITOR::FPID ); break;
    case PG_EDITOR::URL:  aProperty->SetEditor( PG_PROPERTY::EDITOR::URL );  break;
    }

    aProperty->SetEditorFrame( aFrame );
}


bool PGSetColorBackground( TOOLKIT_PG_PROPERTY* aProperty, const KIGFX::COLOR4D& aBackground )
{
    PGPROPERTY_COLOR4D* colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( aProperty );

    if( !colorProp )
        return false;

    colorProp->SetBackgroundColor( aBackground );
    return true;
}


wxString PGGetName( const TOOLKIT_PG_PROPERTY* aProperty )
{
    return aProperty->GetName();
}


void PGSetChoices( TOOLKIT_PG_PROPERTY* aProperty, const PROPERTY_CHOICES& aChoices )
{
    aProperty->SetChoices( aChoices );
}


void PGSetValue( TOOLKIT_PG_PROPERTY* aProperty, const wxVariant& aValue )
{
    aProperty->SetValue( aValue );
}


void PGSetReadOnly( TOOLKIT_PG_PROPERTY* aProperty, bool aReadOnly )
{
    aProperty->SetReadOnly( aReadOnly );
}
