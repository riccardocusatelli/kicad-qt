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
 * @file kipython_frame.h
 * The Python console of KiCad (FRAME_PYTHON) in the Qt build.  The console of the wxWidgets build
 * is the shell of wxPython (kicad_pyshell); here it is a Qt window that runs the lines typed in
 * an interactive interpreter of Python (code.InteractiveConsole), with the module of pcbnew
 * imported when the PCB editor is open.
 */

#ifndef SCRIPTING_KIPYTHON_FRAME_H_
#define SCRIPTING_KIPYTHON_FRAME_H_

#include <memory>

#include <QStringList>

#include <kiway_player.h>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class APP_SETTINGS_BASE;
class KIWAY_MAIL_EVENT;


class KIPYTHON_FRAME : public KIWAY_PLAYER
{
public:
    KIPYTHON_FRAME( KIWAY* aKiway, QWidget* aParent );
    ~KIPYTHON_FRAME() override;

    KIPYTHON_FRAME( const KIPYTHON_FRAME& ) = delete;
    KIPYTHON_FRAME& operator=( const KIPYTHON_FRAME& ) = delete;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override {}
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override {}

    TOOLKIT_WINDOW* GetToolCanvas() const override { return nullptr; }

    void ExecuteRemoteCommand( const char* cmdline ) override {}

    void KiwayMailIn( KIWAY_MAIL_EVENT& aEvent ) override {}
    void ProjectChanged() override {}

    void ShowChangedLanguage() override {}

    /// Make the interpreter of the console (with the module of pcbnew, the startup script).
    void SetupPythonEditor();

    /**
     * Run a line in the console, as typed in it.
     *
     * @return true if the interpreter waits for more lines (a block of code).
     */
    bool RunLine( const QString& aLine );

    /// The text of the console (what was typed and what Python wrote).
    QString GetConsoleText() const;

private:
    bool canCloseWindow( CLOSE_REQUEST& aRequest ) override { return true; }
    void doCloseWindow() override {}

    /// The history of the lines in the input (up and down arrows).
    bool eventFilter( QObject* aObject, QEvent* aEvent ) override;

    void appendOutput( const QString& aText );

    QPlainTextEdit* m_output;
    QLabel*         m_prompt;
    QLineEdit*      m_input;

    QStringList m_history;
    int         m_historyPos;

    /// The interactive interpreter of Python (its object, owned with the GIL).
    struct PY_CONSOLE;
    std::unique_ptr<PY_CONSOLE> m_console;
};


#endif /* SCRIPTING_KIPYTHON_FRAME_H_ */
