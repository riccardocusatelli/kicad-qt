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
 * @file dialog_text_entry.cpp
 * WX_TEXT_ENTRY_DIALOG with Qt (common/dialogs/dialog_text_entry.cpp with wxWidgets).
 */

#include <dialogs/dialog_text_entry.h>

#include <string_utils.h>

#include <qt_strings.h>


WX_TEXT_ENTRY_DIALOG::WX_TEXT_ENTRY_DIALOG( QWidget* aParent, const wxString& aFieldLabel,
                                            const wxString& aCaption, const wxString& aDefaultValue,
                                            bool aExtraWidth ) :
        WX_TEXT_ENTRY_DIALOG_BASE( aParent, aCaption )
{
    if( aFieldLabel.IsEmpty() )
        m_label->hide();
    else
        m_label->setText( ToQString( aFieldLabel ) );

    m_textCtrl->setText( ToQString( aDefaultValue ) );
    m_textCtrl->setMinimumWidth( aExtraWidth ? 700 : 300 );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state
    // between usage cases.
    m_hash_key = TO_UTF8( aCaption + aFieldLabel );

    // The text value is always supplied by the caller through aDefaultValue and is specific to
    // the item being edited.  Persisting and restoring it would clobber that value with whatever
    // was last entered, so opt the control out of DIALOG_SHIM state save/restore.
    OptOut( m_textCtrl );

    SetupStandardButtons();

    SetInitialFocus( m_textCtrl );

    adjustSize();
}


WX_TEXT_ENTRY_DIALOG::~WX_TEXT_ENTRY_DIALOG()
{
}


void WX_TEXT_ENTRY_DIALOG::SetTextValidator( const TEXT_VALIDATOR& aValidator )
{
    m_validator = aValidator.Clone();
    m_textCtrl->setValidator( m_validator->CreateQtValidator( m_textCtrl ) );
}


wxString WX_TEXT_ENTRY_DIALOG::GetValue() const
{
    return ToWxString( m_textCtrl->text() );
}


bool WX_TEXT_ENTRY_DIALOG::Validate()
{
    if( m_validator && !m_validator->Validate( this, GetValue() ) )
    {
        m_textCtrl->setFocus();
        return false;
    }

    return true;
}


bool WX_TEXT_ENTRY_DIALOG::TransferDataFromWindow()
{
    if( m_validator )
        m_validator->TransferFromWindow( GetValue() );

    return true;
}
