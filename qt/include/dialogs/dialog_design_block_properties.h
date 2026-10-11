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

/**
 * @file dialog_design_block_properties.h
 * DIALOG_DESIGN_BLOCK_PROPERTIES of the Qt build: the name, the keywords, the description and
 * the default fields of a design block (common/dialogs/dialog_design_block_properties.h with
 * wxWidgets), on the base generated from its wxFormBuilder file.
 */

#ifndef DIALOG_DESIGN_BLOCK_PROPERTIES_H
#define DIALOG_DESIGN_BLOCK_PROPERTIES_H

#include <dialogs/dialog_design_block_properties_base.h>
#include <json_common.h>

class DESIGN_BLOCK;


class DIALOG_DESIGN_BLOCK_PROPERTIES : public DIALOG_DESIGN_BLOCK_PROPERTIES_BASE
{
public:
    DIALOG_DESIGN_BLOCK_PROPERTIES( QWidget* aParent, DESIGN_BLOCK* aDesignBlock, bool aDisableName = false );
    ~DIALOG_DESIGN_BLOCK_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool TransferDataToGrid();
    bool TransferDataFromGrid();

    void OnAddField( QWidget* aSource ) override;
    void OnDeleteField( QWidget* aSource ) override;
    void OnMoveFieldUp( QWidget* aSource ) override;
    void OnMoveFieldDown( QWidget* aSource ) override;

private:
    /// Swap two rows of the fields grid.
    void swapRows( int aRow1, int aRow2 );

    DESIGN_BLOCK*                             m_designBlock;
    nlohmann::ordered_map<wxString, wxString> m_fields;
};

#endif
