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


#pragma once


/**
 * A request to close a frame: the frame can refuse it (veto) unless the system forces it, for
 * example at the end of the session.
 *
 * The frames receive it in EDA_BASE_FRAME::canCloseWindow(); each GUI build makes it from the
 * close event of its toolkit.
 */
class CLOSE_REQUEST
{
public:
    enum class SOURCE
    {
        USER,               ///< The user, or the program, closes the frame
        SESSION_END_QUERY,  ///< The system asks if the session can end (the frame can refuse)
        SESSION_END         ///< The session ends
    };

    CLOSE_REQUEST( bool aCanVeto, SOURCE aSource = SOURCE::USER ) :
            m_canVeto( aCanVeto ),
            m_source( aSource )
    {}

    /// The close can be refused.
    bool CanVeto() const { return m_canVeto; }

    SOURCE GetSource() const { return m_source; }

    /// The system asks if the session can end.
    bool IsSessionEndQuery() const { return m_source == SOURCE::SESSION_END_QUERY; }

    /// Refuse the close.
    void Veto()
    {
        if( m_canVeto )
            m_vetoed = true;
    }

    bool IsVetoed() const { return m_vetoed; }

private:
    bool   m_canVeto;
    SOURCE m_source;
    bool   m_vetoed = false;
};
