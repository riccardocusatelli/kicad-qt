/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KIWAY_BASE_H
#define KIWAY_BASE_H

#include <array>

#include <kicommon.h>
#include <kiway_services.h>
#include <wx/string.h>


/**
 * The KIWAY without GUI: it loads the KIFACE DSOs, gives access to the project and runs jobs.
 *
 * kicad-cli uses it directly, and loads the headless KIFACEs (_eeschema_cli.kiface,
 * _pcbnew_cli.kiface).  KIWAY (kiway.h) adds the editor frames and the mail between them.
 */
class KICOMMON_API KIWAY_BASE : public KIWAY_SERVICES
{
public:
    /// @param aCtlBits the KFCTL_* flags given to the KIFACEs.
    explicit KIWAY_BASE( int aCtlBits );
    ~KIWAY_BASE() override;

    /**
     * Return the KIFACE* given a FACE_T.
     *
     * If it is not already loaded, the KIFACE is loaded and initialized with a call to
     * KIFACE::OnKifaceStart().
     *
     * @throw IO_ERROR if the *.kiface file could not be loaded.
     */
    KIFACE* KiFACE( FACE_T aFaceId, bool doLoad = true ) override;

    /// The project of the settings manager.
    PROJECT& Prj() const override;

    int ProcessJob( FACE_T aFace, JOB* aJob, REPORTER* aReporter = nullptr,
                    PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    /// Tell the loaded KIFACEs that the project has changed.
    void ProjectChanged() override;

    /// Nothing to update without GUI; KIWAY tells its frames.
    void CommonSettingsChanged( int aFlags = 0 ) override {}

    /// Tell the loaded KIFACEs that the program ends (KIFACE::OnKifaceEnd()).
    void OnKiwayEnd();

    /**
     * Overwrites previously set ctl bits, only for use in kicad.cpp to flip between
     * standalone and manager mode before we actually load anything
     */
    void SetCtlBits( int aCtlBits ) { m_ctl = aCtlBits; }

protected:
    /// Get the [path &] name of the DSO holding the requested FACE_T.
    const wxString dso_search_path( FACE_T aFaceId );

    /// Register a KIFACE that is linked in the program (single_top).
    bool set_kiface( FACE_T aFaceType, KIFACE* aKiface );

    int m_ctl;

private:
    static std::array<KIFACE*, KIWAY_FACE_COUNT> m_kiface;
    static std::array<int, KIWAY_FACE_COUNT>     m_kiface_version;
};

#endif // KIWAY_BASE_H
