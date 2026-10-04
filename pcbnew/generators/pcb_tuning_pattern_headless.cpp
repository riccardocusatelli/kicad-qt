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
 * @file pcb_tuning_pattern_headless.cpp
 *
 * The interactive editing of tuning patterns in the headless kiface (kicad-cli).
 *
 * The editing is in pcb_tuning_pattern_edit.cpp, which needs the board editor (the tools, the
 * router, the dialogs).  The headless kiface never edits a tuning pattern interactively: it is
 * linked with these definitions instead, so that loading, plotting and checking boards with
 * tuning patterns does not bring in the editor.
 *
 * EditStart() is the key function of PCB_TUNING_PATTERN: the vtable is emitted here, and it
 * brings in pcb_tuning_pattern.cpp, which registers the generator.
 */

#include <generators/pcb_tuning_pattern.h>


void PCB_TUNING_PATTERN::EditStart( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
}


bool PCB_TUNING_PATTERN::Update( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
    return false;
}


void PCB_TUNING_PATTERN::EditFinish( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
}


void PCB_TUNING_PATTERN::EditCancel( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
}


void PCB_TUNING_PATTERN::Remove( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* )
{
}


bool PCB_TUNING_PATTERN::MakeEditPoints( EDIT_POINTS& ) const
{
    return false;
}


bool PCB_TUNING_PATTERN::UpdateFromEditPoints( EDIT_POINTS& )
{
    return false;
}


bool PCB_TUNING_PATTERN::UpdateEditPoints( EDIT_POINTS& )
{
    return false;
}


void PCB_TUNING_PATTERN::ViewDraw( int, KIGFX::VIEW* ) const
{
}


void PCB_TUNING_PATTERN::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* )
{
}


std::vector<EDA_ITEM*> PCB_TUNING_PATTERN::GetPreviewItems( GENERATOR_TOOL*, PCB_BASE_EDIT_FRAME*,
                                                            bool )
{
    return {};
}
