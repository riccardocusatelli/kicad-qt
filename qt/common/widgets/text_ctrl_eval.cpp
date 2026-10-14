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
 * @file text_ctrl_eval.cpp
 * TEXT_CTRL_EVAL of the Qt build (see common/widgets/text_ctrl_eval.cpp).
 */

#include <widgets/text_ctrl_eval.h>

#include <QSignalBlocker>

#include <dialog_shim.h>
#include <qt_strings.h>


TEXT_CTRL_EVAL::TEXT_CTRL_EVAL( wxWidgetArgCompat aParent, wxWindowID aId,
                                const wxString& aValue, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle ) :
        wxTextCtrl( aParent, aId, aValue, aPos, aSize, aStyle ),
        m_eval( EDA_UNITS::UNSCALED )
{
    QObject::connect( this, &QLineEdit::returnPressed, this, [this]() { onTextEnter(); } );
}


void TEXT_CTRL_EVAL::SetValue( const wxString& aValue )
{
    setText( ToQString( aValue ) );
    m_eval.Clear();
}


wxString TEXT_CTRL_EVAL::GetValue() const
{
    return ToWxString( text() );
}


void TEXT_CTRL_EVAL::ChangeValue( const wxString& aValue )
{
    QSignalBlocker blocker( this );
    setText( ToQString( aValue ) );
    m_eval.Clear();
}


void TEXT_CTRL_EVAL::focusInEvent( QFocusEvent* aEvent )
{
    wxString oldStr = m_eval.OriginalText();

    if( oldStr.length() )
        SetValue( oldStr );

    QLineEdit::focusInEvent( aEvent );
}


void TEXT_CTRL_EVAL::focusOutEvent( QFocusEvent* aEvent )
{
    if( m_customEval.has_value() )
        m_customEval.value()( this );
    else
        evaluate();

    QLineEdit::focusOutEvent( aEvent );
}


void TEXT_CTRL_EVAL::onTextEnter()
{
    evaluate();

    // Accept the changes and close the parent dialog (from the event loop)
    if( DIALOG_SHIM* dialog = dynamic_cast<DIALOG_SHIM*>( window() ) )
    {
        QMetaObject::invokeMethod( dialog,
                                   [dialog]()
                                   {
                                       if( dialog->IsQuasiModal() )
                                           dialog->EndQuasiModal( wxID_OK );
                                       else
                                           dialog->EndModal( wxID_OK );
                                   },
                                   Qt::QueuedConnection );
    }
}


void TEXT_CTRL_EVAL::evaluate()
{
    if( m_eval.Process( GetValue() ) )
        SetValue( m_eval.Result() );
}
