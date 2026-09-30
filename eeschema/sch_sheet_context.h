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

#ifndef SCH_SHEET_CONTEXT_H
#define SCH_SHEET_CONTEXT_H

class SCH_SHEET_PATH;

/**
 * Extra message-panel context offered by the schematic editor: the sheet being edited.
 *
 * Schematic items get it with dynamic_cast from the MSG_PANEL_CONTEXT they receive in
 * GetMsgPanelInfo(); other editors (symbol editor, viewers) do not provide it.
 */
class SCH_SHEET_CONTEXT
{
public:
    virtual ~SCH_SHEET_CONTEXT() = default;

    virtual SCH_SHEET_PATH& GetCurrentSheet() const = 0;
};

#endif // SCH_SHEET_CONTEXT_H
