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
 * @file validators_qt.cpp
 * The validators of the text controls with Qt (common/validators.cpp with wxWidgets).
 */

#include <validators.h>

#include <QMessageBox>
#include <QValidator>
#include <QWidget>

#include <wx/intl.h>

#include <qt_strings.h>


namespace
{

/**
 * The QValidator of a TEXT_VALIDATOR: the excluded characters can't be typed, the lower case
 * letters are converted to upper case if asked.
 */
class CHAR_FILTER_VALIDATOR : public QValidator
{
public:
    CHAR_FILTER_VALIDATOR( const QString& aExcludes, bool aUpperCase, QObject* aParent ) :
            QValidator( aParent ),
            m_excludes( aExcludes ),
            m_upperCase( aUpperCase )
    {
    }

    State validate( QString& aInput, int& aPos ) const override
    {
        for( QChar c : aInput )
        {
            if( m_excludes.contains( c ) )
                return Invalid;
        }

        return Acceptable;
    }

    void fixup( QString& aInput ) const override
    {
        if( m_upperCase )
            aInput = aInput.toUpper();
    }

private:
    QString m_excludes;
    bool    m_upperCase;
};


/// The QValidator of ENV_VAR_NAME_VALIDATOR: upper case letters, digits (not first) and '_'.
class ENV_VAR_QVALIDATOR : public QValidator
{
public:
    explicit ENV_VAR_QVALIDATOR( QObject* aParent ) :
            QValidator( aParent )
    {
    }

    State validate( QString& aInput, int& aPos ) const override
    {
        aInput = aInput.toUpper();

        for( qsizetype i = 0; i < aInput.size(); ++i )
        {
            QChar c = aInput[i];

            if( c == u'_' || ( c.isLetter() && c.unicode() < 128 ) )
                continue;

            if( c.isDigit() && i > 0 )
                continue;

            return Invalid;
        }

        return Acceptable;
    }
};

} // namespace


wxString TEXT_VALIDATOR::IsValid( const wxString& aValue ) const
{
    for( wxUniChar c : aValue )
    {
        if( m_charExcludes.Find( c ) != wxNOT_FOUND )
            return wxString::Format( _( "The character '%s' is not allowed." ), wxString( c ) );
    }

    return wxString();
}


wxString TEXT_VALIDATOR::ErrorTitle() const
{
    return _( "Validation Failed" );
}


bool TEXT_VALIDATOR::Validate( QWidget* aParent, const wxString& aValue ) const
{
    wxString errorMsg = IsValid( aValue );

    if( errorMsg.empty() )
        return true;

    QMessageBox::warning( aParent, ToQString( ErrorTitle() ), ToQString( errorMsg ) );
    return false;
}


QValidator* TEXT_VALIDATOR::CreateQtValidator( QObject* aParent ) const
{
    return new CHAR_FILTER_VALIDATOR( ToQString( m_charExcludes ), false, aParent );
}


FOOTPRINT_NAME_VALIDATOR::FOOTPRINT_NAME_VALIDATOR( wxString* aValue ) :
        TEXT_VALIDATOR( aValue )
{
    // This list of characters follows the string from footprint.cpp which, in turn mimics the
    // strings from lib_id.cpp
    SetCharExcludes( wxS( "%$<>\t\n\r\"\\/:" ) );
}


ENV_VAR_NAME_VALIDATOR::ENV_VAR_NAME_VALIDATOR( wxString* aValue ) :
        TEXT_VALIDATOR( aValue )
{
}


wxString ENV_VAR_NAME_VALIDATOR::IsValid( const wxString& aValue ) const
{
    for( size_t i = 0; i < aValue.length(); ++i )
    {
        wxUniChar c = aValue[i];

        bool valid = c == '_' || ( c >= 'A' && c <= 'Z' ) || ( i > 0 && c >= '0' && c <= '9' );

        if( !valid )
            return _( "Environment variable names can contain only upper case letters, digits "
                      "(not as the first character) and '_'." );
    }

    return wxString();
}


QValidator* ENV_VAR_NAME_VALIDATOR::CreateQtValidator( QObject* aParent ) const
{
    return new ENV_VAR_QVALIDATOR( aParent );
}


NETNAME_VALIDATOR::NETNAME_VALIDATOR( wxString* aVal ) :
        TEXT_VALIDATOR( aVal ),
        m_allowSpaces( true )
{
}


NETNAME_VALIDATOR::NETNAME_VALIDATOR( bool aAllowSpaces ) :
        TEXT_VALIDATOR(),
        m_allowSpaces( aAllowSpaces )
{
}


wxString NETNAME_VALIDATOR::IsValid( const wxString& str ) const
{
    if( str.Contains( '\r' ) || str.Contains( '\n' ) )
        return _( "Signal names cannot contain CR or LF characters" );

    if( !m_allowSpaces && ( str.Contains( ' ' ) || str.Contains( '\t' ) ) )
        return _( "Signal names cannot contain spaces" );

    return wxString();
}


wxString NETNAME_VALIDATOR::ErrorTitle() const
{
    return _( "Invalid signal name" );
}
