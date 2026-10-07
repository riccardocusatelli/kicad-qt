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


#include "qt_keys.h"

#include <QKeyEvent>

#include <wx/defs.h>            // WXK_ key codes of KiCad
#include <tool/tool_event.h>    // MD_ modifiers


namespace TOOLKIT
{

int KeyCodeFromQt( int aQtKey, bool aKeypad )
{
    switch( aQtKey )
    {
    case Qt::Key_Escape:    return WXK_ESCAPE;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:   return WXK_TAB;
    case Qt::Key_Backspace: return WXK_BACK;
    case Qt::Key_Return:    return WXK_RETURN;
    case Qt::Key_Enter:     return aKeypad ? WXK_NUMPAD_ENTER : WXK_RETURN;
    case Qt::Key_Insert:    return WXK_INSERT;
    case Qt::Key_Delete:    return WXK_DELETE;
    case Qt::Key_Pause:     return WXK_PAUSE;
    case Qt::Key_Print:     return WXK_PRINT;
    case Qt::Key_Clear:     return WXK_CLEAR;
    case Qt::Key_Home:      return WXK_HOME;
    case Qt::Key_End:       return WXK_END;
    case Qt::Key_Left:      return WXK_LEFT;
    case Qt::Key_Up:        return WXK_UP;
    case Qt::Key_Right:     return WXK_RIGHT;
    case Qt::Key_Down:      return WXK_DOWN;
    case Qt::Key_PageUp:    return WXK_PAGEUP;
    case Qt::Key_PageDown:  return WXK_PAGEDOWN;
    case Qt::Key_Shift:     return WXK_SHIFT;
    case Qt::Key_Control:   return WXK_CONTROL;
    case Qt::Key_Alt:
    case Qt::Key_AltGr:     return WXK_ALT;
    case Qt::Key_Meta:      return WXK_WINDOWS_LEFT;
    case Qt::Key_Menu:      return WXK_MENU;
    case Qt::Key_CapsLock:  return WXK_CAPITAL;
    case Qt::Key_NumLock:   return WXK_NUMLOCK;
    case Qt::Key_ScrollLock: return WXK_SCROLL;
    case Qt::Key_Help:      return WXK_HELP;
    case Qt::Key_Space:     return WXK_SPACE;
    default:                break;
    }

    if( aQtKey >= Qt::Key_F1 && aQtKey <= Qt::Key_F24 )
        return WXK_F1 + ( aQtKey - Qt::Key_F1 );

    // The keys of Latin-1 characters have the code of the character (upper case for letters)
    if( aQtKey > 0 && aQtKey <= 0xff )
        return aQtKey;

    return 0;
}


int KeyCodeFromQt( const QKeyEvent& aEvent )
{
    int key = KeyCodeFromQt( aEvent.key(), aEvent.modifiers() & Qt::KeypadModifier );

    // Characters out of Latin-1 (the keys of other layouts): the character typed
    if( key == 0 && !aEvent.text().isEmpty() )
    {
        char32_t c = aEvent.text().toUcs4().value( 0 );

        if( c >= 0x20 )
            key = static_cast<int>( QChar::toUpper( c ) );
    }

    return key;
}


bool IsModifierKey( int aQtKey )
{
    switch( aQtKey )
    {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Hyper_L:
    case Qt::Key_Hyper_R:
    case Qt::Key_Menu:
        return true;

    default:
        return false;
    }
}


int ModifiersFromQt( Qt::KeyboardModifiers aModifiers )
{
    int mods = 0;

    if( aModifiers & Qt::ControlModifier )
        mods |= MD_CTRL;

    if( aModifiers & Qt::AltModifier )
        mods |= MD_ALT;

    if( aModifiers & Qt::ShiftModifier )
        mods |= MD_SHIFT;

    // Meta is the Control key on macOS (where Control is Command), the Windows key elsewhere
    if( aModifiers & Qt::MetaModifier )
    {
#ifdef __APPLE__
        mods |= MD_META;
#else
        mods |= MD_SUPER;
#endif
    }

    return mods;
}

} // namespace TOOLKIT
