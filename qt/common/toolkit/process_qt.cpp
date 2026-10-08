/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file process_qt.cpp
 * The child processes and the delayed functions of the GUI with Qt (see
 * common/toolkit/process_wx.cpp).
 */

#include <toolkit/process.h>

#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

#include <qt_strings.h>


namespace TOOLKIT
{

long ExecuteAsync( const std::vector<wxString>& aArgv, const wxExecuteEnv* aEnv,
                   size_t aMaxOutput, bool aHideConsole, PROCESS_OUTPUT_HANDLER aOnOutput,
                   PROCESS_END_HANDLER aOnEnd )
{
    if( aArgv.empty() )
        return 0;

    QProcess*   process = new QProcess( QCoreApplication::instance() );
    QStringList args;

    for( size_t ii = 1; ii < aArgv.size(); ++ii )
        args << ToQString( aArgv[ii] );

    process->setProgram( ToQString( aArgv[0] ) );
    process->setArguments( args );

    if( aEnv )
    {
        QProcessEnvironment env;

        for( const auto& [name, value] : aEnv->env )
            env.insert( ToQString( name ), ToQString( value ) );

        process->setProcessEnvironment( env );

        if( !aEnv->cwd.IsEmpty() )
            process->setWorkingDirectory( ToQString( aEnv->cwd ) );
    }

#ifdef _WIN32
    if( aHideConsole )
    {
        process->setCreateProcessArgumentsModifier(
                []( QProcess::CreateProcessArguments* aArgs )
                {
                    aArgs->flags |= CREATE_NO_WINDOW;
                } );
    }
#else
    (void) aHideConsole;
#endif

    bool streamOutput = (bool) aOnOutput;

    if( streamOutput )
    {
        // The output is given to the handler as it comes (it is not kept)
        QObject::connect( process, &QProcess::readyReadStandardOutput, process,
                          [process, onOutput = std::move( aOnOutput )]()
                          {
                              onOutput( ToWxString( QString::fromUtf8(
                                      process->readAllStandardOutput() ) ) );
                          } );
    }

    auto readText =
            [aMaxOutput]( const QByteArray& aBytes )
            {
                QByteArray bytes = aBytes.left( (qsizetype) std::min<size_t>( aMaxOutput,
                                                                               aBytes.size() ) );
                return ToWxString( QString::fromUtf8( bytes ) );
            };

    QObject::connect( process, &QProcess::finished, process,
                      [process, streamOutput, readText, onEnd = std::move( aOnEnd )](
                              int aExitCode, QProcess::ExitStatus aStatus )
                      {
                          wxString output;

                          if( !streamOutput )
                              output = readText( process->readAllStandardOutput() );

                          wxString error = readText( process->readAllStandardError() );

                          if( onEnd )
                              onEnd( aStatus == QProcess::CrashExit ? -1 : aExitCode, output, error );

                          process->deleteLater();
                      } );

    process->start();

    if( !process->waitForStarted() )
    {
        delete process;
        return 0;
    }

    return (long) process->processId();
}


void RunOnMainThread( std::function<void()> aFunction, int aDelayMs )
{
    QCoreApplication* app = QCoreApplication::instance();

    if( !app )
        return;

    // Queued to the thread of the application, from any thread
    QMetaObject::invokeMethod( app,
            [app, aFunction = std::move( aFunction ), aDelayMs]()
            {
                QTimer::singleShot( std::max( 0, aDelayMs ), app, aFunction );
            },
            Qt::QueuedConnection );
}

} // namespace TOOLKIT
