/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_properties_panel.h"

#include <font/fontconfig.h>
#include <font/kicad_font_name.h>
#include <frame_type.h>
#include <pgm_base.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <properties/property_mgr.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <board.h>
#include <properties/pg_factory.h>
#include <properties/property.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <generators/pcb_tuning_pattern.h>
#include <pad.h>
#include <footprint.h>
#include <pcb_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <core/ki_assert.h>

static const wxString MISSING_FIELD_SENTINEL = wxS( "\uE000" );

class PCB_FOOTPRINT_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    PCB_FOOTPRINT_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( FOOTPRINT ); }
    size_t BaseHash() const override { return TYPE_HASH( FOOTPRINT ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override
    {
        return PROPERTY_BASE::Writeable( aObject );
    }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        FOOTPRINT* footprint = reinterpret_cast<FOOTPRINT*>( obj );
        PCB_FIELD* field = footprint->GetField( m_name );

        wxString variantName;

        if( footprint->GetBoard() )
            variantName = footprint->GetBoard()->GetCurrentVariant();

        if( !variantName.IsEmpty() )
        {
            // Store the value as a variant override
            FOOTPRINT_VARIANT* variant = footprint->AddVariant( variantName );

            if( variant )
                variant->SetFieldValue( m_name, value );
        }
        else
        {
            // Set the base field value
            if( !field )
            {
                PCB_FIELD* newField = new PCB_FIELD( footprint, FIELD_T::USER, m_name );
                newField->SetText( value );
                footprint->Add( newField );
            }
            else
            {
                field->SetText( value );
            }
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const FOOTPRINT* footprint = reinterpret_cast<const FOOTPRINT*>( obj );
        PCB_FIELD* field = footprint->GetField( m_name );

        if( field )
        {
            wxString variantName;

            if( footprint->GetBoard() )
                variantName = footprint->GetBoard()->GetCurrentVariant();

            wxString text;

            if( !variantName.IsEmpty() )
                text = footprint->GetFieldValueForVariant( variantName, m_name );
            else
                text = field->GetText();

            return wxAny( text );
        }
        else
        {
            return wxAny( MISSING_FIELD_SENTINEL );
        }
    }

private:
    wxString m_name;
};

std::set<wxString> PCB_PROPERTIES_PANEL::m_currentFieldNames;


PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, PCB_BASE_EDIT_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();

    registerEditors();
}


PCB_PROPERTIES_PANEL::~PCB_PROPERTIES_PANEL()
{
    // The toolkit state releases the editors of the grid (the shared editors forget the frame)
}


const SELECTION& PCB_PROPERTIES_PANEL::getSelection( SELECTION& aFallbackSelection )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() && m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        if( BOARD* board = m_frame->GetBoard() )
        {
            if( FOOTPRINT* footprint = board->GetFirstFootprint() )
            {
                aFallbackSelection.Clear();
                aFallbackSelection.Add( footprint );
                return aFallbackSelection;
            }
        }
    }

    return selection;
}


EDA_ITEM* PCB_PROPERTIES_PANEL::getFrontItem()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    return selection.Empty() ? nullptr : selection.Front();
}


void PCB_PROPERTIES_PANEL::UpdateData()
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    // TODO perhaps it could be called less often? use PROPERTIES_TOOL and catch MODEL_RELOAD?
    updateLists( board );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void PCB_PROPERTIES_PANEL::AfterCommit()
{
    if( !m_frame->GetBoard() )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    rebuildProperties( selection );
}


void PCB_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    m_currentFieldNames.clear();

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            continue;

        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

        for( PCB_FIELD* field : footprint->GetFields() )
        {
            KI_CHECK2( field, continue );

            m_currentFieldNames.insert( field->GetCanonicalName() );
        }
    }

    const wxString groupFields = _HKI( "Fields" );

    // Make sure value comes immediately after reference.  (Reference is invariant, so was added by
    // FOOTPRINT_DESC().  We *could* still add it here, but then the whole Fields section comes at
    // the end, which isn't ideal.)
    m_propMgr.AddProperty( new PCB_FOOTPRINT_FIELD_PROPERTY( _HKI( "Value" ) ), groupFields );

    for( const wxString& name : m_currentFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( FOOTPRINT ), name ) )
        {
            m_propMgr.AddProperty( new PCB_FOOTPRINT_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc( [name]( INSPECTABLE* )
                                       {
                                           return PCB_PROPERTIES_PANEL::m_currentFieldNames.count( name );
                                       } );
        }
    }

    PROPERTIES_PANEL::rebuildProperties( aSelection );
}


TOOLKIT_PG_PROPERTY* PCB_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    if( aProperty->TypeHash() == TYPE_HASH( PCB_LAYER_ID ) )
    {
        KI_ASSERT( aProperty->HasChoices() );

        const PROPERTY_CHOICES&               canonicalLayers = aProperty->Choices();
        std::vector<std::pair<wxString, int>> boardLayers;

        for( int ii = 0; ii < (int) aProperty->Choices().GetCount(); ++ii )
        {
            int layer = canonicalLayers.GetValue( ii );

            boardLayers.emplace_back( m_frame->GetBoard()->GetLayerName( ToLAYER_ID( layer ) ),
                                      canonicalLayers.GetValue( ii ) );
        }

        PCB_BASE_EDIT_FRAME* frame = m_frame;

        return PGColorEnumPropertyFactory( aProperty, boardLayers,
                                           [frame]( int aValue )
                                           {
                                               return frame->GetColorSettings()->GetColor(
                                                       ToLAYER_ID( aValue ) );
                                           } );
    }

    TOOLKIT_PG_PROPERTY* prop = PGPropertyFactory( aProperty, m_frame );

    if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        PGSetEditor( prop, PG_EDITOR::FPID, m_frame );
    else if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        PGSetEditor( prop, PG_EDITOR::URL, m_frame );

    return prop;
}


PROPERTY_BASE* PCB_PROPERTIES_PANEL::getPropertyByName( const wxString& aName ) const
{
    EDA_ITEM* item = const_cast<PCB_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsBOARD_ITEM() )
        return nullptr;

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( item );

    KI_CHECK_MSG( firstItem, nullptr, wxT( "getPropertyByName for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ), aName );
    KI_CHECK_MSG( property, nullptr, wxT( "getPropertyByName for a property not found on the selected item!" ) );

    return property;
}


bool PCB_PROPERTIES_PANEL::valueChanging( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents > 0 )
        return true;

    EDA_ITEM* item = getFrontItem();

    PROPERTY_BASE* property = getPropertyByName( aName );
    KI_CHECK( property, true );
    KI_CHECK( item, true );

    wxVariant newValue = aValue;

    if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), item ) )
    {
        wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                              validationFailure->get()->Format( m_frame ) );
        m_frame->ShowInfoBarError( errorMsg );
        return false;
    }

    return true;
}


void PCB_PROPERTIES_PANEL::valueChanged( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents > 0 )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    KI_CHECK( getPropertyByName( aName ), /* void */ );

    wxVariant newValue = aValue;
    BOARD_COMMIT changes( m_frame );

    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *item ), aName );
        KI_CHECK( property, /* void */ );

        if( item->Type() == PCB_TABLECELL_T )
            changes.Modify( item->GetParent(), nullptr, RECURSE_MODE::NO_RECURSE );
        else if( item->Type() == PCB_GENERATOR_T )
            changes.Modify( item, nullptr, RECURSE_MODE::RECURSE );
        else
            changes.Modify( item, nullptr, RECURSE_MODE::NO_RECURSE );

        // In the PCB Editor, we generally restrict pad movement to the footprint (like dragging)
        if( item->Type() == PCB_PAD_T && m_frame
                && m_frame->IsType( FRAME_PCB_EDITOR )
                && !m_frame->GetPcbNewSettings()->m_AllowFreePads
                && ( aName == _HKI( "Position X" )
                     || aName == _HKI( "Position Y" ) ) )
        {
            PAD* pad = static_cast<PAD*>( item );
            FOOTPRINT* fp = pad->GetParentFootprint();

            if( fp )
            {
                VECTOR2I oldPos = pad->GetPosition();
                VECTOR2I newPos = oldPos;

                if( aName == _HKI( "Position X" ) )
                    newPos.x = (int) newValue.GetLong();
                else
                    newPos.y = (int) newValue.GetLong();

                VECTOR2I delta = newPos - oldPos;

                if( delta.x != 0 || delta.y != 0 )
                {
                    changes.Modify( fp );
                    fp->Move( delta );
                }
            }

            continue;
        }

        // Handle variant-aware boolean properties for footprints
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
            wxString   variantName;

            if( footprint->GetBoard() )
                variantName = footprint->GetBoard()->GetCurrentVariant();

            if( !variantName.IsEmpty() )
            {
                wxString propName = aName;

                if( propName == _HKI( "Do not Populate" )
                    || propName == _HKI( "Exclude From Bill of Materials" )
                    || propName == _HKI( "Exclude From Position Files" ) )
                {
                    FOOTPRINT_VARIANT* variant = footprint->GetVariant( variantName );

                    if( !variant )
                        variant = footprint->AddVariant( variantName );

                    if( variant )
                    {
                        bool boolValue = newValue.GetBool();

                        if( propName == _HKI( "Do not Populate" ) )
                            variant->SetDNP( boolValue );
                        else if( propName == _HKI( "Exclude From Bill of Materials" ) )
                            variant->SetExcludedFromBOM( boolValue );
                        else if( propName == _HKI( "Exclude From Position Files" ) )
                            variant->SetExcludedFromPosFiles( boolValue );

                        continue;
                    }
                }
            }
        }

        item->Set( property, newValue );
    }

    changes.Push( _( "Edit Properties" ) );

    m_frame->RefreshCanvas();

    // Perform grid updates as necessary based on value change
    AfterCommit();

    // PointEditor may need to update if locked/unlocked
    if( aName == _HKI( "Locked" ) )
        m_frame->GetToolManager()->ProcessEvent( EVENTS::SelectedEvent );
}


void PCB_PROPERTIES_PANEL::updateLists( const BOARD* aBoard )
{
    PROPERTY_CHOICES layersAll;
    PROPERTY_CHOICES layersCu;
    PROPERTY_CHOICES nets;
    PROPERTY_CHOICES fonts;

    // Regenerate all layers
    for( PCB_LAYER_ID layer : aBoard->GetEnabledLayers().UIOrder() )
        layersAll.Add( LSET::Name( layer ), layer );

    for( PCB_LAYER_ID layer : LSET( aBoard->GetEnabledLayers() & LSET::AllCuMask() ).UIOrder() )
        layersCu.Add( LSET::Name( layer ), layer );

    m_propMgr.GetProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) )->SetChoices( layersAll );
    m_propMgr.GetProperty( TYPE_HASH( PCB_SHAPE ), _HKI( "Layer" ) )->SetChoices( layersAll );

    // Copper only properties
    m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PAD ), _HKI( "Bottom Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PAD ), _HKI( "Top Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Top" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Bottom" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Bottom Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Top Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_TUNING_PATTERN ), _HKI( "Layer" ) )->SetChoices( layersCu );

    // Regenerate nets

    std::vector<std::pair<wxString, int>> netNames;
    netNames.reserve( aBoard->GetNetInfo().NetsByNetcode().size() );

    for( const auto& [ netCode, netInfo ] : aBoard->GetNetInfo().NetsByNetcode() )
        netNames.emplace_back( UnescapeString( netInfo->GetNetname() ), netCode );

    std::sort( netNames.begin(), netNames.end(),
               []( const auto& a, const auto& b )
               {
                   return a.first.CmpNoCase( b.first ) < 0;
               } );

    for( const auto& [ netName, netCode ] : netNames )
        nets.Add( netName, netCode );

    auto netProperty = m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );
    netProperty->SetChoices( nets );

    auto tuningNet = m_propMgr.GetProperty( TYPE_HASH( PCB_TUNING_PATTERN ), _HKI( "Net" ) );
    tuningNet->SetChoices( nets );
}


bool PCB_PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    // For FOOTPRINT variant-aware boolean properties, return variant-specific values
    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        FOOTPRINT*      footprint = static_cast<FOOTPRINT*>( aItem );
        const wxString& propName = aProperty->Name();
        wxString        variantName;

        if( footprint->GetBoard() )
            variantName = footprint->GetBoard()->GetCurrentVariant();

        if( propName == _HKI( "Do not Populate" ) )
        {
            aValue = wxVariant( footprint->GetDNPForVariant( variantName ) );
            return true;
        }
        else if( propName == _HKI( "Exclude From Bill of Materials" ) )
        {
            aValue = wxVariant( footprint->GetExcludedFromBOMForVariant( variantName ) );
            return true;
        }
        else if( propName == _HKI( "Exclude From Position Files" ) )
        {
            aValue = wxVariant( footprint->GetExcludedFromPosFilesForVariant( variantName ) );
            return true;
        }
    }

    return PROPERTIES_PANEL::getItemValue( aItem, aProperty, aValue );
}
