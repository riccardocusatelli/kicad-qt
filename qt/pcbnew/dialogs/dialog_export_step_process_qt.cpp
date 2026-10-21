/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file dialog_export_step_process_qt.cpp
 * DIALOG_EXPORT_STEP_LOG with Qt (pcbnew/dialogs/dialog_export_step_process.cpp with
 * wxWidgets): kicad-cli runs in a QProcess, its output (standard output and errors) is shown
 * as it comes.
 */

#include <dialog_export_step_process.h>

#include <QProcess>
#include <QStringList>

#include <qt_strings.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>


DIALOG_EXPORT_STEP_LOG::DIALOG_EXPORT_STEP_LOG( wxWindow* aParent, const wxString& aStepCmd ) :
        DIALOG_EXPORT_STEP_PROCESS_BASE( aParent ),
        m_process( nullptr )
{
    m_sdbSizerOK->Enable( false );

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_EXPORT_STEP_LOG::onClose, this );

    // Print the command line used to run kicad-cli.
    // it can be useful if kicad-cli as a problem.
    // However it cannot be printed in the Ctor, but only in TransferDataToWindow(),
    // after DIALOG_SHIM initializations
    m_startMessage.Append( _( "Command line:\n" ) );
    m_startMessage.Append( aStepCmd );
    m_startMessage.Append( wxT( "\n\n" ) );

    QStringList args = QProcess::splitCommand( ToQString( aStepCmd ) );

    if( args.isEmpty() )
    {
        m_startMessage.Append( "Unable to launch the export process.\n" );
        return;
    }

    QString program = args.takeFirst();

    m_process = new QProcess( this );
    m_process->setProcessChannelMode( QProcess::MergedChannels );

    QObject::connect( m_process, &QProcess::readyReadStandardOutput, this,
                      [this]()
                      {
                          onOutput();
                      } );

    QObject::connect( m_process, &QProcess::finished, this,
                      [this]( int aExitCode, QProcess::ExitStatus aStatus )
                      {
                          onFinished( aExitCode, aStatus == QProcess::CrashExit );
                      } );

    QObject::connect( m_process, &QProcess::errorOccurred, this,
                      [this]( QProcess::ProcessError aError )
                      {
                          // A process not started never finishes
                          if( aError == QProcess::FailedToStart )
                              onFinished( -1, true );
                      } );

    m_activityGauge->Pulse();

    m_process->start( program, args );
}


DIALOG_EXPORT_STEP_LOG::~DIALOG_EXPORT_STEP_LOG()
{
    if( m_process && m_process->state() != QProcess::NotRunning )
    {
        QObject::disconnect( m_process, nullptr, this, nullptr );
        m_process->kill();
        m_process->waitForFinished( 3000 );
    }

    cleanupTempFiles();
}


bool DIALOG_EXPORT_STEP_LOG::TransferDataToWindow()
{
    m_textCtrlLog->AppendText( m_startMessage );
    return true;
}


void DIALOG_EXPORT_STEP_LOG::SetTempFilesToCleanup( std::vector<wxString> aPaths )
{
    m_tempFiles = std::move( aPaths );
}


void DIALOG_EXPORT_STEP_LOG::cleanupTempFiles()
{
    if( m_tempFilesCleaned )
        return;

    for( const wxString& path : m_tempFiles )
    {
        if( !path.IsEmpty() && wxFileExists( path ) )
            wxRemoveFile( path );
    }

    m_tempFilesCleaned = true;
}


void DIALOG_EXPORT_STEP_LOG::onOutput()
{
    m_textCtrlLog->AppendText( ToWxString( QString::fromLocal8Bit( m_process->readAllStandardOutput() ) ) );
    m_activityGauge->Pulse();
}


void DIALOG_EXPORT_STEP_LOG::onFinished( int aExitCode, bool aCrashed )
{
    // The last output of the process
    if( m_process && m_process->bytesAvailable() > 0 )
        onOutput();

    m_sdbSizerOK->Enable( true );

    // set the progress bar to complete/incomplete base don status
    m_activityGauge->SetRange( 1 );

    if( aCrashed || aExitCode != 0 )
    {
        m_textCtrlLog->SetForegroundColour( *wxRED );

        m_textCtrlLog->AppendText( wxS( "\n*** " ) );
        m_textCtrlLog->AppendText(
                wxString::Format( _( "Process failed with exit code %d" ), aExitCode ) );
        m_textCtrlLog->AppendText( wxS( " ***\n" ) );

        m_activityGauge->SetValue( 0 );
    }
    else
    {
        m_textCtrlLog->AppendText( wxS( "\n*** " ) );
        m_textCtrlLog->AppendText( wxString::Format( _( "Success" ) ) );
        m_textCtrlLog->AppendText( wxS( " ***\n" ) );

        m_activityGauge->SetValue( 1 );
    }

    // The child has exited; it is safe to remove any temp inputs we were keeping alive for it.
    cleanupTempFiles();
}


void DIALOG_EXPORT_STEP_LOG::onClose( wxCloseEvent& aEvent )
{
    if( m_process && m_process->state() != QProcess::NotRunning )
    {
        if( aEvent.CanVeto() )
        {
            wxMessageDialog dlg( this, _( "Do you want to cancel the export process?" ),
                                 _( "Cancel Export" ), wxYES_NO );

            if( dlg.ShowModal() == wxID_NO )
            {
                aEvent.Veto();
                return;
            }
        }

        // The process is stopped: its end isn't reported any more
        QObject::disconnect( m_process, nullptr, this, nullptr );
        m_process->kill();
        m_process->waitForFinished( 3000 );
    }

    cleanupTempFiles();

    // Clear log window message, storing the log data in config has no interest.
    m_textCtrlLog->Clear();

    aEvent.Skip();
}
