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
 * @file net_inspector_panel.h
 * NET_INSPECTOR_PANEL of the Qt build: the base of the net inspectors of the editors, with its
 * search field, its configuration button and its list of nets (a QTreeView).  The wxWidgets build
 * has its own in pcbnew/widgets.
 */

#pragma once

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QWidget>

class BITMAP_BUTTON;
class EDA_BASE_FRAME;
class QLineEdit;
class QTreeView;


class NET_INSPECTOR_PANEL : public QWidget
{
public:
    NET_INSPECTOR_PANEL( QWidget* aParent, EDA_BASE_FRAME* aFrame );
    ~NET_INSPECTOR_PANEL() override;

    /**
     * Rebuild inspector data if project settings updated
     *
     * Called by the parent EDA_EDIT_FRAME on change of settings (e.g. stackup, netclass
     * definitions)
     */
    virtual void OnParentSetupChanged() {};

    /**
     * Save the net inspector settings - called from EDA_EDIT_FRAME when hiding the panel
     */
    virtual void SaveSettings() {};

    /**
     * Prepare the panel when (re-)shown in the editor
     */
    virtual void OnShowPanel() {}

    /**
     * Notification from file loader when board changed and connectivity rebuilt
     */
    virtual void OnBoardChanged() {}

protected:
    // User-driven UI events (override in derived classes as required)
    virtual void OnSearchTextChanged() {}
    virtual void OnConfigButton() {}

    /**
     * Implementation-specific implementation of language update handling
     */
    virtual void OnLanguageChangedImpl() {};

protected:
    EDA_BASE_FRAME* m_frame;
    QLineEdit*      m_searchCtrl;
    BITMAP_BUTTON*  m_configureBtn;
    QTreeView*      m_netsList;
};
