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

#ifndef MSG_PANEL_CONTEXT_H
#define MSG_PANEL_CONTEXT_H

#include <frame_type.h>
#include <units_provider.h>

class EDA_ITEM;
class KIID;


/**
 * What an item needs from the editor to describe itself in the message panel
 * (EDA_ITEM::GetMsgPanelInfo()): units and origin transforms, which editor is asking, text
 * ellipsizing for the panel width and item lookup.
 *
 * Items used to receive the EDA_DRAW_FRAME itself, which made every model item depend on the
 * frame and therefore on the GUI toolkit.  Frames provide an implementation of this interface
 * instead (EDA_DRAW_FRAME::GetMsgPanelContext()); headless code can provide its own.
 */
class MSG_PANEL_CONTEXT : public UNITS_PROVIDER
{
public:
    MSG_PANEL_CONTEXT( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
            UNITS_PROVIDER( aIuScale, aUnits )
    {}

    virtual ~MSG_PANEL_CONTEXT() = default;

    /// The kind of editor displaying the item.
    virtual FRAME_T GetFrameType() const = 0;

    bool IsType( FRAME_T aType ) const { return GetFrameType() == aType; }

    /// Shorten @a aText to fit the message panel (as KIUI::EllipsizeStatusText() does).
    virtual wxString EllipsizeStatusText( const wxString& aText ) const = 0;

    /// Find an item of the current document by UUID.
    virtual EDA_ITEM* ResolveItem( const KIID& aId, bool aAllowNullptrReturn = false ) const = 0;
};

#endif // MSG_PANEL_CONTEXT_H
