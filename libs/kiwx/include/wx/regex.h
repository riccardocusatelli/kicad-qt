/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/regex.h, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>
#include <wx/string.h>

/// Compilation flags.
enum
{
    wxRE_EXTENDED = 0,  ///< extended regex syntax
    wxRE_ADVANCED = 1,  ///< same as wxRE_EXTENDED with PCRE (Tcl word boundaries)
    wxRE_BASIC = 2,     ///< basic regex syntax
    wxRE_ICASE = 4,     ///< ignore case
    wxRE_NOSUB = 8,     ///< no sub-matches
    wxRE_NEWLINE = 16,  ///< '.' does not match '\n', '^' and '$' match at line ends
    wxRE_DEFAULT = wxRE_EXTENDED
};

/// Matching flags.
enum
{
    wxRE_NOTBOL = 32,
    wxRE_NOTEOL = 64,
    wxRE_NOTEMPTY = 128
};


class wxRegExImpl;


/**
 * A regular expression, with the semantics of wxWidgets 3.2: PCRE2 with the wxWidgets
 * translation of the POSIX and Tcl syntaxes.
 */
class wxRegEx
{
public:
    wxRegEx() = default;
    wxRegEx( const wxString& aExpr, int aFlags = wxRE_DEFAULT ) { (void) Compile( aExpr, aFlags ); }
    ~wxRegEx();

    wxRegEx( const wxRegEx& ) = delete;
    wxRegEx& operator=( const wxRegEx& ) = delete;

    bool IsValid() const { return m_impl != nullptr; }
    bool Compile( const wxString& aPattern, int aFlags = wxRE_DEFAULT );

    bool Matches( const wxString& aText, int aFlags = 0 ) const;
    bool Matches( const wxChar* aText, int aFlags, size_t aLen ) const
    {
        return Matches( wxString( aText, aLen ), aFlags );
    }

    bool     GetMatch( size_t* aStart, size_t* aLen, size_t aIndex = 0 ) const;
    wxString GetMatch( const wxString& aText, size_t aIndex = 0 ) const;
    size_t   GetMatchCount() const;

    int Replace( wxString* aText, const wxString& aReplacement, size_t aMaxMatches = 0 ) const;
    int ReplaceFirst( wxString* aText, const wxString& aReplacement ) const
    {
        return Replace( aText, aReplacement, 1 );
    }
    int ReplaceAll( wxString* aText, const wxString& aReplacement ) const
    {
        return Replace( aText, aReplacement, 0 );
    }

    static wxString QuoteMeta( const wxString& aStr );
    static wxString ConvertFromBasic( const wxString& aBre );

private:
    wxRegExImpl* m_impl = nullptr;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
