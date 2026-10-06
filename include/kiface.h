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

#ifndef KIFACE_H_
#define KIFACE_H_

/**
 * @file kiface.h
 * The KIFACE interface implemented by each KiCad DSO, and the DSO identifiers.
 *
 * Split from kiway.h: KIWAY (the GUI-side software bus, a wxEvtHandler) is not needed to
 * declare or query a KIFACE, so data-model code including this header no longer pulls in wx
 * event and window headers.
 */

#include <vector>

#include <wx/string.h>

#include <import_export.h>


/// Identifiers of the KIFACE implementations.  KIWAY derives from this, so the historical
/// spelling KIWAY::FACE_PCB keeps working; data-model code uses KIWAY_FACES::FACE_PCB.
struct KIWAY_FACES
{
    enum FACE_T
    {
        FACE_SCH,               ///< eeschema DSO
        FACE_PCB,               ///< pcbnew DSO
        FACE_CVPCB,
        FACE_GERBVIEW,
        FACE_PL_EDITOR,
        FACE_PCB_CALCULATOR,
        FACE_BMP2CMP,
        FACE_PYTHON,

        KIWAY_FACE_COUNT
    };
};


#define KIFACE_VERSION      1
#define KIFACE_GETTER       KIFACE_1

// The KIFACE acquisition function is declared extern "C" so its name should not
// be mangled.
#define KIFACE_INSTANCE_NAME_AND_VERSION   "KIFACE_1"

#ifndef SWIG
#if defined(__linux__) || defined(__FreeBSD__)
 #define LIB_ENV_VAR    wxT( "LD_LIBRARY_PATH" )
#elif defined(__WXMAC__)
 #define LIB_ENV_VAR    wxT( "DYLD_LIBRARY_PATH" )
#elif defined(_WIN32)
 #define LIB_ENV_VAR    wxT( "PATH" )
#else
 #error Platform support missing
#endif
#endif  // SWIG

class wxConfigBase;
class wxWindow;
class PGM_BASE;
class KIWAY;
class KIWAY_SERVICES;
class KIWAY_PLAYER;
class wxTopLevelWindow;
class TOOL_ACTION;
class JOB;
class REPORTER;
class PROGRESS_REPORTER;
class STARTWIZARD_PROVIDER;
class LOCAL_HISTORY;


/**
 * Implement a participant in the KIWAY alchemy.
 *
 * KIWAY is a minimalistic software bus for communications between various DLLs/DSOs
 * (DSOs) within the same KiCad process.  It makes it possible to call between DSOs
 * without having to link them together.  Most all calls are via virtual functions
 * which means C++ vtables are used to hold function pointers and eliminate the need
 * to link to specific object code libraries.  There is one KIWAY in the launching
 * portion of the process for each open KiCad project.  Each project has its own KIWAY.
 * Within a KIWAY is an actual PROJECT data structure.  A KIWAY also facilitates
 * communicating between DSOs on the topic of the project in question.
 */
struct KIFACE
{
    // The order of functions establishes the vtable sequence, do not change the
    // order of functions in this listing unless you recompile all clients of
    // this interface.

    virtual ~KIFACE() throw() {}

#define KFCTL_STANDALONE ( 1 << 0 )        ///< Running as a standalone Top.
#define KFCTL_CPP_PROJECT_SUITE ( 1 << 1 ) ///< Running under C++ project mgr, possibly with others.
#define KFCTL_CLI ( 1 << 2 )               ///< Running as CLI app
#define KFCTL_QT ( 1 << 3 )                ///< Running under the Qt shell (kicad-qt)


    /**
     * Called just once shortly after the DSO is loaded.
     * It is the second function called, immediately after the KIFACE_GETTER().  However
     * before either of those, static C++ constructors are called.  The DSO implementation
     * should do process level initialization here, not project specific since there will
     * be multiple projects open eventually.
     *
     * @param aProgram is the process block: #PGM_BASE*.
     * @param aCtlBits consists of bit flags from the set of KFCTL_* \#defines above.
     * @param aKiway is the KIWAY loading the DSO: a KIWAY in the GUI programs, a KIWAY_BASE
     *               in kicad-cli.
     * @return true if DSO initialized OK, false if not.  When returning false, the loader
     *         may optionally decide to terminate the process or not, but will not put out
     *         any UI because that is the duty of this function to say why it is returning
     *         false.  Never return false without having reported to the UI why.
     */
    virtual bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY_SERVICES* aKiway ) = 0;

    /**
     * Called just once just before the DSO is to be unloaded.
     *
     * It is called before static C++ destructors are called.  A default implementation
     * is supplied.
     */
    virtual void OnKifaceEnd() = 0;

    /**
     * Reloads global state.
     */
    virtual void Reset() = 0;

    /**
     * Create a wxWindow for the current project.
     *
     * The caller must cast the return value into the known type.
     *
     * @param aParent may be NULL or is otherwise the parent to connect under.  If NULL
     *                then caller may want to connect the returned wxWindow into some
     *                hierarchy after this function returns.
     * @param aClassId identifies which wxFrame or wxDialog to retrieve, using a value
     *                 known to the implementing KIFACE.
     * @param aKIWAY tells the window which KIWAY (and PROJECT) it is a participant in.
     * @param aCtlBits consists of bit flags from the set of KFCTL_* \#defines above.
     * @return the window created and if not NULL, should be cast into the known type using
     *         and old school cast.  dynamic_cast is problematic since it needs typeinfo probably
     *         not contained in the caller's link image.
     */
    virtual wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId,
                                      KIWAY* aKIWAY, int aCtlBits = 0 ) = 0;

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    virtual void SaveFileAs( const wxString& srcProjectBasePath,
                             const wxString& srcProjectName,
                             const wxString& newProjectBasePath,
                             const wxString& newProjectName,
                             const wxString& srcFilePath,
                             wxString& aErrors )
    {
        // If a KIFACE owns files then it needs to implement this....
    }

    /**
     * Return pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an
     * interface, similar to how the KIFACE interface is exported.  But if you know
     * what you are doing use it to retrieve anything you want.  Segfaults are your fault.
     *
     * @param aDataId identifies which object you want the address of, and consists of
     *                choices known in advance by the implementing KIFACE.
     * @return the requested object which must be cast into the known type.
     */
    virtual void* IfaceOrAddress( int aDataId ) = 0;

    /**
     * Append this Kiface's registered actions to the given list.
     */
    virtual void GetActions( std::vector<TOOL_ACTION*>& aActions ) const = 0;

    virtual int HandleJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter )
    {
        return 0;
    }

    virtual bool HandleJobConfig( JOB* aJob, wxWindow* aParent )
    {
        return 0;
    }

    virtual void PreloadLibraries( KIWAY* aKiway ) {}

    virtual void CancelPreload( bool aBlock = true ) {}

    virtual void ProjectChanged() {}
};



/**
 * Point to the one and only KIFACE export.
 *
 * The export's address is looked up via symbolic string and should be extern "C" to avoid name
 * mangling. This function will only be called one time.  The DSO itself however may be asked
 * to support multiple Top windows, i.e. multiple projects within its lifetime.
 *
 * @param aKIFACEversion is where to put the API version implemented by the KIFACE.
 * @param aKIWAYversion tells the KIFACE what KIWAY version will be available.
 * @param aProgram is a pointer to the PGM_BASE for this process.
 * @return unconditionally, cannot fail.
 */
typedef KIFACE* KIFACE_GETTER_FUNC( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );


#ifndef SWIG

/// No name mangling.  Each KIFACE (DSO/DLL) will implement this once.
extern "C" {
#if defined(BUILD_KIWAY_DLL)
    KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );
#else
    KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram );
#endif
}

#endif  // SWIG


#endif // KIFACE_H_
