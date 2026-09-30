/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011-2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file msgpanel_item.h
 * One line of the message panel.  Model items build these in GetMsgPanelInfo(); only the panel
 * itself (msgpanel.h) needs wx GUI headers.
 */

#pragma once

#include <optional>

#include <wx/string.h>

class EDA_MSG_PANEL;
class KIID;

#define MSG_PANEL_DEFAULT_PAD      6  ///< The default number of spaces between each text string.


/**
 * #EDA_MSG_PANEL items for displaying messages.
 */
class MSG_PANEL_ITEM
{
public:
    MSG_PANEL_ITEM( const wxString& aUpperText, const wxString& aLowerText,
                    int aPadding = MSG_PANEL_DEFAULT_PAD ) :
            m_UpperText( aUpperText ),
            m_LowerText( aLowerText ),
            m_Padding( aPadding )
    {
        m_X = 0;
        m_UpperY = 0;
        m_LowerY = 0;
    }

    MSG_PANEL_ITEM() :
            m_Padding( MSG_PANEL_DEFAULT_PAD )

    {
        m_X = 0;
        m_UpperY = 0;
        m_LowerY = 0;
    }

    void SetUpperText( const wxString& aUpperText ) { m_UpperText = aUpperText; }
    const wxString& GetUpperText() const { return m_UpperText; }

    void SetLowerText( const wxString& aLowerText )  { m_LowerText = aLowerText; }
    const wxString& GetLowerText() const { return m_LowerText; }

    void SetPadding( int aPadding )  { m_Padding = aPadding; }
    int GetPadding() const { return m_Padding; }

private:
    friend class EDA_MSG_PANEL;

    int         m_X;
    int         m_UpperY;
    int         m_LowerY;
    wxString    m_UpperText;
    wxString    m_LowerText;
    int         m_Padding;
};


/**
 * Get a formatted UUID string for display in the message panel,
 * according to the current advanced configuration setting.
 *
 * This will be std::nullopt if the configuration setting disables UUID display.
 */
std::optional<wxString> GetMsgPanelDisplayUuid( const KIID& aKiid );
