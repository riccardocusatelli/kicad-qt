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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/language.h>
#include <wx/translation.h>

#include <clocale>




enum wxLayoutDirection
{
    wxLayout_Default,
    wxLayout_LeftToRight,
    wxLayout_RightToLeft
};

enum wxLocaleCategory
{
    wxLOCALE_CAT_NUMBER,
    wxLOCALE_CAT_DATE,
    wxLOCALE_CAT_MONEY,
    wxLOCALE_CAT_DEFAULT,
    wxLOCALE_CAT_MAX
};

enum wxLocaleInfo
{
    wxLOCALE_THOUSANDS_SEP,
    wxLOCALE_DECIMAL_POINT,
    wxLOCALE_SHORT_DATE_FMT,
    wxLOCALE_LONG_DATE_FMT,
    wxLOCALE_DATE_TIME_FMT,
    wxLOCALE_TIME_FMT
};

enum wxLocaleInitFlags
{
    wxLOCALE_DONT_LOAD_DEFAULT = 0x0000,
    wxLOCALE_LOAD_DEFAULT = 0x0001,
    wxLOCALE_CONV_ENCODING = 0x0002
};


struct wxLanguageInfo
{
    int               Language;
    wxString          LocaleTag;
    wxString          CanonicalName;
    wxString          CanonicalRef;
    wxString          Description;
    wxString          DescriptionNative;
    wxLayoutDirection LayoutDirection;

    wxString GetLocaleName() const;
    wxString GetCanonicalWithRegion() const { return CanonicalName; }
};


/**
 * The locale of the program: sets the C locale and loads the message catalogs of the
 * language (wxTranslations).
 */
class wxLocale
{
public:
    wxLocale() = default;
    wxLocale( int aLanguage, int aFlags = wxLOCALE_LOAD_DEFAULT ) { Init( aLanguage, aFlags ); }
    ~wxLocale();

    /// The character set of the environment locale (nl_langinfo(CODESET)).
    static wxString GetSystemEncodingName();

    wxLocale( const wxLocale& ) = delete;
    wxLocale& operator=( const wxLocale& ) = delete;

    bool Init( int aLanguage = wxLANGUAGE_DEFAULT, int aFlags = wxLOCALE_LOAD_DEFAULT );
    bool Init( const wxString& aName, const wxString& aShortName = wxString(),
               const wxString& aLocale = wxString(), bool aLoadDefault = true );

    bool IsOk() const { return m_ok; }
    int  GetLanguage() const { return m_language; }

    const wxString& GetName() const { return m_name; }
    const wxString& GetLocale() const { return m_name; }
    wxString        GetCanonicalName() const { return m_canonical; }
    wxString        GetSysName() const { return m_sysName; }

    bool AddCatalog( const wxString& aDomain );
    bool IsLoaded( const wxString& aDomain ) const;

    const wxString& GetString( const wxString& aStr, const wxString& aDomain = wxString() ) const
    {
        return wxGetTranslation( aStr, aDomain );
    }

    static void AddCatalogLookupPathPrefix( const wxString& aPrefix );

    static int                   GetSystemLanguage();
    static const wxLanguageInfo* GetLanguageInfo( int aLanguage );
    static const wxLanguageInfo* FindLanguageInfo( const wxString& aLocale );
    static wxString              GetLanguageName( int aLanguage );
    static wxString              GetLanguageCanonicalName( int aLanguage );
    static bool                  IsAvailable( int aLanguage );
    static wxString              GetInfo( wxLocaleInfo aIndex, wxLocaleCategory aCat = wxLOCALE_CAT_DEFAULT );

private:
    wxString          m_name;
    wxString          m_canonical;
    wxString          m_sysName;
    std::string       m_oldLocale;
    int               m_language = wxLANGUAGE_UNKNOWN;
    bool              m_ok = false;
    bool              m_initialised = false;
    wxLocale*         m_oldCurrent = nullptr;
};


wxLocale* wxGetLocale();

inline wxString wxSetlocale( int aCategory, const wxString& aLocale )
{
    const char* rc = std::setlocale( aCategory, aLocale.empty() ? nullptr : aLocale.utf8_string().c_str() );
    return rc ? wxString( rc ) : wxString();
}

#ifndef _WIN32
#pragma GCC visibility pop
#endif
