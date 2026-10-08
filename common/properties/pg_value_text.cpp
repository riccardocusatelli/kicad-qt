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
 */

/**
 * @file pg_value_text.cpp
 * The texts of the values of the properties of the property grids and the values given by
 * their editors (see pg_value_text.h), and the conversions of the values of the properties
 * (wxAny) to the values of the grids (wxVariant).
 */

#include <properties/pg_value_text.h>

#include <eda_draw_frame.h>
#include <eda_units.h>
#include <properties/color4d_variant.h>
#include <properties/eda_angle_variant.h>
#include <properties/std_optional_variants.h>
#include <string_utils.h>
#include <widgets/unit_binder.h>
#include <core/ki_assert.h>


// The types of the values of the grids (the type names of wxPropertyGrid)
static const wxString VARIANT_TYPE_LONG = wxS( "long" );
static const wxString VARIANT_TYPE_LONGLONG = wxS( "longlong" );
static const wxString VARIANT_TYPE_ULONGLONG = wxS( "ulonglong" );
static const wxString VARIANT_TYPE_DOUBLE = wxS( "double" );
static const wxString VARIANT_TYPE_STRING = wxS( "string" );


class wxAnyToSTD_OPTIONAL_INT_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToSTD_OPTIONAL_INT_VARIANTRegistrationImpl( wxVariantDataFactory factory )
        : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToSTD_OPTIONAL_INT_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr wxAnyToSTD_OPTIONAL_INT_VARIANTRegistrationImpl::s_instance(
        new wxAnyValueTypeImpl<std::optional<int>>() );


static wxAnyToSTD_OPTIONAL_INT_VARIANTRegistrationImpl
        s_wxAnyToSTD_OPTIONAL_INT_VARIANTRegistration(
                &STD_OPTIONAL_INT_VARIANT_DATA::VariantDataFactory );


class wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistrationImpl( wxVariantDataFactory factory )
        : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistrationImpl::s_instance(
        new wxAnyValueTypeImpl<std::optional<double>>() );


static wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistrationImpl
        s_wxAnyToSTD_OPTIONAL_DOUBLE_VARIANTRegistration(
                &STD_OPTIONAL_DOUBLE_VARIANT_DATA::VariantDataFactory );


class wxAnyToEDA_ANGLE_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToEDA_ANGLE_VARIANTRegistrationImpl( wxVariantDataFactory factory )
        : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr
        wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::s_instance( new wxAnyValueTypeImpl<EDA_ANGLE>() );


static wxAnyToEDA_ANGLE_VARIANTRegistrationImpl
        s_wxAnyToEDA_ANGLE_VARIANTRegistration( &EDA_ANGLE_VARIANT_DATA::VariantDataFactory );


class wxAnyToCOLOR4D_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToCOLOR4D_VARIANTRegistrationImpl( wxVariantDataFactory factory )
            : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToCOLOR4D_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr wxAnyToCOLOR4D_VARIANTRegistrationImpl::s_instance(
        new wxAnyValueTypeImpl<KIGFX::COLOR4D>() );


static wxAnyToCOLOR4D_VARIANTRegistrationImpl
        s_wxAnyToCOLOR4D_VARIANTRegistration( &COLOR4D_VARIANT_DATA::VariantDataFactory );


wxString PGDistanceToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant,
                             ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType )
{
    if( aVariant.GetType() == VARIANT_TYPE_DOUBLE )
    {
        double distanceIU = aVariant.GetDouble();
        ORIGIN_TRANSFORMS& transforms = aFrame->GetOriginTransforms();
        distanceIU = transforms.ToDisplay( distanceIU, aCoordType );
        return aFrame->StringFromValue( distanceIU, true, EDA_DATA_TYPE::DISTANCE );
    }

    long distanceIU;

    if( aVariant.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aVariant.GetData() );

        if( !variantData->Value().has_value() )
            return wxEmptyString;

        distanceIU = variantData->Value().value();
    }
    else if( aVariant.GetType() == VARIANT_TYPE_LONG )
    {
        distanceIU = aVariant.GetLong();
    }
    else if( aVariant.GetType() == VARIANT_TYPE_ULONGLONG )
    {
        distanceIU = static_cast<long>( aVariant.GetULongLong().GetValue() );
    }
    else
    {
        KI_FAIL_MSG( wxT( "Expected int (or std::optional<int>) value type" ) );
        return wxEmptyString;
    }

    ORIGIN_TRANSFORMS& transforms = aFrame->GetOriginTransforms();

    distanceIU = transforms.ToDisplay( static_cast<long long int>( distanceIU ), aCoordType );

    return aFrame->StringFromValue( distanceIU, true, EDA_DATA_TYPE::DISTANCE );
}


wxString PGAreaToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant )
{
    wxLongLong areaIU;

    if( aVariant.GetType() == VARIANT_TYPE_LONGLONG )
    {
        areaIU = aVariant.GetLongLong();
    }
    else if( aVariant.GetType() == VARIANT_TYPE_LONG )
    {
        areaIU = wxLongLong( aVariant.GetLong() );
    }
    else
    {
        KI_FAIL_MSG( wxString::Format( wxS( "Unexpected variant type in PGPROPERTY_AREA: %s" ),
                                      aVariant.GetType() ) );
        return wxEmptyString;
    }

    return aFrame->StringFromValue( areaIU.ToDouble(), true, EDA_DATA_TYPE::AREA );
}


wxString PGTimeToString( EDA_DRAW_FRAME* aFrame, const wxVariant& aVariant )
{
    int value;

    if( aVariant.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aVariant.GetData() );

        if( !variantData->Value().has_value() )
            return wxEmptyString;

        value = variantData->Value().value();
    }
    else if( aVariant.GetType() == VARIANT_TYPE_LONG )
    {
        value = static_cast<int>( aVariant.GetInteger() );
    }
    else
    {
        KI_FAIL_MSG( wxT( "Expected int (or std::optional<int>) value type" ) );
        return wxEmptyString;
    }

    return aFrame->StringFromValue( value, true, EDA_DATA_TYPE::TIME );
}


wxString PGRatioToString( const wxVariant& aVariant )
{
    double value;

    if( aVariant.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aVariant.GetData() );

        if( !variantData->Value().has_value() )
            return wxEmptyString;

        value = variantData->Value().value();
    }
    else if( aVariant.GetType() == VARIANT_TYPE_DOUBLE )
    {
        value = aVariant.GetDouble();
    }
    else
    {
        KI_FAIL_MSG( wxT( "Expected double (or std::optional<double>) value type" ) );
        return wxEmptyString;
    }

    return wxString::Format( wxS( "%g" ), value );
}


wxString PGAngleToString( const wxVariant& aVariant, double aScale )
{
    if( aVariant.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aVariant.GetData() );

        if( variantData->Value().has_value() )
            return wxString::Format( wxS( "%g°" ), variantData->Value().value() / aScale );
        else
            return wxEmptyString;
    }
    else if( aVariant.GetType() == VARIANT_TYPE_DOUBLE )
    {
        return wxString::Format( wxS( "%g°" ), aVariant.GetDouble() / aScale );
    }
    else if( aVariant.GetType() == wxS( "EDA_ANGLE" ) )
    {
        wxString ret;
        static_cast<EDA_ANGLE_VARIANT_DATA*>( aVariant.GetData() )->Write( ret );
        return ret;
    }
    else if( aVariant.GetType() == VARIANT_TYPE_LONG )
    {
        return wxString::Format( wxS( "%g°" ), (double) aVariant.GetLong() / aScale );
    }
    else
    {
        KI_CHECK_MSG( false, wxEmptyString, wxS( "Unexpected variant type in PGPROPERTY_ANGLE" ) );
    }
}


bool PGAngleFromString( wxVariant& aVariant, const wxString& aText, double aScale )
{
    double value = 0.0;

    if( !aText.ToDouble( &value ) )
    {
        aVariant.MakeNull();
        return true;
    }

    value *= aScale;

    if( aVariant.IsNull() || aVariant.GetDouble() != value )
    {
        aVariant = value;
        return true;
    }

    return false;
}


wxString PGStringToString( const wxVariant& aVariant )
{
    if( aVariant.GetType() != VARIANT_TYPE_STRING )
        return wxEmptyString;

    return UnescapeString( aVariant.GetString() );
}


wxVariant PGStringFromString( const wxString& aText )
{
    return wxVariant( EscapeString( aText, CTX_QUOTED_STR ) );
}


wxString PGColorToString( const wxVariant& aVariant )
{
    wxString ret;

    if( aVariant.IsType( wxS( "COLOR4D" ) ) )
        static_cast<COLOR4D_VARIANT_DATA*>( aVariant.GetData() )->Write( ret );
    else
        ret = aVariant.IsNull() ? wxString() : aVariant.GetString();

    return ret;
}


wxVariant PGColorFromString( const wxString& aText )
{
    wxVariant variant;
    variant.SetData( new COLOR4D_VARIANT_DATA( aText ) );
    return variant;
}


void PGUnitBinderShowValue( UNIT_BINDER& aBinder, const wxVariant& aValue, PG_UNIT_KIND aKind,
                            double aAngleScale, bool aUnspecified )
{
    wxVariant var = aValue;

    if( aKind == PG_UNIT_KIND::ANGLE )
    {
        if( var.GetType() == wxT( "EDA_ANGLE" ) )
        {
            EDA_ANGLE_VARIANT_DATA* angleData =
                    static_cast<EDA_ANGLE_VARIANT_DATA*>( var.GetData() );
            aBinder.ChangeAngleValue( angleData->Angle() );
        }
        else if( !aUnspecified )
        {
            aBinder.ChangeDoubleValue( var.GetDouble() / aAngleScale );
        }
    }
    else if( var.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( var.GetData() );

        if( variantData->Value().has_value() )
            aBinder.ChangeValue( variantData->Value().value() );
        else
            aBinder.ChangeValue( wxEmptyString );
    }
    else if( var.GetType() == VARIANT_TYPE_LONG )
    {
        aBinder.ChangeValue( var.GetLong() );
    }
    else if( var.GetType() == VARIANT_TYPE_LONGLONG )
    {
        aBinder.ChangeDoubleValue( var.GetLongLong().ToDouble() );
    }
    else if( var.GetType() == VARIANT_TYPE_DOUBLE )
    {
        aBinder.ChangeValue( var.GetDouble() );
    }
    else if( var.GetType() == wxT( "EDA_ANGLE" ) )
    {
        EDA_ANGLE_VARIANT_DATA* angleData = static_cast<EDA_ANGLE_VARIANT_DATA*>( var.GetData() );
        aBinder.ChangeAngleValue( angleData->Angle() );
    }
    else if( !aUnspecified )
    {
        KI_FAIL_MSG( wxT( "PG_UNIT_EDITOR should only be used with numeric properties!" ) );
    }
}


bool PGUnitBinderGetValue( UNIT_BINDER& aBinder, const wxString& aText, wxVariant& aVariant,
                           PG_UNIT_KIND aKind, double aAngleScale )
{
    if( aText == wxT( "<...>" ) )
    {
        aVariant.MakeNull();
        return true;
    }

    bool changed;

    if( aKind == PG_UNIT_KIND::ANGLE )
    {
        EDA_ANGLE angle = aBinder.GetAngleValue();

        if( aVariant.GetType() == wxT( "EDA_ANGLE" ) )
        {
            EDA_ANGLE_VARIANT_DATA* ad = static_cast<EDA_ANGLE_VARIANT_DATA*>( aVariant.GetData() );
            changed = ( aVariant.IsNull() || angle != ad->Angle() );

            if( changed )
            {
                ad->SetAngle( angle );
                aBinder.SetAngleValue( angle );
            }
        }
        else
        {
            double scaledValue = angle.AsDegrees() * aAngleScale;

            changed = ( aVariant.IsNull() || scaledValue != aVariant.GetDouble() );

            if( changed )
            {
                aVariant = scaledValue;
                aBinder.SetValue( angle.AsDegrees() );
            }
        }
    }
    else if( aKind == PG_UNIT_KIND::AREA )
    {
        wxLongLong result = aBinder.GetValue();
        changed = ( aVariant.IsNull() || result != aVariant.GetLongLong() );

        if( changed )
        {
            aVariant = result;
            aBinder.SetDoubleValue( result.ToDouble() );
        }
    }
    else if( aVariant.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aVariant.GetData() );
        std::optional<int> result;

        if( aBinder.IsNull() )
        {
            changed = ( aVariant.IsNull() || variantData->Value().has_value() );

            if( changed )
            {
                aVariant = wxVariant( std::optional<int>() );
                aBinder.SetValue( wxEmptyString );
            }
        }
        else
        {
            result = std::optional<int>( aBinder.GetValue() );
            changed = ( aVariant.IsNull() || result != variantData->Value() );

            if( changed )
            {
                aVariant = wxVariant( result );
                aBinder.SetValue( result.value() );
            }
        }
    }
    else
    {
        long result = aBinder.GetValue();
        changed = ( aVariant.IsNull() || result != aVariant.GetLong() );

        if( changed )
        {
            aVariant = result;
            aBinder.SetValue( result );
        }
    }

    // Changing unspecified always causes event (returning
    // true here should be enough to trigger it).
    if( !changed && aVariant.IsNull() )
        changed = true;

    return changed;
}


bool PGRatioFromText( const wxString& aText, wxVariant& aVariant, wxString& aNormalizedText )
{
    wxString textVal = aText;

    if( textVal == wxT( "<...>" ) )
    {
        aVariant.MakeNull();
        return true;
    }

    bool changed;

    if( aVariant.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aVariant.GetData() );

        if( textVal.empty() )
        {
            changed = ( aVariant.IsNull() || variantData->Value().has_value() );

            if( changed )
                aVariant = wxVariant( std::optional<double>() );
        }
        else
        {
            double dblValue;
            textVal.ToDouble( &dblValue );
            std::optional<double> result( dblValue );
            changed = ( aVariant.IsNull() || result != variantData->Value() );

            if( changed )
            {
                aVariant = wxVariant( result );
                aNormalizedText = wxString::Format( wxS( "%g" ), dblValue );
            }
        }
    }
    else
    {
        double result;
        textVal.ToDouble( &result );
        changed = ( aVariant.IsNull() || result != aVariant.GetDouble() );

        if( changed )
        {
            aVariant = result;
            aNormalizedText = wxString::Format( wxS( "%g" ), result );
        }
    }

    // Changing unspecified always causes event (returning
    // true here should be enough to trigger it).
    if( !changed && aVariant.IsNull() )
        changed = true;

    return changed;
}
