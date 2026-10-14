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

#include <wx/kiwx_textentry.h>

#include <algorithm>

#include <QLineEdit>
#include <QSignalBlocker>

#include <qt_strings.h>


// wxTextEntry ------------------------------------------------------------------------------

void wxTextEntry::AppendText( const wxString& aText )
{
    SetInsertionPointEnd();
    WriteText( aText );
}


void wxTextEntry::Replace( long aFrom, long aTo, const wxString& aText )
{
    wxString value = GetValue();
    long     to = std::clamp<long>( aTo, 0, long( value.length() ) );
    long     from = std::clamp<long>( aFrom, 0, to );

    value.replace( size_t( from ), size_t( to - from ), aText );
    SetValue( value );
    SetInsertionPoint( from + long( aText.length() ) );
}


wxString wxTextEntry::GetRange( long aFrom, long aTo ) const
{
    wxString value = GetValue();
    long     to = std::clamp<long>( aTo, 0, long( value.length() ) );
    long     from = std::clamp<long>( aFrom, 0, to );

    return value.Mid( size_t( from ), size_t( to - from ) );
}


void wxTextEntry::SelectNone()
{
    long pos = GetInsertionPoint();
    SetSelection( pos, pos );
}


wxString wxTextEntry::GetStringSelection() const
{
    long from = 0;
    long to = 0;
    GetSelection( &from, &to );
    return GetRange( from, to );
}


bool wxTextEntry::HasSelection() const
{
    long from = 0;
    long to = 0;
    GetSelection( &from, &to );
    return to > from;
}


// wxLineEditTextEntryCompat ----------------------------------------------------------------

wxString wxLineEditTextEntryCompat::GetValue() const
{
    QLineEdit* edit = lineEditCompat();
    return edit ? ToWxString( edit->text() ) : wxString();
}


void wxLineEditTextEntryCompat::SetValue( const wxString& aValue )
{
    if( QLineEdit* edit = lineEditCompat() )
    {
        edit->setText( ToQString( aValue ) );
        edit->setModified( false );
    }
}


void wxLineEditTextEntryCompat::ChangeValue( const wxString& aValue )
{
    if( QLineEdit* edit = lineEditCompat() )
    {
        QSignalBlocker blocker( edit );
        SetValue( aValue );
    }
}


void wxLineEditTextEntryCompat::WriteText( const wxString& aText )
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->insert( ToQString( aText ) );
}


void wxLineEditTextEntryCompat::Replace( long aFrom, long aTo, const wxString& aText )
{
    QLineEdit* edit = lineEditCompat();

    if( !edit )
        return;

    // A selection replaced by an insertion: the text can be undone, the cursor follows it
    long length = edit->text().size();
    long to = std::clamp<long>( aTo, 0, length );
    long from = std::clamp<long>( aFrom, 0, to );

    edit->setSelection( int( from ), int( to - from ) );
    edit->insert( ToQString( aText ) );
}


void wxLineEditTextEntryCompat::SetSelection( long aFrom, long aTo )
{
    QLineEdit* edit = lineEditCompat();

    if( !edit )
        return;

    // (-1, -1) selects all the text
    if( aFrom == -1 && aTo == -1 )
    {
        edit->selectAll();
        return;
    }

    long length = edit->text().size();
    long to = aTo < 0 ? length : std::min( aTo, length );
    long from = std::clamp( aFrom, 0L, to );

    if( from == to )
        edit->setCursorPosition( int( from ) );
    else
        edit->setSelection( int( from ), int( to - from ) );
}


void wxLineEditTextEntryCompat::GetSelection( long* aFrom, long* aTo ) const
{
    QLineEdit* edit = lineEditCompat();
    long       start = 0;
    long       length = 0;

    if( edit )
    {
        start = edit->hasSelectedText() ? edit->selectionStart() : edit->cursorPosition();
        length = edit->hasSelectedText() ? edit->selectedText().size() : 0;
    }

    if( aFrom )
        *aFrom = start;

    if( aTo )
        *aTo = start + length;
}


long wxLineEditTextEntryCompat::GetInsertionPoint() const
{
    QLineEdit* edit = lineEditCompat();
    return edit ? edit->cursorPosition() : 0;
}


void wxLineEditTextEntryCompat::SetInsertionPoint( long aPos )
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->setCursorPosition( int( aPos ) );
}


bool wxLineEditTextEntryCompat::IsEditable() const
{
    QLineEdit* edit = lineEditCompat();
    return edit && !edit->isReadOnly();
}


void wxLineEditTextEntryCompat::SetEditable( bool aEditable )
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->setReadOnly( !aEditable );
}


void wxLineEditTextEntryCompat::SetMaxLength( unsigned long aLength )
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->setMaxLength( aLength ? int( aLength ) : 32767 );
}


bool wxLineEditTextEntryCompat::SetHint( const wxString& aHint )
{
    QLineEdit* edit = lineEditCompat();

    if( !edit )
        return false;

    edit->setPlaceholderText( ToQString( aHint ) );
    return true;
}


wxString wxLineEditTextEntryCompat::GetHint() const
{
    QLineEdit* edit = lineEditCompat();
    return edit ? ToWxString( edit->placeholderText() ) : wxString();
}


bool wxLineEditTextEntryCompat::SetMargins( int aLeft, int aTop )
{
    QLineEdit* edit = lineEditCompat();

    if( !edit )
        return false;

    edit->setTextMargins( std::max( aLeft, 0 ), 0, 0, 0 );
    return true;
}


void wxLineEditTextEntryCompat::Undo()
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->undo();
}


void wxLineEditTextEntryCompat::Redo()
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->redo();
}


void wxLineEditTextEntryCompat::Cut()
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->cut();
}


void wxLineEditTextEntryCompat::Copy()
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->copy();
}


void wxLineEditTextEntryCompat::Paste()
{
    if( QLineEdit* edit = lineEditCompat() )
        edit->paste();
}


bool wxLineEditTextEntryCompat::CanUndo() const
{
    QLineEdit* edit = lineEditCompat();
    return edit && edit->isUndoAvailable();
}


bool wxLineEditTextEntryCompat::CanRedo() const
{
    QLineEdit* edit = lineEditCompat();
    return edit && edit->isRedoAvailable();
}
