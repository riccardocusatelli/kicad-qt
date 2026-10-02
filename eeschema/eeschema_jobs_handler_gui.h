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

#ifndef EESCHEMA_JOBS_HANDLER_GUI_H
#define EESCHEMA_JOBS_HANDLER_GUI_H

#include "eeschema_jobs_handler.h"

class KIWAY;
class SCH_EDIT_FRAME;


/**
 * The jobs handler used by the Eeschema KIFACE: adds the job settings dialogs and makes the
 * jobs work on the schematic open in the editor rather than on the schematic file.
 */
class EESCHEMA_JOBS_HANDLER_GUI : public EESCHEMA_JOBS_HANDLER
{
public:
    EESCHEMA_JOBS_HANDLER_GUI( KIWAY* aKiway );

protected:
    SCHEMATIC* getEditorSchematic() override;

private:
    SCH_EDIT_FRAME* schematicEditor() const;

    KIWAY* m_guiKiway;
};

#endif // EESCHEMA_JOBS_HANDLER_GUI_H
