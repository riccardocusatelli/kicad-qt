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
 * @file kiway_player.cpp
 * KIWAY_PLAYER of the Qt build.
 */

#include <kiway_player.h>

#include <QApplication>
#include <QEventLoop>

#include <kiway.h>
#include <kiway_mail.h>
#include <core/ki_assert.h>


KIWAY_PLAYER::KIWAY_PLAYER( KIWAY* aKiway, QWidget* aParent, FRAME_T aFrameType,
                            const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                            long aStyle, const wxString& aFrameName,
                            const EDA_IU_SCALE& aIuScale ) :
        EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName, aKiway,
                        aIuScale ),
        m_modal( false ),
        m_modal_loop( nullptr ),
        m_modal_resultant_parent( nullptr ),
        m_modal_ret_val( false )
{
}


KIWAY_PLAYER::~KIWAY_PLAYER() throw()
{
    // The KIWAY forgets the frame when it is deleted (it holds QPointers)
}


void KIWAY_PLAYER::KiwayMailIn( KIWAY_MAIL_EVENT& aEvent )
{
    // override this in derived classes.
}


bool KIWAY_PLAYER::ShowModal( wxString* aResult, QWidget* aResultantFocusWindow )
{
    KI_ASSERT_MSG( IsModal(), wxT( "ShowModal() shouldn't be called on non-modal frame" ) );

    m_modal_resultant_parent = aResultantFocusWindow;

    // The other windows of the application wait for this one (as a modal dialog)
    setWindowModality( Qt::ApplicationModal );
    show();
    raise();    // Needed on some Window managers to always display the frame
    activateWindow();

    {
        QEventLoop event_loop;
        m_modal_loop = &event_loop;
        event_loop.exec();
        m_modal_loop = nullptr;
    }

    setWindowModality( Qt::NonModal );

    if( aResult )
        *aResult = m_modal_string;

    if( aResultantFocusWindow )
    {
        aResultantFocusWindow->raise();
        aResultantFocusWindow->activateWindow();
        aResultantFocusWindow->setFocus();
    }

    return m_modal_ret_val;
}


bool KIWAY_PLAYER::IsDismissed()
{
    return !m_modal_loop;
}


void KIWAY_PLAYER::DismissModal( bool aRetVal, const wxString& aResult )
{
    m_modal_ret_val = aRetVal;
    m_modal_string  = aResult;

    if( m_modal_loop )
    {
        m_modal_loop->exit();
        m_modal_loop = nullptr;      // this marks it as dismissed.
    }

    hide();
}
