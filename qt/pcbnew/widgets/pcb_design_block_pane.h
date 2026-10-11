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
 * @file pcb_design_block_pane.h
 * PCB_DESIGN_BLOCK_PANE of the Qt build: the design blocks pane of the board editor, with the
 * options of the placement.  The FILEDLG_IMPORT_BOARD_CONTENTS of the wxWidgets build (a hook of
 * wxFileDialog) has no use in this build.
 */

#ifndef PCB_DESIGN_BLOCK_PANE_H
#define PCB_DESIGN_BLOCK_PANE_H

#include <widgets/design_block_pane.h>
#include <design_block_tree_model_adapter.h>
#include <pcbnew_settings.h>

class PCB_EDIT_FRAME;
class PANEL_DESIGN_BLOCK_CHOOSER;
class QCheckBox;


class PCB_DESIGN_BLOCK_PANE : public DESIGN_BLOCK_PANE
{
public:
    PCB_DESIGN_BLOCK_PANE( PCB_EDIT_FRAME* aParent, const LIB_ID* aPreselect, std::vector<LIB_ID>& aHistoryList );

    void OnCheckBox();
    void UpdateCheckboxes();

protected:
    void setLabelsAndTooltips() override;

    QCheckBox* m_repeatedPlacement;
    QCheckBox* m_placeAsGroup;
    QCheckBox* m_keepAnnotations;
};

#endif
