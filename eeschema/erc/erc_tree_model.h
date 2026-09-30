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

#ifndef ERC_TREE_MODEL_H
#define ERC_TREE_MODEL_H

#include <widgets/rc_tree_model.h>


/**
 * A specialisation of the RC_TREE_MODEL class to enable ERC errors / warnings to be resolved in
 * a specific sheet path context. This allows the displayed ERC descriptions in the ERC dialog to
 * reflect component references on a per-sheet basis in hierarchical schematics.
 * @see RC_TREE_MODEL
 */
class ERC_TREE_MODEL : public RC_TREE_MODEL
{
public:
    ERC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView ) :
            RC_TREE_MODEL( aParentFrame, aView )
    {
    }

    ~ERC_TREE_MODEL() {}

    /**
     * Override of RC_TREE_MODEL::GetValue which returns item descriptions in a specific
     * SCH_SHEET_PATH context, if a context is available on the given SCH_MARKER or ERC_ITEM
     * targets.
     */
    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) const override;
};

#endif // ERC_TREE_MODEL_H
