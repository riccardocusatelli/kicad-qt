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
 * @file validators.h
 * The validators of the text controls with Qt (include/validators.h with wxWidgets).
 *
 * A TEXT_VALIDATOR has the rules of a wxTextValidator of KiCad: the characters that can't be
 * typed, the check of the whole text (with its error message) and the string that receives the
 * text when the dialog is accepted.  CreateQtValidator() makes the QValidator filtering what is
 * typed in a QLineEdit.
 */

#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <memory>

#include <kiqt.h>

#include <wx/string.h>

class QObject;
class QValidator;
class QWidget;


class KIQT_API TEXT_VALIDATOR
{
public:
    explicit TEXT_VALIDATOR( wxString* aValue = nullptr ) :
            m_value( aValue )
    {
    }

    virtual ~TEXT_VALIDATOR() = default;

    virtual std::unique_ptr<TEXT_VALIDATOR> Clone() const
    {
        return std::make_unique<TEXT_VALIDATOR>( *this );
    }

    /// The characters that can't be typed.
    void SetCharExcludes( const wxString& aChars ) { m_charExcludes = aChars; }
    const wxString& GetCharExcludes() const { return m_charExcludes; }

    /// @return the error message if @a aValue is invalid, or an empty string.
    virtual wxString IsValid( const wxString& aValue ) const;

    /// The title of the message showing the error.
    virtual wxString ErrorTitle() const;

    /**
     * Check @a aValue, and show the error to the user if it is invalid.
     *
     * @return true if @a aValue is valid.
     */
    bool Validate( QWidget* aParent, const wxString& aValue ) const;

    /// Copy the text of the control to the string of the validator (if any).
    void TransferFromWindow( const wxString& aValue ) const
    {
        if( m_value )
            *m_value = aValue;
    }

    /// The text to show in the control: the string of the validator (if any).
    const wxString* GetValuePtr() const { return m_value; }

    /// A QValidator filtering the characters typed in a QLineEdit, owned by @a aParent.
    virtual QValidator* CreateQtValidator( QObject* aParent ) const;

protected:
    wxString* m_value;
    wxString  m_charExcludes;
};


/**
 * Limit the allowable characters when defining footprint names.
 *
 * Since the introduction of the PRETTY footprint library format, footprint names cannot have
 * any characters that would prevent file creation on any platform.  The characters \/:*?|"<>
 * are illegal and filtered by the validator.
 */
class KIQT_API FOOTPRINT_NAME_VALIDATOR : public TEXT_VALIDATOR
{
public:
    FOOTPRINT_NAME_VALIDATOR( wxString* aValue = nullptr );

    std::unique_ptr<TEXT_VALIDATOR> Clone() const override
    {
        return std::make_unique<FOOTPRINT_NAME_VALIDATOR>( *this );
    }
};


/**
 * Limit the allowable characters when defining an environment variable name.
 *
 * Only uppercase, numbers, and underscore (_) characters are valid and the first character of
 * the name cannot start with a number.  This is according to IEEE Std 1003.1-2001.  The lower
 * case letters are converted to upper case.
 */
class KIQT_API ENV_VAR_NAME_VALIDATOR : public TEXT_VALIDATOR
{
public:
    ENV_VAR_NAME_VALIDATOR( wxString* aValue = nullptr );

    std::unique_ptr<TEXT_VALIDATOR> Clone() const override
    {
        return std::make_unique<ENV_VAR_NAME_VALIDATOR>( *this );
    }

    wxString IsValid( const wxString& aValue ) const override;

    QValidator* CreateQtValidator( QObject* aParent ) const override;
};


class KIQT_API NETNAME_VALIDATOR : public TEXT_VALIDATOR
{
public:
    NETNAME_VALIDATOR( wxString* aVal = nullptr );

    NETNAME_VALIDATOR( bool aAllowSpaces );

    std::unique_ptr<TEXT_VALIDATOR> Clone() const override
    {
        return std::make_unique<NETNAME_VALIDATOR>( *this );
    }

    /// @return the error message if the contents of @a aVal are invalid.
    wxString IsValid( const wxString& aVal ) const override;

    wxString ErrorTitle() const override;

private:
    bool m_allowSpaces;
};


#endif  // #ifndef VALIDATORS_H
