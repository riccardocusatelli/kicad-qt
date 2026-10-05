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

#include <wx/intl.h>

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <langinfo.h>
#include <wx/translation.h>

#include <cctype>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <mutex>
#include <unordered_set>


namespace
{

// The languages KiCad offers, with their canonical (POSIX) names.
struct LANGUAGE
{
    int            id;
    const wchar_t* canonical;
    const wchar_t* description;
    wxLayoutDirection layout;
};

const LANGUAGE s_languages[] = {
    { wxLANGUAGE_ARABIC,               L"ar",    L"Arabic",                wxLayout_RightToLeft },
    { wxLANGUAGE_BULGARIAN,            L"bg_BG", L"Bulgarian",             wxLayout_LeftToRight },
    { wxLANGUAGE_CATALAN,              L"ca_ES", L"Catalan",               wxLayout_LeftToRight },
    { wxLANGUAGE_CHINESE_SIMPLIFIED,   L"zh_CN", L"Chinese (Simplified)",  wxLayout_LeftToRight },
    { wxLANGUAGE_CHINESE_TRADITIONAL,  L"zh_TW", L"Chinese (Traditional)", wxLayout_LeftToRight },
    { wxLANGUAGE_CROATIAN,             L"hr_HR", L"Croatian",              wxLayout_LeftToRight },
    { wxLANGUAGE_CZECH,                L"cs_CZ", L"Czech",                 wxLayout_LeftToRight },
    { wxLANGUAGE_DANISH,               L"da_DK", L"Danish",                wxLayout_LeftToRight },
    { wxLANGUAGE_DUTCH,                L"nl_NL", L"Dutch",                 wxLayout_LeftToRight },
    { wxLANGUAGE_ENGLISH,              L"en_GB", L"English",               wxLayout_LeftToRight },
    { wxLANGUAGE_ENGLISH_UK,           L"en_GB", L"English (U.K.)",        wxLayout_LeftToRight },
    { wxLANGUAGE_ENGLISH_US,           L"en_US", L"English (U.S.)",        wxLayout_LeftToRight },
    { wxLANGUAGE_ESTONIAN,             L"et_EE", L"Estonian",              wxLayout_LeftToRight },
    { wxLANGUAGE_FARSI,                L"fa_IR", L"Farsi",                 wxLayout_RightToLeft },
    { wxLANGUAGE_FINNISH,              L"fi_FI", L"Finnish",               wxLayout_LeftToRight },
    { wxLANGUAGE_FRENCH,               L"fr_FR", L"French",                wxLayout_LeftToRight },
    { wxLANGUAGE_GEORGIAN,             L"ka_GE", L"Georgian",              wxLayout_LeftToRight },
    { wxLANGUAGE_GERMAN,               L"de_DE", L"German",                wxLayout_LeftToRight },
    { wxLANGUAGE_GREEK,                L"el_GR", L"Greek",                 wxLayout_LeftToRight },
    { wxLANGUAGE_HEBREW,               L"he_IL", L"Hebrew",                wxLayout_RightToLeft },
    { wxLANGUAGE_HINDI,                L"hi_IN", L"Hindi",                 wxLayout_LeftToRight },
    { wxLANGUAGE_HUNGARIAN,            L"hu_HU", L"Hungarian",             wxLayout_LeftToRight },
    { wxLANGUAGE_INDONESIAN,           L"id_ID", L"Indonesian",            wxLayout_LeftToRight },
    { wxLANGUAGE_ITALIAN,              L"it_IT", L"Italian",               wxLayout_LeftToRight },
    { wxLANGUAGE_JAPANESE,             L"ja_JP", L"Japanese",              wxLayout_LeftToRight },
    { wxLANGUAGE_KOREAN,               L"ko_KR", L"Korean",                wxLayout_LeftToRight },
    { wxLANGUAGE_LATVIAN,              L"lv_LV", L"Latvian",               wxLayout_LeftToRight },
    { wxLANGUAGE_LITHUANIAN,           L"lt_LT", L"Lithuanian",            wxLayout_LeftToRight },
    { wxLANGUAGE_NORWEGIAN_BOKMAL,     L"nb_NO", L"Norwegian (Bokmal)",    wxLayout_LeftToRight },
    { wxLANGUAGE_POLISH,               L"pl_PL", L"Polish",                wxLayout_LeftToRight },
    { wxLANGUAGE_PORTUGUESE,           L"pt_PT", L"Portuguese",            wxLayout_LeftToRight },
    { wxLANGUAGE_PORTUGUESE_BRAZILIAN, L"pt_BR", L"Portuguese (Brazilian)", wxLayout_LeftToRight },
    { wxLANGUAGE_ROMANIAN,             L"ro_RO", L"Romanian",              wxLayout_LeftToRight },
    { wxLANGUAGE_RUSSIAN,              L"ru_RU", L"Russian",               wxLayout_LeftToRight },
    { wxLANGUAGE_SERBIAN,              L"sr_RS", L"Serbian",               wxLayout_LeftToRight },
    { wxLANGUAGE_SLOVAK,               L"sk_SK", L"Slovak",                wxLayout_LeftToRight },
    { wxLANGUAGE_SLOVENIAN,            L"sl_SI", L"Slovenian",             wxLayout_LeftToRight },
    { wxLANGUAGE_SPANISH,              L"es_ES", L"Spanish",               wxLayout_LeftToRight },
    { wxLANGUAGE_SPANISH_MEXICAN,      L"es_MX", L"Spanish (Mexican)",     wxLayout_LeftToRight },
    { wxLANGUAGE_SWEDISH,              L"sv_SE", L"Swedish",               wxLayout_LeftToRight },
    { wxLANGUAGE_TAMIL,                L"ta_IN", L"Tamil",                 wxLayout_LeftToRight },
    { wxLANGUAGE_TELUGU,               L"te_IN", L"Telugu",                wxLayout_LeftToRight },
    { wxLANGUAGE_THAI,                 L"th_TH", L"Thai",                  wxLayout_LeftToRight },
    { wxLANGUAGE_TURKISH,              L"tr_TR", L"Turkish",               wxLayout_LeftToRight },
    { wxLANGUAGE_UKRAINIAN,            L"uk_UA", L"Ukrainian",             wxLayout_LeftToRight },
    { wxLANGUAGE_VIETNAMESE,           L"vi_VN", L"Vietnamese",            wxLayout_LeftToRight },
};


std::vector<wxLanguageInfo>& languageInfos()
{
    static std::vector<wxLanguageInfo> infos =
            []()
            {
                std::vector<wxLanguageInfo> list;

                for( const LANGUAGE& lang : s_languages )
                {
                    wxLanguageInfo info;
                    info.Language = lang.id;
                    info.CanonicalName = lang.canonical;
                    info.LocaleTag = wxString( lang.canonical ).BeforeFirst( '_' );
                    info.CanonicalRef = lang.canonical;
                    info.Description = lang.description;
                    info.DescriptionNative = lang.description;
                    info.LayoutDirection = lang.layout;
                    list.push_back( info );
                }

                return list;
            }();

    return infos;
}


std::mutex& i18nMutex()
{
    static std::mutex mutex;
    return mutex;
}


wxArrayString& lookupPrefixes()
{
    static wxArrayString prefixes;
    return prefixes;
}


wxTranslations*& currentTranslations()
{
    static wxTranslations* trans = nullptr;
    return trans;
}


wxLocale*& currentLocale()
{
    static wxLocale* locale = nullptr;
    return locale;
}


/// The language of the environment (LC_ALL, LC_MESSAGES, LANG), as "it_IT".
wxString environmentLanguage()
{
    for( const char* var : { "LC_ALL", "LC_MESSAGES", "LANG" } )
    {
        const char* value = std::getenv( var );

        if( value && *value )
        {
            wxString lang( value );
            lang = lang.BeforeFirst( '.' ).BeforeFirst( '@' );
            return lang;
        }
    }

    return wxS( "C" );
}


/// A gettext plural forms expression ("n != 1", "n==1 ? 0 : n%10>=2 && ... ? 1 : 2").
class PLURAL_EXPR
{
public:
    PLURAL_EXPR( const std::string& aExpr, unsigned long aN ) : m_expr( aExpr ), m_n( aN ) {}

    unsigned long Evaluate()
    {
        unsigned long v = ternary();
        return m_ok ? v : 0;
    }

private:
    void skip()
    {
        while( m_pos < m_expr.size() && std::isspace( static_cast<unsigned char>( m_expr[m_pos] ) ) )
            ++m_pos;
    }

    bool accept( const char* aToken )
    {
        skip();
        size_t len = std::strlen( aToken );

        if( m_expr.compare( m_pos, len, aToken ) == 0 )
        {
            m_pos += len;
            return true;
        }

        return false;
    }

    unsigned long ternary()
    {
        unsigned long cond = logicalOr();

        if( accept( "?" ) )
        {
            unsigned long a = ternary();

            if( !accept( ":" ) )
                m_ok = false;

            unsigned long b = ternary();
            return cond ? a : b;
        }

        return cond;
    }

    unsigned long logicalOr()
    {
        unsigned long v = logicalAnd();

        while( accept( "||" ) )
        {
            unsigned long r = logicalAnd();
            v = v || r;
        }

        return v;
    }

    unsigned long logicalAnd()
    {
        unsigned long v = equality();

        while( accept( "&&" ) )
        {
            unsigned long r = equality();
            v = v && r;
        }

        return v;
    }

    unsigned long equality()
    {
        unsigned long v = relational();

        for( ;; )
        {
            if( accept( "==" ) )
                v = v == relational();
            else if( accept( "!=" ) )
                v = v != relational();
            else
                return v;
        }
    }

    unsigned long relational()
    {
        unsigned long v = additive();

        for( ;; )
        {
            if( accept( "<=" ) )
                v = v <= additive();
            else if( accept( ">=" ) )
                v = v >= additive();
            else if( accept( "<" ) )
                v = v < additive();
            else if( accept( ">" ) )
                v = v > additive();
            else
                return v;
        }
    }

    unsigned long additive()
    {
        unsigned long v = multiplicative();

        for( ;; )
        {
            if( accept( "+" ) )
                v = v + multiplicative();
            else if( accept( "-" ) )
                v = v - multiplicative();
            else
                return v;
        }
    }

    unsigned long multiplicative()
    {
        unsigned long v = unary();

        for( ;; )
        {
            if( accept( "*" ) )
            {
                v = v * unary();
            }
            else if( accept( "/" ) )
            {
                unsigned long d = unary();
                v = d ? v / d : 0;
            }
            else if( accept( "%" ) )
            {
                unsigned long d = unary();
                v = d ? v % d : 0;
            }
            else
            {
                return v;
            }
        }
    }

    unsigned long unary()
    {
        if( accept( "!" ) )
            return !unary();

        return primary();
    }

    unsigned long primary()
    {
        if( accept( "(" ) )
        {
            unsigned long v = ternary();

            if( !accept( ")" ) )
                m_ok = false;

            return v;
        }

        if( accept( "n" ) )
            return m_n;

        skip();
        unsigned long v = 0;
        bool          digits = false;

        while( m_pos < m_expr.size() && std::isdigit( static_cast<unsigned char>( m_expr[m_pos] ) ) )
        {
            v = v * 10 + ( m_expr[m_pos++] - '0' );
            digits = true;
        }

        if( !digits )
            m_ok = false;

        return v;
    }

    const std::string& m_expr;
    unsigned long      m_n;
    size_t             m_pos = 0;
    bool               m_ok = true;
};


uint32_t readU32( const std::string& aData, size_t aOffset, bool aSwap )
{
    if( aOffset + 4 > aData.size() )
        return 0;

    const unsigned char* p = reinterpret_cast<const unsigned char*>( aData.data() + aOffset );

    if( aSwap )
        return ( p[0] << 24 ) | ( p[1] << 16 ) | ( p[2] << 8 ) | p[3];

    return ( p[3] << 24 ) | ( p[2] << 16 ) | ( p[1] << 8 ) | p[0];
}

} // namespace


// wxMsgCatalog

wxMsgCatalog* wxMsgCatalog::CreateFromFile( const wxString& aFile, const wxString& aDomain )
{
    std::ifstream in( aFile.utf8_string(), std::ios::binary );

    if( !in )
        return nullptr;

    std::string data( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

    if( data.size() < 28 )
        return nullptr;

    uint32_t magic = readU32( data, 0, false );
    bool     swap;

    if( magic == 0x950412de )
        swap = false;
    else if( magic == 0xde120495 )
        swap = true;
    else
        return nullptr;

    uint32_t count = readU32( data, 8, swap );
    uint32_t origTable = readU32( data, 12, swap );
    uint32_t transTable = readU32( data, 16, swap );

    std::unique_ptr<wxMsgCatalog> catalog = std::make_unique<wxMsgCatalog>();
    catalog->m_domain = aDomain;

    for( uint32_t i = 0; i < count; ++i )
    {
        uint32_t origLen = readU32( data, origTable + i * 8, swap );
        uint32_t origOff = readU32( data, origTable + i * 8 + 4, swap );
        uint32_t transLen = readU32( data, transTable + i * 8, swap );
        uint32_t transOff = readU32( data, transTable + i * 8 + 4, swap );

        if( origOff + origLen > data.size() || transOff + transLen > data.size() )
            return nullptr;

        // msgid: "singular\0plural"; msgstr: "form0\0form1..."
        std::string orig( data, origOff, origLen );
        std::string trans( data, transOff, transLen );
        std::string singular = orig.substr( 0, orig.find( '\0' ) );

        std::vector<wxString> forms;
        size_t                start = 0;

        for( ;; )
        {
            size_t end = trans.find( '\0', start );
            forms.push_back( wxString::FromUTF8( trans.substr( start, end - start ) ) );

            if( end == std::string::npos )
                break;

            start = end + 1;
        }

        if( singular.empty() )
        {
            // The header: "Plural-Forms: nplurals=2; plural=(n != 1);"
            size_t pos = trans.find( "plural=" );

            if( pos != std::string::npos )
            {
                size_t end = trans.find( ';', pos );
                catalog->m_pluralRule = trans.substr( pos + 7, end - pos - 7 );
            }

            continue;
        }

        catalog->m_messages[wxString::FromUTF8( singular ).impl()] = std::move( forms );
    }

    return catalog.release();
}


const wxString* wxMsgCatalog::GetString( const wxString& aStr, unsigned aN,
                                         const wxString& aContext ) const
{
    std::wstring key = aContext.empty() ? aStr.impl()
                                        : aContext.impl() + L'\x04' + aStr.impl();
    auto         it = m_messages.find( key );

    if( it == m_messages.end() || it->second.empty() )
        return nullptr;

    size_t form = 0;

    if( aN != UINT_MAX )
        form = m_pluralRule.empty() ? ( aN != 1 ) : PLURAL_EXPR( m_pluralRule, aN ).Evaluate();

    if( form >= it->second.size() )
        form = 0;

    const wxString& text = it->second[form];
    return text.empty() ? nullptr : &text;
}


// wxTranslations

wxTranslations::~wxTranslations() = default;


wxTranslations* wxTranslations::Get()
{
    return currentTranslations();
}


void wxTranslations::Set( wxTranslations* aTranslations )
{
    std::lock_guard<std::mutex> lock( i18nMutex() );
    delete currentTranslations();
    currentTranslations() = aTranslations;
}


void wxTranslations::SetLanguage( const wxString& aLanguage )
{
    m_language = aLanguage;
}


wxString wxTranslations::GetBestTranslation( const wxString& aDomain, const wxString& )
{
    wxString lang = m_language.empty() ? environmentLanguage() : m_language;
    return lang;
}


void wxTranslations::AddCatalogLookupPathPrefix( const wxString& aPrefix )
{
    std::lock_guard<std::mutex> lock( i18nMutex() );

    if( lookupPrefixes().Index( aPrefix ) == wxNOT_FOUND )
        lookupPrefixes().Add( aPrefix );
}


bool wxTranslations::AddCatalog( const wxString& aDomain, int )
{
    wxString lang = m_language.empty() ? environmentLanguage() : m_language;

    if( lang == wxS( "C" ) || lang == wxS( "POSIX" ) || lang.StartsWith( wxS( "en" ) ) )
        return false;   // the messages are in English

    // it_IT, then it
    wxArrayString candidates;
    candidates.Add( lang );

    if( lang.Contains( wxS( "_" ) ) )
        candidates.Add( lang.BeforeFirst( '_' ) );

    wxArrayString prefixes;
    {
        std::lock_guard<std::mutex> lock( i18nMutex() );
        prefixes = lookupPrefixes();
    }

    prefixes.Add( wxS( "/usr/share/locale" ) );
    prefixes.Add( wxS( "/usr/local/share/locale" ) );

    for( const wxString& candidate : candidates )
    {
        for( const wxString& prefix : prefixes )
        {
            for( const wxString& sub : { wxString( wxS( "/LC_MESSAGES/" ) ), wxString( wxS( "/" ) ) } )
            {
                wxString file = prefix + wxS( "/" ) + candidate + sub + aDomain + wxS( ".mo" );

                if( wxMsgCatalog* cat = wxMsgCatalog::CreateFromFile( file, aDomain ) )
                {
                    m_catalogs.emplace_back( cat );
                    return true;
                }
            }
        }
    }

    return false;
}


bool wxTranslations::IsLoaded( const wxString& aDomain ) const
{
    for( const std::unique_ptr<wxMsgCatalog>& cat : m_catalogs )
    {
        if( cat->GetDomain() == aDomain )
            return true;
    }

    return false;
}


const wxString* wxTranslations::GetTranslatedString( const wxString& aOrig, const wxString& aDomain,
                                                     const wxString& aContext ) const
{
    return GetTranslatedString( aOrig, UINT_MAX, aDomain, aContext );
}


const wxString* wxTranslations::GetTranslatedString( const wxString& aOrig, unsigned aN,
                                                     const wxString& aDomain,
                                                     const wxString& aContext ) const
{
    for( const std::unique_ptr<wxMsgCatalog>& cat : m_catalogs )
    {
        if( !aDomain.empty() && cat->GetDomain() != aDomain )
            continue;

        if( const wxString* text = cat->GetString( aOrig, aN, aContext ) )
            return text;
    }

    return nullptr;
}


namespace
{

/// The untranslated strings returned by reference live here.
const wxString& untranslated( const wxString& aStr )
{
    static std::unordered_set<wxString> strings;
    static std::mutex                   mutex;

    std::lock_guard<std::mutex> lock( mutex );
    return *strings.insert( aStr ).first;
}

} // namespace


const wxString& wxGetTranslation( const wxString& aStr, const wxString& aDomain,
                                  const wxString& aContext )
{
    if( wxTranslations* trans = wxTranslations::Get() )
    {
        if( const wxString* text = trans->GetTranslatedString( aStr, aDomain, aContext ) )
            return *text;
    }

    return untranslated( aStr );
}


const wxString& wxGetTranslation( const wxString& aStr, const wxString& aStrPlural, unsigned aN,
                                  const wxString& aDomain, const wxString& aContext )
{
    if( wxTranslations* trans = wxTranslations::Get() )
    {
        if( const wxString* text = trans->GetTranslatedString( aStr, aN, aDomain, aContext ) )
            return *text;
    }

    return untranslated( aN == 1 ? aStr : aStrPlural );
}


// wxLanguageInfo and wxLocale

wxString wxLanguageInfo::GetLocaleName() const
{
    return CanonicalName + wxS( ".UTF-8" );
}


wxLocale::~wxLocale()
{
    if( m_initialised )
    {
        if( currentLocale() == this )
            currentLocale() = m_oldCurrent;

        wxTranslations::Set( nullptr );

        if( !m_oldLocale.empty() )
            std::setlocale( LC_ALL, m_oldLocale.c_str() );
    }
}


bool wxLocale::Init( int aLanguage, int aFlags )
{
    const wxLanguageInfo* info = nullptr;
    wxString              canonical;

    if( aLanguage == wxLANGUAGE_DEFAULT )
    {
        canonical = environmentLanguage();
        info = FindLanguageInfo( canonical );
        m_language = info ? info->Language : wxLANGUAGE_UNKNOWN;
    }
    else
    {
        info = GetLanguageInfo( aLanguage );

        if( !info )
            return false;

        canonical = info->CanonicalName;
        m_language = aLanguage;
    }

    return Init( info ? info->Description : canonical, canonical.BeforeFirst( '_' ), canonical,
                 aFlags & wxLOCALE_LOAD_DEFAULT );
}


bool wxLocale::Init( const wxString& aName, const wxString& aShortName, const wxString& aLocale,
                     bool )
{
    const char* old = std::setlocale( LC_ALL, nullptr );
    m_oldLocale = old ? old : "";
    m_name = aName;
    m_canonical = aLocale.empty() ? aShortName : aLocale;

    // Set the C locale: the language with UTF-8, else as given, else the environment.
    const char* rc = nullptr;

    if( m_canonical == wxS( "C" ) || m_canonical == wxS( "POSIX" ) || m_canonical.empty() )
    {
        rc = std::setlocale( LC_ALL, "" );
    }
    else
    {
        rc = std::setlocale( LC_ALL, ( m_canonical.utf8_string() + ".UTF-8" ).c_str() );

        if( !rc )
            rc = std::setlocale( LC_ALL, m_canonical.utf8_string().c_str() );
    }

    m_sysName = rc ? wxString( rc ) : wxString();
    m_ok = rc != nullptr || m_canonical.empty() || m_canonical == wxS( "C" );

    wxTranslations* trans = new wxTranslations;
    trans->SetLanguage( m_canonical );
    wxTranslations::Set( trans );

    m_oldCurrent = currentLocale();
    currentLocale() = this;
    m_initialised = true;

    return m_ok;
}


bool wxLocale::AddCatalog( const wxString& aDomain )
{
    wxTranslations* trans = wxTranslations::Get();
    return trans && trans->AddCatalog( aDomain );
}


bool wxLocale::IsLoaded( const wxString& aDomain ) const
{
    wxTranslations* trans = wxTranslations::Get();
    return trans && trans->IsLoaded( aDomain );
}


void wxLocale::AddCatalogLookupPathPrefix( const wxString& aPrefix )
{
    wxTranslations::AddCatalogLookupPathPrefix( aPrefix );
}


int wxLocale::GetSystemLanguage()
{
    const wxLanguageInfo* info = FindLanguageInfo( environmentLanguage() );
    return info ? info->Language : wxLANGUAGE_UNKNOWN;
}


const wxLanguageInfo* wxLocale::GetLanguageInfo( int aLanguage )
{
    if( aLanguage == wxLANGUAGE_DEFAULT )
        aLanguage = GetSystemLanguage();

    for( const wxLanguageInfo& info : languageInfos() )
    {
        if( info.Language == aLanguage )
            return &info;
    }

    return nullptr;
}


const wxLanguageInfo* wxLocale::FindLanguageInfo( const wxString& aLocale )
{
    const wxLanguageInfo* partial = nullptr;

    for( const wxLanguageInfo& info : languageInfos() )
    {
        if( info.CanonicalName.IsSameAs( aLocale, false ) || info.Description.IsSameAs( aLocale, false ) )
            return &info;

        if( !partial && info.CanonicalName.BeforeFirst( '_' ).IsSameAs( aLocale.BeforeFirst( '_' ), false ) )
            partial = &info;
    }

    return partial;
}


wxString wxLocale::GetLanguageName( int aLanguage )
{
    const wxLanguageInfo* info = GetLanguageInfo( aLanguage );
    return info ? info->Description : wxString();
}


wxString wxLocale::GetLanguageCanonicalName( int aLanguage )
{
    const wxLanguageInfo* info = GetLanguageInfo( aLanguage );
    return info ? info->CanonicalName : wxString();
}


bool wxLocale::IsAvailable( int aLanguage )
{
    const wxLanguageInfo* info = GetLanguageInfo( aLanguage );

    if( !info )
        return false;

    locale_t loc = newlocale( LC_ALL_MASK, ( info->CanonicalName.utf8_string() + ".UTF-8" ).c_str(),
                              (locale_t) 0 );

    if( loc )
        freelocale( loc );

    return loc != (locale_t) 0;
}


wxString wxLocale::GetInfo( wxLocaleInfo aIndex, wxLocaleCategory aCat )
{
    struct lconv* lc = std::localeconv();

    switch( aIndex )
    {
    case wxLOCALE_THOUSANDS_SEP:
        return wxString( aCat == wxLOCALE_CAT_MONEY ? lc->mon_thousands_sep : lc->thousands_sep );

    case wxLOCALE_DECIMAL_POINT:
        return wxString( aCat == wxLOCALE_CAT_MONEY ? lc->mon_decimal_point : lc->decimal_point );

    case wxLOCALE_SHORT_DATE_FMT:  return wxS( "%m/%d/%y" );
    case wxLOCALE_LONG_DATE_FMT:   return wxS( "%A, %B %d, %Y" );
    case wxLOCALE_DATE_TIME_FMT:   return wxS( "%m/%d/%y %H:%M:%S" );
    case wxLOCALE_TIME_FMT:        return wxS( "%H:%M:%S" );
    }

    return wxString();
}


wxLocale* wxGetLocale()
{
    return currentLocale();
}


wxString wxLocale::GetSystemEncodingName()
{
    wxString encname;

    {
        // in the default locale of the environment, as wxWidgets' TempLocaleSetter
        std::string old = std::setlocale( LC_CTYPE, nullptr );
        std::setlocale( LC_CTYPE, "" );
        encname = wxString::FromAscii( nl_langinfo( CODESET ) );
        std::setlocale( LC_CTYPE, old.c_str() );
    }

    if( encname.empty() )
    {
        // else the part after the dot in the locale variables
        const char* lang = std::getenv( "LC_ALL" );
        const char* dot = lang ? std::strchr( lang, '.' ) : nullptr;

        if( !dot )
        {
            lang = std::getenv( "LC_CTYPE" );

            if( lang )
                dot = std::strchr( lang, '.' );
        }

        if( !dot )
        {
            lang = std::getenv( "LANG" );

            if( lang )
                dot = std::strchr( lang, '.' );
        }

        if( dot )
            encname = wxString::FromAscii( dot + 1 );
    }

    return encname;
}
