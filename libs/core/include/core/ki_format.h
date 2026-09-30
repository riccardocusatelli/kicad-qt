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

/**
 * @file ki_format.h
 * Argument normalization for the KI_ASSERT / KI_LOG_TRACE family.
 *
 * The macros accept the same arguments the wx ones did (wxString, wxT() literals, c_str()
 * and utf8_str() results, enums, numbers, pointers) and format them printf-style into UTF-8.
 * Every argument goes through KI_FMT::Arg(), which maps it to a plain C type:
 *
 *   const char*, std::string, std::string_view        ->  const char* (UTF-8)
 *   const wchar_t*, std::wstring                       ->  const char* (converted to UTF-8)
 *   arithmetic types, non-string pointers              ->  unchanged
 *   enums                                              ->  their underlying integer
 *   wxString, wxCStrData, wx buffers, wxUniChar        ->  (wx builds only, see below)
 *
 * An unsupported type is a compile error, never a silent misformat.
 *
 * The wx overloads are declared only in translation units that use wx (KICAD_NO_WX not
 * defined).  They are distinct overloads, not alternative definitions, so there is no ODR
 * conflict between wx and non-wx code; they go away when wxString is replaced (step 1.3).
 */

#ifndef KI_FORMAT_H
#define KI_FORMAT_H

#include <string>
#include <string_view>
#include <type_traits>

#include <core/kicore_rt.h>

#ifndef KICAD_NO_WX
#include <wx/string.h>
#include <wx/buffer.h>
#endif


/// printf-style formatting of already normalized arguments (no compile-time format check:
/// formats may come from wide literals or translated strings).
KICORE_RT_API std::string KiStrFormatUnchecked( const char* aFormat, ... );

/// Wide to UTF-8 conversion (UTF-16 on Windows, UTF-32 elsewhere).
KICORE_RT_API std::string KiWideToUtf8( const wchar_t* aText, size_t aLen );


namespace KI_FMT
{

/// A normalized argument: get() yields what is passed to printf.
template <typename T>
struct ARG
{
    T v;
    T get() const { return v; }
};

/// An argument that owns converted UTF-8 text.
struct STR_ARG
{
    std::string s;
    const char* get() const { return s.c_str(); }
};


template <typename T>
inline constexpr bool IS_TEXT_PTR =
        std::is_pointer_v<T>
        && ( std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>
             || std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, wchar_t> );


inline ARG<const char*> Arg( const char* aText )
{
    return { aText ? aText : "(null)" };
}

inline STR_ARG Arg( const wchar_t* aText )
{
    if( !aText )
        return { "(null)" };

    return { KiWideToUtf8( aText, std::char_traits<wchar_t>::length( aText ) ) };
}

inline ARG<const char*> Arg( const std::string& aText ) { return { aText.c_str() }; }

inline STR_ARG Arg( std::string_view aText ) { return { std::string( aText ) }; }

inline STR_ARG Arg( const std::wstring& aText )
{
    return { KiWideToUtf8( aText.data(), aText.size() ) };
}

/// Numbers, bool, chars and non-string pointers pass through unchanged.
template <typename T>
    requires( std::is_arithmetic_v<T> || ( std::is_pointer_v<T> && !IS_TEXT_PTR<T> )
              || std::is_null_pointer_v<T> )
inline ARG<T> Arg( T aValue )
{
    return { aValue };
}

/// Non-const char / wchar_t pointers go to the text overloads.
template <typename T>
    requires( IS_TEXT_PTR<T> && !std::is_const_v<std::remove_pointer_t<T>> )
inline auto Arg( T aText )
{
    return Arg( static_cast<const std::remove_pointer_t<T>*>( aText ) );
}

/// Enums print as their underlying integer, as with wx.
template <typename T>
    requires std::is_enum_v<T>
inline ARG<std::underlying_type_t<T>> Arg( T aValue )
{
    return { static_cast<std::underlying_type_t<T>>( aValue ) };
}


#ifndef KICAD_NO_WX
inline STR_ARG Arg( const wxString& aText ) { return { aText.utf8_string() }; }

inline STR_ARG Arg( const wxCStrData& aText ) { return { aText.AsString().utf8_string() }; }

/// Result of utf8_str(), mb_str(), ToUTF8(): already narrow.
inline ARG<const char*> Arg( const wxScopedCharBuffer& aText )
{
    return { aText.data() ? aText.data() : "(null)" };
}

inline STR_ARG Arg( const wxScopedWCharBuffer& aText ) { return Arg( aText.data() ); }

inline ARG<int> Arg( const wxUniChar& aChar ) { return { static_cast<int>( aChar.GetValue() ) }; }

inline ARG<int> Arg( const wxUniCharRef& aChar ) { return Arg( wxUniChar( aChar ) ); }
#endif


template <typename... P>
inline std::string FormatNormalized( const char* aFormat, const P&... aArgs )
{
    return KiStrFormatUnchecked( aFormat, aArgs.get()... );
}


/// A message used verbatim (assert messages): no '%' interpretation, as with wxASSERT_MSG.
template <typename M>
inline std::string Message( const M& aMessage )
{
    return std::string( Arg( aMessage ).get() );
}


/// Format @a aFormat with @a aArgs (any mix of the supported types) into UTF-8.
/// With no arguments only "%%" is unescaped, so a stray '%' in a runtime string is harmless.
template <typename F, typename... A>
inline std::string Format( const F& aFormat, const A&... aArgs )
{
    if constexpr( sizeof...( A ) == 0 )
    {
        std::string out( Arg( aFormat ).get() );

        for( size_t pos = out.find( "%%" ); pos != std::string::npos; pos = out.find( "%%", pos + 1 ) )
            out.erase( pos, 1 );

        return out;
    }
    else
    {
        return FormatNormalized( Arg( aFormat ).get(), Arg( aArgs )... );
    }
}

} // namespace KI_FMT

#endif // KI_FORMAT_H
