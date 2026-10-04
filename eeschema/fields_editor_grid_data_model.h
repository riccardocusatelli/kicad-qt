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

#pragma once

#include <fields_data_model.h>

#include <wx/grid.h>
#include <widgets/wx_grid.h>


// Columns for the View Fields grid
#define DISPLAY_NAME_COLUMN   0     // The field name in the data model (translated)
#define LABEL_COLUMN          1     // The field name's label for exporting (CSV, etc.)
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
#define VIEW_FIELDS_COL_COUNT 4


// Data model for the list of fields to view (and to group-by) for the Symbol Fields Table
class VIEW_CONTROLS_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    VIEW_CONTROLS_GRID_DATA_MODEL( bool aForBOM ) :
            m_forBOM( aForBOM )
    {}

    ~VIEW_CONTROLS_GRID_DATA_MODEL() override = default;

    int GetNumberRows() override { return (int) m_fields.size(); }
    int GetNumberCols() override { return VIEW_FIELDS_COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override;

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case DISPLAY_NAME_COLUMN:
        case LABEL_COLUMN:        return aTypeName == wxGRID_VALUE_STRING;

        case SHOW_FIELD_COLUMN:
        case GROUP_BY_COLUMN:     return aTypeName == wxGRID_VALUE_BOOL;

        default:                  KI_FAIL(); return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int aRow, int aCol ) override;
    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    void AppendRow( const wxString& aFieldName, const wxString& aBOMName, bool aShow, bool aGroupBy );
    void DeleteRow( int aRow );

    wxString GetCanonicalFieldName( int aRow );
    void SetCanonicalFieldName( int aRow, const wxString& aName );

protected:
    bool                   m_forBOM;
    std::vector<BOM_FIELD> m_fields;
};


/**
 * The FIELDS_DATA_MODEL of the symbol fields table dialog, shown in a WX_GRID.
 */
class FIELDS_EDITOR_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE, public FIELDS_DATA_MODEL
{
public:
    FIELDS_EDITOR_GRID_DATA_MODEL( const SCH_REFERENCE_LIST& aSymbolsList, wxGridCellAttr* aURLEditor ) :
            FIELDS_DATA_MODEL( aSymbolsList ),
            m_urlEditor( aURLEditor ),
            m_textVarRenderer( nullptr )
    {
    }

    ~FIELDS_EDITOR_GRID_DATA_MODEL() override
    {
        wxSafeDecRef( m_urlEditor );
        wxSafeDecRef( m_textVarRenderer );
    }

    using FIELDS_DATA_MODEL::GetValue;

    int GetNumberRows() override { return FIELDS_DATA_MODEL::GetNumberRows(); }
    int GetNumberCols() override { return FIELDS_DATA_MODEL::GetNumberCols(); }

    void SetColLabelValue( int aCol, const wxString& aLabel ) override
    {
        FIELDS_DATA_MODEL::SetColLabelValue( aCol, aLabel );
    }

    wxString GetColLabelValue( int aCol ) override
    {
        return FIELDS_DATA_MODEL::GetColLabelValue( aCol );
    }

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override
    {
        FIELDS_DATA_MODEL::SetValue( aRow, aCol, aValue );
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    bool IsExpanderColumn( int aCol ) const override
    {
        return FIELDS_DATA_MODEL::IsExpanderColumn( aCol );
    }

    GROUP_TYPE GetGroupType( int aRow ) const override
    {
        return FIELDS_DATA_MODEL::GetGroupType( aRow );
    }

    bool     HasUndoStateSerialization() const override { return true; }
    wxString SerializeUndoState() const override { return FIELDS_DATA_MODEL::SerializeUndoState(); }

    void RestoreUndoState( const wxString& aState ) override
    {
        FIELDS_DATA_MODEL::RestoreUndoState( aState );
    }

    bool DeleteRows( size_t aPosition = 0, size_t aNumRows = 1 ) override
    {
        return FIELDS_DATA_MODEL::DeleteRows( aPosition, aNumRows );
    }

    int GetDataWidth( int aCol );

protected:
    void rowsAppended( size_t aCount ) override;
    void rowsInserted( size_t aPos, size_t aCount ) override;
    void rowsDeleted( size_t aPos, size_t aCount ) override;
    void colsDeleted( size_t aPos, size_t aCount ) override;
    void commitPendingEdits() override;
    void dataChanged() override;

private:
    wxGridCellAttr*     m_urlEditor;
    wxGridCellRenderer* m_textVarRenderer; ///< Renderer for cells with text variable references
};
