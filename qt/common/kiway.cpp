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


/**
 * @file kiway.cpp
 * KIWAY of the Qt build: the players are the frames of Qt that the KIFACEs make.
 */

#include <kiway.h>

#include <QCoreApplication>
#include <QMainWindow>
#include <QTimer>

#include <wx/kiwx_window.h>

#include <app_monitor.h>
#include <confirm.h>
#include <eda_base_frame.h>
#include <kiway_player.h>
#include <kiway_mail.h>
#include <local_history.h>
#include <pgm_base.h>
#include <tool/action_manager.h>
#include <core/ki_assert.h>


KIWAY::KIWAY( int aCtlBits, QMainWindow* aTop ) :
        KIWAY_BASE( aCtlBits ),
        m_top( aTop ),
        m_local_history( nullptr )
{
    m_local_history = new LOCAL_HISTORY();
}


KIWAY::~KIWAY()
{
    delete m_local_history;
}


void KIWAY::SetTop( QMainWindow* aTop )
{
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
    // The pointers are cleared by Qt when the frames are deleted
    return dynamic_cast<KIWAY_PLAYER*>( m_players[aFrameType].data() );
}


KIWAY_PLAYER* KIWAY::Player( FRAME_T aFrameType, bool doCreate, QWidget* aParent )
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

            frame = dynamic_cast<KIWAY_PLAYER*>( kiface->CreateKiWindow(
                                            aParent,    // Parent window of frame in modal mode,
                                                        // NULL in non modal mode
                                            aFrameType,
                                            this,
                                            m_ctl       // questionable need, these same flags
                                                        // were passed to KIFACE::OnKifaceStart()
                                            ) );

            if( frame )
                m_players[aFrameType] = frame;

            return frame;
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
            DisplayErrorMessage( nullptr, _( "Error loading editor." ) );
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
        m_players[aFrameType] = nullptr;
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
    if( (unsigned) aFrameType < KIWAY_PLAYER_COUNT )
        m_players[aFrameType] = nullptr;
}


void KIWAY::ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                         QWidget* aSource, bool aFromOtherThread  )
{
    if( aFromOtherThread )
    {
        // Delivered by the event loop of the GUI, with a copy of the payload
        QTimer::singleShot( 0, m_top.data() ? static_cast<QObject*>( m_top.data() ) : qApp,
                            [this, aDestination, aCommand, payload = aPayload]() mutable
                            {
                                if( KIWAY_PLAYER* alive = Player( aDestination, false ) )
                                {
                                    KIWAY_MAIL_EVENT mail( aDestination, aCommand, payload );
                                    alive->KiwayMailIn( mail );
                                }
                            } );
        return;
    }

    if( KIWAY_PLAYER* alive = Player( aDestination, false ) )
    {
        KIWAY_MAIL_EVENT mail( aDestination, aCommand, aPayload, aSource );
        alive->KiwayMailIn( mail );
    }
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

    Pgm().SetLanguageIdentifier( aLanguage );
    ret = Pgm().SetLanguage( errMsg );

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

    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        if( EDA_BASE_FRAME* top = dynamic_cast<EDA_BASE_FRAME*>( m_top.data() ) )
            top->ShowChangedLanguage();
    }

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        if( KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i ) )
            frame->ShowChangedLanguage();
    }
}


void KIWAY::CommonSettingsChanged( int aFlags )
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        if( EDA_BASE_FRAME* top = dynamic_cast<EDA_BASE_FRAME*>( m_top.data() ) )
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
        if( KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i ) )
            frame->CommonSettingsChanged( aFlags );
    }
}


void KIWAY::ProjectChanged()
{
    if( PgmOrNull() && Pgm().m_Quitting )
        return;

    APP_MONITOR::AddNavigationBreadcrumb( "Changing project", "kiway.projectchanged" );

    LocalHistory().ClearAllSavers();

    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        if( EDA_BASE_FRAME* top = dynamic_cast<EDA_BASE_FRAME*>( m_top.data() ) )
            top->ProjectChanged();
    }

    // Cancel an in-progress load of libraries; handled through the schematic and PCB ifaces
    KIWAY_BASE::ProjectChanged();

    for( unsigned i=0;  i < KIWAY_PLAYER_COUNT;  ++i )
    {
        if( KIWAY_PLAYER* frame = GetPlayerFrame( ( FRAME_T )i ) )
            frame->ProjectChanged();
    }
}


wxWindow* KIWAY::GetBlockingDialog()
{
    // the windows of the compatibility layer are a QWidget and a wxWindow
    return dynamic_cast<wxWindow*>( m_blockingDialog.data() );
}


void KIWAY::SetBlockingDialog( wxWindow* aWin )
{
    m_blockingDialog = aWin ? aWin->GetHandle() : nullptr;
}


void KIWAY::OnKiCadExit()
{
    if( m_ctl & KFCTL_CPP_PROJECT_SUITE )
    {
        // A dialog box is displayed if the frame of the project manager has unsaved changes
        if( EDA_BASE_FRAME* top = dynamic_cast<EDA_BASE_FRAME*>( m_top.data() ) )
            top->Close( false );
    }
}
