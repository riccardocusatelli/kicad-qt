/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file dialog_unit_entry.h
 * The dialogs to enter a value (or a point) with units, of the Qt build: the layout of
 * dialog_unit_entry_base.fbp, with the widgets of Qt.
 */
#pragma once

#include <math/vector2d.h>
#include <widgets/unit_binder.h>

#include <dialogs/dialog_unit_entry_base.h>

class EDA_DRAW_FRAME;


/**
 * An extension of WX_TEXT_ENTRY_DIALOG that uses UNIT_BINDER to request a dimension
 * (e.g. mm, inches, etc) from the user according to the selected units (Qt version of
 * include/dialogs/dialog_unit_entry.h).
 */
class WX_UNIT_ENTRY_DIALOG : public WX_UNIT_ENTRY_DIALOG_BASE
{
public:
    WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabel,
                          long long int aDefaultValue );

    /**
     * Return the value in internal units.
     */
    int GetValue();

private:
    UNIT_BINDER m_unit_binder;
};


class WX_PT_ENTRY_DIALOG : public WX_PT_ENTRY_DIALOG_BASE
{
public:
    WX_PT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabelX,
                        const wxString& aLabelY, const VECTOR2I& aDefaultValue, bool aShowResetButt );

    /**
     * Return the value in internal units.
     */
    VECTOR2I GetValue();

    void ResetValues( QWidget* aSource ) override;

private:
    UNIT_BINDER m_unit_binder_x;
    UNIT_BINDER m_unit_binder_y;
};
