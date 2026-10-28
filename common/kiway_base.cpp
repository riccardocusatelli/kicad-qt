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

#include <kiway_base.h>

#include <clocale>

#include <app_monitor.h>
#include <config.h>
#include <ki_exception.h>
#include <kiface.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <core/ki_assert.h>

#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>


std::array<KIFACE*, KIWAY_FACES::FACE_T::KIWAY_FACE_COUNT> KIWAY_BASE::m_kiface;
std::array<int, KIWAY_FACES::FACE_T::KIWAY_FACE_COUNT>     KIWAY_BASE::m_kiface_version;


KIWAY_BASE::KIWAY_BASE( int aCtlBits ) :
        m_ctl( aCtlBits )
{
}


KIWAY_BASE::~KIWAY_BASE()
{
}


const wxString KIWAY_BASE::dso_search_path( FACE_T aFaceId )
{
    const char*   name;

    switch( aFaceId )
    {
    case FACE_SCH:              name = KIFACE_PREFIX "eeschema";            break;
    case FACE_PCB:              name = KIFACE_PREFIX "pcbnew";              break;
    case FACE_CVPCB:            name = KIFACE_PREFIX "cvpcb";               break;
    case FACE_GERBVIEW:         name = KIFACE_PREFIX "gerbview";            break;
    case FACE_PL_EDITOR:        name = KIFACE_PREFIX "pl_editor";           break;
    case FACE_PCB_CALCULATOR:   name = KIFACE_PREFIX "pcb_calculator";      break;
    case FACE_BMP2CMP:          name = KIFACE_PREFIX "bitmap2component";    break;
    case FACE_PYTHON:           name = KIFACE_PREFIX "kipython";            break;

    default:
        KI_ASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }

#ifndef __WXMAC__
    wxString path;

    if( m_ctl & (KFCTL_STANDALONE | KFCTL_CPP_PROJECT_SUITE) )
    {
        // The 2 *.cpp program launchers: single_top.cpp and kicad.cpp expect
        // the *.kiface's to reside in same directory as their binaries do.
        wxString appDir;

        // When running inside an AppImage, the bundled ld-linux is invoked as a wrapper
        // which causes /proc/self/exe to resolve to the dynamic linker rather than the
        // actual binary. Use APPDIR to construct the correct executable path.
        if( wxGetEnv( wxT( "APPDIR" ), &appDir ) )
            path = appDir + wxT( "/usr/bin/kicad" );
        else
            path = wxStandardPaths::Get().GetExecutablePath();
    }

    wxFileName fn = path;
#else
    // we have the dso's in main OSX bundle kicad.app/Contents/PlugIns
    wxFileName fn = Pgm().GetExecutablePath();
    fn.AppendDir( wxT( "Contents" ) );
    fn.AppendDir( wxT( "PlugIns" ) );
#endif

    // kicad-cli loads the KIFACEs built without GUI, kicad-qt the ones of the Qt shell (with
    // their GUI, the editors of KiCad, where they are built)
    wxString dsoName = name;

    if( ( m_ctl & KFCTL_CLI ) && ( aFaceId == FACE_SCH || aFaceId == FACE_PCB ) )
        dsoName += wxT( "_cli" );
    else if( ( m_ctl & KFCTL_QT_GUI )
             && ( aFaceId == FACE_SCH || aFaceId == FACE_PCB || aFaceId == FACE_CVPCB ) )
        dsoName += wxT( "_qtgui" );
    else if( ( m_ctl & KFCTL_QT ) && ( aFaceId == FACE_SCH || aFaceId == FACE_PCB ) )
        dsoName += wxT( "_qt" );

    fn.SetName( dsoName );

    // To speed up development, it's sometimes nice to run kicad from inside
    // the build path.  In that case, each program will be in a subdirectory.
    // To find the DSOs, we need to go up one directory and then enter a subdirectory.
    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
#ifdef __WXMAC__
        // On Mac, all of the kifaces are placed in the kicad.app bundle, even though the individual
        // standalone binaries are placed in separate bundles before the make install step runs.
        // So, we have to jump up to the kicad directory, then the PlugIns section of the kicad
        // bundle.
        fn = wxStandardPaths::Get().GetExecutablePath();

        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "kicad" ) );
        fn.AppendDir( wxT( "kicad.app" ) );
        fn.AppendDir( wxT( "Contents" ) );
        fn.AppendDir( wxT( "PlugIns" ) );
        fn.SetName( dsoName );
#else
        const char*   dirName;

        // The subdirectories usually have the same name as the kiface
        switch( aFaceId )
        {
            case FACE_PL_EDITOR: dirName = "pagelayout_editor";   break;
            case FACE_PYTHON:    dirName = "scripting";           break;
            default:             dirName = name + 1;              break;
        }

        fn.RemoveLastDir();
        fn.AppendDir( dirName );
#endif
    }

    // Here a "suffix" == an extension with a preceding '.',
    // so skip the preceding '.' to get an extension
    fn.SetExt( &KIFACE_SUFFIX[1] );

    return fn.GetFullPath();
}


PROJECT& KIWAY_BASE::Prj() const
{
    return Pgm().GetSettingsManager().Prj();
}


KIFACE* KIWAY_BASE::KiFACE( FACE_T aFaceId, bool doLoad )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFaceId.
    if( (unsigned) aFaceId >= m_kiface.size() )
    {
        KI_ASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return nullptr;
    }

    // return the previously loaded KIFACE, if it was.
    if( m_kiface[aFaceId] )
        return m_kiface[aFaceId];

    // DSO with KIFACE has not been loaded yet, does caller want to load it?
    if( doLoad )
    {
        wxString dname = dso_search_path( aFaceId );

        // Insert DLL search path for kicad_3dsg from build dir
        if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
        {
            wxFileName myPath = wxStandardPaths::Get().GetExecutablePath();

            if( !myPath.GetPath().EndsWith( wxT( "pcbnew" ) ) )
            {
                myPath.RemoveLastDir();
                myPath.AppendDir( wxT( "pcbnew" ) );
                KIPLATFORM::APP::AddDynamicLibrarySearchPath( myPath.GetPath() );
            }
        }

        wxString msg;

#ifdef KICAD_WIN32_VERIFY_CODESIGN
        bool codeSignOk = KIPLATFORM::ENV::VerifyFileSignature( dname );
        if( !codeSignOk )
        {
            msg.Printf( _( "Failed to verify kiface library '%s' signature." ), dname );
            THROW_IO_ERROR( msg );
        }
#endif

        wxDynamicLibrary dso;

        void*   addr = nullptr;

        // For some reason wxDynamicLibrary::Load() crashes in some languages
        // (chinese for instance) when loading the dynamic library.
        // The crash happens for Eeschema.
        // So switch to "C" locale during loading (LC_COLLATE is enough).
        int lc_new_type = LC_COLLATE;
        std::string user_locale = setlocale( lc_new_type, nullptr );
        setlocale( lc_new_type, "C" );

        bool success = dso.Load( dname, wxDL_VERBATIM | wxDL_NOW | wxDL_GLOBAL );

        setlocale( lc_new_type, user_locale.c_str() );

        msg = wxString::Format( "Loading kiface %d", aFaceId );
        APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.kiface" );

        if( !success )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Apparently this is not true on all
            // platforms and/or wxWidgets builds and KiCad will crash.  Throwing the exception
            // here and catching it in the KiCad launcher resolves the crash issue.  See bug
            // report https://bugs.launchpad.net/kicad/+bug/1577786.

            msg.Printf( _( "Failed to load kiface library '%s'." ), dname );
            THROW_IO_ERROR( msg );
        }
        else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == nullptr )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.  Assume the same thing applies here as
            // above with the Load() call.  This has not been tested.
            msg.Printf( _( "Could not read instance name and version from kiface library '%s'." ),
                        dname );
            THROW_IO_ERROR( msg );
        }
        else
        {
            KIFACE_GETTER_FUNC* ki_getter = (KIFACE_GETTER_FUNC*) addr;

            KIFACE* kiface = ki_getter( &m_kiface_version[aFaceId], KIFACE_VERSION, &Pgm() );

            // KIFACE_GETTER_FUNC function comment (API) says the non-NULL is unconditional.
            KI_ASSERT_MSG( kiface,
                          wxT( "attempted DSO has a bug, failed to return a KIFACE*" ) );

            wxDllType dsoHandle = dso.Detach();

            bool startSuccess = false;

            // Give the DSO a single chance to do its "process level" initialization.
            // "Process level" specifically means stay away from any projects in there.

            try
            {
                startSuccess = kiface->OnKifaceStart( &Pgm(), m_ctl, this );
            }
            catch (...)
            {
                // OnKiFaceStart may generate an exception
                // Before we continue and ultimately unload our module to retry we need
                // to process the exception before we delete the free the memory space the
                // exception resides in
                Pgm().HandleException( std::current_exception() );
            }

            if( startSuccess )
            {
                return m_kiface[aFaceId] = kiface;
            }
            else
            {
                // Usually means canceled initial global library setup
                // But it could have been an exception/failure
                // Let the module go out of scope to unload
                dso.Attach( dsoHandle );

                return nullptr;
            }
        }
    }

    return nullptr;
}


bool KIWAY_BASE::set_kiface( FACE_T aFaceType, KIFACE* aKiface )
{
    if( (unsigned) aFaceType < (unsigned) KIWAY_FACE_COUNT )
    {
        m_kiface[aFaceType] = aKiface;
        return true;
    }

    return false;
}


int KIWAY_BASE::ProcessJob( KIWAY_FACES::FACE_T aFace, JOB* aJob, REPORTER* aReporter,
                            PROGRESS_REPORTER* aProgressReporter )
{
    KIFACE* kiface = KiFACE( aFace );

    return kiface->HandleJob( aJob, aReporter, aProgressReporter );
}


void KIWAY_BASE::ProjectChanged()
{
    // Cancel an in-progress load of libraries; handled through the schematic and PCB ifaces.
    // Use doLoad=false: only notify already-loaded kifaces, don't force-load absent ones
    // (e.g. eeschema kiface isn't available in standalone pcbnew).
    if( KIFACE* schface = KiFACE( KIWAY_FACES::FACE_SCH, false ) )
        schface->ProjectChanged();

    if( KIFACE* pcbface = KiFACE( KIWAY_FACES::FACE_PCB, false ) )
        pcbface->ProjectChanged();
}


void KIWAY_BASE::OnKiwayEnd()
{
    for( KIFACE* i : m_kiface )
    {
        if( i )
            i->OnKifaceEnd();
    }
}
