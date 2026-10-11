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
 * @file dialog_text_entry.h
 * The dialog to enter a text with Qt (include/dialogs/dialog_text_entry.h with wxWidgets): the
 * layout of dialog_text_entry_base.fbp, with the widgets of Qt.
 */

#pragma once

#include <memory>

#include <dialogs/dialog_text_entry_base.h>
#include <validators.h>


/**
 * A KICAD version of wxTextEntryDialog which supports the various improvements/work-arounds
 * from DIALOG_SHIM.
 */
class WX_TEXT_ENTRY_DIALOG : public WX_TEXT_ENTRY_DIALOG_BASE
{
public:
    WX_TEXT_ENTRY_DIALOG( QWidget* aParent, const wxString& aLabel, const wxString& aCaption,
                          const wxString& aDefaultValue = wxEmptyString, bool aExtraWidth = false );

    ~WX_TEXT_ENTRY_DIALOG() override;

    /// Filter the text typed, check it on OK and copy it to the string of the validator (if any).
    void SetTextValidator( const TEXT_VALIDATOR& aValidator );

    wxString GetValue() const;

    bool Validate() override;

    bool TransferDataFromWindow() override;

private:
    std::unique_ptr<TEXT_VALIDATOR> m_validator;
};
