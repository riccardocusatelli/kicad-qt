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
 * @file kiwx_validators.h
 * The validators of the wxWidgets compatibility layer of the Qt build (wxcompat): wxValidator,
 * wxTextValidator, wxGenericValidator.  A window keeps its validator (SetValidator()); the data
 * transfers and the validation of the dialogs call the validators of their controls, as with
 * wxWidgets; the text validators filter the characters typed in their text controls.
 */

#ifndef KIWX_VALIDATORS_H
#define KIWX_VALIDATORS_H

#include <kiqt.h>

#include <wx/arrstr.h>
#include <wx/string.h>

class QWidget;
class wxTextCtrl;


/**
 * The window of a validator: a QWidget with the part of the API of wxWindow the validators use
 * on it (m_validatorWindow->IsEnabled()...).
 */
class KIQT_API wxValidatorWindowCompat
{
public:
    wxValidatorWindowCompat( QWidget* aWindow = nullptr ) : m_window( aWindow ) {}

    operator QWidget*() const { return m_window; }
    explicit operator bool() const { return m_window != nullptr; }
    const wxValidatorWindowCompat* operator->() const { return this; }

    bool IsEnabled() const;
    bool IsShown() const;
    void SetFocus() const;
    wxString GetName() const;

private:
    QWidget* m_window;
};


class KIQT_API wxValidator
{
public:
    wxValidator() = default;
    wxValidator( const wxValidator& aOther ) = default;
    virtual ~wxValidator() = default;

    /// A copy of the validator (each window has its own).
    virtual wxValidator* Clone() const { return nullptr; }

    /// Check the value of the window (and show an error to the user).
    virtual bool Validate( QWidget* aParent ) { return true; }

    virtual bool TransferToWindow() { return true; }
    virtual bool TransferFromWindow() { return true; }

    void SetWindow( QWidget* aWindow ) { m_validatorWindow = aWindow; }
    QWidget* GetWindow() const { return m_validatorWindow; }

    static void SuppressBellOnError( bool = true ) {}
    static bool IsSilent() { return true; }

    /// The characters typed in the window that the validator accepts (all by default).
    virtual bool IsValidCharCompat( wxUniChar aChar ) const { return true; }

protected:
    void Copy( const wxValidator& aOther ) { m_validatorWindow = aOther.m_validatorWindow; }

    wxValidatorWindowCompat m_validatorWindow;
};

KIQT_API extern const wxValidator wxDefaultValidator;


enum wxTextValidatorStyle
{
    wxFILTER_NONE = 0x0,
    wxFILTER_EMPTY = 0x1,
    wxFILTER_ASCII = 0x2,
    wxFILTER_ALPHA = 0x4,
    wxFILTER_ALPHANUMERIC = 0x8,
    wxFILTER_DIGITS = 0x10,
    wxFILTER_NUMERIC = 0x20,
    wxFILTER_INCLUDE_LIST = 0x40,
    wxFILTER_INCLUDE_CHAR_LIST = 0x80,
    wxFILTER_EXCLUDE_LIST = 0x100,
    wxFILTER_EXCLUDE_CHAR_LIST = 0x200,
    wxFILTER_XDIGITS = 0x400,
    wxFILTER_SPACE = 0x800
};


class KIQT_API wxTextValidator : public wxValidator
{
public:
    wxTextValidator( long aStyle = wxFILTER_NONE, wxString* aValue = nullptr );
    wxTextValidator( const wxTextValidator& aOther ) = default;

    wxValidator* Clone() const override { return new wxTextValidator( *this ); }

    bool Validate( QWidget* aParent ) override;
    bool TransferToWindow() override;
    bool TransferFromWindow() override;

    long GetStyle() const { return m_validatorStyle; }
    void SetStyle( long aStyle ) { m_validatorStyle = aStyle; }
    bool HasFlag( wxTextValidatorStyle aStyle ) const { return m_validatorStyle & aStyle; }

    void SetCharIncludes( const wxString& aChars ) { m_charIncludes = aChars; }
    void SetCharExcludes( const wxString& aChars ) { m_charExcludes = aChars; }
    void AddCharIncludes( const wxString& aChars ) { m_charIncludes += aChars; }
    void AddCharExcludes( const wxString& aChars ) { m_charExcludes += aChars; }
    const wxString& GetCharIncludes() const { return m_charIncludes; }
    const wxString& GetCharExcludes() const { return m_charExcludes; }

    void SetIncludes( const wxArrayString& aIncludes ) { m_includes = aIncludes; }
    void SetExcludes( const wxArrayString& aExcludes ) { m_excludes = aExcludes; }
    const wxArrayString& GetIncludes() const { return m_includes; }
    const wxArrayString& GetExcludes() const { return m_excludes; }

    /// The error message if @a aValue is not valid, or an empty string.
    virtual wxString IsValid( const wxString& aValue ) const;

    bool IsValidCharCompat( wxUniChar aChar ) const override;

    /// The text control of the validator (nullptr if its window isn't one).
    wxTextCtrl* GetTextEntry();

protected:
    bool IsCharIncluded( wxUniChar aChar ) const;
    bool IsCharExcluded( wxUniChar aChar ) const;

    wxString*     m_stringValue;
    long          m_validatorStyle;
    wxString      m_charIncludes;
    wxString      m_charExcludes;
    wxArrayString m_includes;
    wxArrayString m_excludes;
};


/// The validator of the value of a control (a bool, a string, an int).
class KIQT_API wxGenericValidator : public wxValidator
{
public:
    explicit wxGenericValidator( bool* aValue );
    explicit wxGenericValidator( wxString* aValue );
    explicit wxGenericValidator( int* aValue );

    wxValidator* Clone() const override { return new wxGenericValidator( *this ); }

    bool TransferToWindow() override;
    bool TransferFromWindow() override;

private:
    bool*     m_bool = nullptr;
    wxString* m_string = nullptr;
    int*      m_int = nullptr;
};


/// The validators of the windows (SetValidator(), GetValidator()).
KIQT_API void wxSetValidatorCompat( QWidget* aWindow, const wxValidator& aValidator );
KIQT_API wxValidator* wxGetValidatorCompat( const QWidget* aWindow );

#endif // KIWX_VALIDATORS_H
