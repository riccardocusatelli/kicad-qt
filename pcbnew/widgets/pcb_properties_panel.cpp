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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <tools/edit_tool.h>
#include <tools/constraint_edit_tool.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>
#include <constraints/pcb_constraint.h>
#include <drill/drill_chart_model.h>
#include <drill/drill_enumerator.h>
#include <math/util.h>
#include <base_units.h>
#include <pcb_generated_table.h>
#include <pcb_drill_map.h>
#include <pcb_shape.h>
#include <eda_units.h>
#include <properties/property_mgr.h>
#include <board_design_settings.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <board.h>
#include <properties/pg_factory.h>
#include <properties/property.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <generators/pcb_tuning_pattern.h>
#include <generators/pcb_via_stitch.h>
#include <pad.h>
#include <footprint.h>
#include <pcb_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <core/ki_assert.h>

#include <confirm.h>

#include <cmath>
#include <memory>
#include <vector>


PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, PCB_BASE_EDIT_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() ),
        m_scaleConfirmPending( false )
{
    addCategoryButton( _HKI( "Custom Properties" ), _( "Add Custom Property" ), BITMAPS::small_plus,
                       [this]()
                       {
                           addBlankCustomProperty();
                       } );

    addCategoryButton( _HKI( "Fields" ), _( "Add Field" ), BITMAPS::small_plus,
                       [this]()
                       {
                           addBlankField();
                       } );

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


bool PCB_PROPERTIES_PANEL::isKeyNameInUse( const wxString& aName ) const
{
    EDA_ITEM* item = const_cast<PCB_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item )
        return false;

    return m_propMgr.GetProperty( item, aName ) != nullptr;
}


void PCB_PROPERTIES_PANEL::onKeyRenamed( const wxString& aOldName, const wxString& aNewName )
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    BOARD_COMMIT changes( m_frame );
    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( boardItem->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem );
            PCB_FIELD* field     = footprint->GetField( aOldName );

            if( field && !field->IsMandatory() )
            {
                changes.Modify( footprint, nullptr, RECURSE_MODE::NO_RECURSE );
                field->SetName( aNewName );
                continue;
            }
        }

        wxString value;

        if( boardItem->GetCustomProperty( aOldName, value ) )
        {
            changes.Modify( boardItem, nullptr, RECURSE_MODE::NO_RECURSE );
            boardItem->RemoveCustomProperty( aOldName );
            boardItem->SetCustomProperty( aNewName, value );
        }
    }

    changes.Push( _( "Rename Property" ) );

    AfterCommit();
}


void PCB_PROPERTIES_PANEL::addBlankField()
{
    SELECTION fallbackSelection;
    SELECTION editableSelection = filterOutReadOnlyGenChildren( getSelection( fallbackSelection ) );

    settlePendingLabelEdit();

    if( editableSelection.Empty() )
        return;

    // Pick a unique untranslated placeholder name that doesn't collide with an existing field.
    wxString name;

    for( int n = 0; ; ++n )
    {
        name   = GetUserFieldName( n, UNTRANSLATED );
        bool used = false;

        for( EDA_ITEM* item : editableSelection )
        {
            if( item->Type() == PCB_FOOTPRINT_T && static_cast<FOOTPRINT*>( item )->HasField( name ) )
            {
                used = true;
                break;
            }
        }

        if( !used )
            break;
    }

    BOARD_COMMIT changes( m_frame );
    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* item : editableSelection )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            continue;

        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
        PCB_FIELD* field     = new PCB_FIELD( footprint, FIELD_T::USER, name );

        field->SetText( wxEmptyString );
        field->SetVisible( false );
        changes.Modify( footprint, nullptr, RECURSE_MODE::NO_RECURSE );
        footprint->Add( field );
    }

    changes.Push( _( "Add Field" ) );
    AfterCommit();

    m_pendingNewKey = name;

    beginLabelEdit( name, true );
}


void PCB_PROPERTIES_PANEL::addBlankCustomProperty()
{
    SELECTION fallbackSelection;
    SELECTION editableSelection = filterOutReadOnlyGenChildren( getSelection( fallbackSelection ) );


    settlePendingLabelEdit();

    if( editableSelection.Empty() )
        return;

    wxString name;

    for( int n = 0; ; ++n )
    {
        name   = wxString::Format( wxS( "Property%d" ), n );
        bool used = false;

        for( EDA_ITEM* item : editableSelection )
        {
            wxString dummy;

            if( item->GetCustomProperty( name, dummy ) )
            {
                used = true;
                break;
            }
        }

        if( !used )
            break;
    }

    BOARD_COMMIT changes( m_frame );
    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* item : editableSelection )
    {
        if( item->IsBOARD_ITEM() )
        {
            changes.Modify( item, nullptr, RECURSE_MODE::NO_RECURSE );
            item->SetCustomProperty( name, wxEmptyString );
        }
    }

    changes.Push( _( "Add Custom Property" ) );
    AfterCommit();

    m_pendingNewKey = name;

    beginLabelEdit( name, true );
}


void PCB_PROPERTIES_PANEL::removeField( const wxString& aName )
{
    SELECTION fallbackSelection;
    SELECTION editableSelection = filterOutReadOnlyGenChildren( getSelection( fallbackSelection ) );

    BOARD_COMMIT changes( m_frame );
    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* item : editableSelection )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            continue;

        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
        PCB_FIELD* field     = footprint->GetField( aName );

        if( field && !field->IsMandatory() )
        {
            // BOARD_COMMIT's own field removal path only hides the field (e.g. when a user presses the delete key),
            // we want to actually delete a custom field from this explicit menu action
            changes.Modify( footprint, nullptr, RECURSE_MODE::NO_RECURSE );
            footprint->Remove( field );
        }
    }

    changes.Push( _( "Remove Field" ) );
    AfterCommit();
}


void PCB_PROPERTIES_PANEL::removeCustomProperty( const wxString& aName )
{
    SELECTION fallbackSelection;
    SELECTION editableSelection = filterOutReadOnlyGenChildren( getSelection( fallbackSelection ) );

    BOARD_COMMIT changes( m_frame );
    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* item : editableSelection )
    {
        if( item->IsBOARD_ITEM() )
        {
            changes.Modify( item, nullptr, RECURSE_MODE::NO_RECURSE );
            item->RemoveCustomProperty( aName );
        }
    }

    changes.Push( _( "Remove Custom Property" ) );
    AfterCommit();
}


void PCB_PROPERTIES_PANEL::onNewItemLeftBlank( const wxString& aKey )
{
    // Note: currently assuming that any newly-created item from the panel is either a field or
    // a custom property because those are the types we currently support.
    if( EDA_ITEM* item = getFrontItem();
        item && item->Type() == PCB_FOOTPRINT_T && static_cast<FOOTPRINT*>( item )->HasField( aKey ) )
    {
        removeField( aKey );
    }
    else
    {
        removeCustomProperty( aKey );
    }
}


SELECTION PCB_PROPERTIES_PANEL::filterOutReadOnlyGenChildren( const SELECTION& aSelection )
{
    SELECTION filtered;
    filtered.SetIsHover( aSelection.IsHover() );

    for( EDA_ITEM* item : aSelection )
    {
        if( item->IsBOARD_ITEM() )
        {
            EDA_GROUP* parent = static_cast<BOARD_ITEM*>( item )->GetParentGroup();

            if( parent && parent->AsEdaItem()->Type() == PCB_GENERATOR_T
                && static_cast<PCB_GENERATOR*>( parent->AsEdaItem() )->ChildrenAreReadOnly() )
            {
                continue;
            }
        }

        filtered.Add( item );
    }

    return filtered;
}


void PCB_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aRawSelection )
{
    // Strip read-only generator children (like stitch vias)
    SELECTION        editableSelection = filterOutReadOnlyGenChildren( aRawSelection );
    const SELECTION& aSelection = editableSelection;

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
    // OwnerHash is the class that registered the property.  Routed PCB_ARC items inherit
    // PCB_TRACK::Width, so this catches track arcs without changing unrelated "Width" properties.
    else if( aProperty->OwnerHash() == TYPE_HASH( PCB_TRACK ) && aProperty->Name() == _HKI( "Width" ) )
        setTrackWidthEditor( prop );

    return prop;
}


PROPERTY_BASE* PCB_PROPERTIES_PANEL::getPropertyByName( const wxString& aName ) const
{
    EDA_ITEM* item = const_cast<PCB_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsBOARD_ITEM() )
        return nullptr;

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( item );

    KI_CHECK_MSG( firstItem, nullptr, wxT( "getPropertyByName for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( firstItem, aName );
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
        m_ValueChangeVetoed = true;
        return false;
    }

    // Scaling a footprint that has pads is dangerous (the physical
    // part keeps its original size), so confirm before applying.
    const wxString& propName = aName;

    if( propName == _HKI( "Scale X" ) || propName == _HKI( "Scale Y" ) )
    {
        const double newScale = newValue.GetDouble();

        // Zero, negative, and non-finite scales make the footprint transform degenerate.
        if( !std::isfinite( newScale ) || newScale <= 0.0 )
        {
            m_frame->ShowInfoBarError( _( "Scale must be a positive number." ) );
            m_ValueChangeVetoed = true;
            return false;
        }

        if( newScale != 1.0 )
        {
            SELECTION        fallbackSelection;
            const SELECTION& selection = getSelection( fallbackSelection );
            int              fpWithPads = 0;

            for( EDA_ITEM* edaItem : selection )
            {
                if( edaItem->IsBOARD_ITEM() && static_cast<BOARD_ITEM*>( edaItem )->Type() == PCB_FOOTPRINT_T
                    && !static_cast<FOOTPRINT*>( edaItem )->Pads().empty() )
                {
                    fpWithPads++;
                }
            }

            if( fpWithPads > 0 )
            {
                // A modal dialog must not be shown from this synchronous grid handler.
                // Veto the edit and re-drive it from the main loop once confirmed.
                m_ValueChangeVetoed = true;

                if( m_scaleConfirmPending )
                    return false;

                m_scaleConfirmPending = true;

                wxString msg = wxString::Format( _( "%d footprint(s) in the selection have pads. Scaling changes "
                                                    "the drawn pad positions but not the physical part. Continue?" ),
                                                 fpWithPads );

                wxString scaleProp = aName;

                m_frame->CallAfter(
                        [this, msg, scaleProp, newValue]()
                        {
                            if( IsOK( m_frame, msg ) )
                                applyConfirmedScale( scaleProp, newValue );

                            m_scaleConfirmPending = false;
                        } );

                return false;
            }
        }
    }

    return true;
}


void PCB_PROPERTIES_PANEL::applyConfirmedScale( const wxString& aPropName, const wxVariant& aValue )
{
    // Re-drive the normal changed path now that we are back in the main loop.
    valueChanged( aPropName, aValue );
}


void PCB_PROPERTIES_PANEL::valueChanged( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents > 0 )
        return;

    SELECTION fallbackSelection;
    SELECTION rawSelection = getSelection( fallbackSelection );

    // Strip read-only generator children, last ditch sanity check
    SELECTION filtered = filterOutReadOnlyGenChildren( rawSelection );

    if( filtered.Empty() )
        return;

    const SELECTION& selection = filtered;

    KI_CHECK( getPropertyByName( aName ), /* void */ );

    wxVariant newValue = aValue;
    BOARD_COMMIT changes( m_frame );

    PROPERTY_COMMIT_HANDLER handler( &changes );

    // Multi-footprint scale: rescale each footprint around the selection's
    // geometric (bbox) center instead of around its own anchor. Single-fp
    // edits go through the regular setter path (anchored at the fp itself).
    const wxString& propName = aName;
    const bool     isScaleX = ( propName == _HKI( "Scale X" ) );
    const bool     isScaleY = ( propName == _HKI( "Scale Y" ) );
    int            fpInSelection = 0;
    BOX2I          selectionBBox;

    if( isScaleX || isScaleY )
    {
        for( EDA_ITEM* edaItem : selection )
        {
            if( edaItem->IsBOARD_ITEM() && static_cast<BOARD_ITEM*>( edaItem )->Type() == PCB_FOOTPRINT_T )
            {
                fpInSelection++;
                selectionBBox.Merge( static_cast<FOOTPRINT*>( edaItem )->GetBoundingBox() );
            }
        }
    }

    const bool     useSelectionCenter = fpInSelection > 1;
    const VECTOR2I selectionCenter = useSelectionCenter ? selectionBBox.GetCenter() : VECTOR2I( 0, 0 );

    // Driving length constraints touched by this edit solved again after commit lands
    std::vector<PCB_CONSTRAINT*> drivingConstraints;

    std::set<PCB_GENERATED_TABLE*> generatedTables;

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( item, aName );
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
                if( propName == _HKI( "Do not Populate" )
                    || propName == _HKI( "Exclude From Bill of Materials" )
                    || propName == _HKI( "Exclude From Simulation" )
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
                        else if( propName == _HKI( "Exclude From Simulation" ) )
                            variant->SetExcludedFromSim( boolValue );
                        else if( propName == _HKI( "Exclude From Position Files" ) )
                            variant->SetExcludedFromPosFiles( boolValue );

                        continue;
                    }
                }
            }
        }

        if( useSelectionCenter && item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );
            double     newScale = newValue.GetDouble();
            double     relSx = isScaleX ? newScale / fp->GetScaleX() : 1.0;
            double     relSy = isScaleY ? newScale / fp->GetScaleY() : 1.0;

            fp->RescaleAroundPoint( selectionCenter, relSx, relSy );
            continue;
        }

        // Value mode and Driving value live partly in a board level constraint not item scoped
        // so route them through this commit for a shared undo step
        if( BaseType( item->Type() ) == PCB_DIMENSION_T )
        {
            PCB_DIMENSION_BASE* dim = static_cast<PCB_DIMENSION_BASE*>( item );
            BOARD*              board = m_frame->GetBoard();

            if( propName == _HKI( "Value Mode" ) )
            {
                DIM_VALUE_MODE mode = static_cast<DIM_VALUE_MODE>( newValue.GetLong() );

                // Seed from current display value so a mode switch alone does not move geometry
                std::optional<int> length;

                if( mode == DIM_VALUE_MODE::DRIVING )
                {
                    PCB_CONSTRAINT* existing = FindDimensionLengthConstraint( board, dim );

                    if( existing && existing->GetValue() )
                        length = KiROUND( *existing->GetValue() );
                    else
                        length = dim->GetMeasuredValue();
                }

                // Arbitrary keeps measured text until edited
                std::optional<wxString> overrideText;

                if( mode == DIM_VALUE_MODE::ARBITRARY && !dim->GetOverrideTextEnabled() )
                    overrideText = dim->GetValueText();

                PCB_CONSTRAINT* driving = SetDimensionValueMode(
                        board, dim, mode, length, overrideText,
                        [&]( BOARD_ITEM* aItem )
                        {
                            changes.Modify( aItem );
                        },
                        [&]( BOARD_ITEM* aItem )
                        {
                            changes.Add( aItem );
                        },
                        [&]( BOARD_ITEM* aItem )
                        {
                            changes.Remove( aItem );
                        } );

                if( driving )
                    drivingConstraints.push_back( driving );

                continue;
            }

            if( propName == _HKI( "Value" ) && dim->GetValueMode() == DIM_VALUE_MODE::DRIVING )
            {
                // Parsed in dimension own units to match display validator already blocked
                // non positive entries other selected dims failing parse left unchanged
                double iu = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, dim->GetUnits(),
                                                                       newValue.GetString() );

                if( iu > 0.0 )
                {
                    if( PCB_CONSTRAINT* driving = FindDimensionLengthConstraint( board, dim ) )
                    {
                        changes.Modify( driving );
                        driving->SetValue( KiROUND( iu ) );
                        drivingConstraints.push_back( driving );
                    }
                }

                continue;
            }
        }

        item->Set( property, newValue );

        if( IsGeneratedTableType( item->Type() ) )
            generatedTables.insert( static_cast<PCB_GENERATED_TABLE*>( item ) );
        else if( IsGeneratedTableCell( item ) )
            generatedTables.insert( static_cast<PCB_GENERATED_TABLE*>( item->GetParent() ) );
    }

    // A generated table's cells come from the settings just changed, so they say nothing
    // about the edit until the table has been laid out again. So does a cell's formatting
    for( PCB_GENERATED_TABLE* table : generatedTables )
    {
        if( table->GetBoard() )
            table->RebuildCells( *table->GetBoard() );
    }

    changes.Push( _( "Edit Properties" ) );

    // Edit is authoritative so hold shapes fixed while solving neighbors and fold into this
    // undo with a no op if nothing was touched
    if( CONSTRAINT_EDIT_TOOL* constraintTool = m_frame->GetToolManager()->GetTool<CONSTRAINT_EDIT_TOOL>() )
    {
        std::vector<PCB_SHAPE*> shapes;
        EDIT_TOOL::collectConstraintShapes( selection, shapes );
        constraintTool->SolveAfterEdit( shapes );
    }

    // Driving length now on board so solve bound geometry to it and APPEND_UNDO folds follow
    // up moves into one undo
    if( !drivingConstraints.empty() )
    {
        BOARD_COMMIT solveCommit( m_frame );

        for( PCB_CONSTRAINT* constraint : drivingConstraints )
        {
            ApplyConstraintImmediately( m_frame->GetBoard(), constraint, nullptr,
                                        [&]( BOARD_ITEM* aItem )
                                        {
                                            solveCommit.Modify( aItem );
                                        } );
        }

        if( !solveCommit.Empty() )
            solveCommit.Push( _( "Apply Dimension Length" ), APPEND_UNDO );
    }

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

    auto stitchNet = m_propMgr.GetProperty( TYPE_HASH( PCB_VIA_STITCH ), _HKI( "Net" ) );
    stitchNet->SetChoices( nets );

    auto stitchGuardedNet = m_propMgr.GetProperty( TYPE_HASH( PCB_VIA_STITCH ), _HKI( "Guarded Net" ) );
    stitchGuardedNet->SetChoices( nets );

    // Drill spans come from the stackup rather than from an enum, and enumerating them costs a
    // walk of the board, so only a board that actually has a map pays for it
    if( aBoard->DrillSymbolLayers().any() )
    {
        wxPGChoices spans;
        spans.Add( _( "All spans" ), -1 );

        int index = 0;

        for( const DRILL_SPAN& span : EnumerateDrillSpans( *aBoard ) )
        {
            spans.Add( wxString::Format( wxT( "%s - %s%s" ),
                                         aBoard->GetLayerName( span.TopLayer() ),
                                         aBoard->GetLayerName( span.BottomLayer() ),
                                         span.m_IsBackdrill ? _( " (backdrill)" )
                                                            : wxString( wxEmptyString ) ),
                       index++ );
        }

        m_propMgr.GetProperty( TYPE_HASH( PCB_DRILL_MAP ), _HKI( "Hole Span" ) )
                ->SetChoices( spans );
    }
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
        else if( propName == _HKI( "Exclude From Simulation" ) )
        {
            aValue = wxVariant( footprint->GetExcludedFromSimForVariant( variantName ) );
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
