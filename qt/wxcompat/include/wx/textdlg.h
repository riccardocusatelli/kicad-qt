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
 * @file textdlg.h
 * The dialog asking a line of text (wxTextEntryDialog, wxGetTextFromUser()) of the wxWidgets
 * compatibility layer, on QInputDialog.
 */

#pragma once

#include <wx/kiwx_gui.h>
#include <wx/kiwx_validators.h>

#include <memory>

#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>

#include <qt_strings.h>


#define wxGetTextFromUserPromptStr wxS( "Input Text" )
#define wxTextEntryDialogStyle ( wxOK | wxCANCEL | wxCENTRE )


/// The dialog of wxTextEntryDialog: the validator checks the text before the dialog closes.
class wxTextEntryDialogCompat : public QInputDialog
{
public:
    using QInputDialog::QInputDialog;

    void SetValidator( const wxTextValidator& aValidator )
    {
        m_validator.reset( static_cast<wxTextValidator*>( aValidator.Clone() ) );
    }

    void done( int aResult ) override
    {
        if( aResult == QDialog::Accepted && m_validator )
        {
            wxString error = m_validator->IsValid( ToWxString( textValue() ) );

            if( !error.empty() )
            {
                QMessageBox::warning( this, windowTitle(), ToQString( error ) );
                return;
            }
        }

        QInputDialog::done( aResult );
    }

private:
    std::unique_ptr<wxTextValidator> m_validator;
};


class wxTextEntryDialog
{
public:
    wxTextEntryDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                       const wxString& aCaption = wxGetTextFromUserPromptStr,
                       const wxString& aValue = wxEmptyString, long aStyle = wxTextEntryDialogStyle,
                       const wxPoint& aPos = wxDefaultPosition ) :
            m_dialog( aParent.get() )
    {
        m_dialog.setInputMode( QInputDialog::TextInput );
        m_dialog.setLabelText( ToQString( aMessage ) );
        m_dialog.setWindowTitle( ToQString( aCaption ) );
        m_dialog.setTextValue( ToQString( aValue ) );

        if( aStyle & wxTE_PASSWORD )
            m_dialog.setTextEchoMode( QLineEdit::Password );
    }

    int ShowModal() { return m_dialog.exec() == QDialog::Accepted ? wxID_OK : wxID_CANCEL; }

    wxString GetValue() const { return ToWxString( m_dialog.textValue() ); }
    void SetValue( const wxString& aValue ) { m_dialog.setTextValue( ToQString( aValue ) ); }

    void SetTextValidator( const wxTextValidator& aValidator )
    {
        m_dialog.SetValidator( aValidator );
    }

    void SetTextValidator( long aStyle = wxFILTER_NONE )
    {
        m_dialog.SetValidator( wxTextValidator( aStyle ) );
    }

private:
    wxTextEntryDialogCompat m_dialog;
};


/// Ask a line of text; empty if the user cancels.
inline wxString wxGetTextFromUser( const wxString& aMessage,
                                   const wxString& aCaption = wxGetTextFromUserPromptStr,
                                   const wxString& aDefault = wxEmptyString,
                                   wxWidgetArgCompat aParent = nullptr, int aX = wxDefaultCoord,
                                   int aY = wxDefaultCoord, bool aCentre = true )
{
    wxTextEntryDialog dialog( aParent, aMessage, aCaption, aDefault );
    return dialog.ShowModal() == wxID_OK ? dialog.GetValue() : wxString();
}
