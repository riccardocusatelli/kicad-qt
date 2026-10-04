/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <functional>

#include <wx/string.h>

class JOBSET;
struct JOBSET_DESTINATION;
class JOB_SPECIAL_COPYFILES;
struct JOBSET_JOB;
class KIWAY_SERVICES;
class REPORTER;
class PROGRESS_REPORTER;
class PROJECT;

class JOBS_RUNNER
{
public:
    /// Called when a job starts, with a message saying which (for a progress dialog).
    using JOB_STARTED_HANDLER = std::function<void( const wxString& aMessage )>;

    JOBS_RUNNER( KIWAY_SERVICES* aKiway, JOBSET* aJobsFile, PROJECT* aProject,
                 REPORTER& aReporter, PROGRESS_REPORTER* aProgressReporter,
                 JOB_STARTED_HANDLER aJobStarted = nullptr );

    bool RunJobsAllDestinations( bool aBail = false );
    bool RunJobsForDestination( JOBSET_DESTINATION* aDestination, bool aBail = false );

private:
    int runSpecialExecute( const JOBSET_JOB* aJob, REPORTER* aReporter, PROJECT* aProject );
    int runSpecialCopyFiles( const JOB_SPECIAL_COPYFILES* aJob, PROJECT* aProject,
                             std::vector<wxString>& aPathsWritten );

private:
    KIWAY_SERVICES*         m_kiway;
    JOBSET*                 m_jobsFile;
    REPORTER&               m_reporter;
    PROGRESS_REPORTER*      m_progressReporter;
    JOB_STARTED_HANDLER     m_jobStarted;
    PROJECT*                m_project;
};