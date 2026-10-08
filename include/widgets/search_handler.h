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


#ifndef SEARCH_HANDLER_H
#define SEARCH_HANDLER_H

#include <tuple>
#include <vector>

#include <wx/string.h>


/// The alignment of the cells of a column of the search results.
enum class SEARCH_COLUMN_ALIGN
{
    LEFT,
    RIGHT,
    CENTER
};


/**
 * A search of the search pane: the items matching a query, as the rows of a table.
 */
class SEARCH_HANDLER
{
public:
    SEARCH_HANDLER( const wxString& aName ) :
            m_name( aName )
    {}

    virtual ~SEARCH_HANDLER()
    {}

    wxString GetName() const { return m_name; }

    /// The columns of the results: their names, their proportions (in tenths of the width of
    /// the pane) and the alignments of their cells.
    std::vector<std::tuple<wxString, int, SEARCH_COLUMN_ALIGN>> GetColumns() const
    {
        return m_columns;
    }

    virtual int Search( const wxString& string ) = 0;
    virtual wxString GetResultCell( int row, int col ) = 0;
    virtual void Sort( int aCol, bool aAscending, std::vector<long>* aSelection ) = 0;

    virtual void SelectItems( std::vector<long>& aItemRows ) {}
    virtual void ActivateItem( long aItemRow ) {}

protected:
    wxString                                                    m_name;
    std::vector<std::tuple<wxString, int, SEARCH_COLUMN_ALIGN>> m_columns;
};

#endif // SEARCH_HANDLER_H
