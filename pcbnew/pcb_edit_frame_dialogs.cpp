/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file pcb_edit_frame_dialogs.cpp
 * The parts of PCB_EDIT_FRAME showing the dialogs of the board editor (board setup, find,
 * footprint properties, exchange footprints, reports, item properties, 3D models migration).
 * The dialogs are not yet ported to Qt: this file is in the wxWidgets build only.
 */

#include <pcb_edit_frame.h>

#include <board.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <kiface_base.h>
#include <kiway.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <pcb_table.h>
#include <pcb_target.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/board_inspection_tool.h>
#include <tools/drc_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>
#include <widgets/appearance_controls.h>
#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_search_pane.h>
#include <widgets/wx_infobar.h>
#include <dialog_find.h>
#include <dialog_footprint_properties.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialogs/dialog_migrate_3d_models.h>
#include <dialog_board_setup.h>
#include <dialogs/dialog_dimension_properties.h>
#include <dialogs/dialog_table_properties.h>
#include <dialogs/dialog_book_reporter.h>
#include <core/ki_assert.h>


#define INSPECT_DRC_ERROR_DIALOG_NAME   wxT( "InspectDrcErrorDialog" )
#define INSPECT_CLEARANCE_DIALOG_NAME   wxT( "InspectClearanceDialog" )
#define INSPECT_CONSTRAINTS_DIALOG_NAME wxT( "InspectConstraintsDialog" )
#define FOOTPRINT_DIFF_DIALOG_NAME      wxT( "FootprintDiffDialog" )


void PCB_EDIT_FRAME::ShowBoardSetupDialog( const wxString& aInitialPage, wxWindow* aParent )
{
    static std::mutex dialogMutex; // Local static mutex

    std::unique_lock<std::mutex> dialogLock( dialogMutex, std::try_to_lock );

    // One dialog at a time.
    if( !dialogLock.owns_lock() )
    {
        if( m_boardSetupDlg && m_boardSetupDlg->IsShown() )
        {
            m_boardSetupDlg->Raise(); // Brings the existing dialog to the front
        }

        return;
    }

    // Make sure everything's up-to-date
    GetBoard()->BuildListOfNets();

    DIALOG_BOARD_SETUP dlg( this, aParent );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    // Assign dlg to the m_boardSetupDlg pointer to track its status.
    m_boardSetupDlg = &dlg;

    // QuasiModal required for Scintilla auto-complete
    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        // Note: We must synchronise time domain properties before nets and classes, otherwise the updates
        // called by the board listener events are using stale data
        GetBoard()->SynchronizeTuningProfileProperties();
        GetBoard()->SynchronizeNetsAndNetClasses( true );

        if( !GetBoard()->SynchronizeComponentClasses( std::unordered_set<wxString>() ) )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "Could not load component class assignment rules" ),
                                    wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::GENERIC );
        }

        // We don't know if anything was modified, so err on the side of requiring a save
        OnModify();

        Kiway().CommonSettingsChanged( TEXTVARS_CHANGED );

        Prj().IncrementTextVarsTicker();
        Prj().IncrementNetclassesTicker();

        PCBNEW_SETTINGS* settings = GetPcbNewSettings();
        static LSET      maskAndPasteLayers = LSET( { F_Mask, F_Paste, B_Mask, B_Paste } );

        GetCanvas()->GetView()->UpdateAllItemsConditionally(
                [&]( KIGFX::VIEW_ITEM* aItem ) -> int
                {
                    int flags = 0;

                    if( !aItem->IsBOARD_ITEM() )
                        return flags;

                    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aItem );

                    if( item->Type() == PCB_VIA_T || item->Type() == PCB_PAD_T )
                    {
                        // Note: KIGFX::REPAINT isn't enough for things that go from invisible
                        // to visible as they won't be found in the view layer's itemset for
                        // re-painting.
                        if( ( GetBoard()->GetVisibleLayers() & maskAndPasteLayers ).any() )
                            flags |= KIGFX::ALL;
                    }

                    if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T || item->Type() == PCB_VIA_T )
                    {
                        if( settings->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS )
                            flags |= KIGFX::REPAINT;
                    }

                    if( item->Type() == PCB_PAD_T )
                    {
                        if( settings->m_Display.m_PadClearance )
                            flags |= KIGFX::REPAINT;
                    }

                    if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    {
                        if( text->HasTextVars() )
                        {
                            text->ClearRenderCache();
                            text->ClearBoundingBoxCache();
                            flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                        }
                    }

                    return flags;
                } );

        GetCanvas()->Refresh();

        UpdateUserInterface();
        ReCreateAuxiliaryToolbar();
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        //this event causes the routing tool to reload its design rules information
        TOOL_EVENT toolEvent( TC_COMMAND, TA_MODEL_CHANGE, AS_ACTIVE );
        toolEvent.SetHasPosition( false );
        m_toolManager->ProcessEvent( toolEvent );
    }

    GetCanvas()->SetFocus();

    // Reset m_boardSetupDlg after the dialog is closed
    m_boardSetupDlg = nullptr;
}


void PCB_EDIT_FRAME::ShowFindDialog()
{
    if( !m_findDialog )
    {
        m_findDialog = new DIALOG_FIND( this );
        m_findDialog->SetCallback( std::bind( &PCB_SELECTION_TOOL::FindItem,
                                              m_toolManager->GetTool<PCB_SELECTION_TOOL>(), std::placeholders::_1 ) );
    }

    wxString findString;

    PCB_SELECTION& selection = m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 1 )
    {
        EDA_ITEM* front = selection.Front();

        switch( front->Type() )
        {
        case PCB_FOOTPRINT_T:
            findString = UnescapeString( static_cast<FOOTPRINT*>( front )->GetValue() );
            break;

        case PCB_FIELD_T:
        case PCB_TEXT_T:
            findString = UnescapeString( static_cast<PCB_TEXT*>( front )->GetText() );

            if( findString.Contains( wxT( "\n" ) ) )
                findString = findString.Before( '\n' );

            break;

        default:
            break;
        }
    }

    m_findDialog->Preload( findString );

    m_findDialog->Show( true );
}


void PCB_EDIT_FRAME::FindNext( bool reverse )
{
    if( !m_findDialog )
        ShowFindDialog();

    m_findDialog->FindNext( reverse );
}


void PCB_EDIT_FRAME::ShowFootprintPropertiesDialog( FOOTPRINT* aFootprint )
{
    if( aFootprint == nullptr )
        return;

    DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_RETVALUE retvalue;

    /*
     * Make sure dlg is destroyed before GetCanvas->Refresh is called
     * later or the refresh will try to modify its properties since
     * they share a GL context.
     */
    {
        DIALOG_FOOTPRINT_PROPERTIES dlg( this, aFootprint );

        dlg.ShowQuasiModal();
        retvalue = dlg.GetReturnValue();
    }

    /*
     * retvalue =
     *   FP_PROPS_UPDATE_FP to show Update Footprints dialog
     *   FP_PROPS_CHANGE_FP to show Change Footprints dialog
     *   FP_PROPS_OK for normal edit
     *   FP_PROPS_CANCEL if aborted
     *   FP_PROPS_EDIT_BOARD_FP to load board footprint into Footprint Editor
     *   FP_PROPS_EDIT_LIBRARY_FP to load library footprint into Footprint Editor
     */

    if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_OK )
    {
        // If something edited, push a refresh request
        GetCanvas()->Refresh();
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_EDIT_BOARD_FP )
    {
        if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_EDITOR, true ) )
        {
            FOOTPRINT_EDIT_FRAME* fp_editor = static_cast<FOOTPRINT_EDIT_FRAME*>( frame );

            fp_editor->LoadFootprintFromBoard( aFootprint );
            fp_editor->Show( true );
            fp_editor->Raise();        // Iconize( false );
        }
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_EDIT_LIBRARY_FP )
    {
        if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_EDITOR, true ) )
        {
            FOOTPRINT_EDIT_FRAME* fp_editor = static_cast<FOOTPRINT_EDIT_FRAME*>( frame );

            fp_editor->LoadFootprintFromLibrary( aFootprint->GetFPID() );
            fp_editor->Show( true );
            fp_editor->Raise();        // Iconize( false );
        }
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_UPDATE_FP )
    {
        ShowExchangeFootprintsDialog( aFootprint, true, true );
    }
    else if( retvalue == DIALOG_FOOTPRINT_PROPERTIES::FP_PROPS_CHANGE_FP )
    {
        ShowExchangeFootprintsDialog( aFootprint, false, true );
    }
}


int PCB_EDIT_FRAME::ShowExchangeFootprintsDialog( FOOTPRINT* aFootprint, bool aUpdateMode,
                                                  bool aSelectedMode )
{
    DIALOG_EXCHANGE_FOOTPRINTS dialog( this, aFootprint, aUpdateMode, aSelectedMode );

    return dialog.ShowQuasiModal();
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectDrcErrorDialog()
{
    if( !m_inspectDrcErrorDlg )
    {
        m_inspectDrcErrorDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_DRC_ERROR_DIALOG_NAME,
                                                         _( "Violation Report" ) );
    }

    return m_inspectDrcErrorDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectClearanceDialog()
{
    if( !m_inspectClearanceDlg )
    {
        m_inspectClearanceDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CLEARANCE_DIALOG_NAME,
                                                          _( "Clearance Report" ) );
    }

    return m_inspectClearanceDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetInspectConstraintsDialog()
{
    if( !m_inspectConstraintsDlg )
    {
        m_inspectConstraintsDlg = new DIALOG_BOOK_REPORTER( this, INSPECT_CONSTRAINTS_DIALOG_NAME,
                                                            _( "Constraints Report" ) );
    }

    return m_inspectConstraintsDlg;
}


DIALOG_BOOK_REPORTER* PCB_EDIT_FRAME::GetFootprintDiffDialog()
{
    if( !m_footprintDiffDlg )
    {
        m_footprintDiffDlg = new DIALOG_BOOK_REPORTER( this, FOOTPRINT_DIFF_DIALOG_NAME,
                                                       _( "Compare Footprint with Library" ) );

        m_footprintDiffDlg->ShowApplyButton( _( "Update Footprint from Library..." ) );
    }

    return m_footprintDiffDlg;
}


void PCB_EDIT_FRAME::onCloseModelessBookReporterDialogs( const wxString& aDialogName, int aId )
{
    if( m_inspectDrcErrorDlg && aDialogName == INSPECT_DRC_ERROR_DIALOG_NAME )
    {
        m_inspectDrcErrorDlg->Destroy();
        m_inspectDrcErrorDlg = nullptr;
    }
    else if( m_inspectClearanceDlg && aDialogName == INSPECT_CLEARANCE_DIALOG_NAME )
    {
        m_inspectClearanceDlg->Destroy();
        m_inspectClearanceDlg = nullptr;
    }
    else if( m_inspectConstraintsDlg && aDialogName == INSPECT_CONSTRAINTS_DIALOG_NAME )
    {
        m_inspectConstraintsDlg->Destroy();
        m_inspectConstraintsDlg = nullptr;
    }
    else if( m_footprintDiffDlg && aDialogName == FOOTPRINT_DIFF_DIALOG_NAME )
    {
        if( aId == wxID_APPLY )
        {
            KIID fpUUID = m_footprintDiffDlg->GetUserItemID();

            CallAfter(
                    [this, fpUUID]()
                    {
                        BOARD_ITEM* item = m_pcb->ResolveItem( fpUUID );

                        if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( item ) )
                        {
                            m_toolManager->RunAction<EDA_ITEM*>( ACTIONS::selectItem, footprint );

                            DIALOG_EXCHANGE_FOOTPRINTS dialog( this, footprint, true, true );
                            dialog.ShowQuasiModal();
                        }
                    } );
        }

        m_footprintDiffDlg->Destroy();
        m_footprintDiffDlg = nullptr;
    }
}


void PCB_EDIT_FRAME::OnEditItemRequest( BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
        ShowReferenceImagePropertiesDialog( aItem );
        break;

    case PCB_BARCODE_T:
        ShowBarcodePropertiesDialog( static_cast<PCB_BARCODE*>( aItem ) );
        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
        ShowTextPropertiesDialog( static_cast<PCB_TEXT*>( aItem ) );
        break;

    case PCB_TEXTBOX_T:
        ShowTextBoxPropertiesDialog( static_cast<PCB_TEXTBOX*>( aItem ) );
        break;

    case PCB_TABLE_T:
    {
        DIALOG_TABLE_PROPERTIES dlg( this, static_cast<PCB_TABLE*>( aItem ) );

        //QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_PAD_T:
        ShowPadPropertiesDialog( static_cast<PAD*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        ShowFootprintPropertiesDialog( static_cast<FOOTPRINT*>( aItem ) );
        break;

    case PCB_TARGET_T:
        ShowTargetOptionsDialog( static_cast<PCB_TARGET*>( aItem ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    {
        DIALOG_DIMENSION_PROPERTIES dlg( this, static_cast<PCB_DIMENSION_BASE*>( aItem ) );

        dlg.ShowModal();
        break;
    }

    case PCB_SHAPE_T:
        ShowGraphicItemPropertiesDialog( static_cast<PCB_SHAPE*>( aItem ) );
        break;

    case PCB_ZONE_T:
        Edit_Zone_Params( static_cast<ZONE*>( aItem ) );
        break;

    case PCB_GROUP_T:
        m_toolManager->RunAction( ACTIONS::groupProperties,
                                  static_cast<EDA_GROUP*>( static_cast<PCB_GROUP*>( aItem ) ) );
        break;

    case PCB_GENERATOR_T:
        static_cast<PCB_GENERATOR*>( aItem )->ShowPropertiesDialog( this );
        break;

    case PCB_MARKER_T:
        m_toolManager->GetTool<DRC_TOOL>()->CrossProbe( static_cast<PCB_MARKER*>( aItem ) );
        break;

    case PCB_POINT_T:
        break;

    default:
        break;
    }
}


void PCB_EDIT_FRAME::checkWrlModels()
{
    // Silently replace references whose filename uniquely identifies a
    // STEP sibling.  Leaves ambiguous cases for the infobar below.
    DIALOG_MIGRATE_3D_MODELS::AutoMigrateByFilename( this );

    const int unresolved = DIALOG_MIGRATE_3D_MODELS::CountUnresolvedWrlReferences( this );

    if( unresolved > 0 )
    {
        wxString msg = wxString::Format( wxPLURAL( "%d WRL 3D model could not be matched "
                                                   "to an equivalent STEP model.",
                                                   "%d WRL 3D models could not be matched "
                                                   "to equivalent STEP models.",
                                                   unresolved ),
                                         unresolved );

        m_loadNoticeInfoBar->RemoveAllButtons();
        m_loadNoticeInfoBar->AddButton( _( "Show options" ),
                [this]()
                {
                    DIALOG_MIGRATE_3D_MODELS dlg( this );
                    dlg.ShowModal();

                    // Dismiss the infobar if nothing remains to resolve;
                    // otherwise leave it so the user can try again.
                    if( DIALOG_MIGRATE_3D_MODELS::CountUnresolvedWrlReferences( this ) == 0 )
                        m_loadNoticeInfoBar->Dismiss();
                } );
        m_loadNoticeInfoBar->AddCloseButton();
        m_loadNoticeInfoBar->ShowMessageFor( msg, 10000, wxICON_INFORMATION );
    }
}


void PCB_EDIT_FRAME::destroyModelessDialogs()
{
    if( m_findDialog )
    {
        m_findDialog->Destroy();
        m_findDialog = nullptr;
    }

    if( m_inspectDrcErrorDlg )
    {
        m_inspectDrcErrorDlg->Destroy();
        m_inspectDrcErrorDlg = nullptr;
    }

    if( m_inspectClearanceDlg )
    {
        m_inspectClearanceDlg->Destroy();
        m_inspectClearanceDlg = nullptr;
    }

    if( m_inspectConstraintsDlg )
    {
        m_inspectConstraintsDlg->Destroy();
        m_inspectConstraintsDlg = nullptr;
    }

    if( m_footprintDiffDlg )
    {
        m_footprintDiffDlg->Destroy();
        m_footprintDiffDlg = nullptr;
    }
}
