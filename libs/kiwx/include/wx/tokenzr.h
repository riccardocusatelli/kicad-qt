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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/arrstr.h>

#define wxDEFAULT_DELIMITERS wxT( " \t\r\n" )

enum wxStringTokenizerMode
{
    wxTOKEN_INVALID = -1,
    wxTOKEN_DEFAULT,        ///< strtok() for whitespace delimiters, RET_EMPTY else
    wxTOKEN_RET_EMPTY,      ///< empty tokens in the middle of the string
    wxTOKEN_RET_EMPTY_ALL,  ///< and the trailing empty token too
    wxTOKEN_RET_DELIMS,     ///< tokens end with their delimiter (implies RET_EMPTY)
    wxTOKEN_STRTOK          ///< as strtok(3): no empty tokens
};


class wxStringTokenizer
{
public:
    wxStringTokenizer() = default;
    wxStringTokenizer( const wxString& aStr, const wxString& aDelims = wxDEFAULT_DELIMITERS,
                       wxStringTokenizerMode aMode = wxTOKEN_DEFAULT )
    {
        SetString( aStr, aDelims, aMode );
    }

    void SetString( const wxString& aStr, const wxString& aDelims = wxDEFAULT_DELIMITERS,
                    wxStringTokenizerMode aMode = wxTOKEN_DEFAULT );
    void Reinit( const wxString& aStr );

    size_t   CountTokens() const;
    bool     HasMoreTokens() const;
    wxString GetNextToken();

    wxString GetString() const { return m_string.Mid( m_pos ); }
    size_t   GetPosition() const { return m_pos; }
    wxChar   GetLastDelimiter() const { return m_lastDelim; }

    wxStringTokenizerMode GetMode() const { return m_mode; }
    bool AllowEmpty() const { return m_mode != wxTOKEN_STRTOK; }

private:
    wxString              m_string;
    wxString              m_delims;
    size_t                m_pos = 0;
    wxChar                m_lastDelim = 0;
    wxStringTokenizerMode m_mode = wxTOKEN_INVALID;
};


wxArrayString wxStringTokenize( const wxString& aStr, const wxString& aDelims = wxDEFAULT_DELIMITERS,
                                wxStringTokenizerMode aMode = wxTOKEN_DEFAULT );

#pragma GCC visibility pop
