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
 * @file search_pane_tab.h
 * The tabs of the results of the search pane with Qt (see include/widgets/search_pane_tab.h).
 */

#ifndef SEARCH_PANE_TAB_H
#define SEARCH_PANE_TAB_H

#include <memory>
#include <vector>

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QTableView>
#include <QWidget>

#include <wx/string.h>

class SEARCH_HANDLER;
class SEARCH_RESULTS_MODEL;


/**
 * The table of the results of a search: the rows are the items found by the handler.
 */
class SEARCH_PANE_LISTVIEW : public QTableView
{
public:
    SEARCH_PANE_LISTVIEW( const std::shared_ptr<SEARCH_HANDLER>& aHandler, QWidget* aParent );

    virtual ~SEARCH_PANE_LISTVIEW();

    void RefreshColumnNames();

    /// Set the number of rows (the results of the last search).
    void SetItemCount( int aCount );

    std::vector<long> Sort();

protected:
    void keyPressEvent( QKeyEvent* aEvent ) override;
    void contextMenuEvent( QContextMenuEvent* aEvent ) override;

    void onColClicked( int aColumn );

    void GetSelectRowsList( std::vector<long>& aSelectedList );
    void CopySelectionToClipboard();

private:
    std::shared_ptr<SEARCH_HANDLER> m_handler;
    SEARCH_RESULTS_MODEL*           m_model;
    int                             m_sortCol;
    bool                            m_sortAscending;
    bool                            m_selectionDirty;
};


class SEARCH_PANE_TAB : public QWidget
{
public:
    SEARCH_PANE_TAB( const std::shared_ptr<SEARCH_HANDLER>& aHandler, QWidget* aParent );

    void Search( wxString& query );
    void Clear();
    void RefreshColumnNames();

    std::shared_ptr<SEARCH_HANDLER> GetSearchHandler() const { return m_handler; }

private:
    std::shared_ptr<SEARCH_HANDLER> m_handler;
    SEARCH_PANE_LISTVIEW*           m_listView;
};

#endif
