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

#ifndef PROPERTY_CHOICES_H
#define PROPERTY_CHOICES_H

#include <algorithm>
#include <climits>
#include <vector>

#include <wx/arrstr.h>
#include <wx/dynarray.h>
#include <wx/string.h>


/**
 * One entry of a PROPERTY_CHOICES list.
 */
class PROPERTY_CHOICE
{
public:
    PROPERTY_CHOICE( const wxString& aText, int aValue ) :
            m_text( aText ),
            m_value( aValue )
    {}

    const wxString& GetText() const { return m_text; }
    void            SetText( const wxString& aText ) { m_text = aText; }
    int             GetValue() const { return m_value; }
    void            SetValue( int aValue ) { m_value = aValue; }

private:
    wxString m_text;
    int      m_value;
};


/**
 * The limited set of values an enum-like property can take: labels with integer values.
 *
 * This replaces wxPGChoices in the property system, so that model items (which declare their
 * properties) do not depend on wxPropertyGrid, a GUI library.  The API is the subset of
 * wxPGChoices that KiCad uses, with the same semantics: Add() without a value assigns the
 * entry's index; Index() returns -1 when not found.  Unlike wxPGChoices it has plain value
 * semantics (copies do not share data).
 *
 * The property panels convert it with ToWxPGChoices() (properties/pg_properties.h).
 */
class PROPERTY_CHOICES
{
public:
    /// Same sentinel as wxPG_INVALID_VALUE: "use the entry index as value".
    static constexpr int INVALID_VALUE = INT_MAX;

    PROPERTY_CHOICES() = default;

    PROPERTY_CHOICE& Add( const wxString& aLabel, int aValue = INVALID_VALUE )
    {
        m_entries.emplace_back( aLabel, aValue == INVALID_VALUE ? static_cast<int>( m_entries.size() )
                                                                : aValue );
        return m_entries.back();
    }

    PROPERTY_CHOICE& Insert( const wxString& aLabel, int aIndex, int aValue = INVALID_VALUE )
    {
        size_t pos = ( aIndex < 0 || static_cast<size_t>( aIndex ) > m_entries.size() )
                             ? m_entries.size()
                             : static_cast<size_t>( aIndex );

        auto it = m_entries.emplace( m_entries.begin() + pos, aLabel,
                                     aValue == INVALID_VALUE ? static_cast<int>( pos ) : aValue );
        return *it;
    }

    void Clear() { m_entries.clear(); }

    void RemoveAt( size_t aIndex, size_t aCount = 1 )
    {
        if( aIndex >= m_entries.size() )
            return;

        size_t last = std::min( m_entries.size(), aIndex + aCount );
        m_entries.erase( m_entries.begin() + aIndex, m_entries.begin() + last );
    }

    unsigned int GetCount() const { return static_cast<unsigned int>( m_entries.size() ); }
    bool         IsEmpty() const { return m_entries.empty(); }

    const wxString& GetLabel( unsigned int aIndex ) const { return m_entries.at( aIndex ).GetText(); }
    int             GetValue( unsigned int aIndex ) const { return m_entries.at( aIndex ).GetValue(); }

    /// Index of the entry with label @a aLabel, or -1.
    int Index( const wxString& aLabel ) const
    {
        for( size_t i = 0; i < m_entries.size(); ++i )
        {
            if( m_entries[i].GetText() == aLabel )
                return static_cast<int>( i );
        }

        return -1;
    }

    /// Index of the entry with value @a aValue, or -1.
    int Index( int aValue ) const
    {
        for( size_t i = 0; i < m_entries.size(); ++i )
        {
            if( m_entries[i].GetValue() == aValue )
                return static_cast<int>( i );
        }

        return -1;
    }

    wxArrayString GetLabels() const
    {
        wxArrayString labels;

        for( const PROPERTY_CHOICE& entry : m_entries )
            labels.Add( entry.GetText() );

        return labels;
    }

    /// Values of the entries whose labels are in @a aStrings (unknown labels are skipped).
    wxArrayInt GetValuesForStrings( const wxArrayString& aStrings ) const
    {
        wxArrayInt values;

        for( const wxString& str : aStrings )
        {
            int idx = Index( str );

            if( idx >= 0 )
                values.Add( m_entries[idx].GetValue() );
        }

        return values;
    }

    PROPERTY_CHOICE&       operator[]( unsigned int aIndex ) { return m_entries.at( aIndex ); }
    const PROPERTY_CHOICE& operator[]( unsigned int aIndex ) const { return m_entries.at( aIndex ); }

    auto begin() const { return m_entries.begin(); }
    auto end() const { return m_entries.end(); }

private:
    std::vector<PROPERTY_CHOICE> m_entries;
};

#endif // PROPERTY_CHOICES_H
