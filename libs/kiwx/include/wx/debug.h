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

#include <wx/defs.h>

class wxString;

/// The assert handler: file, line, function, condition, message.
typedef void ( *wxAssertHandler_t )( const wxString& aFile, int aLine, const wxString& aFunc,
                                     const wxString& aCond, const wxString& aMsg );

wxAssertHandler_t wxSetAssertHandler( wxAssertHandler_t aHandler );
void              wxSetDefaultAssertHandler();
bool              wxIsDebuggerRunning();

extern WXDLLIMPEXP_DATA_BASE( wxAssertHandler_t ) wxTheAssertHandler;

/// Whether the debugger trap is used after an assert (never in kiwx: there is no dialog).
extern WXDLLIMPEXP_DATA_BASE( bool ) wxTrapInAssert;

class wxString;

namespace kiwx
{
/// Prints an assert message as the wxWidgets console applications do.
void PrintAssert( const wxString& aFile, int aLine, const wxString& aFunc, const wxString& aCond,
                  const wxString& aMsg );
}

void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const char* aMsg = nullptr );
void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const wxString& aMsg );
void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const wchar_t* aMsg );
void wxOnAssert( const wxString& aFile, int aLine, const wxString& aFunc, const wxString& aCond,
                 const wxString& aMsg );

inline void wxTrap()
{
}

inline void wxDisableAsserts()
{
    wxSetAssertHandler( nullptr );
}

// The macros have the structure of the wxWidgets ones, so the condition texts in the messages
// are the same (expanded once by wxASSERT_MSG_AT(), stringized twice by wxCHECK and wxFAIL).
#define wxASSERT_MSG_AT( cond, msg, file, line, func )                                           \
    do                                                                                         \
    {                                                                                          \
        if( cond )                                                                             \
        {                                                                                      \
        }                                                                                      \
        else if( wxTheAssertHandler )                                                          \
            wxOnAssert( file, line, func, #cond, msg );                                        \
    } while( 0 )

#define wxASSERT_MSG( cond, msg ) wxASSERT_MSG_AT( cond, msg, __FILE__, __LINE__, __func__ )
#define wxASSERT( cond ) wxASSERT_MSG( cond, (const char*) nullptr )

#define wxFAIL_COND_MSG_AT( cond, msg, file, line, func )                                        \
    do                                                                                         \
    {                                                                                          \
        if( wxTheAssertHandler )                                                               \
            wxOnAssert( file, line, func, #cond, msg );                                        \
    } while( 0 )

#define wxFAIL_COND_MSG( cond, msg ) wxFAIL_COND_MSG_AT( cond, msg, __FILE__, __LINE__, __func__ )
#define wxFAIL_MSG( msg ) wxFAIL_COND_MSG( "Assert failure", msg )
#define wxFAIL wxFAIL_MSG( (const char*) nullptr )
#define wxFAIL_MSG_AT( msg, file, line, func ) wxFAIL_COND_MSG_AT( "Assert failure", msg, file, line, func )

#define wxCHECK2_MSG( cond, op, msg )                                                          \
    if( cond )                                                                                 \
    {                                                                                          \
    }                                                                                          \
    else                                                                                       \
    {                                                                                          \
        wxFAIL_COND_MSG( #cond, msg );                                                         \
        op;                                                                                    \
    }

#define wxCHECK_MSG( cond, rc, msg ) wxCHECK2_MSG( cond, return rc, msg )
#define wxCHECK( cond, rc ) wxCHECK_MSG( cond, rc, (const char*) nullptr )
#define wxCHECK_RET( cond, msg ) wxCHECK2_MSG( cond, return, msg )
#define wxCHECK2( cond, op ) wxCHECK2_MSG( cond, op, (const char*) nullptr )

#define wxCOMPILE_TIME_ASSERT( expr, msg ) static_assert( expr, #msg )
#define wxCOMPILE_TIME_ASSERT2( expr, msg, text ) static_assert( expr, #msg )

#define wxDISABLE_DEBUG_SUPPORT()
#define wxDISABLE_ASSERTS_IN_RELEASE_BUILD()

#ifndef _WIN32
#pragma GCC visibility pop
#endif
