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
 * @file pgm_gui_qt.cpp
 * The parts of PGM_GUI with Qt: the toolkit, the dialogs, the splash screen (see
 * common/pgm_gui_wx.cpp).
 */

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QWidget>

#include <build_version.h>
#include <core/ki_assert.h>
#include <pgm_gui.h>
#include <wx/filename.h>

#include <qt_strings.h>


void PGM_GUI::initGuiToolkit()
{
    // The image formats and the fonts are the ones of Qt (the theme of the application sets
    // the fonts)
}


void PGM_GUI::configureGuiToolkit( bool aHeadless )
{
    // The display server, as the wxGTK build reports it
    QString platform = QGuiApplication::platformName();

    if( platform == QLatin1String( "xcb" ) )
        SetDisplayServerInfo( wxS( "X11" ) );
    else if( platform.startsWith( QLatin1String( "wayland" ) ) )
        SetDisplayServerInfo( wxS( "Wayland" ) );
    else if( platform == QLatin1String( "offscreen" ) || platform == QLatin1String( "minimal" ) )
        SetDisplayServerInfo( wxS( "None" ) );
    else
        SetDisplayServerInfo( ToWxString( platform ) );

#if defined( KICAD_DESKTOP_APP_NAME )
    // The desktop file of the application, for the Wayland compositors (the window icon, the
    // launch feedback): the main manager keeps the bare app name; the other GUI apps are
    // prefixed.  Executables without a desktop file (pl_editor, kicad-cli) are left alone.
    wxString pgm_name = programName();
    wxString desktopId;

    if( pgm_name == wxT( "kicad" ) )
    {
        desktopId = wxT( KICAD_DESKTOP_APP_NAME );
    }
    else if( pgm_name == wxT( "eeschema" ) || pgm_name == wxT( "pcbnew" )
             || pgm_name == wxT( "gerbview" ) || pgm_name == wxT( "bitmap2component" )
             || pgm_name == wxT( "pcb_calculator" ) )
    {
        // pcb_calculator installs as pcbcalculator to satisfy freedesktop naming rules.
        wxString appName = pgm_name == wxT( "pcb_calculator" ) ? wxT( "pcbcalculator" ) : pgm_name;
        desktopId = wxString( wxT( KICAD_DESKTOP_APP_PREFIX ) ) + wxT( "." ) + appName;
    }

    if( !desktopId.IsEmpty() )
        QGuiApplication::setDesktopFileName( ToQString( desktopId ) );
#endif

    // The tooltips of Qt stay while the mouse is on their widget, and the update UI functions
    // run after the input events (KIQT::RegisterUpdateUI)
    (void) aHeadless;
}


bool PGM_GUI::ConfirmAction( const wxString& aTitle, const wxString& aMessage )
{
    return QMessageBox::question( nullptr, ToQString( aTitle ), ToQString( aMessage ),
                                  QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes;
}


void PGM_GUI::SafeYield()
{
    // As wxSafeYield(): the pending events, without the input of the user
    QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
}


QApplication& PGM_GUI::App()
{
    QApplication* app = qobject_cast<QApplication*>( QCoreApplication::instance() );
    KI_ASSERT( app );
    return *app;
}


const wxString PGM_GUI::AskUserForPreferredEditor( const wxString& aDefaultEditor )
{
    // Create a mask representing the executable files in the current platform
#ifdef __WINDOWS__
    QString mask = ToQString( _( "Executable file" ) ) + QStringLiteral( " (*.exe)" );
#else
    QString mask = ToQString( _( "Executable file" ) ) + QStringLiteral( " (*)" );
#endif

    // Start from the path of the default editor (even if the editor's name was empty)
    wxFileName defaultEditor( aDefaultEditor );

    // Show the modal editor and return the file chosen (may be empty if the user cancels
    // the dialog).
    return ToWxString( QFileDialog::getOpenFileName( nullptr,
                                                     ToQString( _( "Select Preferred Editor" ) ),
                                                     ToQString( defaultEditor.GetFullPath() ),
                                                     mask ) );
}


void PGM_GUI::ShowSplash()
{
    // Disabled, as in the wxWidgets build (until the DSOs are loaded at startup rather than
    // lazily when needed)
}


void PGM_GUI::HideSplash()
{
    if( !m_splash )
        return;

    m_splash->close();
    m_splash->deleteLater();
    m_splash = nullptr;
}
