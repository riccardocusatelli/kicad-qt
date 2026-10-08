/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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
 * @file panel_selection_filter.h
 * PANEL_SELECTION_FILTER of the Qt build: the check boxes of the item types the selection tool
 * selects (see pcbnew/widgets/panel_selection_filter.h).
 */

#ifndef KICAD_PANEL_SELECTION_FILTER_H
#define KICAD_PANEL_SELECTION_FILTER_H

#include <widgets/panel_selection_filter_base.h>

#include <map>

#include <QColor>
#include <QTimer>


class PCB_BASE_EDIT_FRAME;
class PCB_SELECTION_TOOL;
struct PCB_SELECTION_FILTER_OPTIONS;


class PANEL_SELECTION_FILTER : public PANEL_SELECTION_FILTER_BASE
{
public:
    PANEL_SELECTION_FILTER( QWidget* aParent );

    ~PANEL_SELECTION_FILTER() override;

    void SetCheckboxesFromFilter( PCB_SELECTION_FILTER_OPTIONS& aOptions );

    /// Flash the check boxes of the item types of @a aOptions (the types a selection skipped).
    void Flash( const PCB_SELECTION_FILTER_OPTIONS& aOptions );

protected:
    void OnFilterChanged( QWidget* aSource ) override;
    void OnLanguageChanged();

private:
    bool setFilterFromCheckboxes( PCB_SELECTION_FILTER_OPTIONS& aOptions );

    /// The menu of a check box: select only its item type.
    void onRightClick( QCheckBox* aCheckBox, const QPoint& aPos );

    void flashCheckbox( QCheckBox* aBox );
    void onFlashTimer();

    /// Show the flash of a check box (the highlight color blended by the step).
    void paintFlash( QCheckBox* aBox, int aStep );

private:
    PCB_BASE_EDIT_FRAME*                     m_frame;
    PCB_SELECTION_TOOL*                      m_tool;
    std::map<QCheckBox*, int>                m_flashCounters;
    QTimer                                   m_flashTimer;
    int                                      m_flashSteps;
};


#endif // KICAD_PANEL_SELECTION_FILTER_H
