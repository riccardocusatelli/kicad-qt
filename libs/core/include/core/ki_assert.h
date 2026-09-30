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
 * @file ki_assert.h
 * Toolkit-independent assertion macros for the low level libraries (core, kimath, sexpr…).
 *
 * They mirror the wxWidgets macros one to one, and accept the same message types
 * (see core/ki_format.h):
 *
 *   wxASSERT( c )                ->  KI_ASSERT( c )
 *   wxASSERT_MSG( c, m )         ->  KI_ASSERT_MSG( c, m )
 *   wxFAIL                       ->  KI_FAIL()
 *   wxFAIL_MSG( m )              ->  KI_FAIL_MSG( m )
 *   wxCHECK( c, rv )             ->  KI_CHECK( c, rv )
 *   wxCHECK_MSG( c, rv, m )      ->  KI_CHECK_MSG( c, rv, m )
 *   wxCHECK_RET( c, m )          ->  KI_CHECK_RET( c, m )
 *   wxCHECK2( c, op )            ->  KI_CHECK2( c, op )
 *   wxCHECK2_MSG( c, op, m )     ->  KI_CHECK2_MSG( c, op, m )
 *
 * A failure calls the installed handler.  The default one prints to stderr; applications
 * built on a GUI toolkit install their own (see common/core_wx_bridge.cpp, which forwards to
 * wxOnAssert so wxSetAssertHandler() and the QA assert throwers keep working unchanged).
 */

#ifndef KI_ASSERT_H
#define KI_ASSERT_H

#include <string_view>

#include <core/kicore_rt.h>
#include <core/ki_format.h>


/// Called on a failed assertion.  Strings are UTF-8; @a aMsg may be empty.
using KI_ASSERT_HANDLER = void ( * )( const char* aFile, int aLine, const char* aFunc,
                                      const char* aCond, std::string_view aMsg );

/// Install a handler; nullptr restores the default one.  Returns the previous handler.
KICORE_RT_API KI_ASSERT_HANDLER KiSetAssertHandler( KI_ASSERT_HANDLER aHandler );

/// Report a failed assertion through the current handler.
KICORE_RT_API void KiAssertFailed( const char* aFile, int aLine, const char* aFunc,
                                  const char* aCond, std::string_view aMsg = {} );


#define KI_ASSERT_CONCAT_( a, b ) a##b
#define KI_ASSERT_CONCAT( a, b )  KI_ASSERT_CONCAT_( a, b )

/// Messages may be any type KI_FMT::Arg() accepts (char or wide literal, std::string,
/// wxString in wx code...); they are used verbatim, not as formats.
#define KI_ASSERT_REPORT( cond_str, msg )                                           \
    ::KiAssertFailed( __FILE__, __LINE__, __func__, cond_str, ::KI_FMT::Message( msg ) )

#define KI_ASSERT_MSG( cond, msg )                                                  \
    do                                                                              \
    {                                                                               \
        if( !( cond ) )                                                             \
            KI_ASSERT_REPORT( #cond, msg );                                         \
    } while( 0 )

#define KI_ASSERT( cond ) KI_ASSERT_MSG( cond, "" )

#define KI_FAIL_MSG( msg ) KI_ASSERT_REPORT( "Assert failure", msg )
#define KI_FAIL()          KI_FAIL_MSG( "" )

#define KI_CHECK2_MSG( cond, op, msg )                                              \
    if( cond )                                                                      \
    {                                                                               \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        KI_ASSERT_REPORT( #cond, msg );                                             \
        op;                                                                         \
    }                                                                               \
    struct KI_ASSERT_CONCAT( KiDummyCheckStruct, __LINE__ ) /* to force a semicolon */

#define KI_CHECK2( cond, op )           KI_CHECK2_MSG( cond, op, "" )
#define KI_CHECK_MSG( cond, rval, msg ) KI_CHECK2_MSG( cond, return rval, msg )
#define KI_CHECK( cond, rval )          KI_CHECK_MSG( cond, rval, "" )
#define KI_CHECK_RET( cond, msg )       KI_CHECK2_MSG( cond, return, msg )

#endif // KI_ASSERT_H
