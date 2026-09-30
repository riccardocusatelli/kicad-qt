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


#include <marker_base.h>
#include <rc_item.h>
#include <rc_json_schema.h>
#include <eda_item.h>
#include <base_units.h>
#include <units_provider.h>
#include <core/ki_assert.h>



wxString RC_ITEM::GetErrorMessage( bool aTranslate ) const
{
    if( m_errorMessage.IsEmpty() )
        return GetErrorText( aTranslate );
    else
        return m_errorMessage;
}


static wxString showCoord( UNITS_PROVIDER* aUnitsProvider, const VECTOR2I& aPos )
{
    return wxString::Format( wxT( "@(%s, %s)" ),
                             aUnitsProvider->MessageTextFromValue( aPos.x ),
                             aUnitsProvider->MessageTextFromValue( aPos.y ) );
}


void RC_ITEM::AddItem( EDA_ITEM* aItem )
{
    m_ids.push_back( aItem->m_Uuid );
}


void RC_ITEM::SetItems( const EDA_ITEM* aItem, const EDA_ITEM* bItem,
                        const EDA_ITEM* cItem, const EDA_ITEM* dItem )
{
    m_ids.clear();

    if( aItem )
        m_ids.push_back( aItem->m_Uuid );

    if( bItem )
        m_ids.push_back( bItem->m_Uuid );

    if( cItem )
        m_ids.push_back( cItem->m_Uuid );

    if( dItem )
        m_ids.push_back( dItem->m_Uuid );
}


wxString RC_ITEM::getItemDescription( EDA_ITEM* aItem, int /*aIndex*/,
                                      UNITS_PROVIDER* aUnitsProvider ) const
{
    return aItem->GetItemDescription( aUnitsProvider, true );
}


wxString RC_ITEM::getSeverityString( SEVERITY aSeverity )
{
    wxString severity;

    switch( aSeverity )
    {
    case RPT_SEVERITY_ERROR:     severity = wxS( "error" );     break;
    case RPT_SEVERITY_WARNING:   severity = wxS( "warning" );   break;
    case RPT_SEVERITY_ACTION:    severity = wxS( "action" );    break;
    case RPT_SEVERITY_INFO:      severity = wxS( "info" );      break;
    case RPT_SEVERITY_EXCLUSION: severity = wxS( "exclusion" ); break;
    case RPT_SEVERITY_DEBUG:     severity = wxS( "debug" );     break;
    default:;
    };

    return severity;
}


wxString RC_ITEM::ShowReport( UNITS_PROVIDER* aUnitsProvider, SEVERITY aSeverity,
                              const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    wxString severity = getSeverityString( aSeverity );
    bool     excluded = m_parent && m_parent->IsTreatedAsExcluded();

    if( excluded )
        severity += wxT( " (excluded)" );

    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    auto ii = aItemMap.find( GetMainItemID() );

    if( ii != aItemMap.end() )
        mainItem = ii->second;

    ii = aItemMap.find( GetAuxItemID() );

    if( ii != aItemMap.end() )
        auxItem = ii->second;

    // Note: some customers machine-process these.  So:
    // 1) don't translate
    // 2) try not to re-order or change syntax
    // 3) report settings key (which should be more stable) in addition to message

    wxString msg;

    if( mainItem && auxItem )
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n    %s: %s\n    %s: %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity,
                    showCoord( aUnitsProvider, mainItem->GetPosition()),
                    getItemDescription( mainItem, 0, aUnitsProvider ),
                    showCoord( aUnitsProvider, auxItem->GetPosition()),
                    getItemDescription( auxItem, 1, aUnitsProvider ) );
    }
    else if( mainItem )
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n    %s: %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity,
                    showCoord( aUnitsProvider, mainItem->GetPosition()),
                    getItemDescription( mainItem, 0, aUnitsProvider ) );
    }
    else
    {
        msg.Printf( wxT( "[%s]: %s\n    %s; %s\n" ),
                    GetSettingsKey(),
                    GetErrorMessage( false ),
                    GetViolatingRuleDesc( false ),
                    severity );
    }

    if( excluded && m_parent && !m_parent->GetComment().IsEmpty() )
        msg += wxString::Format( wxS( "    %s\n" ), m_parent->GetComment() );

    return msg;
}


void RC_ITEM::GetJsonViolation( RC_JSON::VIOLATION& aViolation, UNITS_PROVIDER* aUnitsProvider,
                                SEVERITY aSeverity, const std::map<KIID, EDA_ITEM*>& aItemMap ) const
{
    aViolation.severity = getSeverityString( aSeverity );
    aViolation.description = GetErrorMessage( false );
    aViolation.type = GetSettingsKey();

    if( m_parent && m_parent->IsTreatedAsExcluded() )
    {
        aViolation.excluded = true;
        aViolation.comment = m_parent->GetComment();
    }
    else
    {
        aViolation.excluded = false;
    }

    EDA_ITEM* mainItem = nullptr;
    EDA_ITEM* auxItem = nullptr;

    auto ii = aItemMap.find( GetMainItemID() );

    if( ii != aItemMap.end() )
        mainItem = ii->second;

    ii = aItemMap.find( GetAuxItemID() );

    if( ii != aItemMap.end() )
        auxItem = ii->second;

    if( mainItem )
    {
        RC_JSON::AFFECTED_ITEM item;
        item.description = getItemDescription( mainItem, 0, aUnitsProvider );
        item.uuid = mainItem->m_Uuid.AsString();
        item.pos.x = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     mainItem->GetPosition().x );
        item.pos.y = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     mainItem->GetPosition().y );
        aViolation.items.emplace_back( item );
    }

    if( auxItem )
    {
        RC_JSON::AFFECTED_ITEM item;
        item.description = getItemDescription( auxItem, 1, aUnitsProvider );
        item.uuid = auxItem->m_Uuid.AsString();
        item.pos.x = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     auxItem->GetPosition().x );
        item.pos.y = EDA_UNIT_UTILS::UI::ToUserUnit( aUnitsProvider->GetIuScale(),
                                                     aUnitsProvider->GetUserUnits(),
                                                     auxItem->GetPosition().y );
        aViolation.items.emplace_back( item );
    }
}
