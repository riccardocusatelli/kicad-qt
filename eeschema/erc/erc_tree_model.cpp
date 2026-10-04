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

#include <wx/html/m_templ.h>
#include <wx/html/styleparams.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <erc/erc_tree_model.h>
#include <marker_base.h>
#include <schematic.h>
#include <sch_marker.h>
#include <i18n_utility.h>
#include <units_provider.h>
#include <core/ki_assert.h>
#include <sch_edit_frame.h>


/**
 * Override of RC_TREE_MODEL::GetValue which returns item descriptions in a specific
 * SCH_SHEET_PATH context, if a context is available on the given SCH_MARKER or ERC_ITEM
 * targets.
 */
void ERC_TREE_MODEL::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                               unsigned int aCol ) const
{
    SCH_EDIT_FRAME*           schEditFrame = static_cast<SCH_EDIT_FRAME*>( m_editFrame );
    const RC_TREE_NODE*       node = ToNode( aItem );
    std::shared_ptr<ERC_ITEM> ercItem = std::static_pointer_cast<ERC_ITEM>( node->m_RcItem );
    MARKER_BASE*              marker = ercItem->GetParent();
    wxString                  msg;

    auto getItemDesc =
            [&]( EDA_ITEM* aCurrItem, SCH_SHEET_PATH& aSheet )
            {
                SCH_SHEET_PATH curSheet = schEditFrame->GetCurrentSheet();
                wxString       desc;

                if( aSheet != curSheet )
                {
                    // Use the schematic-level setter to avoid the view side effects of
                    // SCH_EDIT_FRAME::SetCurrentSheet, which recreates the drawing sheet
                    // proxy with potentially stale page number state.
                    schEditFrame->Schematic().SetCurrentSheet( aSheet );
                    aSheet.UpdateAllScreenReferences();
                    {
                        desc = aCurrItem->GetItemDescription( m_editFrame, true );
                    }
                    schEditFrame->Schematic().SetCurrentSheet( curSheet );
                    curSheet.UpdateAllScreenReferences();
                }
                else
                {
                    desc = aCurrItem->GetItemDescription( m_editFrame, true );
                }

                return desc;
            };

    switch( node->m_Type )
    {
    case RC_TREE_NODE::MARKER:
        if( marker )
        {
            SEVERITY severity = ercItem->GetParent()->GetSeverity();

            if( severity == RPT_SEVERITY_EXCLUSION )
            {
                if( schEditFrame->GetSeverity( ercItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
                    msg = _( "Excluded warning: " );
                else
                    msg = _( "Excluded error: " );
            }
            else if( severity == RPT_SEVERITY_WARNING )
            {
                msg = _( "Warning: " );
            }
            else
            {
                msg = _( "Error: " );
            }
        }

        msg += ercItem->GetErrorMessage( true );
        break;

    case RC_TREE_NODE::MAIN_ITEM:
        if( marker && marker->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
        {
            msg = _( "Drawing Sheet" );
        }
        else
        {
            msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetMainItemID() ),
                               ercItem->MainItemHasSheetPath() ? ercItem->GetMainItemSheetPath()
                                                               : schEditFrame->GetCurrentSheet() );
        }

        break;

    case RC_TREE_NODE::AUX_ITEM:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItemID() ),
                           ercItem->AuxItemHasSheetPath() ? ercItem->GetAuxItemSheetPath()
                                                          : schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::AUX_ITEM2:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItem2ID() ),
                           schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::AUX_ITEM3:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItem3ID() ),
                           schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::COMMENT:
        if( marker )
            msg = marker->GetComment();

        break;
    }

    msg.Replace( wxS( "\n" ), wxS( " " ) );
    aVariant = msg;
}
