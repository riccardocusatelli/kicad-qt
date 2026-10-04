/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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
 * @file pcb_tuning_pattern.cpp
 *
 * The tuning pattern of the data model: its outline, properties and registration.  The
 * interactive editing is in pcb_tuning_pattern_edit.cpp (pcb_tuning_pattern_headless.cpp in
 * the headless kiface).
 */

#include <pcb_generator.h>
#include <generators_mgr.h>

#include <functional>
#include <optional>

#include <wx/log.h>

#include <geometry/shape_circle.h>
#include <geometry/geometry_utils.h>
#include <core/mirror.h>
#include <string_utils.h>
#include <msg_panel_context.h>
#include <widgets/msgpanel_item.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_group.h>

#include <router/pns_meander_placer_base.h>
#include <router/pns_meander.h>

#include <generators/pcb_tuning_pattern.h>
#include <project/project_file.h>
#include <project/tuning_profiles.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <core/ki_assert.h>

static LENGTH_TUNING_MODE tuningFromString( const std::string& aStr )
{
    if( aStr == "single" )
        return LENGTH_TUNING_MODE::SINGLE;
    else if( aStr == "diff_pair" )
        return LENGTH_TUNING_MODE::DIFF_PAIR;
    else if( aStr == "diff_pair_skew" )
        return LENGTH_TUNING_MODE::DIFF_PAIR_SKEW;
    else
    {
        KI_FAIL_MSG( wxS( "Unknown length tuning token" ) );
        return LENGTH_TUNING_MODE::SINGLE;
    }
}


static std::string tuningToString( const LENGTH_TUNING_MODE aTuning )
{
    switch( aTuning )
    {
    case LENGTH_TUNING_MODE::SINGLE:         return "single";
    case LENGTH_TUNING_MODE::DIFF_PAIR:      return "diff_pair";
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return "diff_pair_skew";
    default:            KI_FAIL();              return "";
    }
}


static PNS::MEANDER_SIDE sideFromString( const std::string& aStr )
{
    if( aStr == "default" )
        return PNS::MEANDER_SIDE_DEFAULT;
    else if( aStr == "left" )
        return PNS::MEANDER_SIDE_LEFT;
    else if( aStr == "right" )
        return PNS::MEANDER_SIDE_RIGHT;
    else
    {
        KI_FAIL_MSG( wxS( "Unknown length-tuning side token" ) );
        return PNS::MEANDER_SIDE_DEFAULT;
    }
}


static std::string statusToString( const PNS::MEANDER_PLACER_BASE::TUNING_STATUS aStatus )
{
    switch( aStatus )
    {
    case PNS::MEANDER_PLACER_BASE::TOO_LONG:  return "too_long";
    case PNS::MEANDER_PLACER_BASE::TOO_SHORT: return "too_short";
    case PNS::MEANDER_PLACER_BASE::TUNED:     return "tuned";
    default:           KI_FAIL();                return "";
    }
}


static PNS::MEANDER_PLACER_BASE::TUNING_STATUS statusFromString( const std::string& aStr )
{
    if( aStr == "too_long" )
        return PNS::MEANDER_PLACER_BASE::TOO_LONG;
    else if( aStr == "too_short" )
        return PNS::MEANDER_PLACER_BASE::TOO_SHORT;
    else if( aStr == "tuned" )
        return PNS::MEANDER_PLACER_BASE::TUNED;
    else
    {
        KI_FAIL_MSG( wxS( "Unknown tuning status token" ) );
        return PNS::MEANDER_PLACER_BASE::TUNED;
    }
}


static std::string sideToString( const PNS::MEANDER_SIDE aValue )
{
    switch( aValue )
    {
    case PNS::MEANDER_SIDE_DEFAULT: return "default";
    case PNS::MEANDER_SIDE_LEFT:    return "left";
    case PNS::MEANDER_SIDE_RIGHT:   return "right";
    default:        KI_FAIL();         return "";
    }
}


PCB_TUNING_PATTERN::PCB_TUNING_PATTERN( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer,
                                        LENGTH_TUNING_MODE aMode ) :
        PCB_GENERATOR( aParent, aLayer ),
        m_trackWidth( 0 ),
        m_diffPairGap( 0 ),
        m_tuningMode( aMode ),
        m_tuningLength( 0 ),
        m_tuningStatus( PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED ),
        m_updateSideFromEnd( false )
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;
    m_end = VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 );
    m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
}


SHAPE_LINE_CHAIN PCB_TUNING_PATTERN::getOutline() const
{
    if( m_baseLine )
    {
        int clampedMaxAmplitude = m_settings.m_maxAmplitude;
        int minAllowedAmplitude = 0;
        int baselineOffset = m_tuningMode == DIFF_PAIR ? ( m_diffPairGap + m_trackWidth ) / 2 : 0;

        if( m_settings.m_cornerStyle == PNS::MEANDER_STYLE::MEANDER_STYLE_ROUND )
        {
            minAllowedAmplitude = baselineOffset + m_trackWidth;
        }
        else
        {
            int correction = m_trackWidth * tan( 1 - tan( DEG2RAD( 22.5 ) ) );
            minAllowedAmplitude = baselineOffset + correction;
        }

        clampedMaxAmplitude = std::max( clampedMaxAmplitude, minAllowedAmplitude );

        if( m_settings.m_singleSided )
        {
            SHAPE_LINE_CHAIN clBase = *m_baseLine;
            SHAPE_LINE_CHAIN left, right;

            if( m_tuningMode != DIFF_PAIR )
            {
                int amplitude = clampedMaxAmplitude + KiROUND( m_trackWidth / 2.0 );

                SHAPE_LINE_CHAIN chain;

                if( clBase.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                       true ) )
                {
                    chain.Append( m_settings.m_initialSide >= 0 ? right : left );
                    chain.Append( clBase.Reverse() );
                    chain.SetClosed( true );

                    return chain;
                }
            }
            else if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
            {
                int amplitude = clampedMaxAmplitude + m_trackWidth + KiROUND( m_diffPairGap / 2.0 );

                SHAPE_LINE_CHAIN clCoupled = *m_baseLineCoupled;
                SHAPE_LINE_CHAIN chain1, chain2;

                if( clBase.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                       true ) )
                {
                    if( m_settings.m_initialSide >= 0 )
                        chain1.Append( right );
                    else
                        chain1.Append( left );

                    if( clBase.OffsetLine( KiROUND( m_trackWidth / 2.0 ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                           ARC_LOW_DEF, left, right, true ) )
                    {
                        if( m_settings.m_initialSide >= 0 )
                            chain1.Append( left.Reverse() );
                        else
                            chain1.Append( right.Reverse() );
                    }

                    chain1.SetClosed( true );
                }

                if( clCoupled.OffsetLine( amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, left, right,
                                          true ) )
                {
                    if( m_settings.m_initialSide >= 0 )
                        chain2.Append( right );
                    else
                        chain2.Append( left );

                    if( clCoupled.OffsetLine( KiROUND( m_trackWidth / 2.0 ), CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                              ARC_LOW_DEF, left, right, true ) )
                    {
                        if( m_settings.m_initialSide >= 0 )
                            chain2.Append( left.Reverse() );
                        else
                            chain2.Append( right.Reverse() );
                    }

                    chain2.SetClosed( true );
                }

                SHAPE_POLY_SET merged;
                merged.BooleanAdd( chain1, chain2 );

                if( merged.OutlineCount() > 0 )
                    return merged.Outline( 0 );
            }
        }

        // Not single-sided / fallback
        SHAPE_POLY_SET   poly;
        SHAPE_LINE_CHAIN cl = *m_baseLine;

        int amplitude = 0;

        if( m_tuningMode == DIFF_PAIR )
            amplitude = clampedMaxAmplitude + m_diffPairGap / 2 + KiROUND( m_trackWidth );
        else
            amplitude = clampedMaxAmplitude + KiROUND( m_trackWidth / 2.0 );

        poly.OffsetLineChain( *m_baseLine, amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_LOW_DEF, false );

        if( m_tuningMode == DIFF_PAIR && m_baseLineCoupled )
        {
            SHAPE_POLY_SET polyCoupled;
            polyCoupled.OffsetLineChain( *m_baseLineCoupled, amplitude, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                         ARC_LOW_DEF, false );

            poly.ClearArcs();
            polyCoupled.ClearArcs();

            SHAPE_POLY_SET merged;
            merged.BooleanAdd( poly, polyCoupled );

            if( merged.OutlineCount() > 0 )
                return merged.Outline( 0 );
        }

        if( poly.OutlineCount() > 0 )
            return poly.Outline( 0 );
    }

    return SHAPE_LINE_CHAIN();
}


const STRING_ANY_MAP PCB_TUNING_PATTERN::GetProperties() const
{
    STRING_ANY_MAP props = PCB_GENERATOR::GetProperties();

    props.set( "tuning_mode", tuningToString( m_tuningMode ) );
    props.set( "initial_side", sideToString( m_settings.m_initialSide ) );
    props.set( "last_status", statusToString( m_tuningStatus ) );
    props.set( "is_time_domain", m_settings.m_isTimeDomain );

    props.set( "end", m_end );
    props.set( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    props.set( "single_sided", m_settings.m_singleSided );
    props.set( "rounded", m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND );

    props.set_iu( "max_amplitude", m_settings.m_maxAmplitude );
    props.set_iu( "min_amplitude", m_settings.m_minAmplitude );
    props.set_iu( "min_spacing", m_settings.m_spacing );
    props.set_iu( "target_length_min", m_settings.m_targetLength.Min() );
    props.set_iu( "target_length", m_settings.m_targetLength.Opt() );
    props.set_iu( "target_length_max", m_settings.m_targetLength.Max() );
    props.set_iu( "target_delay_min", m_settings.m_targetLengthDelay.Min() );
    props.set_iu( "target_delay", m_settings.m_targetLengthDelay.Opt() );
    props.set_iu( "target_delay_max", m_settings.m_targetLengthDelay.Max() );
    props.set_iu( "target_skew_min", m_settings.m_targetSkew.Min() );
    props.set_iu( "target_skew", m_settings.m_targetSkew.Opt() );
    props.set_iu( "target_skew_max", m_settings.m_targetSkew.Max() );
    props.set_iu( "last_track_width", m_trackWidth );
    props.set_iu( "last_diff_pair_gap", m_diffPairGap );
    props.set_iu( "last_tuning_length", m_tuningLength );

    props.set( "last_netname", m_lastNetName );
    props.set( "override_custom_rules", m_settings.m_overrideCustomRules );

    if( m_baseLine )
        props.set( "base_line", wxAny( *m_baseLine ) );

    if( m_baseLineCoupled )
        props.set( "base_line_coupled", wxAny( *m_baseLineCoupled ) );

    return props;
}


void PCB_TUNING_PATTERN::SetProperties( const STRING_ANY_MAP& aProps )
{
    PCB_GENERATOR::SetProperties( aProps );

    wxString tuningMode;
    aProps.get_to( "tuning_mode", tuningMode );
    m_tuningMode = tuningFromString( tuningMode.utf8_string() );

    wxString side;
    aProps.get_to( "initial_side", side );
    m_settings.m_initialSide = sideFromString( side.utf8_string() );

    wxString status;
    aProps.get_to( "last_status", status );
    m_tuningStatus = statusFromString( status.utf8_string() );

    aProps.get_to( "is_time_domain", m_settings.m_isTimeDomain );

    aProps.get_to( "end", m_end );
    aProps.get_to( "corner_radius_percent", m_settings.m_cornerRadiusPercentage );
    aProps.get_to( "single_sided", m_settings.m_singleSided );
    aProps.get_to( "side", m_settings.m_initialSide );

    bool rounded = false;
    aProps.get_to( "rounded", rounded );
    m_settings.m_cornerStyle = rounded ? PNS::MEANDER_STYLE_ROUND : PNS::MEANDER_STYLE_CHAMFER;

    long long int val;

    aProps.get_to_iu( "target_length", val );
    m_settings.SetTargetLength( val );

    if( aProps.get_to_iu( "target_length_min", val ) )
        m_settings.m_targetLength.SetMin( val );

    if( aProps.get_to_iu( "target_length_max", val ) )
        m_settings.m_targetLength.SetMax( val );

    aProps.get_to_iu( "target_delay", val );
    m_settings.SetTargetLengthDelay( val );

    if( aProps.get_to_iu( "target_delay_min", val ) )
        m_settings.m_targetLengthDelay.SetMin( val );

    if( aProps.get_to_iu( "target_delay_max", val ) )
        m_settings.m_targetLengthDelay.SetMax( val );

    int int_val;

    aProps.get_to_iu( "target_skew", int_val );
    m_settings.SetTargetSkew( int_val );

    if( aProps.get_to_iu( "target_skew_min", int_val ) )
        m_settings.m_targetSkew.SetMin( int_val );

    if( aProps.get_to_iu( "target_skew_max", int_val ) )
        m_settings.m_targetSkew.SetMax( int_val );

    aProps.get_to_iu( "max_amplitude", m_settings.m_maxAmplitude );
    aProps.get_to_iu( "min_amplitude", m_settings.m_minAmplitude );
    aProps.get_to_iu( "min_spacing", m_settings.m_spacing );
    aProps.get_to_iu( "last_track_width", m_trackWidth );
    aProps.get_to_iu( "last_diff_pair_gap", m_diffPairGap );
    aProps.get_to_iu( "last_tuning_length", m_tuningLength );
    aProps.get_to( "override_custom_rules", m_settings.m_overrideCustomRules );

    aProps.get_to( "last_netname", m_lastNetName );

    if( auto baseLine = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line" ) )
        m_baseLine = *baseLine;

    if( auto baseLineCoupled = aProps.get_opt<SHAPE_LINE_CHAIN>( "base_line_coupled" ) )
        m_baseLineCoupled = *baseLineCoupled;

    // Reconstruct m_tuningInfo from loaded length and status
    if( m_tuningLength != 0 )
    {
        wxString statusMessage;

        switch( m_tuningStatus )
        {
        case PNS::MEANDER_PLACER_BASE::TOO_LONG:  statusMessage = _( "too long" );  break;
        case PNS::MEANDER_PLACER_BASE::TOO_SHORT: statusMessage = _( "too short" ); break;
        case PNS::MEANDER_PLACER_BASE::TUNED:     statusMessage = _( "tuned" );     break;
        default:                                  statusMessage = _( "unknown" );   break;
        }

        EDA_UNITS units = m_settings.m_isTimeDomain ? EDA_UNITS::PS : EDA_UNITS::MM;
        wxString  lengthStr = EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, units,
                                                                        (double) m_tuningLength );

        m_tuningInfo.Printf( wxS( "%s (%s)" ), lengthStr, statusMessage );
    }
}


void PCB_TUNING_PATTERN::GetMsgPanelInfo( MSG_PANEL_CONTEXT* aContext,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString      msg;
    NETINFO_ITEM* primaryNet = nullptr;
    NETINFO_ITEM* coupledNet = nullptr;
    PCB_TRACK*    primaryItem = nullptr;
    PCB_TRACK*    coupledItem = nullptr;
    NETCLASS*     netclass = nullptr;
    int           width = 0;
    bool          mixedWidth = false;

    EDA_DATA_TYPE unitType = m_settings.m_isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

    aList.emplace_back( _( "Type" ), GetFriendlyName() );

    for( EDA_ITEM* member : GetItems() )
    {
        if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( member ) )
        {
            if( !primaryNet )
            {
                primaryItem = track;
                primaryNet = track->GetNet();
            }
            else if( !coupledNet && track->GetNet() != primaryNet )
            {
                coupledItem = track;
                coupledNet = track->GetNet();
            }

            if( !netclass )
                netclass = track->GetEffectiveNetClass();

            if( !width )
                width = track->GetWidth();
            else if( width != track->GetWidth() )
                mixedWidth = true;
        }
    }

    if( coupledNet )
    {
        aList.emplace_back( _( "Nets" ), UnescapeString( primaryNet->GetNetname() )
                                         + wxS( ", " )
                                         + UnescapeString( coupledNet->GetNetname() ) );
    }
    else if( primaryNet )
    {
        aList.emplace_back( _( "Net" ), UnescapeString( primaryNet->GetNetname() ) );
    }

    if( netclass )
        aList.emplace_back( _( "Resolved Netclass" ),
                            UnescapeString( netclass->GetHumanReadableName() ) );

    aList.emplace_back( _( "Layer" ), LayerMaskDescribe() );

    if( width && !mixedWidth )
        aList.emplace_back( _( "Width" ), aContext->MessageTextFromValue( width ) );

    BOARD*                       board = GetBoard();
    std::shared_ptr<DRC_ENGINE>& drcEngine = board->GetDesignSettings().m_DRCEngine;
    DRC_CONSTRAINT               constraint;

    // Display full track length (in Pcbnew)
    if( board && primaryItem && primaryItem->GetNetCode() > 0 )
    {
        int    count = 0;
        double trackLen = 0.0;
        double lenPadToDie = 0.0;
        double trackDelay = 0.0;
        double delayPadToDie = 0.0;

        std::tie( count, trackLen, lenPadToDie, trackDelay, delayPadToDie ) = board->GetTrackLength( *primaryItem );

        if( coupledItem && coupledItem->GetNetCode() > 0 )
        {
            double coupledLen = 0.0;
            double coupledLenPadToDie = 0.0;
            double coupledTrackDelay = 0.0;
            double doubledDelayPadToDie = 0.0;

            std::tie( count, coupledLen, coupledLenPadToDie, coupledTrackDelay, doubledDelayPadToDie ) =
                    board->GetTrackLength( *coupledItem );

            if( trackDelay == 0.0 || coupledTrackDelay == 0.0 )
            {
                aList.emplace_back( _( "Routed Lengths" ), aContext->MessageTextFromValue( trackLen ) + wxS( ", " )
                                                                   + aContext->MessageTextFromValue( coupledLen ) );
            }
            else
            {
                aList.emplace_back(
                        _( "Routed Delays" ),
                        aContext->MessageTextFromValue( trackDelay, true, EDA_DATA_TYPE::TIME ) + wxS( ", " )
                                + aContext->MessageTextFromValue( coupledTrackDelay, true, EDA_DATA_TYPE::TIME ) );
            }
        }
        else
        {
            if( trackDelay == 0.0 )
            {
                aList.emplace_back( _( "Routed Length" ), aContext->MessageTextFromValue( trackLen ) );
            }
            else
            {
                aList.emplace_back( _( "Routed Delay" ),
                                    aContext->MessageTextFromValue( trackDelay, true, EDA_DATA_TYPE::TIME ) );
            }
        }

        if( lenPadToDie != 0 && delayPadToDie == 0.0 )
        {
            msg = aContext->MessageTextFromValue( lenPadToDie );
            aList.emplace_back( _( "Pad To Die Length" ), msg );

            msg = aContext->MessageTextFromValue( trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg );
        }
        else if( delayPadToDie > 0.0 )
        {
            msg = aContext->MessageTextFromValue( delayPadToDie, true, EDA_DATA_TYPE::TIME );
            aList.emplace_back( _( "Pad To Die Delay" ), msg );

            msg = aContext->MessageTextFromValue( trackDelay + delayPadToDie, true, EDA_DATA_TYPE::TIME );
            aList.emplace_back( _( "Full Delay" ), msg );
        }
    }

    if( m_tuningMode == DIFF_PAIR_SKEW )
    {
        constraint = drcEngine->EvalRules( SKEW_CONSTRAINT, primaryItem, coupledItem, GetLayer() );

        if( constraint.IsNull() || m_settings.m_overrideCustomRules )
        {
            msg = aContext->MessageTextFromValue( m_settings.m_targetSkew.Opt() );

            aList.emplace_back( wxString::Format( _( "Target Skew: %s" ), msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = aContext->MessageTextFromMinOptMax( constraint.GetValue() );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( _( "Skew Constraints: %s" ), msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
    else
    {
        constraint = drcEngine->EvalRules( LENGTH_CONSTRAINT, primaryItem, coupledItem, GetLayer() );

        if( constraint.IsNull() || m_settings.m_overrideCustomRules )
        {
            wxString caption;

            if( m_settings.m_isTimeDomain )
            {
                caption = _( "Target Delay: %s" );
                msg = aContext->MessageTextFromValue( static_cast<double>( m_settings.m_targetLengthDelay.Opt() ), true,
                                                    EDA_DATA_TYPE::TIME );
            }
            else
            {
                caption = _( "Target Length: %s" );
                msg = aContext->MessageTextFromValue( static_cast<double>( m_settings.m_targetLength.Opt() ) );
            }

            aList.emplace_back( wxString::Format( caption, msg ),
                                wxString::Format( _( "(from tuning pattern properties)" ) ) );
        }
        else
        {
            msg = aContext->MessageTextFromMinOptMax( constraint.GetValue(), unitType );

            wxString caption = m_settings.m_isTimeDomain ? _( "Delay Constraints: %s" ) : _( "Length Constraints: %s" );

            if( !msg.IsEmpty() )
            {
                aList.emplace_back( wxString::Format( caption, msg ),
                                    wxString::Format( _( "(from %s)" ), constraint.GetName() ) );
            }
        }
    }
}


const wxString PCB_TUNING_PATTERN::DISPLAY_NAME = _HKI( "Tuning Pattern" );
const wxString PCB_TUNING_PATTERN::GENERATOR_TYPE = wxS( "tuning_pattern" );


static struct PCB_TUNING_PATTERN_DESC
{
    PCB_TUNING_PATTERN_DESC()
    {
        ENUM_MAP<LENGTH_TUNING_MODE>::Instance()
                .Map( LENGTH_TUNING_MODE::SINGLE, _HKI( "Single track" ) )
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR, _HKI( "Differential pair" ) )
                .Map( LENGTH_TUNING_MODE::DIFF_PAIR_SKEW, _HKI( "Diff pair skew" ) );

        ENUM_MAP<PNS::MEANDER_SIDE>::Instance()
                .Map( PNS::MEANDER_SIDE_LEFT, _HKI( "Left" ) )
                .Map( PNS::MEANDER_SIDE_RIGHT, _HKI( "Right" ) )
                .Map( PNS::MEANDER_SIDE_DEFAULT, _HKI( "Default" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TUNING_PATTERN );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TUNING_PATTERN, PCB_GENERATOR> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TUNING_PATTERN, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TUNING_PATTERN ), TYPE_HASH( PCB_GENERATOR ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TUNING_PATTERN ), TYPE_HASH( BOARD_ITEM ) );

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        auto layer = new PROPERTY_ENUM<PCB_TUNING_PATTERN, PCB_LAYER_ID>(
                _HKI( "Layer" ), &PCB_TUNING_PATTERN::SetLayer, &PCB_TUNING_PATTERN::GetLayer );
        layer->SetChoices( layerEnum.Choices() );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layer );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Width" ),
                                     &PCB_TUNING_PATTERN::SetWidth, &PCB_TUNING_PATTERN::GetWidth,
                                     PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, int>( _HKI( "Net" ),
                                     &PCB_TUNING_PATTERN::SetNetCode, &PCB_TUNING_PATTERN::GetNetCode, PT_NET ) );

        const wxString groupTechLayers = _HKI( "Technical Layers" );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>( _HKI( "Soldermask" ),
                                     &PCB_TUNING_PATTERN::SetHasSolderMask, &PCB_TUNING_PATTERN::HasSolderMask ),
                             groupTechLayers );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>( _HKI( "Soldermask Margin Override" ),
                                     &PCB_TUNING_PATTERN::SetLocalSolderMaskMargin,
                                     &PCB_TUNING_PATTERN::GetLocalSolderMaskMargin,
                                     PROPERTY_DISPLAY::PT_SIZE ),
                             groupTechLayers );

        const wxString groupTab = _HKI( "Pattern Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "End X" ),
                                     &PCB_TUNING_PATTERN::SetEndX, &PCB_TUNING_PATTERN::GetEndX,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "End Y" ),
                                     &PCB_TUNING_PATTERN::SetEndY, &PCB_TUNING_PATTERN::GetEndY,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, LENGTH_TUNING_MODE>( _HKI( "Tuning Mode" ),
                                     NO_SETTER( PCB_TUNING_PATTERN, LENGTH_TUNING_MODE ),
                                     &PCB_TUNING_PATTERN::GetTuningMode ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Min Amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMinAmplitude, &PCB_TUNING_PATTERN::GetMinAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Max Amplitude" ),
                                     &PCB_TUNING_PATTERN::SetMaxAmplitude, &PCB_TUNING_PATTERN::GetMaxAmplitude,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TUNING_PATTERN, PNS::MEANDER_SIDE>( _HKI( "Initial Side" ),
                                     &PCB_TUNING_PATTERN::SetInitialSide, &PCB_TUNING_PATTERN::GetInitialSide ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Min Spacing" ),
                                     &PCB_TUNING_PATTERN::SetSpacing, &PCB_TUNING_PATTERN::GetSpacing,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Corner Radius %" ),
                                     &PCB_TUNING_PATTERN::SetCornerRadiusPercentage,
                                     &PCB_TUNING_PATTERN::GetCornerRadiusPercentage,
                                     PROPERTY_DISPLAY::PT_DEFAULT, ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab );

        auto isSkew =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_TUNING_PATTERN* pattern = dynamic_cast<PCB_TUNING_PATTERN*>( aItem ) )
                        return pattern->GetTuningMode() == DIFF_PAIR_SKEW;

                    return false;
                };

        auto isTimeDomain = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_TUNING_PATTERN* pattern = dynamic_cast<PCB_TUNING_PATTERN*>( aItem ) )
                return pattern->GetSettings().m_isTimeDomain;

            return false;
        };

        auto isLengthIsSpaceDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return !isSkew( aItem ) && !isTimeDomain( aItem );
        };

        auto isLengthIsTimeDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return !isSkew( aItem ) && isTimeDomain( aItem );
        };

        auto isSkewIsSpaceDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return isSkew( aItem ) && !isTimeDomain( aItem );
        };

        auto isSkewIsTimeDomain = [&]( INSPECTABLE* aItem ) -> bool
        {
            return isSkew( aItem ) && isTimeDomain( aItem );
        };

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>( _HKI( "Target Length" ),
                                     &PCB_TUNING_PATTERN::SetTargetLength, &PCB_TUNING_PATTERN::GetTargetLength,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( isLengthIsSpaceDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, std::optional<int>>( _HKI( "Target Delay" ),
                                     &PCB_TUNING_PATTERN::SetTargetDelay, &PCB_TUNING_PATTERN::GetTargetDelay,
                                     PROPERTY_DISPLAY::PT_TIME, ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab )
                .SetAvailableFunc( isLengthIsTimeDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Target Skew" ),
                                     &PCB_TUNING_PATTERN::SetTargetSkew, &PCB_TUNING_PATTERN::GetTargetSkew,
                                     PROPERTY_DISPLAY::PT_SIZE, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupTab )
                .SetAvailableFunc( isSkewIsSpaceDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, int>( _HKI( "Target Skew Delay" ),
                                     &PCB_TUNING_PATTERN::SetTargetSkewDelay, &PCB_TUNING_PATTERN::GetTargetSkewDelay,
                                     PROPERTY_DISPLAY::PT_TIME, ORIGIN_TRANSFORMS::NOT_A_COORD ),
                             groupTab )
                .SetAvailableFunc( isSkewIsTimeDomain );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>( _HKI( "Override Custom Rules" ),
                                     &PCB_TUNING_PATTERN::SetOverrideCustomRules,
                                     &PCB_TUNING_PATTERN::GetOverrideCustomRules ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>( _HKI( "Single-sided" ),
                                     &PCB_TUNING_PATTERN::SetSingleSided, &PCB_TUNING_PATTERN::IsSingleSided ),
                             groupTab );

        propMgr.AddProperty( new PROPERTY<PCB_TUNING_PATTERN, bool>( _HKI( "Rounded" ),
                                     &PCB_TUNING_PATTERN::SetRounded, &PCB_TUNING_PATTERN::IsRounded ),
                             groupTab );
    }
} _PCB_TUNING_PATTERN_DESC;

ENUM_TO_WXANY( LENGTH_TUNING_MODE )
ENUM_TO_WXANY( PNS::MEANDER_SIDE )

static GENERATORS_MGR::REGISTER<PCB_TUNING_PATTERN> registerMe;

// Also register under the 7.99 name
template <typename T>
struct REGISTER_LEGACY_TUNING_PATTERN
{
    REGISTER_LEGACY_TUNING_PATTERN()
    {
        GENERATORS_MGR::Instance().Register( wxS( "meanders" ), T::DISPLAY_NAME,
                                             []()
                                             {
                                                 return new T;
                                             } );
    }
};

static REGISTER_LEGACY_TUNING_PATTERN<PCB_TUNING_PATTERN> registerMeToo;
