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

#include <memory>

#include <dialog_shim.h>
#include <math/vector2d.h>

class QLabel;
class QLineEdit;
class QPushButton;
class EDA_DRAW_FRAME;
class UNIT_BINDER;


/**
 * An extension of WX_TEXT_ENTRY_DIALOG that uses UNIT_BINDER to request a dimension
 * (e.g. grid spacing or line width) from the user according to the selected units.
 */
class WX_UNIT_ENTRY_DIALOG : public DIALOG_SHIM
{
public:
    WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabel,
                          long long int aDefaultValue );

    ~WX_UNIT_ENTRY_DIALOG();

    /**
     * Return the value in internal units.
     */
    int GetValue();

private:
    QLabel*    m_label;
    QLineEdit* m_textCtrl;
    QLabel*    m_unit_label;

    std::unique_ptr<UNIT_BINDER> m_unit_binder;
};


/**
 * A dialog to request a point (x and y) from the user, with the units of the frame.
 */
class WX_PT_ENTRY_DIALOG : public DIALOG_SHIM
{
public:
    WX_PT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabelX,
                        const wxString& aLabelY, const VECTOR2I& aDefaultValue, bool aShowResetButt );

    ~WX_PT_ENTRY_DIALOG();

    /**
     * Return the value in internal units.
     */
    VECTOR2I GetValue();

    void ResetValues();

private:
    QLabel*      m_labelX;
    QLineEdit*   m_textCtrlX;
    QLabel*      m_unitsX;
    QLabel*      m_labelY;
    QLineEdit*   m_textCtrlY;
    QLabel*      m_unitsY;
    QPushButton* m_ButtonReset;

    std::unique_ptr<UNIT_BINDER> m_unit_binder_x;
    std::unique_ptr<UNIT_BINDER> m_unit_binder_y;
};
