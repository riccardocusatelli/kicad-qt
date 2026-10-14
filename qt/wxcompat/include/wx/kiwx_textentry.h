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
 * @file kiwx_textentry.h
 * The text entries of the wxWidgets compatibility layer of the Qt build (wxcompat): wxTextEntry,
 * the API of the controls with a text (wxTextCtrl, wxComboBox, wxComboCtrl).  The positions are
 * the indices of the characters of the text.
 */

#ifndef KIWX_TEXTENTRY_H
#define KIWX_TEXTENTRY_H

#include <kiqt.h>

#include <wx/arrstr.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

class QLineEdit;


/// The API of the controls with a text (wxTextEntryBase).
class KIQT_API wxTextEntryBase
{
public:
    virtual ~wxTextEntryBase() = default;

    virtual wxString GetValue() const = 0;
    virtual void SetValue( const wxString& aValue ) = 0;

    /// Set the text without the change event.
    virtual void ChangeValue( const wxString& aValue ) = 0;

    /// Insert a text at the insertion point (in place of the selection).
    virtual void WriteText( const wxString& aText ) = 0;
    virtual void AppendText( const wxString& aText );
    virtual void Replace( long aFrom, long aTo, const wxString& aText );
    void Remove( long aFrom, long aTo ) { Replace( aFrom, aTo, wxEmptyString ); }
    virtual wxString GetRange( long aFrom, long aTo ) const;

    virtual void Clear() { SetValue( wxEmptyString ); }
    bool IsEmpty() const { return GetValue().IsEmpty(); }

    /// Select the characters in [aFrom, aTo); (-1, -1) selects all the text.
    virtual void SetSelection( long aFrom, long aTo ) = 0;
    virtual void GetSelection( long* aFrom, long* aTo ) const = 0;
    void SelectAll() { SetSelection( -1, -1 ); }
    void SelectNone();
    wxString GetStringSelection() const;
    bool HasSelection() const;

    virtual long GetInsertionPoint() const = 0;
    virtual void SetInsertionPoint( long aPos ) = 0;
    void SetInsertionPointEnd() { SetInsertionPoint( GetLastPosition() ); }
    virtual long GetLastPosition() const { return long( GetValue().length() ); }

    virtual bool IsEditable() const = 0;
    virtual void SetEditable( bool aEditable ) = 0;

    virtual void SetMaxLength( unsigned long aLength ) {}
    virtual bool SetHint( const wxString& aHint ) { return false; }
    virtual wxString GetHint() const { return wxString(); }
    virtual bool SetMargins( int aLeft, int aTop = -1 ) { return false; }
    bool SetMargins( const wxPoint& aPt ) { return SetMargins( aPt.x, aPt.y ); }

    virtual void Undo() {}
    virtual void Redo() {}
    virtual void Cut() {}
    virtual void Copy() {}
    virtual void Paste() {}
    virtual bool CanUndo() const { return false; }
    virtual bool CanRedo() const { return false; }
    virtual bool CanCut() const { return HasSelection() && IsEditable(); }
    virtual bool CanCopy() const { return HasSelection(); }
    virtual bool CanPaste() const { return IsEditable(); }

    void ForceUpper() {}
    bool AutoComplete( const wxArrayString& aChoices ) { return false; }
};

/// The controls with a text (wxTextEntry).
class KIQT_API wxTextEntry : public wxTextEntryBase
{
};


/// The API of wxTextEntry on a QLineEdit (the one of a text control, of a combo box...).
class KIQT_API wxLineEditTextEntryCompat : public wxTextEntry
{
public:
    wxString GetValue() const override;
    void SetValue( const wxString& aValue ) override;
    void ChangeValue( const wxString& aValue ) override;
    void WriteText( const wxString& aText ) override;
    void Replace( long aFrom, long aTo, const wxString& aText ) override;
    void SetSelection( long aFrom, long aTo ) override;
    void GetSelection( long* aFrom, long* aTo ) const override;
    long GetInsertionPoint() const override;
    void SetInsertionPoint( long aPos ) override;
    bool IsEditable() const override;
    void SetEditable( bool aEditable ) override;
    void SetMaxLength( unsigned long aLength ) override;
    bool SetHint( const wxString& aHint ) override;
    wxString GetHint() const override;
    bool SetMargins( int aLeft, int aTop = -1 ) override;
    using wxTextEntryBase::SetMargins;
    void Undo() override;
    void Redo() override;
    void Cut() override;
    void Copy() override;
    void Paste() override;
    bool CanUndo() const override;
    bool CanRedo() const override;

protected:
    /// The line edit of the control (nullptr if it has none: a read only combo box).
    virtual QLineEdit* lineEditCompat() const = 0;
};

#endif // KIWX_TEXTENTRY_H
