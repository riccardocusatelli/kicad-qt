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
 * @file wx/defs.h
 *
 * kiwx: KiCad's implementation of the part of the wxWidgets base API that the data model
 * uses.  The KICAD_WX_FREE build (the core and kicad-cli) compiles against these headers
 * instead of wxWidgets; the GUI build keeps using wxWidgets.  The API follows wxWidgets 3.2
 * (the version KiCad requires) so that the model code compiles and behaves the same in both
 * builds; behaviour is checked against wxWidgets by qa/tests/kiwx.
 */

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <cstddef>
#include <cstdint>
#include <climits>

#define KIWX 1

// The wxWidgets version the API follows (code testing wxCHECK_VERSION takes the same branches
// as in the GUI build).
#define wxMAJOR_VERSION      3
#define wxMINOR_VERSION      2
#define wxRELEASE_NUMBER     4
#define wxSUBRELEASE_NUMBER  0
#define wxVERSION_STRING     L"kiwx (wxWidgets 3.2.4 API)"
#define wxVERSION_NUM_DOT_STRING "3.2.4"

#define wxCHECK_VERSION( major, minor, release )                                               \
    ( wxMAJOR_VERSION > ( major )                                                              \
      || ( wxMAJOR_VERSION == ( major ) && wxMINOR_VERSION > ( minor ) )                       \
      || ( wxMAJOR_VERSION == ( major ) && wxMINOR_VERSION == ( minor )                        \
           && wxRELEASE_NUMBER >= ( release ) ) )

#define wxCHECK_VERSION_FULL( major, minor, release, subrel ) wxCHECK_VERSION( major, minor, release )

#if defined( __unix__ ) || defined( __APPLE__ )
#ifndef __UNIX__
#define __UNIX__ 1
#endif
#endif

#if defined( __linux__ ) && !defined( __LINUX__ )
#define __LINUX__ 1
#endif

#if defined( _WIN32 ) && !defined( __WINDOWS__ )
#define __WINDOWS__
#endif

#define wxUSE_UNICODE        1
#define wxUSE_UNICODE_WCHAR  1
#define wxUSE_UNICODE_UTF8   0
#define wxUSE_STD_STRING     1
#define wxUSE_STL            0
#define wxUSE_BASE64         1
#define wxUSE_STREAMS        1
#define wxUSE_ZIPSTREAM      1
#define wxUSE_XML            1
#define wxUSE_INTL           1
#define wxUSE_GUI            0
#define wxDEBUG_LEVEL        1

// The classes that wxWidgets exports explicitly (most of the kiwx API is exported as a whole,
// see the visibility pragmas of the headers)
#define WXDLLIMPEXP_BASE __attribute__( ( visibility( "default" ) ) )
#define WXDLLIMPEXP_CORE
#define WXDLLIMPEXP_XML
#define WXDLLIMPEXP_NET
#define WXDLLIMPEXP_FWD_BASE
#define WXDLLIMPEXP_FWD_CORE

#define WXUNUSED( identifier )
#define wxOVERRIDE override
#define wxFINAL final
#define wxNOEXCEPT noexcept
#define wxEXPLICIT explicit
#define wxDEPRECATED( x ) [[deprecated]] x
#define wxDEPRECATED_MSG( msg ) [[deprecated( msg )]]
#define wxCONCAT_HELPER( text, line ) text##line
#define wxCONCAT( text, line ) wxCONCAT_HELPER( text, line )
#define wxSTRINGIZE_HELPER( x ) #x
#define wxSTRINGIZE( x ) wxSTRINGIZE_HELPER( x )

#define wxDECLARE_NO_COPY_CLASS( classname )                                                   \
    classname( const classname& ) = delete;                                                    \
    classname& operator=( const classname& ) = delete

#define wxDECLARE_NO_ASSIGN_CLASS( classname ) classname& operator=( const classname& ) = delete

template <typename T>
inline void wxUnusedVar( const T& )
{
}

typedef wchar_t        wxChar;
typedef wchar_t        wxSChar;
typedef wchar_t        wxUChar;
typedef int8_t         wxInt8;
typedef uint8_t        wxUint8;
typedef int16_t        wxInt16;
typedef uint16_t       wxUint16;
typedef int32_t        wxInt32;
typedef uint32_t       wxUint32;
typedef int64_t        wxInt64;
typedef uint64_t       wxUint64;

// as wxWidgets with gcc: long long, a type different from long even on LP64 systems
// long is 32 bit on Windows (LLP64), 64 bit elsewhere (LP64)

#ifdef _MSC_VER
// POSIX type, used by the wx API (as wxWidgets defines it for MSVC)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#define wxLongLong_t   long long
#define wxULongLong_t  unsigned long long
#define wxLongLongFmtSpec "ll"
#define wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
#define wxUSE_LONGLONG 1
#define wxHAS_INT64 1

#define wxINT8_MIN   INT8_MIN
#define wxINT8_MAX   INT8_MAX
#define wxUINT8_MAX  UINT8_MAX
#define wxINT16_MIN  INT16_MIN
#define wxINT16_MAX  INT16_MAX
#define wxUINT16_MAX UINT16_MAX
#define wxINT32_MIN  INT32_MIN
#define wxINT32_MAX  INT32_MAX
#define wxUINT32_MAX UINT32_MAX
#define wxINT64_MIN  INT64_MIN
#define wxINT64_MAX  INT64_MAX
#define wxUINT64_MAX UINT64_MAX
#define wxLL( x ) x##LL
#define wxULL( x ) x##ULL
typedef intptr_t       wxIntPtr;
typedef uintptr_t      wxUIntPtr;
typedef unsigned char  wxByte;
typedef double         wxDouble;
typedef int64_t        wxFileOffset;
typedef int            wxCoord;

#define wxInvalidOffset ( (wxFileOffset) -1 )

enum
{
    wxNOT_FOUND = -1
};

enum wxPathFormat
{
    wxPATH_NATIVE = 0,
    wxPATH_UNIX,
    wxPATH_BEOS = wxPATH_UNIX,
    wxPATH_MAC,
    wxPATH_DOS,
    wxPATH_WIN = wxPATH_DOS,
    wxPATH_OS2 = wxPATH_DOS,
    wxPATH_VMS,

    wxPATH_MAX
};

enum wxSeekMode
{
    wxFromStart,
    wxFromCurrent,
    wxFromEnd
};

enum wxEOL
{
    wxEOL_NATIVE,
    wxEOL_UNIX,
    wxEOL_MAC,
    wxEOL_DOS
};

/// The line end of the text files of this system (wxEOL_NATIVE).
#ifdef _WIN32
constexpr wxEOL kiwxNativeEOL = wxEOL_DOS;
#else
constexpr wxEOL kiwxNativeEOL = wxEOL_UNIX;
#endif

#define wxMin( a, b ) ( ( ( b ) < ( a ) ) ? ( b ) : ( a ) )
#define wxMax( a, b ) ( ( ( a ) < ( b ) ) ? ( b ) : ( a ) )

#define WXSIZEOF( array ) ( sizeof( array ) / sizeof( array[0] ) )

/// The identifier of the main thread (wxThread::IsMain()).
bool wxIsMainThread();


/// Deletes and nulls a pointer.
template <typename T>
inline void wxDELETE( T*& aPtr )
{
    delete aPtr;
    aPtr = nullptr;
}

template <typename T>
inline void wxDELETEA( T*& aPtr )
{
    delete[] aPtr;
    aPtr = nullptr;
}




#include <wx/kiwx_constants.h>

// The paper sizes and print modes (data types of wxWidgets defs.h)
enum wxPaperSize
{
    wxPAPER_NONE,               /*  Use specific dimensions */
    wxPAPER_LETTER,             /*  Letter, 8 1/2 by 11 inches */
    wxPAPER_LEGAL,              /*  Legal, 8 1/2 by 14 inches */
    wxPAPER_A4,                 /*  A4 Sheet, 210 by 297 millimeters */
    wxPAPER_CSHEET,             /*  C Sheet, 17 by 22 inches */
    wxPAPER_DSHEET,             /*  D Sheet, 22 by 34 inches */
    wxPAPER_ESHEET,             /*  E Sheet, 34 by 44 inches */
    wxPAPER_LETTERSMALL,        /*  Letter Small, 8 1/2 by 11 inches */
    wxPAPER_TABLOID,            /*  Tabloid, 11 by 17 inches */
    wxPAPER_LEDGER,             /*  Ledger, 17 by 11 inches */
    wxPAPER_STATEMENT,          /*  Statement, 5 1/2 by 8 1/2 inches */
    wxPAPER_EXECUTIVE,          /*  Executive, 7 1/4 by 10 1/2 inches */
    wxPAPER_A3,                 /*  A3 sheet, 297 by 420 millimeters */
    wxPAPER_A4SMALL,            /*  A4 small sheet, 210 by 297 millimeters */
    wxPAPER_A5,                 /*  A5 sheet, 148 by 210 millimeters */
    wxPAPER_B4,                 /*  B4 sheet, 250 by 354 millimeters */
    wxPAPER_B5,                 /*  B5 sheet, 182-by-257-millimeter paper */
    wxPAPER_FOLIO,              /*  Folio, 8-1/2-by-13-inch paper */
    wxPAPER_QUARTO,             /*  Quarto, 215-by-275-millimeter paper */
    wxPAPER_10X14,              /*  10-by-14-inch sheet */
    wxPAPER_11X17,              /*  11-by-17-inch sheet */
    wxPAPER_NOTE,               /*  Note, 8 1/2 by 11 inches */
    wxPAPER_ENV_9,              /*  #9 Envelope, 3 7/8 by 8 7/8 inches */
    wxPAPER_ENV_10,             /*  #10 Envelope, 4 1/8 by 9 1/2 inches */
    wxPAPER_ENV_11,             /*  #11 Envelope, 4 1/2 by 10 3/8 inches */
    wxPAPER_ENV_12,             /*  #12 Envelope, 4 3/4 by 11 inches */
    wxPAPER_ENV_14,             /*  #14 Envelope, 5 by 11 1/2 inches */
    wxPAPER_ENV_DL,             /*  DL Envelope, 110 by 220 millimeters */
    wxPAPER_ENV_C5,             /*  C5 Envelope, 162 by 229 millimeters */
    wxPAPER_ENV_C3,             /*  C3 Envelope, 324 by 458 millimeters */
    wxPAPER_ENV_C4,             /*  C4 Envelope, 229 by 324 millimeters */
    wxPAPER_ENV_C6,             /*  C6 Envelope, 114 by 162 millimeters */
    wxPAPER_ENV_C65,            /*  C65 Envelope, 114 by 229 millimeters */
    wxPAPER_ENV_B4,             /*  B4 Envelope, 250 by 353 millimeters */
    wxPAPER_ENV_B5,             /*  B5 Envelope, 176 by 250 millimeters */
    wxPAPER_ENV_B6,             /*  B6 Envelope, 176 by 125 millimeters */
    wxPAPER_ENV_ITALY,          /*  Italy Envelope, 110 by 230 millimeters */
    wxPAPER_ENV_MONARCH,        /*  Monarch Envelope, 3 7/8 by 7 1/2 inches */
    wxPAPER_ENV_PERSONAL,       /*  6 3/4 Envelope, 3 5/8 by 6 1/2 inches */
    wxPAPER_FANFOLD_US,         /*  US Std Fanfold, 14 7/8 by 11 inches */
    wxPAPER_FANFOLD_STD_GERMAN, /*  German Std Fanfold, 8 1/2 by 12 inches */
    wxPAPER_FANFOLD_LGL_GERMAN, /*  German Legal Fanfold, 8 1/2 by 13 inches */

    wxPAPER_ISO_B4,             /*  B4 (ISO) 250 x 353 mm */
    wxPAPER_JAPANESE_POSTCARD,  /*  Japanese Postcard 100 x 148 mm */
    wxPAPER_9X11,               /*  9 x 11 in */
    wxPAPER_10X11,              /*  10 x 11 in */
    wxPAPER_15X11,              /*  15 x 11 in */
    wxPAPER_ENV_INVITE,         /*  Envelope Invite 220 x 220 mm */
    wxPAPER_LETTER_EXTRA,       /*  Letter Extra 9 \275 x 12 in */
    wxPAPER_LEGAL_EXTRA,        /*  Legal Extra 9 \275 x 15 in */
    wxPAPER_TABLOID_EXTRA,      /*  Tabloid Extra 11.69 x 18 in */
    wxPAPER_A4_EXTRA,           /*  A4 Extra 9.27 x 12.69 in */
    wxPAPER_LETTER_TRANSVERSE,  /*  Letter Transverse 8 \275 x 11 in */
    wxPAPER_A4_TRANSVERSE,      /*  A4 Transverse 210 x 297 mm */
    wxPAPER_LETTER_EXTRA_TRANSVERSE, /*  Letter Extra Transverse 9\275 x 12 in */
    wxPAPER_A_PLUS,             /*  SuperA/SuperA/A4 227 x 356 mm */
    wxPAPER_B_PLUS,             /*  SuperB/SuperB/A3 305 x 487 mm */
    wxPAPER_LETTER_PLUS,        /*  Letter Plus 8.5 x 12.69 in */
    wxPAPER_A4_PLUS,            /*  A4 Plus 210 x 330 mm */
    wxPAPER_A5_TRANSVERSE,      /*  A5 Transverse 148 x 210 mm */
    wxPAPER_B5_TRANSVERSE,      /*  B5 (JIS) Transverse 182 x 257 mm */
    wxPAPER_A3_EXTRA,           /*  A3 Extra 322 x 445 mm */
    wxPAPER_A5_EXTRA,           /*  A5 Extra 174 x 235 mm */
    wxPAPER_B5_EXTRA,           /*  B5 (ISO) Extra 201 x 276 mm */
    wxPAPER_A2,                 /*  A2 420 x 594 mm */
    wxPAPER_A3_TRANSVERSE,      /*  A3 Transverse 297 x 420 mm */
    wxPAPER_A3_EXTRA_TRANSVERSE, /*  A3 Extra Transverse 322 x 445 mm */

    wxPAPER_DBL_JAPANESE_POSTCARD,/* Japanese Double Postcard 200 x 148 mm */
    wxPAPER_A6,                 /* A6 105 x 148 mm */
    wxPAPER_JENV_KAKU2,         /* Japanese Envelope Kaku #2 */
    wxPAPER_JENV_KAKU3,         /* Japanese Envelope Kaku #3 */
    wxPAPER_JENV_CHOU3,         /* Japanese Envelope Chou #3 */
    wxPAPER_JENV_CHOU4,         /* Japanese Envelope Chou #4 */
    wxPAPER_LETTER_ROTATED,     /* Letter Rotated 11 x 8 1/2 in */
    wxPAPER_A3_ROTATED,         /* A3 Rotated 420 x 297 mm */
    wxPAPER_A4_ROTATED,         /* A4 Rotated 297 x 210 mm */
    wxPAPER_A5_ROTATED,         /* A5 Rotated 210 x 148 mm */
    wxPAPER_B4_JIS_ROTATED,     /* B4 (JIS) Rotated 364 x 257 mm */
    wxPAPER_B5_JIS_ROTATED,     /* B5 (JIS) Rotated 257 x 182 mm */
    wxPAPER_JAPANESE_POSTCARD_ROTATED,/* Japanese Postcard Rotated 148 x 100 mm */
    wxPAPER_DBL_JAPANESE_POSTCARD_ROTATED,/* Double Japanese Postcard Rotated 148 x 200 mm */
    wxPAPER_A6_ROTATED,         /* A6 Rotated 148 x 105 mm */
    wxPAPER_JENV_KAKU2_ROTATED, /* Japanese Envelope Kaku #2 Rotated */
    wxPAPER_JENV_KAKU3_ROTATED, /* Japanese Envelope Kaku #3 Rotated */
    wxPAPER_JENV_CHOU3_ROTATED, /* Japanese Envelope Chou #3 Rotated */
    wxPAPER_JENV_CHOU4_ROTATED, /* Japanese Envelope Chou #4 Rotated */
    wxPAPER_B6_JIS,             /* B6 (JIS) 128 x 182 mm */
    wxPAPER_B6_JIS_ROTATED,     /* B6 (JIS) Rotated 182 x 128 mm */
    wxPAPER_12X11,              /* 12 x 11 in */
    wxPAPER_JENV_YOU4,          /* Japanese Envelope You #4 */
    wxPAPER_JENV_YOU4_ROTATED,  /* Japanese Envelope You #4 Rotated */
    wxPAPER_P16K,               /* PRC 16K 146 x 215 mm */
    wxPAPER_P32K,               /* PRC 32K 97 x 151 mm */
    wxPAPER_P32KBIG,            /* PRC 32K(Big) 97 x 151 mm */
    wxPAPER_PENV_1,             /* PRC Envelope #1 102 x 165 mm */
    wxPAPER_PENV_2,             /* PRC Envelope #2 102 x 176 mm */
    wxPAPER_PENV_3,             /* PRC Envelope #3 125 x 176 mm */
    wxPAPER_PENV_4,             /* PRC Envelope #4 110 x 208 mm */
    wxPAPER_PENV_5,             /* PRC Envelope #5 110 x 220 mm */
    wxPAPER_PENV_6,             /* PRC Envelope #6 120 x 230 mm */
    wxPAPER_PENV_7,             /* PRC Envelope #7 160 x 230 mm */
    wxPAPER_PENV_8,             /* PRC Envelope #8 120 x 309 mm */
    wxPAPER_PENV_9,             /* PRC Envelope #9 229 x 324 mm */
    wxPAPER_PENV_10,            /* PRC Envelope #10 324 x 458 mm */
    wxPAPER_P16K_ROTATED,       /* PRC 16K Rotated */
    wxPAPER_P32K_ROTATED,       /* PRC 32K Rotated */
    wxPAPER_P32KBIG_ROTATED,    /* PRC 32K(Big) Rotated */
    wxPAPER_PENV_1_ROTATED,     /* PRC Envelope #1 Rotated 165 x 102 mm */
    wxPAPER_PENV_2_ROTATED,     /* PRC Envelope #2 Rotated 176 x 102 mm */
    wxPAPER_PENV_3_ROTATED,     /* PRC Envelope #3 Rotated 176 x 125 mm */
    wxPAPER_PENV_4_ROTATED,     /* PRC Envelope #4 Rotated 208 x 110 mm */
    wxPAPER_PENV_5_ROTATED,     /* PRC Envelope #5 Rotated 220 x 110 mm */
    wxPAPER_PENV_6_ROTATED,     /* PRC Envelope #6 Rotated 230 x 120 mm */
    wxPAPER_PENV_7_ROTATED,     /* PRC Envelope #7 Rotated 230 x 160 mm */
    wxPAPER_PENV_8_ROTATED,     /* PRC Envelope #8 Rotated 309 x 120 mm */
    wxPAPER_PENV_9_ROTATED,     /* PRC Envelope #9 Rotated 324 x 229 mm */
    wxPAPER_PENV_10_ROTATED,    /* PRC Envelope #10 Rotated 458 x 324 m */
    wxPAPER_A0,                 /* A0 Sheet 841 x 1189 mm */
    wxPAPER_A1                  /* A1 Sheet 594 x 841 mm */
};

/* Printing orientation */
enum wxPrintOrientation
{
   wxPORTRAIT = 1,
   wxLANDSCAPE
};

/* Duplex printing modes
 */

enum wxDuplexMode
{
    wxDUPLEX_SIMPLEX, /*  Non-duplex */
    wxDUPLEX_HORIZONTAL,
    wxDUPLEX_VERTICAL
};

#define wxGCC_WARNING_SUPPRESS( x )
#define wxGCC_WARNING_RESTORE( x )
#define WXDLLIMPEXP_DATA_BASE( type ) type
#define wxNOEXCEPT noexcept

// as in wxWidgets, the asserts come with the basic definitions
#include <wx/debug.h>

#pragma GCC visibility pop
