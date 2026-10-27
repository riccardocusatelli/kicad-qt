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
 * @file choicdlg.h
 * The dialog choosing one string of a list (wxSingleChoiceDialog, wxGetSingleChoiceIndex()) of
 * the wxWidgets compatibility layer.
 */

#pragma once

#include <wx/kiwx_gui.h>

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include <dialog_shim.h>

#include <qt_strings.h>


#define wxCHOICEDLG_STYLE ( wxDEFAULT_DIALOG_STYLE | wxOK | wxCANCEL | wxCENTRE )


/**
 * A dialog choosing a string of a list (wxSingleChoiceDialog): a dialog of KiCad (DIALOG_SHIM)
 * with a message, a list and the OK and Cancel buttons.  A double click chooses the item.
 */
class wxSingleChoiceDialog : public DIALOG_SHIM
{
public:
    wxSingleChoiceDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                          const wxString& aCaption, const wxArrayString& aChoices,
                          void** aClientData = nullptr, long aStyle = wxCHOICEDLG_STYLE,
                          const wxPoint& aPos = wxDefaultPosition ) :
            DIALOG_SHIM( aParent, aCaption )
    {
        QVBoxLayout* mainSizer = new QVBoxLayout( this );

        QLabel* label = new QLabel( ToQString( aMessage ), this );
        mainSizer->addWidget( label );

        m_listbox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, aChoices );
        m_listbox->setMinimumSize( 300, 200 );
        mainSizer->addWidget( m_listbox );

        if( !aChoices.IsEmpty() )
            m_listbox->SetSelection( 0 );

        QObject::connect( m_listbox, &QListWidget::itemDoubleClicked, this,
                          [this]()
                          {
                              EndModal( wxID_OK );
                          } );

        wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer( this );
        connectStandardButton( buttons->AddStandardButton( QDialogButtonBox::Ok, wxID_OK ),
                               wxID_OK );
        connectStandardButton( buttons->AddStandardButton( QDialogButtonBox::Cancel, wxID_CANCEL ),
                               wxID_CANCEL );
        mainSizer->addWidget( buttons );

        SetupStandardButtons();
        SetInitialFocus( m_listbox );
    }

    int GetSelection() const { return m_listbox->GetSelection(); }
    void SetSelection( int aIndex ) { m_listbox->SetSelection( aIndex ); }
    wxString GetStringSelection() const { return m_listbox->GetStringSelection(); }

protected:
    wxListBox* m_listbox;
};


/// Choose a string of a list; the index of the choice, -1 if the user cancels.
inline int wxGetSingleChoiceIndex( const wxString& aMessage, const wxString& aCaption,
                                   const wxArrayString& aChoices,
                                   wxWidgetArgCompat aParent = nullptr, int aX = wxDefaultCoord,
                                   int aY = wxDefaultCoord, bool aCentre = true,
                                   int aWidth = 0, int aHeight = 0, int aInitialSelection = 0 )
{
    wxSingleChoiceDialog dialog( aParent, aMessage, aCaption, aChoices );
    dialog.SetSelection( aInitialSelection );
    return dialog.ShowModal() == wxID_OK ? dialog.GetSelection() : -1;
}
