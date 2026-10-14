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
 * @file kiwx_msgdlg.h
 * The message dialogs of the wxWidgets compatibility layer of the Qt build (wxcompat):
 * wxMessageDialog and wxRichMessageDialog on QMessageBox.  ShowModal() gives the codes of
 * wxWidgets (wxID_OK, wxID_YES, wxID_NO, wxID_CANCEL).
 */

#ifndef KIWX_MSGDLG_H
#define KIWX_MSGDLG_H

#include <kiqt.h>

#include <map>

#include <wx/string.h>
#include <wx/kiwx_window.h>
#include <wx/toplevel.h>
#include <wx/translation.h>


/// A message dialog (wxMessageDialog).
class KIQT_API wxMessageDialog
{
public:
    /// A label of a button: a text or a stock id (wxID_SAVE...).
    class ButtonLabel
    {
    public:
        ButtonLabel( const wxString& aLabel ) : m_label( aLabel ), m_stockId( -1 ) {}
        ButtonLabel( const char* aLabel ) : m_label( aLabel ), m_stockId( -1 ) {}
        ButtonLabel( const wchar_t* aLabel ) : m_label( aLabel ), m_stockId( -1 ) {}
        ButtonLabel( int aStockId ) : m_stockId( aStockId ) {}

        wxString GetAsString() const;
        int GetStockId() const { return m_stockId; }

    private:
        wxString m_label;
        int      m_stockId;
    };

    wxMessageDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                     const wxString& aCaption = wxString( "Message" ), long aStyle = wxOK | wxCENTRE,
                     const wxPoint& aPos = wxDefaultPosition );
    virtual ~wxMessageDialog() = default;

    /// Show the dialog: wxID_OK, wxID_YES, wxID_NO or wxID_CANCEL.
    virtual int ShowModal();

    /// Show the dialog (modally: a message dialog of Qt has no other mode).
    virtual bool Show( bool aShow = true )
    {
        if( aShow )
            wxMessageDialog::ShowModal();

        return true;
    }

    void SetMessage( const wxString& aMessage ) { m_message = aMessage; }
    void SetExtendedMessage( const wxString& aMessage ) { m_extendedMessage = aMessage; }
    wxString GetMessage() const { return m_message; }
    void SetTitle( const wxString& aTitle ) { m_caption = aTitle; }

    virtual bool SetYesNoLabels( const ButtonLabel& aYes, const ButtonLabel& aNo );
    virtual bool SetYesNoCancelLabels( const ButtonLabel& aYes, const ButtonLabel& aNo,
                               const ButtonLabel& aCancel );
    virtual bool SetOKLabel( const ButtonLabel& aOk );
    virtual bool SetOKCancelLabels( const ButtonLabel& aOk, const ButtonLabel& aCancel );
    virtual bool SetHelpLabel( const ButtonLabel& aHelp ) { return true; }

    long GetMessageDialogStyle() const { return m_style; }

protected:
    QWidget*                m_parent;
    wxString                m_message;
    wxString                m_extendedMessage;
    wxString                m_caption;
    long                    m_style;
    std::map<int, wxString> m_labels;    ///< The labels of the buttons, by their wxID_ code

    // The ones of a rich message dialog
    wxString                m_checkBoxText;
    bool                    m_checkBoxValue = false;
    wxString                m_detailedText;
};

using wxGenericMessageDialog = wxMessageDialog;


/// A message dialog with a check box and a detailed text (wxRichMessageDialog).
class KIQT_API wxRichMessageDialog : public wxMessageDialog
{
public:
    using wxMessageDialog::wxMessageDialog;

    void ShowCheckBox( const wxString& aText, bool aChecked = false )
    {
        m_checkBoxText = aText;
        m_checkBoxValue = aChecked;
    }

    wxString GetCheckBoxText() const { return m_checkBoxText; }
    bool IsCheckBoxChecked() const { return m_checkBoxValue; }

    void ShowDetailedText( const wxString& aText ) { m_detailedText = aText; }
    wxString GetDetailedText() const { return m_detailedText; }
};

using wxGenericRichMessageDialog = wxRichMessageDialog;

#endif // KIWX_MSGDLG_H
