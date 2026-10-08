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
 * @file search_pane.h
 * SEARCH_PANE of the Qt build: the search of the items of the editors, a tab of results for
 * each SEARCH_HANDLER (see include/widgets/search_pane.h).
 */

#ifndef SEARCH_PANE_H
#define SEARCH_PANE_H

#include <memory>
#include <vector>

#include <widgets/search_handler.h>
#include <widgets/search_pane_base.h>


class ACTION_MENU;
class EDA_DRAW_FRAME;
class SEARCH_PANE_TAB;


class SEARCH_PANE : public SEARCH_PANE_BASE
{
public:
    SEARCH_PANE( EDA_DRAW_FRAME* aFrame );
    virtual ~SEARCH_PANE();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    SEARCH_PANE( const SEARCH_PANE& ) = delete;
    SEARCH_PANE& operator=( const SEARCH_PANE& ) = delete;

    void AddSearcher( const std::shared_ptr<SEARCH_HANDLER>& aHandler );
    void OnSearchTextEntry( QWidget* aSource ) override;
    void OnNotebookPageChanged( int aPage ) override;

    void RefreshSearch();
    void FocusSearch();
    void ClearAllResults();

    /// @return true if the pane is visible on the screen (shown, in a shown dock and not
    ///         behind another tab of the dock).
    bool IsShownOnScreen() const;

protected:
    void             onLanguageChange();
    SEARCH_PANE_TAB* GetCurrentTab() const;

    /// The pane was closed by the user: save the settings of the frame.
    void             onClosed();

    void showEvent( QShowEvent* aEvent ) override;

private:
    std::vector<std::shared_ptr<SEARCH_HANDLER>> m_handlers;
    std::vector<SEARCH_PANE_TAB*>                m_tabs;
    wxString                                     m_lastQuery;
    EDA_DRAW_FRAME*                              m_frame;
    ACTION_MENU*                                 m_menu;
    bool                                         m_closeWatched;
};

#endif
