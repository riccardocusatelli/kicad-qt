/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

/**
 * @file ui_common_qt.cpp
 * The functions of KIUI (include/widgets/ui_common.h) with Qt (common/widgets/ui_common.cpp
 * with wxWidgets): the fonts, the sizes of the texts and the state of the controls.  The menu
 * functions of wxWidgets have no Qt version (the menus of the Qt build are ACTION_MENUs).
 */

#include <widgets/ui_common.h>
#include <wx/kiwx_window.h>

#include <QAbstractSpinBox>
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSlider>
#include <QTableWidget>
#include <QTextEdit>

#include <string_utils.h>
#include <wx/kiwx_gui.h>

#include <qt_strings.h>
#include <qt_ui.h>


const wxString KIUI::s_FocusStealableInputName = wxS( "KI_NOFOCUS" );


int KIUI::GetStdMargin()
{
    // This is the value used in (most) wxFB dialogs
    return 5;
}


wxSize KIUI::GetTextSize( const wxString& aSingleLine, wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    QFontMetrics metrics( aWindow ? aWindow->font() : QApplication::font() );
    QSize        size = metrics.size( Qt::TextSingleLine, ToQString( aSingleLine ) );

    return wxSize( size.width(), size.height() );
}


wxFont KIUI::GetMonospacedUIFont()
{
    return QFontDatabase::systemFont( QFontDatabase::FixedFont );
}


wxFont KIUI::GetControlFont( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    return aWindow ? aWindow->font() : QApplication::font();
}


wxFont KIUI::GetInfoFont( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    return KIQT::InfoFont( aWindow );
}


wxFont KIUI::GetSmallInfoFont( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    QFont font = KIQT::InfoFont( aWindow );

    if( font.pointSizeF() > 1 )
        font.setPointSizeF( font.pointSizeF() - 1 );

    return font;
}


wxFont KIUI::GetDockedPaneFont( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    return KIQT::DockedPaneFont( aWindow );
}


wxFont KIUI::GetStatusFont( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    return KIQT::InfoFont( aWindow );
}


bool KIUI::EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString )
{
    QWidget* window = aCtrl->parentWidget() ? aCtrl->parentWidget() : aCtrl;
    wxString ctrlText = aString ? *aString : aCtrl->GetValue();
    wxSize   textSize = GetTextSize( ctrlText, wxWindowFromQt( window ) );

    if( aCtrl->width() < textSize.x + 10 )
    {
        aCtrl->setMinimumWidth( textSize.x + 10 );
        return true;
    }

    return false;
}


wxString KIUI::EllipsizeStatusText( wxWindow* aWindowCompat, const wxString& aString )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    wxString msg = UnescapeString( aString );

    msg.Replace( wxT( "\n" ), wxT( " " ) );
    msg.Replace( wxT( "\r" ), wxT( " " ) );
    msg.Replace( wxT( "\t" ), wxT( " " ) );

    int statusWidth = aWindow ? aWindow->width() : 0;

    // 30% of the first 800 pixels plus 60% of the remaining width
    int textWidth = std::min( statusWidth, 800 ) * 0.3 + std::max( statusWidth - 800, 0 ) * 0.6;

    QFontMetrics metrics( aWindow ? aWindow->font() : QApplication::font() );
    return ToWxString( metrics.elidedText( ToQString( msg ), Qt::ElideRight, textWidth ) );
}


void KIUI::SelectReferenceNumber( wxTextEntry* aTextEntry )
{
    wxString ref = aTextEntry->GetValue();

    if( ref.find_first_of( '?' ) != ref.npos )
    {
        aTextEntry->SetSelection( ref.find_first_of( '?' ), ref.find_last_of( '?' ) + 1 );
    }
    else if( ref.find_first_of( '*' ) != ref.npos )
    {
        aTextEntry->SetSelection( ref.find_first_of( '*' ), ref.find_last_of( '*' ) + 1 );
    }
    else
    {
        wxString num = ref;

        while( !num.IsEmpty() && ( !isdigit( num.Last() ) || !isdigit( num.GetChar( 0 ) ) ) )
        {
            // Trim non-digit from end
            if( !isdigit( num.Last() ) )
                num.RemoveLast();

            // Trim non-digit from the start
            if( !num.IsEmpty() && !isdigit( num.GetChar( 0 ) ) )
                num = num.Right( num.Length() - 1 );
        }

        aTextEntry->SetSelection( ref.Find( num ), ref.Find( num ) + num.Length() );

        if( num.IsEmpty() )
            aTextEntry->SetSelection( -1, -1 );
    }
}


bool KIUI::IsInputControlFocused( wxWindow* aFocusCompat )
{
    QWidget* aFocus = aFocusCompat ? aFocusCompat->GetHandle() : nullptr;

    if( aFocus == nullptr )
        aFocus = QApplication::focusWidget();

    if( !aFocus )
        return false;

    // These widgets are never considered focused
    if( ToWxString( aFocus->objectName() ) == s_FocusStealableInputName )
        return false;

    return qobject_cast<QLineEdit*>( aFocus ) || qobject_cast<QPlainTextEdit*>( aFocus )
           || qobject_cast<QTextEdit*>( aFocus ) || qobject_cast<QAbstractSpinBox*>( aFocus )
           || qobject_cast<QComboBox*>( aFocus ) || qobject_cast<QListWidget*>( aFocus )
           || qobject_cast<QAbstractButton*>( aFocus ) || qobject_cast<QSlider*>( aFocus )
           || qobject_cast<QAbstractItemView*>( aFocus );
}


bool KIUI::IsInputControlEditable( wxWindow* aControlCompat )
{
    QWidget* aControl = aControlCompat ? aControlCompat->GetHandle() : nullptr;

    if( QLineEdit* lineEdit = qobject_cast<QLineEdit*>( aControl ) )
        return !lineEdit->isReadOnly();

    if( QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>( aControl ) )
        return !textEdit->isReadOnly();

    if( QTextEdit* textEdit = qobject_cast<QTextEdit*>( aControl ) )
        return !textEdit->isReadOnly();

    // Must return true if we can't determine the state, intentionally true for non inputs as well.
    return true;
}


bool KIUI::IsModalDialogFocused()
{
    return QApplication::activeModalWidget() != nullptr;
}


void KIUI::FocusNextWindow( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    // The focus chain of Qt, after the children of the window
    for( QWidget* test = aWindow->nextInFocusChain(); test && test != aWindow;
         test = test->nextInFocusChain() )
    {
        if( aWindow->isAncestorOf( test ) || test->window() != aWindow->window() )
            continue;

        if( test->isVisible() && test->isEnabled() && ( test->focusPolicy() & Qt::TabFocus ) )
        {
            test->setFocus( Qt::TabFocusReason );
            return;
        }
    }
}


void KIUI::Disable( wxWindow* aWindowCompat )
{
    QWidget* aWindow = aWindowCompat ? aWindowCompat->GetHandle() : nullptr;

    if( QTableWidget* grid = qobject_cast<QTableWidget*>( aWindow ) )
    {
        // The cells are read only, the grid can still be scrolled
        grid->setEditTriggers( QAbstractItemView::NoEditTriggers );
    }
    else if( QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>( aWindow ) )
    {
        textEdit->setReadOnly( true );
    }
    else if( qobject_cast<QAbstractScrollArea*>( aWindow ) && aWindow->findChildren<QWidget*>().size() > 0
             && !qobject_cast<QAbstractItemView*>( aWindow ) )
    {
        for( QWidget* child : aWindow->findChildren<QWidget*>( Qt::FindDirectChildrenOnly ) )
            Disable( wxWindowFromQt( child ) );
    }
    else if( aWindow->findChildren<QWidget*>( Qt::FindDirectChildrenOnly ).isEmpty()
             || qobject_cast<QAbstractItemView*>( aWindow ) || qobject_cast<QComboBox*>( aWindow )
             || qobject_cast<QAbstractSpinBox*>( aWindow ) )
    {
        // A control
        aWindow->setEnabled( false );
    }
    else
    {
        for( QWidget* child : aWindow->findChildren<QWidget*>( Qt::FindDirectChildrenOnly ) )
            Disable( wxWindowFromQt( child ) );
    }
}
