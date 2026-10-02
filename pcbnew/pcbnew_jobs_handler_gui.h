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

#ifndef PCBNEW_JOBS_HANDLER_GUI_H
#define PCBNEW_JOBS_HANDLER_GUI_H

#include "pcbnew_jobs_handler.h"

class KIWAY;
class PCB_EDIT_FRAME;


/**
 * The jobs handler used by the Pcbnew KIFACE: adds the job settings dialogs and makes the jobs
 * work on the board open in the editor rather than on the board file.
 */
class PCBNEW_JOBS_HANDLER_GUI : public PCBNEW_JOBS_HANDLER
{
public:
    PCBNEW_JOBS_HANDLER_GUI( KIWAY* aKiway );

protected:
    BOARD*        getEditorBoard() override;
    TOOL_MANAGER* getEditorToolManager() override;
    bool          getEditorNetlist( std::string& aNetlist ) override;

private:
    PCB_EDIT_FRAME* boardEditor() const;

    KIWAY* m_guiKiway;
};

#endif // PCBNEW_JOBS_HANDLER_GUI_H
