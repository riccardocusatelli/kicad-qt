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

#include <wx/dc.h>
#include <wx/propgrid/propgrid.h>

#include <macros.h>
#include <validators.h>
#include <eda_draw_frame.h>
#include <eda_units.h>
#include <properties/color4d_variant.h>
#include <properties/std_optional_variants.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_properties.h>
#include <properties/pg_value_text.h>
#include <properties/pg_editors.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <string_utils.h>
#include <widgets/color_swatch.h>
#include <core/ki_assert.h>


// The conversions of the values of the properties (wxAny) to the values of the grid are
// registered in pg_value_text.cpp


wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty, EDA_DRAW_FRAME* aFrame )
{
    wxPGProperty* ret = nullptr;
    PROPERTY_DISPLAY display = aProperty->Display();

    switch( display )
    {
    case PROPERTY_DISPLAY::PT_TIME:
        ret = new PGPROPERTY_TIME( aFrame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_SIZE:
        ret = new PGPROPERTY_SIZE( aFrame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_AREA:
        ret = new PGPROPERTY_AREA( aFrame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_COORD:
        ret = new PGPROPERTY_COORD( aFrame, aProperty->CoordType() );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;

    case PROPERTY_DISPLAY::PT_DECIDEGREE:
    case PROPERTY_DISPLAY::PT_DEGREE:
    {
        PGPROPERTY_ANGLE* prop = new PGPROPERTY_ANGLE();

        if( display == PROPERTY_DISPLAY::PT_DECIDEGREE )
            prop->SetScale( 10.0 );

        ret = prop;
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( aFrame ) );
        break;
    }

    case PROPERTY_DISPLAY::PT_RATIO:
        ret = new PGPROPERTY_RATIO();
        break;

    case PROPERTY_DISPLAY::PT_NET:
        ret = new PGPROPERTY_NET( ToWxPGChoices( aProperty->Choices() ) );
        break;

    default:
        KI_FAIL();
        KI_FALLTHROUGH;
        /* fall through */
    case PROPERTY_DISPLAY::PT_DEFAULT:
    {
        // Create a corresponding wxPGProperty
        size_t typeId = aProperty->TypeHash();

        // Enum property
        if( aProperty->HasChoices() )
        {
            const_cast<PROPERTY_BASE*>( aProperty )->TranslateChoices();

            wxPGChoices choices = ToWxPGChoices( aProperty->Choices() );
            ret = new wxEnumProperty( wxPG_LABEL, wxPG_LABEL, choices );
        }
        else if( typeId == TYPE_HASH( int ) || typeId == TYPE_HASH( long ) )
        {
            ret = new wxIntProperty();
        }
        else if( typeId == TYPE_HASH( unsigned int ) || typeId == TYPE_HASH( unsigned long ) )
        {
            ret = new wxUIntProperty();
        }
        else if( typeId == TYPE_HASH( float ) || typeId == TYPE_HASH( double ) )
        {
            ret = new wxFloatProperty();
        }
        else if( typeId == TYPE_HASH( bool ) )
        {
            ret = new PGPROPERTY_BOOL();
        }
        else if( typeId == TYPE_HASH( wxString ) )
        {
            ret = new PGPROPERTY_STRING();
        }
        else if( typeId == TYPE_HASH( COLOR4D ) )
        {
            ret = new PGPROPERTY_COLOR4D();
        }
        else
        {
            KI_FAIL_MSG( wxString::Format( wxS( "Property %s not supported by PGPropertyFactory" ),
                                          aProperty->Name() ) );
            ret = new wxPropertyCategory();
            ret->Enable( false );
        }

        break;
    }
    }

    if( ret )
    {
        ret->SetLabel( wxGetTranslation( aProperty->Name() ) );
        ret->SetName( aProperty->Name() );
        ret->SetHelpString( wxGetTranslation( aProperty->Name() ) );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
    }

    return ret;
}


TOOLKIT_PG_PROPERTY* PGColorEnumPropertyFactory( const PROPERTY_BASE* aProperty,
                                                 const std::vector<std::pair<wxString, int>>& aChoices,
                                                 std::function<KIGFX::COLOR4D( int aValue )> aColorFunc )
{
    wxArrayString names;
    wxArrayInt    values;

    for( const auto& [name, value] : aChoices )
    {
        names.push_back( name );
        values.push_back( value );
    }

    auto ret = new PGPROPERTY_COLORENUM( new wxPGChoices( names, values ) );

    ret->SetColorFunc(
            [aColorFunc]( int aValue ) -> wxColour
            {
                return aColorFunc( aValue ).ToColour();
            } );

    ret->SetLabel( wxGetTranslation( aProperty->Name() ) );
    ret->SetName( aProperty->Name() );
    ret->SetHelpString( wxGetTranslation( aProperty->Name() ) );
    ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );

    return ret;
}


void PGSetEditor( TOOLKIT_PG_PROPERTY* aProperty, PG_EDITOR aEditor, EDA_DRAW_FRAME* aFrame )
{
    switch( aEditor )
    {
    case PG_EDITOR::FPID: aProperty->SetEditor( PG_FPID_EDITOR::BuildEditorName( aFrame ) ); break;
    case PG_EDITOR::URL:  aProperty->SetEditor( PG_URL_EDITOR::BuildEditorName( aFrame ) );  break;
    }
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
    aProperty->SetChoices( ToWxPGChoices( aChoices ) );
}


void PGSetValue( TOOLKIT_PG_PROPERTY* aProperty, const wxVariant& aValue )
{
    aProperty->SetValue( aValue );
}


void PGSetReadOnly( TOOLKIT_PG_PROPERTY* aProperty, bool aReadOnly )
{
    aProperty->ChangeFlag( wxPG_PROP_READONLY, aReadOnly );
}


PGPROPERTY_DISTANCE::PGPROPERTY_DISTANCE( EDA_DRAW_FRAME* aParentFrame,
                                          ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        m_parentFrame( aParentFrame ),
        m_coordType( aCoordType )
{
}


PGPROPERTY_DISTANCE::~PGPROPERTY_DISTANCE()
{
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_DISTANCE::StringToDistance( wxVariant& aVariant, const wxString& aText,
                                            wxPGPropValFormatFlags aFlags ) const
#else
bool PGPROPERTY_DISTANCE::StringToDistance( wxVariant& aVariant, const wxString& aText,
                                            int aFlags ) const
#endif
{
    KI_CHECK_MSG( false, false, wxS( "PGPROPERTY_DISTANCE::StringToDistance should not be used." ) );
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_DISTANCE::DistanceToString( wxVariant& aVariant,
                                                wxPGPropValFormatFlags aFlags ) const
#else
wxString PGPROPERTY_DISTANCE::DistanceToString( wxVariant& aVariant, int aFlags ) const
#endif
{
    return PGDistanceToString( m_parentFrame, aVariant, m_coordType );
}


PGPROPERTY_AREA::PGPROPERTY_AREA( EDA_DRAW_FRAME* aParentFrame ) :
        wxIntProperty( wxPG_LABEL, wxPG_LABEL, 0 ),
        m_parentFrame( aParentFrame )
{
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_AREA::StringToValue( wxVariant& aVariant, const wxString& aText,
                                     wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_AREA::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
#endif
{
    KI_CHECK_MSG( false, false, wxS( "PGPROPERTY_AREA::StringToValue should not be used." ) );
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_AREA::ValueToString( wxVariant& aVariant, wxPGPropValFormatFlags aArgFlags ) const
#else
wxString PGPROPERTY_AREA::ValueToString( wxVariant& aVariant, int aArgFlags ) const
#endif
{
    return PGAreaToString( m_parentFrame, aVariant );
}


wxValidator* PGPROPERTY_AREA::DoGetValidator() const
{
    return nullptr;
}


PGPROPERTY_SIZE::PGPROPERTY_SIZE( EDA_DRAW_FRAME* aParentFrame ) :
        wxUIntProperty( wxPG_LABEL, wxPG_LABEL, 0 ),
        PGPROPERTY_DISTANCE( aParentFrame, ORIGIN_TRANSFORMS::NOT_A_COORD )
{
}


bool PGPROPERTY_SIZE::ValidateValue( wxVariant& aValue, wxPGValidationInfo& aValidationInfo ) const
{
    if( aValue.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* data = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aValue.GetData() );

        if( !data->Value().has_value() )
            return wxEmptyString;

        wxVariant value( data->Value().value() );
        return wxUIntProperty::ValidateValue( value, aValidationInfo );
    }

    return wxUIntProperty::ValidateValue( aValue, aValidationInfo );
}


wxValidator* PGPROPERTY_SIZE::DoGetValidator() const
{
    return nullptr;
}


PGPROPERTY_COORD::PGPROPERTY_COORD( EDA_DRAW_FRAME* aParentFrame, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        wxIntProperty( wxPG_LABEL, wxPG_LABEL, 0 ),
        PGPROPERTY_DISTANCE( aParentFrame, aCoordType )
{
}


wxValidator* PGPROPERTY_COORD::DoGetValidator() const
{
    return nullptr;
}


PGPROPERTY_RATIO::PGPROPERTY_RATIO() :
        wxFloatProperty( wxPG_LABEL, wxPG_LABEL, 0 )
{
    SetEditor( PG_RATIO_EDITOR::EDITOR_NAME );
}


const wxPGEditor* PGPROPERTY_RATIO::DoGetEditorClass() const
{
    KI_CHECK_MSG( m_customEditor, wxPGEditor_TextCtrl,
                 wxT( "Make sure to RegisterEditorClass() for PGPROPERTY_RATIO!" ) );
    return m_customEditor;
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_RATIO::StringToValue( wxVariant& aVariant, const wxString& aText,
                                      wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_RATIO::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
#endif
{
    KI_CHECK_MSG( false, false, wxS( "PGPROPERTY_RATIO::StringToValue should not be used." ) );
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_RATIO::ValueToString( wxVariant& aVariant, wxPGPropValFormatFlags aArgFlags ) const
#else
wxString PGPROPERTY_RATIO::ValueToString( wxVariant& aVariant, int aArgFlags ) const
#endif
{
    return PGRatioToString( aVariant );
}


bool PGPROPERTY_RATIO::ValidateValue( wxVariant& aValue, wxPGValidationInfo& aValidationInfo ) const
{
    if( aValue.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* data = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aValue.GetData() );

        if( !data->Value().has_value() )
            return wxEmptyString;

        wxVariant value( data->Value().value() );
        return wxFloatProperty::ValidateValue( value, aValidationInfo );
    }

    return wxFloatProperty::ValidateValue( aValue, aValidationInfo );
}


wxValidator* PGPROPERTY_RATIO::DoGetValidator() const
{
    return nullptr;
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_ANGLE::StringToValue( wxVariant& aVariant, const wxString& aText,
                                      wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_ANGLE::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
#endif
{
    return PGAngleFromString( aVariant, aText, m_scale );
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_ANGLE::ValueToString( wxVariant& aVariant, wxPGPropValFormatFlags aArgFlags ) const
#else
wxString PGPROPERTY_ANGLE::ValueToString( wxVariant& aVariant, int aArgFlags ) const
#endif
{
    return PGAngleToString( aVariant, m_scale );
}


wxValidator* PGPROPERTY_ANGLE::DoGetValidator() const
{
    return nullptr;
}


wxSize PGPROPERTY_COLORENUM::OnMeasureImage( int aItem ) const
{
    wxSize size( 16, -1 );

    if( wxPropertyGrid* pg = GetGrid() )
        size = pg->FromDIP( size );

    return size;
}


void PGPROPERTY_COLORENUM::OnCustomPaint( wxDC& aDC, const wxRect& aRect,
                                          wxPGPaintData& aPaintData )
{
    int index = aPaintData.m_choiceItem;

    if( index < 0 )
        index = GetIndex();

    // GetIndex can return -1 when the control hasn't been set up yet
    if( index < 0 || index >= static_cast<int>( GetChoices().GetCount() ) )
        return;

    wxColour color = GetColor( GetChoices().GetValue( index ) );

    if( color == wxNullColour )
        return;

    aDC.SetPen( *wxTRANSPARENT_PEN );
    aDC.SetBrush( wxBrush( color ) );
    aDC.DrawRectangle( aRect );

    aPaintData.m_drawnWidth = aRect.width;
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_STRING::ValueToString( wxVariant& aValue, wxPGPropValFormatFlags aFlags ) const
#else
wxString PGPROPERTY_STRING::ValueToString( wxVariant& aValue, int aFlags ) const
#endif
{
    return PGStringToString( aValue );
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_STRING::StringToValue( wxVariant& aVariant, const wxString& aString,
                                       wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_STRING::StringToValue( wxVariant& aVariant, const wxString& aString, int aFlags ) const
#endif
{
    aVariant = PGStringFromString( aString );
    return true;
}


PGPROPERTY_BOOL::PGPROPERTY_BOOL( const wxString& aLabel, const wxString& aName, bool aValue ) :
            wxBoolProperty( aLabel, aName, aValue )
{
    SetEditor( PG_CHECKBOX_EDITOR::EDITOR_NAME );
}


const wxPGEditor* PGPROPERTY_BOOL::DoGetEditorClass() const
{
    KI_CHECK_MSG( m_customEditor, wxPGEditor_CheckBox,
                 wxT( "Make sure to RegisterEditorClass() for PGPROPERTY_BOOL!" ) );
    return m_customEditor;
}


PGPROPERTY_COLOR4D::PGPROPERTY_COLOR4D( const wxString& aLabel, const wxString& aName,
                                        COLOR4D aValue, COLOR4D aBackgroundColor ) :
        wxStringProperty( aLabel, aName, aValue.ToCSSString() ),
        m_backgroundColor( aBackgroundColor )
{
    SetEditor( PG_COLOR_EDITOR::EDITOR_NAME );
#if wxCHECK_VERSION( 3, 3, 1 )
    SetFlag( wxPGFlags::NoEditor );
#elif wxCHECK_VERSION( 3, 3, 0 )
    SetFlag( wxPGPropertyFlags::NoEditor );
#else
    SetFlag( wxPG_PROP_NOEDITOR );
#endif
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_COLOR4D::StringToValue( wxVariant& aVariant, const wxString& aString,
                                        wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_COLOR4D::StringToValue( wxVariant& aVariant, const wxString& aString, int aFlags ) const
#endif
{
    aVariant = PGColorFromString( aString );
    return true;
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_COLOR4D::ValueToString( wxVariant& aValue, wxPGPropValFormatFlags aFlags ) const
#else
wxString PGPROPERTY_COLOR4D::ValueToString( wxVariant& aValue, int aFlags ) const
#endif
{
    wxString ret;

    if( aValue.IsType( wxS( "COLOR4D" ) ) )
        static_cast<COLOR4D_VARIANT_DATA*>( aValue.GetData() )->Write( ret );
    else
        return wxStringProperty::ValueToString( aValue, aFlags );

    return ret;
}


PGPROPERTY_TIME::PGPROPERTY_TIME( EDA_DRAW_FRAME* aParentFrame ) :
        wxIntProperty( wxPG_LABEL, wxPG_LABEL, 0 ),
        m_parentFrame( aParentFrame )
{
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool PGPROPERTY_TIME::StringToValue( wxVariant& aVariant, const wxString& aText,
                                     wxPGPropValFormatFlags aArgFlags ) const
#else
bool PGPROPERTY_TIME::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
#endif
{
    KI_CHECK_MSG( false, false, wxS( "PGPROPERTY_RATIO::StringToValue should not be used." ) );
}


#if wxCHECK_VERSION( 3, 3, 0 )
wxString PGPROPERTY_TIME::ValueToString( wxVariant& aVariant, wxPGPropValFormatFlags aArgFlags ) const
#else
wxString PGPROPERTY_TIME::ValueToString( wxVariant& aVariant, int aArgFlags ) const
#endif
{
    return PGTimeToString( m_parentFrame, aVariant );
}


bool PGPROPERTY_TIME::ValidateValue( wxVariant& aValue, wxPGValidationInfo& aValidationInfo ) const
{
    return true;
}


wxValidator* PGPROPERTY_TIME::DoGetValidator() const
{
    return nullptr;
}


PGPROPERTY_NET::PGPROPERTY_NET( const wxPGChoices& aChoices ) :
        wxEnumProperty( wxPG_LABEL, wxPG_LABEL, const_cast<wxPGChoices&>( aChoices ) )
{
    SetEditor( wxS( "PG_NET_SELECTOR_EDITOR" ) );
}


const wxPGEditor* PGPROPERTY_NET::DoGetEditorClass() const
{
    KI_CHECK_MSG( m_customEditor, wxPGEditor_Choice,
                 wxT( "Make sure to RegisterEditorClass() for PGPROPERTY_NET!" ) );

    return m_customEditor;
}
