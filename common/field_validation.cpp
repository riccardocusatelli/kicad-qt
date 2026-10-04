/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
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
 * @file validators.cpp
 * @brief Custom text control validator implementations.
 */

#include <field_validation.h>

#include <refdes_utils.h>
#include <wx/arrstr.h>
#include <wx/intl.h>


wxString FieldExcludedChars( FIELD_T aFieldId )
{
    // Fields cannot contain carriage returns, line feeds, or tabs.
    wxString excludes( wxT( "\r\n\t" ) );

    // The reference field cannot contain spaces, the sheet name cannot contain slashes.
    if( aFieldId == FIELD_T::REFERENCE )
        excludes += wxT( " " );
    else if( aFieldId == FIELD_T::SHEET_NAME )
        excludes += wxT( "/" );

    return excludes;
}


bool FieldCannotBeEmpty( FIELD_T aFieldId )
{
    // The reference, sheetname and sheetfilename fields cannot be empty.
    return aFieldId == FIELD_T::REFERENCE || aFieldId == FIELD_T::SHEET_NAME
           || aFieldId == FIELD_T::SHEET_FILENAME;
}


wxString GetFieldValidationErrorMessage( FIELD_T aFieldId, const wxString& aValue )
{
    wxString msg;

    if( FieldCannotBeEmpty( aFieldId ) && aValue.empty() )
    {
        switch( aFieldId )
        {
        case FIELD_T::SHEET_NAME:     msg = _( "A sheet must have a name." );               break;
        case FIELD_T::SHEET_FILENAME: msg = _( "A sheet must have a file specified." );     break;
        default:                      msg = _( "The value of the field cannot be empty." ); break;
        }
    }

    if( msg.empty() )
    {
        wxArrayString badCharsFound;

        for( const wxUniChar& excludeChar : FieldExcludedChars( aFieldId ) )
        {
            if( aValue.Find( excludeChar ) != wxNOT_FOUND )
            {
                if( excludeChar == '\r' )
                    badCharsFound.Add( _( "carriage return" ) );
                else if( excludeChar == '\n' )
                    badCharsFound.Add( _( "line feed" ) );
                else if( excludeChar == '\t' )
                    badCharsFound.Add( _( "tab" ) );
                else if( excludeChar == ' ' )
                    badCharsFound.Add( _( "space" ) );
                else
                    badCharsFound.Add( wxString::Format( wxT( "'%c'" ), excludeChar ) );
            }
        }

        if( !badCharsFound.IsEmpty() )
        {
            wxString badChars;

            for( size_t i = 0; i < badCharsFound.GetCount(); i++ )
            {
                if( !badChars.IsEmpty() )
                {
                    if( badCharsFound.GetCount() == 2 )
                    {
                        badChars += _( " or " );
                    }
                    else
                    {
                        if( i < badCharsFound.GetCount() - 2 )
                            badChars += _( ", or " );
                        else
                            badChars += wxT( ", " );
                    }
                }

                badChars += badCharsFound.Item( i );
            }

            switch( aFieldId )
            {
            case FIELD_T::REFERENCE:
                msg.Printf( _( "The reference designator cannot contain %s character(s)." ), badChars );
                break;

            case FIELD_T::VALUE:
                msg.Printf( _( "The value field cannot contain %s character(s)." ), badChars );
                break;

            case FIELD_T::FOOTPRINT:
                msg.Printf( _( "The footprint field cannot contain %s character(s)." ), badChars );
                break;

            case FIELD_T::DATASHEET:
                msg.Printf( _( "The datasheet field cannot contain %s character(s)." ), badChars );
                break;

            case FIELD_T::SHEET_NAME:
                msg.Printf( _( "The sheet name cannot contain %s character(s)." ), badChars );
                break;

            case FIELD_T::SHEET_FILENAME:
                msg.Printf( _( "The sheet filename cannot contain %s character(s)." ), badChars );
                break;

            default:
                msg.Printf( _( "The field cannot contain %s character(s)." ), badChars );
                break;
            };
        }
    }

    if( msg.empty() )
    {
        if( aFieldId == FIELD_T::REFERENCE && aValue.Contains( wxT( "${" ) ) )
        {
            msg.Printf( _( "The reference designator cannot contain text variable references" ) );
        }
        else if( aFieldId == FIELD_T::REFERENCE && UTIL::GetRefDesPrefix( aValue ).IsEmpty() )
        {
            msg.Printf( _( "References must start with a letter." ) );
        }
    }

    return msg;
}
