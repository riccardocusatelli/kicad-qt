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
 * @file dialog_export_step_process.h
 * DIALOG_EXPORT_STEP_LOG of the Qt build (pcbnew/dialogs/dialog_export_step_process.h with
 * wxWidgets): the log of the export of a STEP file by kicad-cli, run by a QProcess.
 */

#pragma once

#include <dialogs/dialog_export_step_process_base.h>

#include <vector>

class QProcess;


class DIALOG_EXPORT_STEP_LOG : public DIALOG_EXPORT_STEP_PROCESS_BASE
{
public:
    DIALOG_EXPORT_STEP_LOG( wxWindow* aParent, const wxString& aStepCmd );
    ~DIALOG_EXPORT_STEP_LOG() override;

    /**
     * Register files that should be removed once the subprocess is known to have terminated.
     * The dialog takes ownership of cleanup timing to avoid races where a still-running
     * kicad-cli process would read files that the caller already deleted.
     */
    void SetTempFilesToCleanup( std::vector<wxString> aPaths );

private:
    bool TransferDataToWindow() override;

    /// The output of the process (its standard output and errors).
    void onOutput();

    /// The process ended (normally or not).
    void onFinished( int aExitCode, bool aCrashed );

    void onClose( wxCloseEvent& aEvent );

    void cleanupTempFiles();

    QProcess*             m_process;
    wxString              m_startMessage;
    std::vector<wxString> m_tempFiles;
    bool                  m_tempFilesCleaned = false;
};
