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

#include <cstring>

#include <app_monitor.h>
#include <core/ignore.h>
#include <macros.h>
#include <kiway.h>
#include <kiway_player.h>
#include <kiway_mail.h>
#include <pgm_base.h>
#include <config.h>
#include <id.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <logging.h>
#include <local_history.h>

#include <wx/dynlib.h>
#include <wx/stdpaths.h>
#include <wx/debug.h>
#include <wx/utils.h>
#include <confirm.h>
#include <core/ki_assert.h>




KIWAY::KIWAY( int aCtlBits, wxFrame* aTop ):
     KIWAY_BASE( aCtlBits ), m_top( nullptr ), m_blockingDialog( wxID_NONE ), m_local_history( nullptr )
{
    m_local_history = new LOCAL_HISTORY();
    SetTop( aTop );     // hook player_destroy_handler() into aTop.

    // Set the array of all known frame window IDs to empty = wxID_NONE,
    // once they are be created, they are added with FRAME_T as index to this array.
    // Note: A non empty entry does not mean the frame still exists.
    //   It means only the frame was created at least once. It can be destroyed after.
    //   These entries are not cleared automatically on window closing. The purpose is just
    //   to allow a call to wxWindow::FindWindowById() using a FRAME_T frame type
    for( int n = 0; n < KIWAY_PLAYER_COUNT; n++ )
        m_playerFrameId[n] = wxID_NONE;
}


KIWAY::~KIWAY()
{
    delete m_local_history;
}


#if 0
// Any event types derived from wxCommandEvt, like wxWindowDestroyEvent, are
// propagated upwards to parent windows if not handled below.  Therefore the
// m_top window should receive all wxWindowDestroyEvents originating from
// KIWAY_PLAYERs.  It does anyways, but now player_destroy_handler eavesdrops
// on that event stream looking for KIWAY_PLAYERs being closed.

void KIWAY::player_destroy_handler( wxWindowDestroyEvent& event )
{
    // Currently : do nothing
    event.Skip();  // skip to who, the wxApp?  I'm the top window.
}
#endif


void KIWAY::SetTop( wxFrame* aTop )
{
#if 0
    if( m_top )
    {
        m_top->Disconnect( wxEVT_DESTROY,
                           wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ),
                           nullptr, this );
    }

    if( aTop )
    {
        aTop->Connect( wxEVT_DESTROY,
                       wxWindowDestroyEventHandler( KIWAY::player_destroy_handler ),
                       nullptr, this );
    }
#endif

    m_top = aTop;
}


KIWAY_FACES::FACE_T KIWAY::KifaceType( FRAME_T aFrameType )
{
    switch( aFrameType )
    {
    case FRAME_SCH:
    case FRAME_SCH_SYMBOL_EDITOR:
    case FRAME_SCH_VIEWER:
    case FRAME_SYMBOL_CHOOSER:
    case FRAME_SIMULATOR:
        return FACE_SCH;

    case FRAME_PCB_EDITOR:
    case FRAME_FOOTPRINT_EDITOR:
    case FRAME_FOOTPRINT_VIEWER:
    case FRAME_FOOTPRINT_CHOOSER:
    case FRAME_FOOTPRINT_WIZARD:
    case FRAME_PCB_DISPLAY3D:
        return FACE_PCB;

    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return FACE_CVPCB;

    case FRAME_PYTHON:
        return FACE_PYTHON;

    case FRAME_GERBER:
        return FACE_GERBVIEW;

    case FRAME_PL_EDITOR:
        return FACE_PL_EDITOR;

    case FRAME_CALC:
        return FACE_PCB_CALCULATOR;

    case FRAME_BM2CMP:
        return FACE_BMP2CMP;

    default:
        return FACE_T( -1 );
    }
}


KIWAY_PLAYER* KIWAY::GetPlayerFrame( FRAME_T aFrameType )
{
    wxWindowID storedId = m_playerFrameId[aFrameType];

    if( storedId == wxID_NONE )
        return nullptr;

    wxWindow* frame = wxWindow::FindWindowById( storedId );

    // Since wxWindow::FindWindow*() is not cheap (especially if the window does not exist),
    // clear invalid entries to save CPU on repeated calls that do not lead to frame creation
    if( !frame )
        m_playerFrameId[aFrameType].compare_exchange_strong( storedId, wxID_NONE );

    return static_cast<KIWAY_PLAYER*>( frame );
}


KIWAY_PLAYER* KIWAY::Player( FRAME_T aFrameType, bool doCreate, wxTopLevelWindow* aParent )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( (unsigned) aFrameType >= KIWAY_PLAYER_COUNT )
    {
        KI_ASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return nullptr;
    }

    // return the previously opened window
    KIWAY_PLAYER* frame = GetPlayerFrame( aFrameType );

    if( frame )
        return frame;

    if( doCreate )
    {
        try
        {
            wxString msg = wxString::Format( "Creating window type %d", aFrameType );
            APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.player" );

            FACE_T  face_type = KifaceType( aFrameType );
            KIFACE* kiface = KiFACE( face_type );

            if( !kiface )
                return nullptr;

            frame = (KIWAY_PLAYER*) kiface->CreateKiWindow(
                                            aParent,    // Parent window of frame in modal mode,
                                                        // NULL in non modal mode
                                            aFrameType,
                                            this,
                                            m_ctl       // questionable need, these same flags
                                                        // were passed to KIFACE::OnKifaceStart()
                                            );
            if( frame )
                m_playerFrameId[aFrameType].store( frame->GetId() );

            return frame;
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
            wxLogError( _( "Error loading editor." ) );
        }
    }

    return nullptr;
}


bool KIWAY::PlayerClose( FRAME_T aFrameType, bool doForce )
{
    // Since this will be called from python, cannot assume that code will
    // not pass a bad aFrameType.
    if( (unsigned) aFrameType >= KIWAY_PLAYER_COUNT )
    {
        KI_ASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFrameType" ) );
        return false;
    }

    KIWAY_PLAYER* frame = GetPlayerFrame( aFrameType );

    if( frame == nullptr ) // Already closed
        return true;

    wxString msg = wxString::Format( "Closing window type %d", aFrameType );
    APP_MONITOR::AddNavigationBreadcrumb( msg, "kiway.playerclose" );

    if( frame->NonUserClose( doForce ) )
    {
        m_playerFrameId[aFrameType] = wxID_NONE;
        return true;
    }

    return false;
}


bool KIWAY::PlayersClose( bool doForce )
{
    bool ret = true;

    for( unsigned i=0; i < KIWAY_PLAYER_COUNT;  ++i )
        ret = ret && PlayerClose( FRAME_T( i ), doForce );

    return ret;
}


void KIWAY::PlayerDidClose( FRAME_T aFrameType )
{
    m_playerFrameId[aFrameType] = wxID_NONE;
}


void KIWAY::ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                         wxWindow* aSource, bool aFromOtherThread  )
{
    std::unique_ptr<KIWAY_MAIL_EVENT> mail =
            std::make_unique<KIWAY_MAIL_EVENT>( aDestination, aCommand, aPayload, aSource );

    if( aFromOtherThread )
        QueueEvent( mail.release() );
    else
        ProcessEvent( *mail );
}


void KIWAY::GetActions( std::vector<TOOL_ACTION*>& aActions ) const
{
    for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
        aActions.push_back( action );
}


void KIWAY::SetLanguage( int aLanguage )
{
    wxString errMsg;
    bool     ret = false;

    {
        // Only allow the traces to be logged by wx. We use our own system to log when the
        // OS doesn't support the language, so we want to hide the wx error.
        WX_LOG_TRACE_ONLY logtraceOnly;
        Pgm().SetLanguageIdentifier( aLanguage );
        ret = Pgm().SetLanguage( errMsg );
    }

    if( !ret )
    {
        wxString lang;

        for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
        {
            if( aLanguage == LanguagesList[ii].m_KI_Lang_Identifier )
            {
                if( LanguagesList[ii].m_DoNotTranslate )
                    lang = LanguagesList[ii].m_Lang_Label;
                else
                    lang = wxGetTranslation( LanguagesList[ii].m_Lang_Label );

                break;
            }
        }

        DisplayErrorMessage( nullptr,
                             wxString::Format( _( "Unable to switch language to %s" ), lang ),
                             errMsg );
        return;
    }

#if 1
    // This is a risky hack that goes away if we allow the language to be
    // set only from the top most frame if !Kiface.IsSingle()

    // Only for the C++ project manager, and not for the python one and not for
    // single_top do we look for the EDA_BASE_FRAME as the top level window.
    // For single_top this is not needed because that window is registered in
    // the array below.
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if ( top )
        {
            top->ShowChangedLanguage();
            wxCommandEvent e( EDA_LANG_CHANGED );
            top->GetEventHandler()->ProcessEvent( e );
        }
    }
#endif

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
        {
            frame->ShowChangedLanguage();
            wxCommandEvent e( EDA_LANG_CHANGED );
            frame->GetEventHandler()->ProcessEvent( e );
        }
    }
}


void KIWAY::CommonSettingsChanged( int aFlags )
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->CommonSettingsChanged( aFlags );
    }

    if( aFlags & ENVVARS_CHANGED )
    {
        if( KIFACE* sch = KiFACE( FACE_SCH, false ) )
            sch->PreloadLibraries( this );

        if( KIFACE* pcb = KiFACE( FACE_PCB, false ) )
            pcb->PreloadLibraries( this );
    }

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->CommonSettingsChanged( aFlags );
    }
}


void KIWAY::ClearFileHistory()
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->ClearFileHistory();
    }

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->ClearFileHistory();
    }
}


void KIWAY::ProjectChanged()
{
    // Skip project change notifications during application shutdown to avoid
    // clearing savers and re-registering them unnecessarily
    if( PgmOrNull() && Pgm().m_Quitting )
        return;

    APP_MONITOR::AddNavigationBreadcrumb( "Changing project", "kiway.projectchanged" );

    LocalHistory().ClearAllSavers();

    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->ProjectChanged();
    }

    // Cancel an in-progress load of libraries; handled through the schematic and PCB ifaces.
    KIWAY_BASE::ProjectChanged();

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i );

        if( frame )
            frame->ProjectChanged();
    }
}


wxWindow* KIWAY::GetBlockingDialog()
{
    return wxWindow::FindWindowById( m_blockingDialog );
}


void KIWAY::SetBlockingDialog( wxWindow* aWin )
{
    if( !aWin )
        m_blockingDialog = wxID_NONE;
    else
        m_blockingDialog = aWin->GetId();
}


bool KIWAY::ProcessEvent( wxEvent& aEvent )
{
    KIWAY_MAIL_EVENT* mail = dynamic_cast<KIWAY_MAIL_EVENT*>( &aEvent );

    if( mail )
    {
        FRAME_T dest = mail->Dest();

        // see if recipient is alive
        KIWAY_PLAYER* alive = Player( dest, false );

        if( alive )
        {
#if 1
            return alive->ProcessEvent( aEvent );
#else
            alive->KiwayMailIn( *mail );
            return true;
#endif
        }
    }

    return false;
}


void KIWAY::QueueEvent( wxEvent* aEvent )
{
    KIWAY_MAIL_EVENT* mail = dynamic_cast<KIWAY_MAIL_EVENT*>( aEvent );

    if( mail )
    {
        FRAME_T dest = mail->Dest();

        // see if recipient is alive
        KIWAY_PLAYER* alive = Player( dest, false );

        if( alive )
        {
            alive->GetEventHandler()->QueueEvent( aEvent );
        }
    }
}


bool KIWAY::ProcessJobConfigDialog( KIWAY_FACES::FACE_T aFace, JOB* aJob, wxWindow* aWindow )
{
    KIFACE* kiface = KiFACE( aFace );

    return kiface->HandleJobConfig( aJob, aWindow );
}


void KIWAY::OnKiCadExit()
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dynamic_cast could be better, but creates link issues
        // (some basic_frame functions not found) on some platforms,
        // so a static_cast is used.
        EDA_BASE_FRAME* top = static_cast<EDA_BASE_FRAME*>( m_top );

        if( top )
            top->Close( false );
    }
}
