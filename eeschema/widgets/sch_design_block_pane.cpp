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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file sch_design_block_pane.cpp
 * What SCH_DESIGN_BLOCK_PANE does without the GUI toolkit, and the options of the placement in
 * the file dialog importing a sheet.  The widgets of the pane are in sch_design_block_pane_wx.cpp
 * (and in qt/eeschema/widgets/sch_design_block_pane_qt.cpp).
 */

#include <design_block.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/design_block_preview_widget.h>
#include <widgets/panel_design_block_chooser.h>
#include <eeschema_settings.h>
#include <core/ki_assert.h>


// Do not make these static wxStrings; they need to respond to language changes
#define REPEATED_PLACEMENT _( "Place repeated copies" )
#define PLACE_AS_SHEET     _( "Place as sheet" )
#define PLACE_AS_GROUP     _( "Place as group" )
#define KEEP_ANNOTATIONS   _( "Keep annotations" )

void SCH_DESIGN_BLOCK_PANE::ProjectChanged()
{
    m_chooserPanel->RefreshLibs();
    // Project change will blow up the default project; re-create any active preview canvas
    if( DESIGN_BLOCK_PREVIEW_WIDGET* preview = m_chooserPanel->GetPreviewWidget() )
        preview->DisplayDesignBlock( GetSelectedDesignBlock( true, true ) );
}


FILEDLG_IMPORT_SHEET_CONTENTS::FILEDLG_IMPORT_SHEET_CONTENTS( EESCHEMA_SETTINGS* aSettings ) :
        m_cbRepeatedPlacement( nullptr ), m_cbPlaceAsGroup( nullptr ), m_cbPlaceAsSheet( nullptr ),
        m_cbKeepAnnotations( nullptr )
{
    KI_ASSERT( aSettings );
    m_settings = aSettings;
};


void FILEDLG_IMPORT_SHEET_CONTENTS::TransferDataFromCustomControls()
{
    m_settings->m_DesignBlockChooserPanel.repeated_placement = m_cbRepeatedPlacement->GetValue();
    m_settings->m_DesignBlockChooserPanel.place_as_group = m_cbPlaceAsGroup->GetValue();
    m_settings->m_DesignBlockChooserPanel.place_as_sheet = m_cbPlaceAsSheet->GetValue();
    m_settings->m_DesignBlockChooserPanel.keep_annotations = m_cbKeepAnnotations->GetValue();
}


void FILEDLG_IMPORT_SHEET_CONTENTS::AddCustomControls( wxFileDialogCustomize& customizer )
{
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

    m_cbRepeatedPlacement = customizer.AddCheckBox( REPEATED_PLACEMENT );
    m_cbRepeatedPlacement->SetValue( m_settings->m_DesignBlockChooserPanel.repeated_placement );
    m_cbPlaceAsGroup = customizer.AddCheckBox( PLACE_AS_GROUP );
    m_cbPlaceAsGroup->SetValue( m_settings->m_DesignBlockChooserPanel.place_as_group );
    m_cbPlaceAsSheet = customizer.AddCheckBox( PLACE_AS_SHEET );
    m_cbPlaceAsSheet->SetValue( m_settings->m_DesignBlockChooserPanel.place_as_sheet );
    m_cbKeepAnnotations = customizer.AddCheckBox( KEEP_ANNOTATIONS );
    m_cbKeepAnnotations->SetValue( m_settings->m_DesignBlockChooserPanel.keep_annotations );
}
