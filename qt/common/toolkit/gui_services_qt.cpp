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


#include <toolkit/gui_services.h>

#include <set>

#include <QApplication>
#include <QKeyEvent>

#include <wx/defs.h>        // WXK_ key codes

#include "qt_keys.h"


namespace
{

/// The keys pressed, from the key events of the application (Qt can only tell the state of
/// the modifiers).
class KEY_STATE : public QObject
{
public:
    static KEY_STATE& Get()
    {
        static KEY_STATE* state = nullptr;

        if( !state )
        {
            state = new KEY_STATE();
            qApp->installEventFilter( state );
        }

        return *state;
    }

    bool IsDown( int aKeyCode ) const
    {
        if( aKeyCode == WXK_SHIFT || aKeyCode == WXK_CONTROL || aKeyCode == WXK_ALT )
        {
            Qt::KeyboardModifiers mods = QGuiApplication::queryKeyboardModifiers();

            if( aKeyCode == WXK_SHIFT )
                return mods & Qt::ShiftModifier;
            else if( aKeyCode == WXK_CONTROL )
                return mods & Qt::ControlModifier;
            else
                return mods & Qt::AltModifier;
        }

        return m_down.count( aKeyCode ) > 0;
    }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        if( aEvent->type() == QEvent::KeyPress || aEvent->type() == QEvent::KeyRelease )
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>( aEvent );
            int        key = TOOLKIT::KeyCodeFromQt( ke->key() );

            if( key >= 'a' && key <= 'z' )
                key -= 'a' - 'A';

            if( key )
            {
                if( aEvent->type() == QEvent::KeyPress )
                    m_down.insert( key );
                else if( !ke->isAutoRepeat() )
                    m_down.erase( key );
            }
        }
        else if( aEvent->type() == QEvent::ApplicationDeactivate )
        {
            m_down.clear();
        }

        return false;
    }

private:
    std::set<int> m_down;
};


int g_yieldDepth = 0;

} // namespace


namespace TOOLKIT
{

void TrackKeyState()
{
    KEY_STATE::Get();
}


void Bell()
{
    QApplication::beep();
}


bool IsKeyDown( int aKeyCode )
{
    if( aKeyCode >= 'a' && aKeyCode <= 'z' )
        aKeyCode -= 'a' - 'A';

    return KEY_STATE::Get().IsDown( aKeyCode );
}


void YieldEvents()
{
    g_yieldDepth++;
    QCoreApplication::processEvents( QEventLoop::AllEvents );
    g_yieldDepth--;
}


void ProcessPendingEvents()
{
    QCoreApplication::sendPostedEvents();
}


bool IsYielding()
{
    return g_yieldDepth > 0;
}

} // namespace TOOLKIT
