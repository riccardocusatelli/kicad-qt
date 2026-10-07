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

#include "kiqt.h"     // Qt, without its METHOD macro

class QKeyEvent;


namespace TOOLKIT
{

/**
 * The key code of KiCad (a character, or a WXK_ code) of a key of Qt.
 *
 * Letters are upper case and the other characters are the ones typed (Shift+/ gives '?' on a
 * US keyboard), as in the key events of wxWidgets that the hotkeys of KiCad were made for.
 *
 * @return 0 for a key that has no code in KiCad.
 */
KIQT_API int KeyCodeFromQt( int aQtKey, bool aKeypad = false );

/// The key code of KiCad of a key event.
KIQT_API int KeyCodeFromQt( const QKeyEvent& aEvent );

/// @return true for Shift, Control, Alt, Meta...: the keys that are only used with another one.
KIQT_API bool IsModifierKey( int aQtKey );

/// The modifiers of KiCad (MD_SHIFT, MD_CTRL...) of modifiers of Qt.
KIQT_API int ModifiersFromQt( Qt::KeyboardModifiers aModifiers );

/**
 * Follow the keys pressed in the application, for IsKeyDown() (gui_services.h): Qt can only
 * tell the state of the modifiers.  Called before the first key is pressed.
 */
KIQT_API void TrackKeyState();

} // namespace TOOLKIT
