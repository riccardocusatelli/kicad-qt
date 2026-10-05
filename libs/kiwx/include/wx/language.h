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

/// The languages KiCad is translated to (a subset of the wxWidgets list, with other values).
enum wxLanguage
{
    wxLANGUAGE_DEFAULT,
    wxLANGUAGE_UNKNOWN,
    wxLANGUAGE_ARABIC,
    wxLANGUAGE_BULGARIAN,
    wxLANGUAGE_CATALAN,
    wxLANGUAGE_CHINESE_SIMPLIFIED,
    wxLANGUAGE_CHINESE_TRADITIONAL,
    wxLANGUAGE_CROATIAN,
    wxLANGUAGE_CZECH,
    wxLANGUAGE_DANISH,
    wxLANGUAGE_DUTCH,
    wxLANGUAGE_ENGLISH,
    wxLANGUAGE_ENGLISH_US,
    wxLANGUAGE_ENGLISH_UK,
    wxLANGUAGE_ESTONIAN,
    wxLANGUAGE_FARSI,
    wxLANGUAGE_FINNISH,
    wxLANGUAGE_FRENCH,
    wxLANGUAGE_GEORGIAN,
    wxLANGUAGE_GERMAN,
    wxLANGUAGE_GREEK,
    wxLANGUAGE_HEBREW,
    wxLANGUAGE_HINDI,
    wxLANGUAGE_HUNGARIAN,
    wxLANGUAGE_INDONESIAN,
    wxLANGUAGE_ITALIAN,
    wxLANGUAGE_JAPANESE,
    wxLANGUAGE_KOREAN,
    wxLANGUAGE_LATVIAN,
    wxLANGUAGE_LITHUANIAN,
    wxLANGUAGE_NORWEGIAN_BOKMAL,
    wxLANGUAGE_POLISH,
    wxLANGUAGE_PORTUGUESE,
    wxLANGUAGE_PORTUGUESE_BRAZILIAN,
    wxLANGUAGE_ROMANIAN,
    wxLANGUAGE_RUSSIAN,
    wxLANGUAGE_SERBIAN,
    wxLANGUAGE_SLOVAK,
    wxLANGUAGE_SLOVENIAN,
    wxLANGUAGE_SPANISH,
    wxLANGUAGE_SPANISH_MEXICAN,
    wxLANGUAGE_SWEDISH,
    wxLANGUAGE_TAMIL,
    wxLANGUAGE_TELUGU,
    wxLANGUAGE_THAI,
    wxLANGUAGE_TURKISH,
    wxLANGUAGE_UKRAINIAN,
    wxLANGUAGE_VIETNAMESE,

    wxLANGUAGE_USER_DEFINED
};

#pragma GCC visibility pop
