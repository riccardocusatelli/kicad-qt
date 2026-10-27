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

#include <wx/kiwx_msgdlg.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>

#include <wx/intl.h>
#include <wx/log.h>
#include <qt_strings.h>


wxString wxMessageDialog::ButtonLabel::GetAsString() const
{
    if( m_stockId < 0 )
        return m_label;

    switch( m_stockId )
    {
    case wxID_OK:      return _( "OK" );
    case wxID_CANCEL:  return _( "Cancel" );
    case wxID_YES:     return _( "Yes" );
    case wxID_NO:      return _( "No" );
    case wxID_SAVE:    return _( "Save" );
    case wxID_DELETE:  return _( "Delete" );
    case wxID_APPLY:   return _( "Apply" );
    case wxID_CLOSE:   return _( "Close" );
    case wxID_HELP:    return _( "Help" );
    default:           return wxString();
    }
}


wxMessageDialog::wxMessageDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                                  const wxString& aCaption, long aStyle, const wxPoint& aPos ) :
        m_parent( aParent.get() ),
        m_message( aMessage ),
        m_caption( aCaption ),
        m_style( aStyle )
{
}


bool wxMessageDialog::SetYesNoLabels( const ButtonLabel& aYes, const ButtonLabel& aNo )
{
    m_labels[wxID_YES] = aYes.GetAsString();
    m_labels[wxID_NO] = aNo.GetAsString();
    return true;
}


bool wxMessageDialog::SetYesNoCancelLabels( const ButtonLabel& aYes, const ButtonLabel& aNo,
                                            const ButtonLabel& aCancel )
{
    SetYesNoLabels( aYes, aNo );
    m_labels[wxID_CANCEL] = aCancel.GetAsString();
    return true;
}


bool wxMessageDialog::SetOKLabel( const ButtonLabel& aOk )
{
    m_labels[wxID_OK] = aOk.GetAsString();
    return true;
}


bool wxMessageDialog::SetOKCancelLabels( const ButtonLabel& aOk, const ButtonLabel& aCancel )
{
    SetOKLabel( aOk );
    m_labels[wxID_CANCEL] = aCancel.GetAsString();
    return true;
}


int wxMessageDialog::ShowModal()
{
    QMessageBox::Icon icon = QMessageBox::NoIcon;

    if( m_style & wxICON_ERROR )
        icon = QMessageBox::Critical;
    else if( m_style & wxICON_WARNING )
        icon = QMessageBox::Warning;
    else if( m_style & wxICON_QUESTION )
        icon = QMessageBox::Question;
    else if( m_style & wxICON_INFORMATION )
        icon = QMessageBox::Information;

    QMessageBox box( icon, ToQString( m_caption ), ToQString( m_message ), QMessageBox::NoButton,
                     m_parent );

    if( !m_extendedMessage.IsEmpty() )
        box.setInformativeText( ToQString( m_extendedMessage ) );

    if( !m_detailedText.IsEmpty() )
        box.setDetailedText( ToQString( m_detailedText ) );

    QCheckBox* checkBox = nullptr;

    if( !m_checkBoxText.IsEmpty() )
    {
        checkBox = new QCheckBox( ToQString( m_checkBoxText ) );
        checkBox->setChecked( m_checkBoxValue );
        box.setCheckBox( checkBox );
    }

    // The buttons of the style, with the labels of KiCad
    std::map<QAbstractButton*, int> codes;

    auto addButton =
            [&]( QMessageBox::StandardButton aButton, int aCode )
            {
                QPushButton* button = box.addButton( aButton );
                auto         label = m_labels.find( aCode );

                if( label != m_labels.end() && !label->second.IsEmpty() )
                    button->setText( ToQString( label->second ) );

                codes[button] = aCode;
                return button;
            };

    if( m_style & wxYES_NO )
    {
        QPushButton* yes = addButton( QMessageBox::Yes, wxID_YES );
        QPushButton* no = addButton( QMessageBox::No, wxID_NO );
        box.setDefaultButton( ( m_style & wxNO_DEFAULT ) ? no : yes );
    }
    else
    {
        box.setDefaultButton( addButton( QMessageBox::Ok, wxID_OK ) );
    }

    if( m_style & wxCANCEL )
    {
        QPushButton* cancel = addButton( QMessageBox::Cancel, wxID_CANCEL );

        if( m_style & wxCANCEL_DEFAULT )
            box.setDefaultButton( cancel );
    }

    box.exec();

    if( checkBox )
        m_checkBoxValue = checkBox->isChecked();

    auto it = codes.find( box.clickedButton() );

    if( it != codes.end() )
        return it->second;

    // Closed by the window: Cancel if there is one, else No or OK
    return ( m_style & wxCANCEL ) ? wxID_CANCEL : ( m_style & wxYES_NO ) ? wxID_NO : wxID_OK;
}


/// The messages of wxLogGui::Flush() in a message box, as wxWidgets.
static void showLogMessagesCompat( const wxArrayString& aMessages, wxLogLevel aLevel )
{
    QStringList lines;

    for( const wxString& message : aMessages )
        lines.append( ToQString( message ) );

    QMessageBox box( aLevel == wxLOG_Error   ? QMessageBox::Critical
                     : aLevel == wxLOG_Warning ? QMessageBox::Warning
                                               : QMessageBox::Information,
                     aLevel == wxLOG_Error   ? ToQString( _( "Error" ) )
                     : aLevel == wxLOG_Warning ? ToQString( _( "Warning" ) )
                                               : ToQString( _( "Information" ) ),
                     lines.join( QLatin1Char( '\n' ) ), QMessageBox::Ok, QApplication::activeWindow() );
    box.exec();
}


static const bool s_logGuiCompat = []()
{
    wxLogGui::SetShowFunction( &showLogMessagesCompat );
    return true;
}();
