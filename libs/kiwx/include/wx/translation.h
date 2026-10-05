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

#include <wx/crt.h>   // as wxWidgets, through wx/hashmap.h
#include <wx/arrstr.h>
#include <wx/language.h>
#include <wx/string.h>

#include <climits>
#include <map>
#include <memory>
#include <vector>


/// The messages of one gettext catalog (.mo file).
class wxMsgCatalog
{
public:
    /// Load @a aFile; @return nullptr if it is not a valid catalog.
    static wxMsgCatalog* CreateFromFile( const wxString& aFile, const wxString& aDomain );

    const wxString& GetDomain() const { return m_domain; }

    const wxString* GetString( const wxString& aStr, unsigned aN = UINT_MAX,
                               const wxString& aContext = wxString() ) const;

private:
    wxString m_domain;

    /// msgid (with the context prefix "ctxt\x04") -> translations (one per plural form)
    std::map<std::wstring, std::vector<wxString>> m_messages;

    /// The plural forms rule: the index of the form for n.
    std::string m_pluralRule;
};


/**
 * The translations of the program: the catalogs of the chosen language, searched in the
 * lookup path prefixes as <prefix>/<lang>/LC_MESSAGES/<domain>.mo (and <prefix>/<lang>/).
 */
class wxTranslations
{
public:
    wxTranslations() = default;
    ~wxTranslations();

    static wxTranslations* Get();
    static void            Set( wxTranslations* aTranslations );

    void     SetLanguage( const wxString& aLanguage );
    wxString GetBestTranslation( const wxString& aDomain, const wxString& aMsgIdLanguage = wxS( "en" ) );
    bool     AddCatalog( const wxString& aDomain, int aMsgIdLanguage = 0 );
    bool     AddStdCatalog() { return true; }
    bool     IsLoaded( const wxString& aDomain ) const;

    const wxString* GetTranslatedString( const wxString& aOrig, const wxString& aDomain = wxString(),
                                         const wxString& aContext = wxString() ) const;
    const wxString* GetTranslatedString( const wxString& aOrig, unsigned aN,
                                         const wxString& aDomain = wxString(),
                                         const wxString& aContext = wxString() ) const;

    static void AddCatalogLookupPathPrefix( const wxString& aPrefix );

private:
    wxString                                   m_language;
    std::vector<std::unique_ptr<wxMsgCatalog>> m_catalogs;
};


const wxString& wxGetTranslation( const wxString& aStr, const wxString& aDomain = wxString(),
                                  const wxString& aContext = wxString() );
const wxString& wxGetTranslation( const wxString& aStr, const wxString& aStrPlural, unsigned aN,
                                  const wxString& aDomain = wxString(),
                                  const wxString& aContext = wxString() );

#ifndef _
#define _( s ) wxGetTranslation( ( s ) )
#endif

#define wxPLURAL( sing, plur, n ) wxGetTranslation( ( sing ), ( plur ), n )
#define wxGETTEXT_IN_CONTEXT( c, s ) wxGetTranslation( ( s ), wxString(), ( c ) )
#define wxGETTEXT_IN_CONTEXT_PLURAL( c, sing, plur, n )                                        \
    wxGetTranslation( ( sing ), ( plur ), n, wxString(), ( c ) )

#pragma GCC visibility pop
