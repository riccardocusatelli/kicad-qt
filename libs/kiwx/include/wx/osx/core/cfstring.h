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
 * @file cfstring.h
 *
 * wxCFStringRef of kiwx (macOS only): a CFString owned by the object, made from a wxString, and
 * the conversion of a CFString or an NSString to a wxString, as in wxWidgets (UTF-8 throughout).
 */

#pragma once

#include <wx/string.h>

#include <CoreFoundation/CoreFoundation.h>

#include <cstring>
#include <string>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif


class wxCFStringRef
{
public:
    explicit wxCFStringRef( const wxString& aString )
    {
        std::string utf8 = aString.utf8_string();
        m_ref = CFStringCreateWithCString( kCFAllocatorDefault, utf8.c_str(),
                                           kCFStringEncodingUTF8 );
    }

    /// Takes the ownership of @a aRef (as wxWidgets).
    explicit wxCFStringRef( CFStringRef aRef ) : m_ref( aRef ) {}

    ~wxCFStringRef()
    {
        if( m_ref )
            CFRelease( m_ref );
    }

    wxCFStringRef( const wxCFStringRef& ) = delete;
    wxCFStringRef& operator=( const wxCFStringRef& ) = delete;

    operator CFStringRef() const { return m_ref; }
    CFStringRef get() const { return m_ref; }

    wxString AsString() const { return AsString( m_ref ); }

    static wxString AsString( CFStringRef aRef )
    {
        if( !aRef )
            return wxString();

        CFIndex length = CFStringGetLength( aRef );
        CFIndex size = CFStringGetMaximumSizeForEncoding( length, kCFStringEncodingUTF8 ) + 1;
        std::string buffer( size, '\0' );

        if( !CFStringGetCString( aRef, buffer.data(), size, kCFStringEncodingUTF8 ) )
            return wxString();

        buffer.resize( std::strlen( buffer.c_str() ) );
        return wxString::FromUTF8( buffer.c_str() );
    }

#ifdef __OBJC__
    NSString* AsNSString() const { return (__bridge NSString*) m_ref; }

    static wxString AsString( NSString* aString )
    {
        return AsString( (__bridge CFStringRef) aString );
    }
#endif

private:
    CFStringRef m_ref = nullptr;
};
