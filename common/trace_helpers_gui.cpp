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
 * @file trace_helpers_gui.cpp
 * Debug helpers for wxKeyEvent (the trace masks themselves are in trace_helpers.cpp).
 */

#include <wx/event.h>

#include <trace_helpers.h>


const char* GetVirtualKeyCodeName(int keycode)
{
    switch ( keycode )
    {
#define WXK_(x) \
        case WXK_##x: return #x;

        WXK_(BACK)
        WXK_(TAB)
        WXK_(RETURN)
        WXK_(ESCAPE)
        WXK_(SPACE)
        WXK_(DELETE)
        WXK_(START)
        WXK_(LBUTTON)
        WXK_(RBUTTON)
        WXK_(CANCEL)
        WXK_(MBUTTON)
        WXK_(CLEAR)
        WXK_(SHIFT)
        WXK_(ALT)
        WXK_(CONTROL)
        WXK_(MENU)
        WXK_(PAUSE)
        WXK_(CAPITAL)
        WXK_(END)
        WXK_(HOME)
        WXK_(LEFT)
        WXK_(UP)
        WXK_(RIGHT)
        WXK_(DOWN)
        WXK_(SELECT)
        WXK_(PRINT)
        WXK_(EXECUTE)
        WXK_(SNAPSHOT)
        WXK_(INSERT)
        WXK_(HELP)
        WXK_(NUMPAD0)
        WXK_(NUMPAD1)
        WXK_(NUMPAD2)
        WXK_(NUMPAD3)
        WXK_(NUMPAD4)
        WXK_(NUMPAD5)
        WXK_(NUMPAD6)
        WXK_(NUMPAD7)
        WXK_(NUMPAD8)
        WXK_(NUMPAD9)
        WXK_(MULTIPLY)
        WXK_(ADD)
        WXK_(SEPARATOR)
        WXK_(SUBTRACT)
        WXK_(DECIMAL)
        WXK_(DIVIDE)
        WXK_(F1)
        WXK_(F2)
        WXK_(F3)
        WXK_(F4)
        WXK_(F5)
        WXK_(F6)
        WXK_(F7)
        WXK_(F8)
        WXK_(F9)
        WXK_(F10)
        WXK_(F11)
        WXK_(F12)
        WXK_(F13)
        WXK_(F14)
        WXK_(F15)
        WXK_(F16)
        WXK_(F17)
        WXK_(F18)
        WXK_(F19)
        WXK_(F20)
        WXK_(F21)
        WXK_(F22)
        WXK_(F23)
        WXK_(F24)
        WXK_(NUMLOCK)
        WXK_(SCROLL)
        WXK_(PAGEUP)
        WXK_(PAGEDOWN)
        WXK_(NUMPAD_SPACE)
        WXK_(NUMPAD_TAB)
        WXK_(NUMPAD_ENTER)
        WXK_(NUMPAD_F1)
        WXK_(NUMPAD_F2)
        WXK_(NUMPAD_F3)
        WXK_(NUMPAD_F4)
        WXK_(NUMPAD_HOME)
        WXK_(NUMPAD_LEFT)
        WXK_(NUMPAD_UP)
        WXK_(NUMPAD_RIGHT)
        WXK_(NUMPAD_DOWN)
        WXK_(NUMPAD_PAGEUP)
        WXK_(NUMPAD_PAGEDOWN)
        WXK_(NUMPAD_END)
        WXK_(NUMPAD_BEGIN)
        WXK_(NUMPAD_INSERT)
        WXK_(NUMPAD_DELETE)
        WXK_(NUMPAD_EQUAL)
        WXK_(NUMPAD_MULTIPLY)
        WXK_(NUMPAD_ADD)
        WXK_(NUMPAD_SEPARATOR)
        WXK_(NUMPAD_SUBTRACT)
        WXK_(NUMPAD_DECIMAL)
        WXK_(NUMPAD_DIVIDE)

        WXK_(WINDOWS_LEFT)
        WXK_(WINDOWS_RIGHT)
#ifdef __WXOSX__
        WXK_(RAW_CONTROL)
#endif
#undef WXK_

    default:
        return nullptr;
    }
}


wxString GetKeyName( const wxKeyEvent &aEvent )
{
    int keycode = aEvent.GetKeyCode();
    const char* virt = GetVirtualKeyCodeName( keycode );

    if( virt )
        return virt;

    if( keycode > 0 && keycode < 32 )
        return wxString::Format( "Ctrl-%c", (unsigned char)('A' + keycode - 1) );

    if( keycode >= 32 && keycode < 128 )
        return wxString::Format( "'%c'", (unsigned char)keycode );

#if wxUSE_UNICODE
    int uc = aEvent.GetUnicodeKey();

    if( uc != WXK_NONE )
        return wxString::Format( "'%c'", uc );
#endif

    return "unknown";
}


wxString dump( const wxKeyEvent& aEvent )
{
    wxString msg;
    wxString eventType = wxS( "unknown" );

    if( aEvent.GetEventType() == wxEVT_KEY_DOWN )
        eventType = wxS( "KeyDown" );
    else if( aEvent.GetEventType() == wxEVT_KEY_UP )
        eventType = wxS( "KeyUp" );
    else if( aEvent.GetEventType() == wxEVT_CHAR )
        eventType = wxS( "Char" );
    else if( aEvent.GetEventType() == wxEVT_CHAR_HOOK )
        eventType = wxS( "Hook" );

    // event  key_name  KeyCode  modifiers  Unicode  raw_code raw_flags pos
    msg.Printf( "%7s %15s %5d   %c%c%c%c"
#if wxUSE_UNICODE
                "%5d (U+%04x)"
#else
                "    none   "
#endif
#ifdef wxHAS_RAW_KEY_CODES
                "  %7lu    0x%08lx"
#else
                "  not-set    not-set"
#endif
                "  (%5d,%5d)",
                eventType,
                GetKeyName( aEvent ),
                aEvent.GetKeyCode(),
                aEvent.ControlDown() ? 'C' : '-',
                aEvent.AltDown()     ? 'A' : '-',
                aEvent.ShiftDown()   ? 'S' : '-',
                aEvent.MetaDown()    ? 'M' : '-'
#if wxUSE_UNICODE
                , aEvent.GetUnicodeKey()
                , aEvent.GetUnicodeKey()
#endif
#ifdef wxHAS_RAW_KEY_CODES
                , (unsigned long) aEvent.GetRawKeyCode()
                , (unsigned long) aEvent.GetRawKeyFlags()
#endif
                , aEvent.GetX()
                , aEvent.GetY()
        );

    return msg;
}
