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

#include <fields_editor_grid_data_model.h>

#include <wx/settings.h>

#include <sch_field.h>
#include <sch_symbol.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <widgets/ui_common.h>
#include <core/ki_assert.h>


/**      
 * Cell renderer that shows the expanded result of text variables (e.g. "${VALUE}" is
 * displayed as "10K").  The actual cell still stores the raw variable so it can be                                   
 * edited directly.                                                                                                   
 */
class GRID_CELL_RESOLVED_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_RESOLVED_TEXT_RENDERER() :
            wxGridCellStringRenderer()
    {
    }

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow, int aCol,
               bool isSelected ) override
    {
        wxString value = aGrid.GetCellValue( aRow, aCol );

        if( auto* model = dynamic_cast<FIELDS_EDITOR_GRID_DATA_MODEL*>( aGrid.GetTable() ) )
            value = model->GetResolvedValue( aRow, aCol );

        wxRect rect = aRect;
        rect.Inflate( -1 );

        wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );
        SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
        aGrid.DrawTextRectangle( aDC, value, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
    }

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow, int aCol ) override
    {
        wxString value = aGrid.GetCellValue( aRow, aCol );

        if( auto* model = dynamic_cast<FIELDS_EDITOR_GRID_DATA_MODEL*>( aGrid.GetTable() ) )
            value = model->GetResolvedValue( aRow, aCol );

        return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, value );
    }

    wxGridCellRenderer* Clone() const override { return new GRID_CELL_RESOLVED_TEXT_RENDERER(); }
};


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN: return _( "Field" );
    case LABEL_COLUMN:        return m_forBOM ? _( "BOM Name" ) : _( "Label" );
    case SHOW_FIELD_COLUMN:   return _( "Include" );
    case GROUP_BY_COLUMN:     return _( "Group By" );
    default:                  return wxT( "unknown column" );
    };
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    KI_CHECK( aRow < GetNumberRows(), wxT( "bad row!" ) );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        for( FIELD_T fieldId : MANDATORY_FIELDS )
        {
            if( GetDefaultFieldName( fieldId, !DO_TRANSLATE ) == rowData.name )
                return GetDefaultFieldName( fieldId, DO_TRANSLATE );
        }

        return rowData.name;

    case LABEL_COLUMN:
        return rowData.label;

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool VIEW_CONTROLS_GRID_DATA_MODEL::GetValueAsBool( int aRow, int aCol )
{
    KI_CHECK( aRow < GetNumberRows(), false );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: return rowData.show;
    case GROUP_BY_COLUMN:   return rowData.groupBy;

    default:
        KI_FAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString &aValue )
{
    KI_CHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        // Not editable
        break;

    case LABEL_COLUMN:
        rowData.label = aValue;
        break;

    default:
        KI_FAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
    }

    GetView()->Refresh();
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    KI_CHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: rowData.show = aValue;    break;
    case GROUP_BY_COLUMN:   rowData.groupBy = aValue; break;

    default:
        KI_FAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::AppendRow( const wxString& aFieldName, const wxString& aBOMName,
                                               bool aShow, bool aGroupBy )
{
    m_fields.emplace_back( BOM_FIELD{ aFieldName, aBOMName, aShow, aGroupBy } );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
        grid->ProcessTableMessage( msg );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::DeleteRow( int aRow )
{
    KI_CHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    m_fields.erase( m_fields.begin() + aRow );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
        grid->ProcessTableMessage( msg );
    }
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetCanonicalFieldName( int aRow )
{
    KI_CHECK( aRow >= 0 && aRow < GetNumberRows(), wxEmptyString );

    BOM_FIELD& rowData = m_fields[aRow];

    return rowData.name;
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetCanonicalFieldName( int aRow, const wxString& aName )
{
    KI_CHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    BOM_FIELD& rowData = m_fields[aRow];

    rowData.name = aName;
}


wxString FIELDS_EDITOR_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    GetView()->SetReadOnly( aRow, aCol, IsCellReadOnly( aRow, aCol ) );
    return FIELDS_DATA_MODEL::GetValue( aRow, aCol );
}


wxGridCellAttr* FIELDS_EDITOR_GRID_DATA_MODEL::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    wxGridCellAttr* attr = nullptr;
    bool            needsUrlEditor = false;
    bool            needsVariantHighlight = false;
    bool            needsTextVarRenderer = false;
    wxColour        highlightColor;

    // Check if we need URL editor
    if( GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET )
            || IsURL( GetValue( m_rows[aRow], aCol ) ) )
    {
        if( m_urlEditor )
            needsUrlEditor = true;
    }

    // Check if the raw value contains a text variable that should be resolved for display
    needsTextVarRenderer = CellHasTextVariable( aRow, aCol );

    // Check if we need variant highlighting
    if( CellDiffersFromDefault( aRow, aCol ) )
    {
        needsVariantHighlight = true;

        // Use a subtle highlight color that works in both light and dark themes
        wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
        bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

        if( isDark )
            highlightColor = wxColour( 80, 80, 40 );   // Dark gold/brown
        else
            highlightColor = wxColour( 255, 255, 200 ); // Light yellow
    }

    // If we don't need any custom attributes, use the base class behavior
    if( !needsUrlEditor && !needsVariantHighlight && !needsTextVarRenderer )
        return WX_GRID_TABLE_BASE::GetAttr( aRow, aCol, aKind );

    // URL cells: use m_urlEditor as base, potentially with variant highlight overlay
    if( needsUrlEditor )
    {
        if( needsVariantHighlight )
        {
            // Clone the URL editor attribute and add highlight color
            attr = m_urlEditor->Clone();
            attr->SetBackgroundColour( highlightColor );
        }
        else
        {
            // Just use the URL editor attribute directly
            m_urlEditor->IncRef();
            attr = m_urlEditor;
        }

        return enhanceAttr( attr, aRow, aCol, aKind );
    }

    // Non-URL cells: start with column attributes if they exist.
    // This preserves checkbox renderers and other column-specific settings.
    if( m_colAttrs.find( aCol ) != m_colAttrs.end() && m_colAttrs[aCol] )
    {
        attr = m_colAttrs[aCol]->Clone();
    }
    else
    {
        attr = new wxGridCellAttr();
    }

    if( needsVariantHighlight )
        attr->SetBackgroundColour( highlightColor );

    if( needsTextVarRenderer )
    {
        if( !m_textVarRenderer )
            m_textVarRenderer = new GRID_CELL_RESOLVED_TEXT_RENDERER();

        m_textVarRenderer->IncRef();
        attr->SetRenderer( m_textVarRenderer );

        // Tint text-var cells if not already highlighted by variant
        if( !needsVariantHighlight )
        {
            wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
            bool     isDark = ( bg.Red() + bg.Green() + bg.Blue() ) < 384;

            attr->SetBackgroundColour( isDark ? wxColour( 80, 70, 30 )       // Dark amber
                                              : wxColour( 255, 252, 200 ) ); // Light yellow
        }
    }

    return enhanceAttr( attr, aRow, aCol, aKind );
}


int FIELDS_EDITOR_GRID_DATA_MODEL::GetDataWidth( int aCol )
{
    int width = 0;

    for( const wxString& text : GetColumnTexts( aCol ) )
        width = std::max( width, KIUI::GetTextSize( text, GetView() ).x );

    return width;
}


void FIELDS_EDITOR_GRID_DATA_MODEL::rowsAppended( size_t aCount )
{
    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, aCount );
        grid->ProcessTableMessage( msg );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::rowsInserted( size_t aPos, size_t aCount )
{
    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, aPos, aCount );
        grid->ProcessTableMessage( msg );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::rowsDeleted( size_t aPos, size_t aCount )
{
    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPos, aCount );
        grid->ProcessTableMessage( msg );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::colsDeleted( size_t aPos, size_t aCount )
{
    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_COLS_DELETED, aPos, aCount );
        grid->ProcessTableMessage( msg );
    }
}


void FIELDS_EDITOR_GRID_DATA_MODEL::commitPendingEdits()
{
    if( WX_GRID* grid = static_cast<WX_GRID*>( GetView() ) )
        grid->CommitPendingChanges( true );
}


void FIELDS_EDITOR_GRID_DATA_MODEL::dataChanged()
{
    if( wxGrid* grid = GetView() )
        grid->ForceRefresh();
}
