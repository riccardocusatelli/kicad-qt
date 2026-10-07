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
 * @file kiway_mail.h
 * KIWAY_MAIL_EVENT of the Qt build: the mail between the KIWAY_PLAYERs (a message, not an
 * event of the GUI toolkit).
 */

#ifndef KIWAY_MAIL_H
#define KIWAY_MAIL_H

#include <string>

#include <frame_type.h>
#include <mail_type.h>
#include <kicommon.h>
#include <toolkit/toolkit_types.h>


/**
 * Carry a payload from one #KIWAY_PLAYER to another within a #PROJECT.
 */
class KICOMMON_API KIWAY_MAIL_EVENT
{
public:
    /**
     * Return the destination player id of the message.
     */
    FRAME_T Dest() { return m_destination; }

    /**
     * Returns the #MAIL_T associated with this mail.
     */
    MAIL_T Command() { return m_command; }

    /**
     * Return the payload, which can be any text but it typically self identifying s-expression.
     */
    std::string&  GetPayload() { return m_payload; }
    void SetPayload( const std::string& aPayload ) { m_payload = aPayload; }

    /// The window that sent the mail, if any.
    TOOLKIT_WINDOW* GetSource() const { return m_source; }

    KIWAY_MAIL_EVENT( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                      TOOLKIT_WINDOW* aSource = nullptr ) :
            m_destination( aDestination ),
            m_command( aCommand ),
            m_payload( aPayload ),
            m_source( aSource )
    {
    }

private:
    FRAME_T         m_destination;    ///< could have been a bitmap indicating multiple recipients.
    MAIL_T          m_command;
    std::string&    m_payload;        ///< very often s-expression text, but not always.
    TOOLKIT_WINDOW* m_source;
};

#endif  // KIWAY_MAIL_H
