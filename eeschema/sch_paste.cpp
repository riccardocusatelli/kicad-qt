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

#include <sch_paste.h>

#include <vector>

#include <core/ki_assert.h>
#include <core/ki_log.h>
#include <lib_symbol.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <string_utils.h>
#include <trace_helpers.h>


/**
 * Flag to enable schematic paste debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceSchPaste[] = wxT( "KICAD_SCH_PASTE" );


const LIB_SYMBOL* SCH_PASTE::ChooseLibSymbol( const SCH_SCREEN* aClipboardScreen,
                                              const SCH_SCREEN* aDestScreen,
                                              const wxString&   aLibSymbolName )
{
    // The clipboard's cached library symbol is a matched pair with the pasted instance, so it
    // must win over the destination's same-named cache. Pasting from the destination cache would
    // silently remap the instance to a different definition and drop in-place edits such as
    // renumbered pins (issue 21401) or a changed power type (issue 22162). Fall back to the
    // destination cache only when the clipboard carries no copy.
    if( aClipboardScreen )
    {
        auto clipIt = aClipboardScreen->GetLibSymbols().find( aLibSymbolName );

        if( clipIt != aClipboardScreen->GetLibSymbols().end() )
            return clipIt->second;
    }

    if( aDestScreen )
    {
        auto destIt = aDestScreen->GetLibSymbols().find( aLibSymbolName );

        if( destIt != aDestScreen->GetLibSymbols().end() )
            return destIt->second;
    }

    return nullptr;
}


void SCH_PASTE::UpdateSymbolInstance( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aPastePath,
                                      const KIID_PATH& aClipPath, const wxString& aProjectName,
                                      bool aKeepAnnotation )
{
    KI_CHECK( aSymbol, /* void */ );

    SCH_SYMBOL_INSTANCE newInstance;
    bool                instanceFound = false;
    KIID_PATH           pasteLookupPath = aClipPath;

    for( const SCH_SYMBOL_INSTANCE& tmp : aSymbol->GetInstances() )
    {
        if( ( tmp.m_Path.empty() && aClipPath.empty() ) || ( !aClipPath.empty() && tmp.m_Path.EndsWith( aClipPath ) ) )
        {
            newInstance = tmp;
            instanceFound = true;

            KI_LOG_TRACE( traceSchPaste, wxS( "Pasting found symbol instance with reference %s, unit %d:\n"
                                            "\tClipboard path: %s\n"
                                            "\tSymbol UUID: %s." ),
                        tmp.m_Reference,
                        tmp.m_Unit,
                        aClipPath.AsString(),
                        aSymbol->m_Uuid.AsString() );

            break;
        }
    }

    // The pasted symbol look up paths include the symbol UUID.
    pasteLookupPath.push_back( aSymbol->m_Uuid );

    if( !instanceFound )
    {
        KI_LOG_TRACE( traceSchPaste, wxS( "Clipboard symbol instance **not** found:\n\tClipboard path: %s\n"
                                        "\tSymbol UUID: %s." ),
                    aClipPath.AsString(),
                    aSymbol->m_Uuid.AsString() );

        // Some legacy versions saved value fields escaped.  While we still do in the symbol
        // editor, we don't anymore in the schematic, so be sure to unescape them.
        SCH_FIELD* valueField = aSymbol->GetField( FIELD_T::VALUE );
        valueField->SetText( UnescapeString( valueField->GetText() ) );

        // Pasted from notepad or an older instance of eeschema.  Use the values in the fields
        // instead.
        newInstance.m_Reference = aSymbol->GetField( FIELD_T::REFERENCE )->GetText();
        newInstance.m_Unit = aSymbol->GetUnit();
    }

    newInstance.m_Path = aPastePath.Path();
    newInstance.m_ProjectName = aProjectName;

    aSymbol->AddHierarchicalReference( newInstance );

    if( !aKeepAnnotation )
        aSymbol->ClearAnnotation( &aPastePath, false );

    // We might clear annotations but always leave the original unit number from the paste.
    aSymbol->SetUnit( newInstance.m_Unit );
}


void SCH_PASTE::PruneSymbolInstances( SCH_SYMBOL* aSymbol, const wxString& aProjectName )
{
    KI_CHECK( aSymbol, /* void */ );

    std::vector<KIID_PATH> instancePathsToRemove;

    for( const SCH_SYMBOL_INSTANCE& instance : aSymbol->GetInstances() )
    {
        if( instance.m_ProjectName != aProjectName || instance.m_Path.empty() )
            instancePathsToRemove.emplace_back( instance.m_Path );
    }

    for( const KIID_PATH& path : instancePathsToRemove )
        aSymbol->RemoveInstance( path );
}
