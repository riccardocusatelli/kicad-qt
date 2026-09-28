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

#ifndef PANEL_RECENT_PROJECTS_H
#define PANEL_RECENT_PROJECTS_H

#include <wx/panel.h>
#include <wx/datetime.h>

class KICAD_MANAGER_FRAME;
class wxScrolledWindow;
class wxStaticText;
class wxBoxSizer;


/**
 * One clickable row of the recent projects list.  Drawn entirely in OnPaint so the whole card
 * reacts to hover and click (child controls would swallow the mouse events).
 */
class RECENT_PROJECT_ITEM : public wxPanel
{
public:
    RECENT_PROJECT_ITEM( wxWindow* aParent, KICAD_MANAGER_FRAME* aFrame, const wxString& aProjectFile );

    bool Exists() const { return m_exists; }

private:
    void onPaint( wxPaintEvent& aEvent );
    void onEnter( wxMouseEvent& aEvent );
    void onLeave( wxMouseEvent& aEvent );
    void onClick( wxMouseEvent& aEvent );

    KICAD_MANAGER_FRAME* m_frame;
    wxString             m_projectFile;
    wxString             m_name;
    wxString             m_folder;
    wxDateTime           m_modified;
    bool                 m_exists;
    bool                 m_hasSchematic;
    bool                 m_hasBoard;
    bool                 m_hover;
};


/**
 * Home column shown in place of the project file tree when no project is open: lists the
 * project file history as cards (name, folder, last modification, which documents exist).
 */
class PANEL_RECENT_PROJECTS : public wxPanel
{
public:
    PANEL_RECENT_PROJECTS( KICAD_MANAGER_FRAME* aFrame, wxWindow* aParent );

    /// Rebuild the list from the frame's file history.
    void RebuildList();

    /// Human readable "2 hours ago" style string.
    static wxString FormatRelativeTime( const wxDateTime& aTime );

private:
    void onThemeChanged( wxSysColourChangedEvent& aEvent );

    KICAD_MANAGER_FRAME* m_frame;
    wxScrolledWindow*    m_list;
    wxBoxSizer*          m_listSizer;
    wxStaticText*        m_footer;
};

#endif // PANEL_RECENT_PROJECTS_H
