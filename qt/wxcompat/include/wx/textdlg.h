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

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <dialog_shim.h>

#include <qt_strings.h>


#define wxGetTextFromUserPromptStr wxS( "Input Text" )
#define wxTextEntryDialogStyle ( wxOK | wxCANCEL | wxCENTRE )


/**
 * A dialog asking a line of text (wxTextEntryDialog): a dialog of KiCad (DIALOG_SHIM) with a
 * message, a text control and the OK and Cancel buttons.  As wxWidgets, the derived dialogs can
 * check the text in TransferDataFromWindow() (the dialog stays open if it returns false).
 */
class wxTextEntryDialog : public DIALOG_SHIM
{
public:
    wxTextEntryDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                       const wxString& aCaption = wxGetTextFromUserPromptStr,
                       const wxString& aValue = wxEmptyString, long aStyle = wxTextEntryDialogStyle,
                       const wxPoint& aPos = wxDefaultPosition ) :
            DIALOG_SHIM( aParent, aCaption )
    {
        QVBoxLayout* mainSizer = new QVBoxLayout( this );

        QLabel* label = new QLabel( ToQString( aMessage ), this );
        mainSizer->addWidget( label );

        m_textctrl = new wxTextCtrl( this, wxID_ANY, aValue );
        m_textctrl->setMinimumWidth( 300 );

        if( aStyle & wxTE_PASSWORD )
            m_textctrl->setEchoMode( QLineEdit::Password );

        mainSizer->addWidget( m_textctrl );

        wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer( this );
        connectStandardButton( buttons->AddStandardButton( QDialogButtonBox::Ok, wxID_OK ),
                               wxID_OK );
        connectStandardButton( buttons->AddStandardButton( QDialogButtonBox::Cancel, wxID_CANCEL ),
                               wxID_CANCEL );
        mainSizer->addWidget( buttons );

        SetupStandardButtons();
        SetInitialFocus( m_textctrl );
        m_textctrl->selectAll();
    }

    wxString GetValue() const { return m_textctrl->GetValue(); }
    void SetValue( const wxString& aValue ) { m_textctrl->SetValue( aValue ); }

    void SetTextValidator( const wxTextValidator& aValidator )
    {
        m_textctrl->SetValidator( aValidator );
    }

    void SetTextValidator( long aStyle = wxFILTER_NONE )
    {
        m_textctrl->SetValidator( wxTextValidator( aStyle ) );
    }

protected:
    wxTextCtrl* m_textctrl;
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
