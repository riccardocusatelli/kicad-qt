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
 * @file msgpanel.h
 * EDA_MSG_PANEL of the Qt build: the panel of messages at the bottom of the frames.
 */

#pragma once

#include <optional>
#include <vector>

#include <kiqt.h>
#include <QWidget>

#include <gal/color4d.h>
#include <widgets/msgpanel_item.h>

using KIGFX::COLOR4D;


/**
 * A panel to display various information messages: pairs of texts (an upper one and a
 * lower one) side by side.
 */
class KIQT_API EDA_MSG_PANEL : public QWidget
{
public:
    EDA_MSG_PANEL( QWidget* aParent );
    ~EDA_MSG_PANEL();

    /**
     * Clear all messages from the message panel.
     */
    void EraseMsgBox();

    /**
     * Append a message to the message panel.
     *
     * This method automatically adjusts for the width of the text string.
     * Making consecutive calls to AppendMessage will append each message
     * to the right of the last message.  This message is not compatible
     * with Affiche_1_Parametre.
     *
     * @param aUpperText The text to be displayed in top line.
     * @param aLowerText The text to be displayed in bottom line.
     * @param aPadding Number of spaces to pad between messages (default = 6).
     */
    void AppendMessage( const wxString& aUpperText, const wxString& aLowerText, int aPadding = 6 );

    /**
     * Append \a aMessageItem to the message panel.
     *
     * @param aMessageItem is a reference to an #MSG_PANEL_ITEM containing the message to
     *                     append to the panel.
     */
    void AppendMessage( const MSG_PANEL_ITEM& aMessageItem )
    {
        AppendMessage( aMessageItem.GetUpperText(), aMessageItem.GetLowerText(),
                       aMessageItem.GetPadding() );
    }

    /// The messages shown.
    const std::vector<MSG_PANEL_ITEM>& GetItems() const { return m_Items; }

    QSize sizeHint() const override;

protected:
    void paintEvent( QPaintEvent* aEvent ) override;

protected:
    std::vector<MSG_PANEL_ITEM> m_Items;
};
