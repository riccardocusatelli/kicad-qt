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

#ifndef KIWAY_SERVICES_H
#define KIWAY_SERVICES_H

#include <kiface.h>

class JOB;
class PROJECT;
class PROGRESS_REPORTER;
class REPORTER;


/**
 * The part of the KIWAY that non-GUI code uses: access to the other KIFACEs and to the
 * project, job execution, and change notifications.
 *
 * KIWAY (include/kiway.h) is a wxEvtHandler that also owns the editor frames and routes mail
 * between them.  Code that only needs the services below (settings, project, jobs, kicad-cli
 * commands) depends on this interface instead, and therefore not on the GUI toolkit.
 */
class KIWAY_SERVICES : public KIWAY_FACES
{
public:
    virtual ~KIWAY_SERVICES() = default;

    /**
     * Return the KIFACE implementing @a aFaceId, loading its DSO if @a doLoad is true.
     * @return nullptr if the KIFACE is not loaded (and @a doLoad is false) or cannot be loaded.
     */
    virtual KIFACE* KiFACE( FACE_T aFaceId, bool doLoad = true ) = 0;

    /// The project this KIWAY works on.
    virtual PROJECT& Prj() const = 0;

    /**
     * Run @a aJob in the KIFACE @a aFace.
     * @return a CLI::EXIT_CODES value.
     */
    virtual int ProcessJob( FACE_T aFace, JOB* aJob, REPORTER* aReporter = nullptr,
                            PROGRESS_REPORTER* aProgressReporter = nullptr ) = 0;

    /// The project has been loaded or closed: tell every participant.
    virtual void ProjectChanged() = 0;

    /// The common settings have changed: tell every participant.
    virtual void CommonSettingsChanged( int aFlags = 0 ) = 0;
};

#endif // KIWAY_SERVICES_H
