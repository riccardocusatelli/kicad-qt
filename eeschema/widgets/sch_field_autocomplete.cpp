/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

// Autocompletion for field values typed in a Scintilla control (field properties dialog and
// fields grid).  Moved out of SCH_FIELD so the schematic model does not depend on wxSTC.

#include <widgets/sch_field_autocomplete.h>

#include <wx/stc/stc.h>

#include <scintilla_tricks.h>
#include <sch_field.h>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <reporter.h>
#include <embedded_files.h>
#include <sim/sim_lib_mgr.h>
#include <sim/sim_model.h>


void SchFieldOnScintillaCharAdded( const SCH_FIELD* aField, SCINTILLA_TRICKS* aScintillaTricks,
                                   wxStyledTextEvent& aEvent )
{
    SCH_ITEM*  parent = dynamic_cast<SCH_ITEM*>( aField->GetParent() );
    SCHEMATIC* schematic = parent ? parent->Schematic() : nullptr;

    if( !schematic )
        return;

    wxStyledTextCtrl* scintilla = aScintillaTricks->Scintilla();
    int               key = aEvent.GetKey();

    wxArrayString autocompleteTokens;
    int           pos = scintilla->GetCurrentPos();
    int           start = scintilla->WordStartPosition( pos, true );
    wxString      partial;

    // Multi-line fields are not allowed. So remove '\n' if entered.
    if( key == '\n' )
    {
        wxString text = scintilla->GetText();
        int      currpos = scintilla->GetCurrentPos();
        text.Replace( wxS( "\n" ), wxS( "" ) );
        scintilla->SetText( text );
        scintilla->GotoPos( currpos - 1 );
        return;
    }

    auto textVarRef =
            [&]( int pt )
            {
                return pt >= 2 && scintilla->GetCharAt( pt - 2 ) == '$' && scintilla->GetCharAt( pt - 1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && scintilla->GetCharAt( start - 1 ) == ':' )
    {
        int refStart = scintilla->WordStartPosition( start - 1, true );

        if( textVarRef( refStart ) )
        {
            partial = scintilla->GetRange( start, pos );

            wxString ref = scintilla->GetRange( refStart, start - 1 );

            if( ref == wxS( "OP" ) )
            {
                // SPICE operating points use ':' syntax for ports
                if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( parent ) )
                {
                    NULL_REPORTER   devnull;
                    SCH_SHEET_PATH& sheet = schematic->CurrentSheet();
                    wxString        variant = schematic->GetCurrentVariant();
                    SIM_LIB_MGR     mgr( &schematic->Project() );

                    std::vector<EMBEDDED_FILES*> embeddedFilesStack;
                    embeddedFilesStack.push_back( schematic->GetEmbeddedFiles() );

                    if( EMBEDDED_FILES* symbolEmbeddedFiles = symbol->GetEmbeddedFiles() )
                    {
                        embeddedFilesStack.push_back( symbolEmbeddedFiles );
                        symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( embeddedFilesStack );
                    }

                    mgr.SetFilesStack( std::move( embeddedFilesStack ) );

                    SIM_MODEL& model = mgr.CreateModel( &sheet, *symbol, true, 0, variant, devnull ).model;

                    for( wxString pin : model.GetPinNames() )
                    {
                        if( pin.StartsWith( '<' ) && pin.EndsWith( '>' ) )
                            autocompleteTokens.push_back( pin.Mid( 1, pin.Length() - 2 ) );
                        else
                            autocompleteTokens.push_back( pin );
                    }

                    // add the synthetic port for power measurements
                    autocompleteTokens.push_back( wxT( "power" ) );
                }
            }
            else
            {
                SCH_REFERENCE_LIST refs;
                SCH_SYMBOL*        refSymbol = nullptr;

                schematic->Hierarchy().GetSymbols( refs, SYMBOL_FILTER_ALL );

                for( size_t jj = 0; jj < refs.GetCount(); jj++ )
                {
                    if( refs[jj].GetSymbol()->GetRef( &refs[jj].GetSheetPath(), true ) == ref )
                    {
                        refSymbol = refs[jj].GetSymbol();
                        break;
                    }
                }

                if( refSymbol )
                    refSymbol->GetContextualTextVars( &autocompleteTokens );
            }
        }
    }
    else if( textVarRef( start ) )
    {
        partial = scintilla->GetTextRange( start, pos );

        SCH_SYMBOL*     symbol = dynamic_cast<SCH_SYMBOL*>( parent );
        SCH_SHEET*      sheet = dynamic_cast<SCH_SHEET*>( parent );
        SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( parent );

        if( symbol )
        {
            symbol->GetContextualTextVars( &autocompleteTokens );

            if( schematic->CurrentSheet().Last() )
                schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );
        }

        if( sheet )
            sheet->GetContextualTextVars( &autocompleteTokens );

        if( label )
            label->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : schematic->Project().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    aScintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    scintilla->SetFocus();
}
