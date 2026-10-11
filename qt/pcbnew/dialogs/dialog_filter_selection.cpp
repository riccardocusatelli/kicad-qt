/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

/**
 * @file dialog_filter_selection.cpp
 * DIALOG_FILTER_SELECTION with Qt (pcbnew/dialogs/dialog_filter_selection.cpp with wxWidgets).
 */

#include <dialog_filter_selection.h>

#include <pcb_base_frame.h>


DIALOG_FILTER_SELECTION::DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions ) :
        DIALOG_FILTER_SELECTION_BASE( aParent ),
        m_options( aOptions )
{
    // "All Items" shows if some, but not all, of the items are included
    m_All_Items->setTristate( true );

    // The spacer keeps the layout of the dialog: it isn't an item
    m_Hidden_Spacer->setVisible( false );

    SetupStandardButtons();
}


std::vector<QCheckBox*> DIALOG_FILTER_SELECTION::itemCheckBoxes() const
{
    return { m_Include_Modules, m_IncludeLockedModules, m_Include_Tracks, m_Include_Vias,
             m_Include_Zones, m_Include_Draw_Items, m_Include_Edges_Items, m_Include_PcbTexts };
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( QWidget* aSource )
{
    m_IncludeLockedModules->setEnabled( m_Include_Modules->isChecked() );

    // Update "All Items" checkbox based on how many items are currently checked
    QSignalBlocker blocker( m_All_Items );
    m_All_Items->setCheckState( GetSuggestedAllItemsState() );
}


bool DIALOG_FILTER_SELECTION::TransferDataToWindow()
{
    m_Include_Modules->setChecked( m_options.includeFootprints );
    m_IncludeLockedModules->setChecked( m_options.includeLockedFootprints );
    m_IncludeLockedModules->setEnabled( m_Include_Modules->isChecked() );

    m_Include_Tracks->setChecked( m_options.includeTracks );
    m_Include_Vias->setChecked( m_options.includeVias );
    m_Include_Zones->setChecked( m_options.includeZones );
    m_Include_Draw_Items->setChecked( m_options.includeItemsOnTechLayers );
    m_Include_Edges_Items->setChecked( m_options.includeBoardOutlineLayer );
    m_Include_PcbTexts->setChecked( m_options.includePcbTexts );

    // Update "All Items" checkbox based on how many items are currently checked
    QSignalBlocker blocker( m_All_Items );
    m_All_Items->setCheckState( GetSuggestedAllItemsState() );

    return true;
}


void DIALOG_FILTER_SELECTION::forceCheckboxStates( bool aNewState )
{
    for( QCheckBox* cb : itemCheckBoxes() )
    {
        QSignalBlocker blocker( cb );
        cb->setChecked( aNewState );
    }

    // Make enable state match checkbox state
    m_IncludeLockedModules->setEnabled( aNewState );
}


Qt::CheckState DIALOG_FILTER_SELECTION::GetSuggestedAllItemsState()
{
    int numChecked = 0;
    int numCheckboxes = static_cast<int>( itemCheckBoxes().size() );

    // Tally how many checkboxes are checked.  Only include "locked footprints" in the total
    // if "footprints" is checked.
    if( m_Include_Modules->isChecked() )
    {
        numChecked++;

        if( m_IncludeLockedModules->isChecked() )
            numChecked++;
    }
    else
    {
        // If include modules isn't checked then ignore the "Locked Footprints" checkbox in tally
        numCheckboxes--;
    }

    for( QCheckBox* cb : { m_Include_Tracks, m_Include_Vias, m_Include_Zones, m_Include_Draw_Items,
                           m_Include_Edges_Items, m_Include_PcbTexts } )
    {
        if( cb->isChecked() )
            numChecked++;
    }

    // Change suggestion if all or none are checked
    if( !numChecked )
        return Qt::Unchecked;
    else if( numChecked == numCheckboxes )
        return Qt::Checked;

    return Qt::PartiallyChecked;
}


void DIALOG_FILTER_SELECTION::allItemsClicked( QWidget* aSource )
{
    // A click goes from "some items" to "all items", as with wxWidgets
    bool allItems = m_All_Items->checkState() != Qt::Unchecked;

    {
        QSignalBlocker blocker( m_All_Items );
        m_All_Items->setCheckState( allItems ? Qt::Checked : Qt::Unchecked );
    }

    forceCheckboxStates( allItems );
}


bool DIALOG_FILTER_SELECTION::TransferDataFromWindow()
{
    m_options.includeFootprints        = m_Include_Modules->isChecked();
    m_options.includeLockedFootprints  = m_IncludeLockedModules->isChecked();
    m_options.includeTracks            = m_Include_Tracks->isChecked();
    m_options.includeVias              = m_Include_Vias->isChecked();
    m_options.includeZones             = m_Include_Zones->isChecked();
    m_options.includeItemsOnTechLayers = m_Include_Draw_Items->isChecked();
    m_options.includeBoardOutlineLayer = m_Include_Edges_Items->isChecked();
    m_options.includePcbTexts          = m_Include_PcbTexts->isChecked();

    return true;
}
