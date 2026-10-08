/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include "sch_properties_panel.h"

#include <font/fontconfig.h>
#include <pgm_base.h>
#include <common.h>
#include <confirm.h>
#include <connection_graph.h>
#include <properties/pg_factory.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include <set>
#include <core/ki_assert.h>

static const wxString MISSING_FIELD_SENTINEL = wxS( "\uE000" );

class SCH_SYMBOL_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    { }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
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

        SCH_SYMBOL* symbol = reinterpret_cast<SCH_SYMBOL*>( obj );
        SCH_FIELD*  field = symbol->GetField( m_name );

        wxString variantName;
        const SCH_SHEET_PATH* sheetPath = nullptr;

        if( symbol->Schematic() )
        {
            variantName = symbol->Schematic()->GetCurrentVariant();
            sheetPath = &symbol->Schematic()->CurrentSheet();
        }

        if( !field )
        {
            SCH_FIELD newField( symbol, FIELD_T::USER, m_name );
            newField.SetText( value, sheetPath, variantName );
            symbol->AddField( newField );
        }
        else
        {
            field->SetText( value, sheetPath, variantName );
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );
        const SCH_FIELD*  field = symbol->GetField( m_name );

        if( field )
        {
            wxString variantName;
            const SCH_SHEET_PATH* sheetPath = nullptr;

            if( symbol->Schematic() )
            {
                variantName = symbol->Schematic()->GetCurrentVariant();
                sheetPath = &symbol->Schematic()->CurrentSheet();
            }

            wxString text;

            if( !variantName.IsEmpty() && sheetPath )
                text = field->GetText( sheetPath, variantName );
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

class SCH_SHEET_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SHEET_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SHEET ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SHEET ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override { return PROPERTY_BASE::Writeable( aObject ); }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        SCH_SHEET* sheet = reinterpret_cast<SCH_SHEET*>( obj );
        SCH_FIELD* field = sheet->GetField( m_name );

        wxString              variantName;
        const SCH_SHEET_PATH* sheetPath = nullptr;

        if( sheet->Schematic() )
        {
            variantName = sheet->Schematic()->GetCurrentVariant();
            sheetPath = &sheet->Schematic()->CurrentSheet();
        }

        if( !field )
        {
            SCH_FIELD newField( sheet, FIELD_T::USER, m_name );
            newField.SetText( value, sheetPath, variantName );
            sheet->AddField( newField );
        }
        else
        {
            field->SetText( value, sheetPath, variantName );
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SHEET* sheet = reinterpret_cast<const SCH_SHEET*>( obj );
        const SCH_FIELD* field = sheet->GetField( m_name );

        if( field )
        {
            wxString              variantName;
            const SCH_SHEET_PATH* sheetPath = nullptr;

            if( sheet->Schematic() )
            {
                variantName = sheet->Schematic()->GetCurrentVariant();
                sheetPath = &sheet->Schematic()->CurrentSheet();
            }

            wxString text;

            if( !variantName.IsEmpty() && sheetPath )
                text = field->GetText( sheetPath, variantName );
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

std::set<wxString> SCH_PROPERTIES_PANEL::m_currentSymbolFieldNames;
std::set<wxString> SCH_PROPERTIES_PANEL::m_currentSheetFieldNames;

SCH_PROPERTIES_PANEL::SCH_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, SCH_BASE_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();

    registerEditors();
}


SCH_PROPERTIES_PANEL::~SCH_PROPERTIES_PANEL()
{
    // The toolkit state releases the editors of the grid (the shared editors forget the frame)
}


std::string SCH_PROPERTIES_PANEL::symbolNetlist() const
{
    SCH_SELECTION& sel = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>()->GetSelection();
    LIB_SYMBOL*    libSymbol = nullptr;

    for( EDA_ITEM* item : sel )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( !libSymbol )
                libSymbol = symbol->GetLibSymbolRef().get();
            else if( libSymbol != symbol->GetLibSymbolRef().get() )
                return std::string( "" );
        }
    }

    if( !libSymbol )
        return std::string( "" );

    wxString      symbolNetlist;
    wxArrayString pins;

    for( SCH_PIN* pin : libSymbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
        pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );

    if( !pins.IsEmpty() )
        symbolNetlist << EscapeString( wxJoin( pins, '\t' ), CTX_LINE );

    symbolNetlist << wxS( "\r" );

    wxArrayString fpFilters = libSymbol->GetFPFilters();

    if( !fpFilters.IsEmpty() )
        symbolNetlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

    symbolNetlist << wxS( "\r" );

    return symbolNetlist.ToStdString();
}


const SELECTION& SCH_PROPERTIES_PANEL::getSelection( SELECTION& aFallbackSelection )
{
    SCH_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() && m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* symbolFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        if( symbolFrame->GetCurSymbol() )
        {
            aFallbackSelection.Clear();
            aFallbackSelection.Add( symbolFrame->GetCurSymbol() );
            return aFallbackSelection;
        }
    }

    return selection;
}


EDA_ITEM* SCH_PROPERTIES_PANEL::getFrontItem()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    return selection.Empty() ? nullptr : selection.Front();
}


void SCH_PROPERTIES_PANEL::UpdateData()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::AfterCommit()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    m_currentSymbolFieldNames.clear();
    m_currentSheetFieldNames.clear();

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( const SCH_FIELD& field : symbol->GetFields() )
            {
                if( field.IsPrivate() )
                    continue;

                m_currentSymbolFieldNames.insert( field.GetCanonicalName() );
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( const SCH_FIELD& field : sheet->GetFields() )
            {
                if( field.IsPrivate() )
                    continue;

                m_currentSheetFieldNames.insert( field.GetCanonicalName() );
            }
        }
    }

    const wxString groupFields = _HKI( "Fields" );

    for( const wxString& name : m_currentSymbolFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), name ) )
        {
            m_propMgr.AddProperty( new SCH_SYMBOL_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc(
                            [name]( INSPECTABLE* )
                            {
                                return SCH_PROPERTIES_PANEL::m_currentSymbolFieldNames.count( name );
                            } );
        }
    }

    for( const wxString& name : m_currentSheetFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SHEET ), name ) )
        {
            m_propMgr.AddProperty( new SCH_SHEET_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc(
                            [name]( INSPECTABLE* )
                            {
                                return SCH_PROPERTIES_PANEL::m_currentSheetFieldNames.count( name );
                            } );
        }
    }

    PROPERTIES_PANEL::rebuildProperties( aSelection );
}


TOOLKIT_PG_PROPERTY* SCH_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    TOOLKIT_PG_PROPERTY* prop = PGPropertyFactory( aProperty, m_frame );

    COLOR4D bg = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    PGSetColorBackground( prop, bg );

    if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        PGSetEditor( prop, PG_EDITOR::FPID, m_frame );
    else if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        PGSetEditor( prop, PG_EDITOR::URL, m_frame );

    return prop;
}


PROPERTY_BASE* SCH_PROPERTIES_PANEL::getPropertyByName( const wxString& aName ) const
{
    EDA_ITEM* item = const_cast<SCH_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsSCH_ITEM() )
        return nullptr;

    SCH_ITEM* firstItem = static_cast<SCH_ITEM*>( item );

    KI_CHECK_MSG( firstItem, nullptr, wxT( "getPropertyByName for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ), aName );
    KI_CHECK_MSG( property, nullptr, wxT( "getPropertyByName for a property not found on the selected item!" ) );

    return property;
}


bool SCH_PROPERTIES_PANEL::valueChanging( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents )
        return true;

    EDA_ITEM* frontItem = getFrontItem();

    if( !frontItem )
        return true;

    if( PROPERTY_BASE* property = getPropertyByName( aName ) )
    {
        wxVariant newValue = aValue;

        if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), frontItem ) )
        {
            wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                                  validationFailure->get()->Format( m_frame ) );
            m_frame->ShowInfoBarError( errorMsg );
            return false;
        }
    }

    return true;
}


void SCH_PROPERTIES_PANEL::valueChanged( const wxString& aName, const wxVariant& aValue )
{
    if( m_SuppressGridChangeEvents )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    KI_CHECK( getPropertyByName( aName ), /* void */ );

    wxVariant   newValue = aValue;
    SCH_COMMIT  changes( m_frame );
    SCH_SCREEN* screen = m_frame->GetScreen();

    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *item ), aName );
        KI_CHECK2( property, continue );

        // Editing reference text in the schematic must go through the parent symbol in order to handle
        // symbol instance data properly.
        if( item->Type() == SCH_FIELD_T && static_cast<SCH_FIELD*>( item )->GetId() == FIELD_T::REFERENCE
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );
            KI_CHECK2( symbol, continue );

            changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
            symbol->SetRefProp( newValue.GetString() );
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property );
            continue;
        }

        // Editing field text in the schematic when a variant is active must use variant-aware
        // SetText to properly store the value as a variant override.
        if( item->Type() == SCH_FIELD_T
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );

            if( symbol && symbol->Schematic() )
            {
                wxString variantName = symbol->Schematic()->GetCurrentVariant();

                if( !variantName.IsEmpty() )
                {
                    changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
                    field->SetText( newValue.GetString(), &symbol->Schematic()->CurrentSheet(), variantName );
                    symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, nullptr, variantName );
                    continue;
                }
            }
        }

        // Changing a sheet's filename field requires file operations to match the dialog behavior.
        if( item->Type() == SCH_FIELD_T
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );
            SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item->GetParent() );

            if( sheet && field->GetId() == FIELD_T::SHEET_FILENAME )
            {
                SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( m_frame );

                if( !handleSheetFilenameChange( editFrame, sheet, changes, newValue.GetString() ) )
                {
                    UpdateData();
                    return;
                }

                continue;
            }
        }

        if( item->Type() == SCH_TABLECELL_T )
            changes.Modify( item->GetParent(), screen, RECURSE_MODE::NO_RECURSE );
        else
            changes.Modify( item, screen, RECURSE_MODE::NO_RECURSE );

        item->Set( property, newValue );

        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property,
                                    symbol->Schematic()->GetCurrentVariant() );
        }
    }

    changes.Push( _( "Edit Properties" ) );

    // Force a repaint of the items whose properties were changed
    // This is necessary to update field displays in the schematic view
    for( EDA_ITEM* edaItem : selection )
        m_frame->UpdateItem( edaItem );

    // Perform grid updates as necessary based on value change
    AfterCommit();
}


bool SCH_PROPERTIES_PANEL::handleSheetFilenameChange( SCH_EDIT_FRAME* aFrame, SCH_SHEET* aSheet,
                                                       SCH_COMMIT& aChanges,
                                                       const wxString& aNewFilename )
{
    wxString newFilename = EnsureFileExtension( aNewFilename, FILEEXT::KiCadSchematicFileExtension );

    if( newFilename.IsEmpty() || !IsFullFileNameValid( newFilename ) )
    {
        DisplayError( aFrame, _( "A sheet must have a valid file name." ) );
        return false;
    }

    // Normalize separators to unix notation
    newFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString oldFilename = aSheet->GetFileName();
    oldFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    if( newFilename == oldFilename )
        return true;

    if( !aFrame->ChangeSheetFile( aSheet, newFilename ) )
        return false;

    SCH_SCREEN* currentScreen = aFrame->GetCurrentSheet().LastScreen();
    aChanges.Modify( aSheet, currentScreen, RECURSE_MODE::NO_RECURSE );
    aSheet->SetFileName( newFilename );

    return true;
}


bool SCH_PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    // For SCH_FIELD "Text" property, return the variant-aware value when a variant is active
    if( aItem->Type() == SCH_FIELD_T
            && m_frame->IsType( FRAME_SCH )
            && aProperty->Name() == wxT( "Text" ) )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( aItem );
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( field->GetParentSymbol() );

        if( symbol && symbol->Schematic() )
        {
            wxString variantName = symbol->Schematic()->GetCurrentVariant();

            if( !variantName.IsEmpty() )
            {
                wxString text = field->GetText( &symbol->Schematic()->CurrentSheet(), variantName );
                aValue = wxVariant( text );
                return true;
            }
        }
    }

    return PROPERTIES_PANEL::getItemValue( aItem, aProperty, aValue );
}
