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

#include <pybind11/embed.h>  // Python first (the slots of its structures)

#include <algorithm>

#include <kipython_frame.h>

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>

#include <frame_type.h>
#include <kiway.h>
#include <python_scripting.h>
#include <qt_strings.h>
#include <settings/settings_manager.h>
#include <wx/filename.h>
#include <wx/translation.h>


// The interactive interpreter of the console: the lines are pushed to code.InteractiveConsole,
// what Python writes meanwhile (the results, the tracebacks) is given back to the window.
static const char* const CONSOLE_CLASS = R"(
import code
import io
import sys


class KiCadQtConsole( code.InteractiveConsole ):
    def __init__( self, namespace ):
        code.InteractiveConsole.__init__( self, namespace, "<console>" )
        self.written = io.StringIO()

    def write( self, data ):
        self.written.write( data )

    def run( self, line ):
        saved = ( sys.stdout, sys.stderr )
        sys.stdout = sys.stderr = self.written

        try:
            more = self.push( line )
        finally:
            sys.stdout, sys.stderr = saved

        text = self.written.getvalue()
        self.written.seek( 0 )
        self.written.truncate()
        return more, text

    def run_file( self, path ):
        saved = ( sys.stdout, sys.stderr )
        sys.stdout = sys.stderr = self.written

        try:
            with open( path ) as script:
                exec( compile( script.read(), path, "exec" ), self.locals )
        except Exception:
            self.showtraceback()
        finally:
            sys.stdout, sys.stderr = saved

        text = self.written.getvalue()
        self.written.seek( 0 )
        self.written.truncate()
        return text
)";


struct KIPYTHON_FRAME::PY_CONSOLE
{
    pybind11::object console;
};


KIPYTHON_FRAME::KIPYTHON_FRAME( KIWAY* aKiway, QWidget* aParent ) :
            KIWAY_PLAYER( aKiway, aParent, FRAME_PYTHON, wxT( "KiPython" ), wxDefaultPosition,
                          wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, wxT( "KiPython" ),
                          unityScale ),
            m_output( nullptr ),
            m_prompt( nullptr ),
            m_input( nullptr ),
            m_historyPos( 0 )
{
    setWindowTitle( ToQString( _( "Scripting Console" ) ) );

    QWidget*     central = new QWidget( this );
    QVBoxLayout* layout = new QVBoxLayout( central );
    QFont        font = QFontDatabase::systemFont( QFontDatabase::FixedFont );

    m_output = new QPlainTextEdit( central );
    m_output->setReadOnly( true );
    m_output->setFont( font );
    m_output->setLineWrapMode( QPlainTextEdit::WidgetWidth );
    layout->addWidget( m_output, 1 );

    QHBoxLayout* inputRow = new QHBoxLayout();
    m_prompt = new QLabel( QStringLiteral( ">>>" ), central );
    m_prompt->setFont( font );
    m_input = new QLineEdit( central );
    m_input->setFont( font );
    m_input->installEventFilter( this );
    inputRow->addWidget( m_prompt );
    inputRow->addWidget( m_input, 1 );
    layout->addLayout( inputRow );

    setCentralWidget( central );
    resize( 800, 500 );

    QObject::connect( m_input, &QLineEdit::returnPressed,
                      [this]()
                      {
                          QString line = m_input->text();
                          m_input->clear();
                          RunLine( line );
                      } );

    SetupPythonEditor();
    m_input->setFocus();
}


KIPYTHON_FRAME::~KIPYTHON_FRAME()
{
    // The objects of Python are released with the GIL
    if( m_console && Py_IsInitialized() )
    {
        PyLOCK lock;
        m_console.reset();
    }
}


void KIPYTHON_FRAME::SetupPythonEditor()
{
    if( !Py_IsInitialized() )
    {
        appendOutput( ToQString( _( "Python is not available in this program." ) ) );
        m_input->setEnabled( false );
        return;
    }

    using namespace pybind11::literals;

    PyLOCK lock;

    try
    {
        pybind11::dict scope;
        pybind11::exec( CONSOLE_CLASS, scope );

        // The namespace of the console: a fresh __main__, as the shell of wxPython
        pybind11::module_ types = pybind11::module_::import( "types" );
        pybind11::object main = types.attr( "ModuleType" )( "__main__" );
        pybind11::dict   ns = pybind11::dict( main.attr( "__dict__" ).attr( "copy" )() );
        ns["__builtins__"] = pybind11::module_::import( "builtins" );

        m_console = std::make_unique<PY_CONSOLE>();
        m_console->console = scope["KiCadQtConsole"]( ns );

        pybind11::module_ sys = pybind11::module_::import( "sys" );
        QString intro = QStringLiteral( "Python %1\n\n" )
                                .arg( QString::fromStdString(
                                        pybind11::str( sys.attr( "version" ) ) ) );

        intro += ToQString( _( "Deprecation Notice:\n\nThis SWIG-based Python interface to the PCB "
                               "editor is deprecated and will be removed in a future version of "
                               "KiCad.  Please plan to move to the new IPC API and/or make use of "
                               "the kicad-cli tool for your KiCad automation needs.\n" ) );
        appendOutput( intro );

        // The module of pcbnew only with the board editor (as the shell of wxPython)
        if( Kiway().Player( FRAME_PCB_EDITOR, false ) )
        {
            pybind11::tuple result = m_console->console.attr( "run" )( "import pcbnew" );
            QString         text = QString::fromStdString( pybind11::str( result[1] ) );

            if( !text.isEmpty() )
                appendOutput( text );

            // The startup script of the user (the one of the shell of wxPython)
            wxFileName startup( SETTINGS_MANAGER::GetUserSettingsPath(),
                                wxT( "PyShell_pcbnew_startup.py" ) );

            if( startup.FileExists() )
            {
                pybind11::object written = m_console->console.attr( "run_file" )(
                        std::string( startup.GetFullPath().ToUTF8() ) );
                QString startupText = QString::fromStdString( pybind11::str( written ) );

                if( !startupText.isEmpty() )
                    appendOutput( startupText );
            }
        }
    }
    catch( const std::exception& e )
    {
        appendOutput( QString::fromUtf8( e.what() ) );
        m_console.reset();
        m_input->setEnabled( false );
    }
}


bool KIPYTHON_FRAME::RunLine( const QString& aLine )
{
    appendOutput( m_prompt->text() + QLatin1Char( ' ' ) + aLine );

    if( !aLine.trimmed().isEmpty() )
    {
        m_history.removeAll( aLine );
        m_history.append( aLine );
    }

    m_historyPos = m_history.size();

    if( !m_console )
        return false;

    bool more = false;

    {
        PyLOCK lock;

        try
        {
            pybind11::tuple result = m_console->console.attr( "run" )( aLine.toStdString() );
            more = result[0].cast<bool>();

            QString text = QString::fromStdString( pybind11::str( result[1] ) );

            if( text.endsWith( QLatin1Char( '\n' ) ) )
                text.chop( 1 );

            if( !text.isEmpty() )
                appendOutput( text );
        }
        catch( const std::exception& e )
        {
            appendOutput( QString::fromUtf8( e.what() ) );
        }
    }

    m_prompt->setText( more ? QStringLiteral( "..." ) : QStringLiteral( ">>>" ) );
    return more;
}


QString KIPYTHON_FRAME::GetConsoleText() const
{
    return m_output->toPlainText();
}


bool KIPYTHON_FRAME::eventFilter( QObject* aObject, QEvent* aEvent )
{
    if( aObject == m_input && aEvent->type() == QEvent::KeyPress )
    {
        int key = static_cast<QKeyEvent*>( aEvent )->key();

        if( ( key == Qt::Key_Up || key == Qt::Key_Down ) && !m_history.isEmpty() )
        {
            m_historyPos += key == Qt::Key_Up ? -1 : 1;
            m_historyPos = std::clamp( m_historyPos, 0, (int) m_history.size() );

            m_input->setText( m_historyPos < m_history.size() ? m_history[m_historyPos]
                                                              : QString() );
            return true;
        }
    }

    return KIWAY_PLAYER::eventFilter( aObject, aEvent );
}


void KIPYTHON_FRAME::appendOutput( const QString& aText )
{
    m_output->appendPlainText( aText );
    m_output->verticalScrollBar()->setValue( m_output->verticalScrollBar()->maximum() );
}
