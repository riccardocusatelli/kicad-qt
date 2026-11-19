/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ranges>

#include <widgets/footprint_wizard_properties_panel.h>

#include <footprint_wizard.h>
#include <geometry/eda_angle.h>
#include <footprint_wizard_frame.h>
#ifndef KICAD_WX_FREE
#include <properties/pg_editors.h>
#endif
#include <properties/pg_properties.h>
#include <widgets/ui_common.h>

#include <wx/regex.h>

#ifndef KICAD_WX_FREE
#include <wx/stattext.h>
#endif


FOOTPRINT_WIZARD_PROPERTIES_PANEL::FOOTPRINT_WIZARD_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent,
                                                                      FOOTPRINT_WIZARD_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_unitEditorInstance( nullptr ),
        m_checkboxEditorInstance( nullptr ),
        m_ratioEditorInstance( nullptr )
{
#ifdef KICAD_WX_FREE
    // The caption of the dock is enough
    m_caption->hide();
    m_grid->SetEditorFrame( m_frame );
#else
    // AUI panel caption is enough
    m_caption->Hide();

    wxASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );
    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_UNIT_EDITOR* new_editor = new PG_UNIT_EDITOR( m_frame );
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new_editor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_CHECKBOX_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_CHECKBOX_EDITOR* cbEditor = new PG_CHECKBOX_EDITOR();
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( wxPropertyGrid::RegisterEditorClass( cbEditor ) );
    }
    else
    {
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_RATIO_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_RATIO_EDITOR* ratioEditor = new PG_RATIO_EDITOR();
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( wxPropertyGrid::RegisterEditorClass( ratioEditor ) );
    }
    else
    {
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( it->second );
    }
#endif
}


FOOTPRINT_WIZARD_PROPERTIES_PANEL::~FOOTPRINT_WIZARD_PROPERTIES_PANEL()
{
#ifndef KICAD_WX_FREE
    if( m_unitEditorInstance )
        m_unitEditorInstance->UpdateFrame( nullptr );
#endif
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::UpdateData()
{
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::RebuildParameters( FOOTPRINT_WIZARD* aWizard )
{
    SUPPRESS_GRID_CHANGED_EVENTS raii( this );

    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();

#ifdef KICAD_WX_FREE
    m_params.clear();
#endif

    if( !aWizard )
        return;

    std::map<kiapi::common::types::WizardParameterCategory, std::vector<WIZARD_PARAMETER*>> params;

    for( const std::unique_ptr<WIZARD_PARAMETER>& param : aWizard->Info().parameters )
        params[param->category].emplace_back( param.get() );

    for( kiapi::common::types::WizardParameterCategory category : params | std::views::keys )
    {
#ifdef KICAD_WX_FREE
        m_grid->Append( new PGPROPERTY_CATEGORY( WIZARD_PARAMETER::ParameterCategoryName( category ) ) );

        for( WIZARD_PARAMETER* param : params[category] )
        {
            if( TOOLKIT_PG_PROPERTY* prop = createPGProperty( param ) )
            {
                m_params[param->identifier] = param;
                m_grid->Append( prop );
            }
        }
#else
        auto groupItem = new wxPropertyCategory( WIZARD_PARAMETER::ParameterCategoryName( category ) );
        m_grid->Append( groupItem );

        for( WIZARD_PARAMETER* param : params[category] )
        {
            if( wxPGProperty* prop = createPGProperty( param ) )
                m_grid->Append( prop );
        }
#endif
    }

    RecalculateSplitterPos();
}


#ifdef KICAD_WX_FREE
TOOLKIT_PG_PROPERTY* FOOTPRINT_WIZARD_PROPERTIES_PANEL::createPGProperty( WIZARD_PARAMETER* aParam ) const
{
    PG_PROPERTY* ret = nullptr;

    switch( aParam->type )
    {
    case kiapi::common::types::WPDT_DISTANCE: ret = new PGPROPERTY_SIZE( m_frame ); break;
    case kiapi::common::types::WPDT_AREA:     ret = new PGPROPERTY_AREA( m_frame ); break;
    case kiapi::common::types::WPDT_TIME:     ret = new PGPROPERTY_FLOAT();         break;
    case kiapi::common::types::WPDT_ANGLE:    ret = new PGPROPERTY_ANGLE();         break;
    case kiapi::common::types::WPDT_STRING:   ret = new PGPROPERTY_STRING();        break;
    case kiapi::common::types::WPDT_INTEGER:  ret = new PGPROPERTY_INT();           break;
    case kiapi::common::types::WPDT_REAL:     ret = new PGPROPERTY_FLOAT();         break;
    case kiapi::common::types::WPDT_BOOL:     ret = new PGPROPERTY_BOOL();          break;

    case kiapi::common::types::WPDT_ENUM:
        if( auto param = dynamic_cast<const WIZARD_ENUM_PARAMETER*>( aParam ) )
        {
            PROPERTY_CHOICES choices;

            for( size_t i = 0; i < param->choices.size(); ++i )
                choices.Add( wxGetTranslation( param->choices[i].second ), static_cast<int>( i ) );

            ret = new PGPROPERTY_ENUM( choices );
        }

        break;

    default:
        break;
    }

    if( ret )
    {
        ret->SetLabel( wxGetTranslation( aParam->name ) );
        ret->SetName( aParam->identifier );
        ret->SetHelpString( wxGetTranslation( aParam->description ) );
        ret->SetEditorFrame( m_frame );

        if( auto ip = dynamic_cast<const WIZARD_INT_PARAMETER*>( aParam ) )
            ret->SetValue( wxVariant( long( ip->value ) ) );
        else if( auto rp = dynamic_cast<const WIZARD_REAL_PARAMETER*>( aParam ) )
            ret->SetValue( wxVariant( rp->value ) );
        else if( auto bp = dynamic_cast<const WIZARD_BOOL_PARAMETER*>( aParam ) )
            ret->SetValue( wxVariant( bp->value ) );
        else if( auto sp = dynamic_cast<const WIZARD_STRING_PARAMETER*>( aParam ) )
            ret->SetValue( wxVariant( sp->value ) );
        else if( auto ep = dynamic_cast<const WIZARD_ENUM_PARAMETER*>( aParam ) )
            ret->SetValue( wxVariant( long( ep->value ) ) );
    }

    return ret;
}


WIZARD_PARAMETER* FOOTPRINT_WIZARD_PROPERTIES_PANEL::getParamFromName( const wxString& aName ) const
{
    auto it = m_params.find( aName );
    return it != m_params.end() ? it->second : nullptr;
}


bool FOOTPRINT_WIZARD_PROPERTIES_PANEL::valueChanging( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents )
        return true;

    WIZARD_PARAMETER* param = getParamFromName( aName );
    wxCHECK( param, false );

    return checkValue( param, aValue.GetAny() );
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::valueChanged( const wxString& aName, const wxVariant& aValue )
{
    WIZARD_PARAMETER* param = getParamFromName( aName );
    wxCHECK( param, /* void */ );

    applyValue( param, aValue.GetAny() );
}

#else
wxPGProperty* FOOTPRINT_WIZARD_PROPERTIES_PANEL::createPGProperty( WIZARD_PARAMETER* aParam ) const
{
    wxPGProperty* ret = nullptr;

    switch( aParam->type )
    {
    case kiapi::common::types::WPDT_DISTANCE:
        ret = new PGPROPERTY_SIZE( m_frame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_AREA:
        ret = new PGPROPERTY_AREA( m_frame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_VOLUME:
        wxASSERT_MSG( false, "Volume properties are not currently implemented" );
        break;

    case kiapi::common::types::WPDT_TIME:
        ret = new wxFloatProperty();
        break;

    case kiapi::common::types::WPDT_ANGLE:
        ret = new PGPROPERTY_ANGLE();;
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_STRING:
        ret = new PGPROPERTY_STRING();
        break;

    case kiapi::common::types::WPDT_INTEGER:
        ret = new wxIntProperty();
        break;

    case kiapi::common::types::WPDT_REAL:
        ret = new wxFloatProperty();
        break;

    case kiapi::common::types::WPDT_BOOL:
        ret = new PGPROPERTY_BOOL();
        break;

    case kiapi::common::types::WPDT_ENUM:
    {
        if( auto param = dynamic_cast<const WIZARD_ENUM_PARAMETER*>( aParam ) )
        {
            wxPGChoices pgChoices;

            for( size_t i = 0; i < param->choices.size(); ++i )
                pgChoices.Add( wxGetTranslation( param->choices[i].second ), static_cast<int>( i ) );

            ret = new wxEnumProperty( wxPG_LABEL, wxPG_LABEL, pgChoices );
        }

        break;
    }

    case kiapi::common::types::WPDT_UNKNOWN:
    default:
        break;
    }

    if( ret )
    {
        ret->SetLabel( wxGetTranslation( aParam->name ) );
        ret->SetName( aParam->identifier );
        ret->SetHelpString( wxGetTranslation( aParam->description ) );
        ret->SetClientData( aParam );

        if( auto ip = dynamic_cast<const WIZARD_INT_PARAMETER*>( aParam ) )
            ret->SetValue( ip->value );
        else if( auto rp = dynamic_cast<const WIZARD_REAL_PARAMETER*>( aParam ) )
            ret->SetValue( rp->value );
        else if( auto bp = dynamic_cast<const WIZARD_BOOL_PARAMETER*>( aParam ) )
            ret->SetValue( bp->value );
        else if( auto sp = dynamic_cast<const WIZARD_STRING_PARAMETER*>( aParam ) )
            ret->SetValue( sp->value );
        else if( auto ep = dynamic_cast<const WIZARD_ENUM_PARAMETER*>( aParam ) )
            ret->SetValue( ep->value );
    }

    return ret;
}


WIZARD_PARAMETER* FOOTPRINT_WIZARD_PROPERTIES_PANEL::getParamFromEvent( const wxPropertyGridEvent& aEvent )
{
    return static_cast<WIZARD_PARAMETER*>( aEvent.GetProperty()->GetClientData() );
}
#endif


wxString FOOTPRINT_WIZARD_PROPERTIES_PANEL::formatConstraintValue( const WIZARD_PARAMETER* aParam, int aValue ) const
{
    switch( aParam->type )
    {
    case kiapi::common::types::WPDT_DISTANCE:
        return m_frame->MessageTextFromValue( aValue, true, EDA_DATA_TYPE::DISTANCE );

    case kiapi::common::types::WPDT_AREA:
        return m_frame->MessageTextFromValue( aValue, true, EDA_DATA_TYPE::AREA );

    case kiapi::common::types::WPDT_TIME:
        return m_frame->MessageTextFromValue( aValue, true, EDA_DATA_TYPE::TIME );

    default:
        return wxString::Format( wxS( "%d" ), aValue );
    }
}


wxString FOOTPRINT_WIZARD_PROPERTIES_PANEL::formatConstraintValue( const WIZARD_PARAMETER* aParam, double aValue ) const
{
    switch( aParam->type )
    {
    case kiapi::common::types::WPDT_ANGLE:
        return m_frame->MessageTextFromValue( EDA_ANGLE( aValue, DEGREES_T ) );

    default:
        return wxString::Format( wxS( "%.3f" ), aValue );
    }
}


#ifndef KICAD_WX_FREE
void FOOTPRINT_WIZARD_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    WIZARD_PARAMETER* param = getParamFromEvent( aEvent );
    wxCHECK( param, /* void */ );

    if( !checkValue( param, aEvent.GetPropertyValue() ) )
    {
        aEvent.Veto();
        return;
    }

    return true;
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    WIZARD_PARAMETER* param = getParamFromEvent( aEvent );
    wxCHECK( param, /* void */ );

    applyValue( param, aEvent.GetPropertyValue() );
}
#endif


bool FOOTPRINT_WIZARD_PROPERTIES_PANEL::checkValue( WIZARD_PARAMETER* param, const wxAny& newValue ) const
{
    if( WIZARD_INT_PARAMETER* ip = dynamic_cast<WIZARD_INT_PARAMETER*>( param ) )
    {
        int value = newValue.As<int>();

        if( ip->min && value < *ip->min )
        {
            m_frame->SetBuildMessage( wxString::Format( _( "%s must be at least %s." ),
                                                        wxGetTranslation( ip->name ),
                                                        formatConstraintValue( ip, *ip->min ) ) );
            return false;
        }

        if( ip->max && value > *ip->max )
        {
            m_frame->SetBuildMessage( wxString::Format( _( "%s must be at most %s." ),
                                                        wxGetTranslation( ip->name ),
                                                        formatConstraintValue( ip, *ip->max ) ) );
            return false;
        }

        if( ip->multiple && ( value % *ip->multiple ) != 0 )
        {
            m_frame->SetBuildMessage( wxString::Format( _( "%s must be a multiple of %s." ),
                                                        wxGetTranslation( ip->name ),
                                                        formatConstraintValue( ip, *ip->multiple ) ) );
            return false;
        }
    }
    else if( WIZARD_REAL_PARAMETER* rp = dynamic_cast<WIZARD_REAL_PARAMETER*>( param ) )
    {
        double value = newValue.As<double>();

        if( rp->min && value < *rp->min )
        {
            m_frame->SetBuildMessage( wxString::Format( _( "%s must be at least %s." ),
                                                        wxGetTranslation( rp->name ),
                                                        formatConstraintValue( rp, *rp->min ) ) );
            return false;
        }

        if( rp->max && value > *rp->max )
        {
            m_frame->SetBuildMessage( wxString::Format( _( "%s must be at most %s." ),
                                                        wxGetTranslation( rp->name ),
                                                        formatConstraintValue( rp, *rp->max ) ) );
            return false;
        }
    }
    else if( WIZARD_STRING_PARAMETER* sp = dynamic_cast<WIZARD_STRING_PARAMETER*>( param ) )
    {
        if( sp->validation_regex )
        {
            if( wxRegEx re( *sp->validation_regex, wxRE_ADVANCED ); re.IsValid() )
            {
                if( wxString value = newValue.As<wxString>(); !re.Matches( value ) )
                {
                    m_frame->SetBuildMessage( wxString::Format( _( "%s does not match the required format '%s'." ),
                                                                wxGetTranslation( sp->name ), *sp->validation_regex ) );
                    return false;
                }
            }
        }
    }

    return true;
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::applyValue( WIZARD_PARAMETER* param, const wxAny& newValue )
{
    if( WIZARD_INT_PARAMETER* ip = dynamic_cast<WIZARD_INT_PARAMETER*>( param ) )
    {
        ip->value = newValue.As<int>();
    }
    else if( WIZARD_REAL_PARAMETER* rp = dynamic_cast<WIZARD_REAL_PARAMETER*>( param ) )
    {
        rp->value = newValue.As<double>();
    }
    else if( WIZARD_BOOL_PARAMETER* bp = dynamic_cast<WIZARD_BOOL_PARAMETER*>( param ) )
    {
        bp->value = newValue.As<bool>();
    }
    else if( WIZARD_STRING_PARAMETER* sp = dynamic_cast<WIZARD_STRING_PARAMETER*>( param ) )
    {
        sp->value = newValue.As<wxString>();
    }
    else if( WIZARD_ENUM_PARAMETER* ep = dynamic_cast<WIZARD_ENUM_PARAMETER*>( param ) )
    {
        ep->value = newValue.As<int>();
    }

    m_frame->OnWizardParametersChanged();
}
