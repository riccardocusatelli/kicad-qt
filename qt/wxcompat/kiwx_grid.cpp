/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file kiwx_grid.cpp
 * The grids of the wxWidgets compatibility layer of the Qt build.
 */

#include <wx/kiwx_grid.h>

#include <QAbstractTableModel>
#include <QAbstractButton>
#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QScrollBar>
#include <QStyledItemDelegate>

#include <qt_strings.h>


// Attributes -------------------------------------------------------------------------------

wxGridCellAttr::wxGridCellAttr( wxGridCellAttr* aDefault ) :
        m_defAttr( aDefault ),
        m_hasFont( false ),
        m_hAlign( -1 ),
        m_vAlign( -1 ),
        m_overflow( true ),
        m_readMode( Unset ),
        m_renderer( nullptr ),
        m_editor( nullptr ),
        m_kind( Cell )
{
}


wxGridCellAttr::~wxGridCellAttr()
{
    wxSafeDecRef( m_renderer );
    wxSafeDecRef( m_editor );
}


wxGridCellAttr* wxGridCellAttr::Clone() const
{
    wxGridCellAttr* attr = new wxGridCellAttr( m_defAttr );

    attr->m_textColour = m_textColour;
    attr->m_backColour = m_backColour;
    attr->m_font = m_font;
    attr->m_hasFont = m_hasFont;
    attr->m_hAlign = m_hAlign;
    attr->m_vAlign = m_vAlign;
    attr->m_overflow = m_overflow;
    attr->m_readMode = m_readMode;
    attr->m_kind = m_kind;

    if( m_renderer )
    {
        m_renderer->IncRef();
        attr->m_renderer = m_renderer;
    }

    if( m_editor )
    {
        m_editor->IncRef();
        attr->m_editor = m_editor;
    }

    return attr;
}


void wxGridCellAttr::MergeWith( wxGridCellAttr* aOther )
{
    if( !aOther )
        return;

    if( aOther->HasTextColour() )
        m_textColour = aOther->m_textColour;

    if( aOther->HasBackgroundColour() )
        m_backColour = aOther->m_backColour;

    if( aOther->HasFont() )
        SetFont( aOther->m_font );

    if( aOther->m_hAlign != -1 )
        m_hAlign = aOther->m_hAlign;

    if( aOther->m_vAlign != -1 )
        m_vAlign = aOther->m_vAlign;

    if( aOther->HasReadWriteMode() )
        m_readMode = aOther->m_readMode;

    if( aOther->m_renderer )
    {
        aOther->m_renderer->IncRef();
        SetRenderer( aOther->m_renderer );
    }

    if( aOther->m_editor )
    {
        aOther->m_editor->IncRef();
        SetEditor( aOther->m_editor );
    }
}


void wxGridCellAttr::SetRenderer( wxGridCellRenderer* aRenderer )
{
    wxSafeDecRef( m_renderer );
    m_renderer = aRenderer;
}


void wxGridCellAttr::SetEditor( wxGridCellEditor* aEditor )
{
    wxSafeDecRef( m_editor );
    m_editor = aEditor;
}


wxColour wxGridCellAttr::GetTextColour() const
{
    if( HasTextColour() )
        return m_textColour;

    if( m_defAttr && m_defAttr != this )
        return m_defAttr->GetTextColour();

    return QApplication::palette().color( QPalette::Text );
}


wxColour wxGridCellAttr::GetBackgroundColour() const
{
    if( HasBackgroundColour() )
        return m_backColour;

    if( m_defAttr && m_defAttr != this )
        return m_defAttr->GetBackgroundColour();

    return QApplication::palette().color( QPalette::Base );
}


wxFont wxGridCellAttr::GetFont() const
{
    if( HasFont() )
        return m_font;

    if( m_defAttr && m_defAttr != this )
        return m_defAttr->GetFont();

    return QApplication::font();
}


void wxGridCellAttr::GetAlignment( int* aHAlign, int* aVAlign ) const
{
    int h = m_hAlign;
    int v = m_vAlign;

    if( ( h == -1 || v == -1 ) && m_defAttr && m_defAttr != this )
    {
        int defH, defV;
        m_defAttr->GetAlignment( &defH, &defV );

        if( h == -1 )
            h = defH;

        if( v == -1 )
            v = defV;
    }

    if( aHAlign )
        *aHAlign = h == -1 ? wxALIGN_LEFT : h;

    if( aVAlign )
        *aVAlign = v == -1 ? wxALIGN_CENTER_VERTICAL : v;
}


wxGridCellRenderer* wxGridCellAttr::GetRenderer( const wxGrid* aGrid, int aRow, int aCol ) const
{
    if( m_renderer )
    {
        m_renderer->IncRef();
        return m_renderer;
    }

    if( m_defAttr && m_defAttr != this )
        return m_defAttr->GetRenderer( aGrid, aRow, aCol );

    return aGrid ? aGrid->GetDefaultRendererForCell( aRow, aCol ) : new wxGridCellStringRenderer;
}


wxGridCellEditor* wxGridCellAttr::GetEditor( const wxGrid* aGrid, int aRow, int aCol ) const
{
    if( m_editor )
    {
        m_editor->IncRef();
        return m_editor;
    }

    if( m_defAttr && m_defAttr != this )
        return m_defAttr->GetEditor( aGrid, aRow, aCol );

    return aGrid ? aGrid->GetDefaultEditorForCell( aRow, aCol ) : new wxGridCellTextEditor;
}


wxGridCellAttrProvider::~wxGridCellAttrProvider()
{
    for( auto& [key, attr] : m_cellAttrs )
        wxSafeDecRef( attr );

    for( auto& [key, attr] : m_rowAttrs )
        wxSafeDecRef( attr );

    for( auto& [key, attr] : m_colAttrs )
        wxSafeDecRef( attr );
}


wxGridCellAttr* wxGridCellAttrProvider::GetAttr( int aRow, int aCol,
                                                 wxGridCellAttr::wxAttrKind aKind ) const
{
    auto find = [&]( auto& aMap, auto aKey ) -> wxGridCellAttr*
                {
                    auto it = aMap.find( aKey );
                    return it != aMap.end() ? it->second : nullptr;
                };

    wxGridCellAttr* cell = find( m_cellAttrs, std::make_pair( aRow, aCol ) );
    wxGridCellAttr* row = find( m_rowAttrs, aRow );
    wxGridCellAttr* col = find( m_colAttrs, aCol );

    switch( aKind )
    {
    case wxGridCellAttr::Cell: if( cell ) cell->IncRef(); return cell;
    case wxGridCellAttr::Row:  if( row ) row->IncRef();   return row;
    case wxGridCellAttr::Col:  if( col ) col->IncRef();   return col;
    default:                   break;
    }

    // Any: the merged attributes (the ones of the cell over the row's over the column's)
    int count = ( cell ? 1 : 0 ) + ( row ? 1 : 0 ) + ( col ? 1 : 0 );

    if( count == 0 )
        return nullptr;

    if( count == 1 )
    {
        wxGridCellAttr* attr = cell ? cell : row ? row : col;
        attr->IncRef();
        return attr;
    }

    wxGridCellAttr* merged = new wxGridCellAttr;
    merged->MergeWith( col );
    merged->MergeWith( row );
    merged->MergeWith( cell );
    merged->SetKind( wxGridCellAttr::Merged );
    return merged;
}


void wxGridCellAttrProvider::SetAttr( wxGridCellAttr* aAttr, int aRow, int aCol )
{
    auto key = std::make_pair( aRow, aCol );
    wxSafeDecRef( m_cellAttrs[key] );

    if( aAttr )
        m_cellAttrs[key] = aAttr;
    else
        m_cellAttrs.erase( key );
}


void wxGridCellAttrProvider::SetRowAttr( wxGridCellAttr* aAttr, int aRow )
{
    wxSafeDecRef( m_rowAttrs[aRow] );

    if( aAttr )
        m_rowAttrs[aRow] = aAttr;
    else
        m_rowAttrs.erase( aRow );
}


void wxGridCellAttrProvider::SetColAttr( wxGridCellAttr* aAttr, int aCol )
{
    wxSafeDecRef( m_colAttrs[aCol] );

    if( aAttr )
        m_colAttrs[aCol] = aAttr;
    else
        m_colAttrs.erase( aCol );
}


void wxGridCellAttrProvider::UpdateAttrRows( size_t aPos, int aNumRows )
{
    std::map<std::pair<int, int>, wxGridCellAttr*> cells;

    for( auto& [key, attr] : m_cellAttrs )
    {
        int row = key.first;

        if( row >= int( aPos ) )
        {
            if( aNumRows < 0 && row < int( aPos ) - aNumRows )
            {
                wxSafeDecRef( attr );
                continue;
            }

            row += aNumRows;
        }

        cells[std::make_pair( row, key.second )] = attr;
    }

    m_cellAttrs = std::move( cells );

    std::map<int, wxGridCellAttr*> rows;

    for( auto& [row, attr] : m_rowAttrs )
    {
        int newRow = row;

        if( row >= int( aPos ) )
        {
            if( aNumRows < 0 && row < int( aPos ) - aNumRows )
            {
                wxSafeDecRef( attr );
                continue;
            }

            newRow += aNumRows;
        }

        rows[newRow] = attr;
    }

    m_rowAttrs = std::move( rows );
}


void wxGridCellAttrProvider::UpdateAttrCols( size_t aPos, int aNumCols )
{
    std::map<int, wxGridCellAttr*> cols;

    for( auto& [col, attr] : m_colAttrs )
    {
        int newCol = col;

        if( col >= int( aPos ) )
        {
            if( aNumCols < 0 && col < int( aPos ) - aNumCols )
            {
                wxSafeDecRef( attr );
                continue;
            }

            newCol += aNumCols;
        }

        cols[newCol] = attr;
    }

    m_colAttrs = std::move( cols );
}


// Tables -----------------------------------------------------------------------------------

wxGridTableBase::wxGridTableBase() :
        m_view( nullptr )
{
}


wxGridTableBase::~wxGridTableBase()
{
}


bool wxGridTableBase::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return aTypeName == wxGRID_VALUE_STRING;
}


bool wxGridTableBase::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


long wxGridTableBase::GetValueAsLong( int aRow, int aCol )
{
    long value = 0;
    GetValue( aRow, aCol ).ToLong( &value );
    return value;
}


double wxGridTableBase::GetValueAsDouble( int aRow, int aCol )
{
    double value = 0;
    GetValue( aRow, aCol ).ToDouble( &value );
    return value;
}


bool wxGridTableBase::GetValueAsBool( int aRow, int aCol )
{
    return wxGridCellBoolEditor::IsTrueValue( GetValue( aRow, aCol ) );
}


void wxGridTableBase::SetValueAsLong( int aRow, int aCol, long aValue )
{
    SetValue( aRow, aCol, wxString::Format( wxT( "%ld" ), aValue ) );
}


void wxGridTableBase::SetValueAsDouble( int aRow, int aCol, double aValue )
{
    SetValue( aRow, aCol, wxString::Format( wxT( "%g" ), aValue ) );
}


void wxGridTableBase::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    SetValue( aRow, aCol, aValue ? wxString( wxS( "1" ) ) : wxString() );
}


wxString wxGridTableBase::GetRowLabelValue( int aRow )
{
    return wxString::Format( wxT( "%d" ), aRow + 1 );
}


wxString wxGridTableBase::GetColLabelValue( int aCol )
{
    // A, B, ..., Z, AA, AB...
    wxString label;
    int      n = aCol;

    do
    {
        label.Prepend( wxUniChar( 'A' + n % 26 ) );
        n = n / 26 - 1;
    } while( n >= 0 );

    return label;
}


void wxGridTableBase::SetAttrProvider( wxGridCellAttrProvider* aProvider )
{
    m_attrProvider.reset( aProvider );
}


bool wxGridTableBase::CanHaveAttributes()
{
    if( !m_attrProvider )
        m_attrProvider = std::make_unique<wxGridCellAttrProvider>();

    return true;
}


wxGridCellAttr* wxGridTableBase::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    return m_attrProvider ? m_attrProvider->GetAttr( aRow, aCol, aKind ) : nullptr;
}


void wxGridTableBase::SetAttr( wxGridCellAttr* aAttr, int aRow, int aCol )
{
    if( CanHaveAttributes() )
        m_attrProvider->SetAttr( aAttr, aRow, aCol );
    else
        wxSafeDecRef( aAttr );
}


void wxGridTableBase::SetRowAttr( wxGridCellAttr* aAttr, int aRow )
{
    if( CanHaveAttributes() )
        m_attrProvider->SetRowAttr( aAttr, aRow );
    else
        wxSafeDecRef( aAttr );
}


void wxGridTableBase::SetColAttr( wxGridCellAttr* aAttr, int aCol )
{
    if( CanHaveAttributes() )
        m_attrProvider->SetColAttr( aAttr, aCol );
    else
        wxSafeDecRef( aAttr );
}


wxGridStringTable::wxGridStringTable( int aNumRows, int aNumCols ) :
        m_data( std::max( aNumRows, 0 ), std::vector<wxString>( std::max( aNumCols, 0 ) ) ),
        m_numCols( std::max( aNumCols, 0 ) )
{
}


wxString wxGridStringTable::GetValue( int aRow, int aCol )
{
    if( aRow < 0 || aRow >= int( m_data.size() ) || aCol < 0 || aCol >= m_numCols )
        return wxString();

    return m_data[aRow][aCol];
}


void wxGridStringTable::SetValue( int aRow, int aCol, const wxString& aValue )
{
    if( aRow < 0 || aRow >= int( m_data.size() ) || aCol < 0 || aCol >= m_numCols )
        return;

    m_data[aRow][aCol] = aValue;
}


void wxGridStringTable::Clear()
{
    for( std::vector<wxString>& row : m_data )
        std::fill( row.begin(), row.end(), wxString() );
}


static void notifyView( wxGridTableBase* aTable, int aId, int aInt1, int aInt2 = -1 )
{
    if( wxGrid* grid = aTable->GetView() )
    {
        wxGridTableMessage message( aTable, aId, aInt1, aInt2 );
        grid->ProcessTableMessage( message );
    }
}


bool wxGridStringTable::InsertRows( size_t aPos, size_t aNumRows )
{
    aPos = std::min( aPos, m_data.size() );
    m_data.insert( m_data.begin() + aPos, aNumRows, std::vector<wxString>( m_numCols ) );
    notifyView( this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, int( aPos ), int( aNumRows ) );
    return true;
}


bool wxGridStringTable::AppendRows( size_t aNumRows )
{
    m_data.insert( m_data.end(), aNumRows, std::vector<wxString>( m_numCols ) );
    notifyView( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, int( aNumRows ) );
    return true;
}


bool wxGridStringTable::DeleteRows( size_t aPos, size_t aNumRows )
{
    if( aPos >= m_data.size() )
        return false;

    aNumRows = std::min( aNumRows, m_data.size() - aPos );
    m_data.erase( m_data.begin() + aPos, m_data.begin() + aPos + aNumRows );
    notifyView( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, int( aPos ), int( aNumRows ) );
    return true;
}


bool wxGridStringTable::InsertCols( size_t aPos, size_t aNumCols )
{
    aPos = std::min( aPos, size_t( m_numCols ) );

    for( std::vector<wxString>& row : m_data )
        row.insert( row.begin() + aPos, aNumCols, wxString() );

    m_numCols += int( aNumCols );
    notifyView( this, wxGRIDTABLE_NOTIFY_COLS_INSERTED, int( aPos ), int( aNumCols ) );
    return true;
}


bool wxGridStringTable::AppendCols( size_t aNumCols )
{
    return InsertCols( m_numCols, aNumCols ) || true;
}


bool wxGridStringTable::DeleteCols( size_t aPos, size_t aNumCols )
{
    if( aPos >= size_t( m_numCols ) )
        return false;

    aNumCols = std::min( aNumCols, size_t( m_numCols ) - aPos );

    for( std::vector<wxString>& row : m_data )
        row.erase( row.begin() + aPos, row.begin() + aPos + aNumCols );

    m_numCols -= int( aNumCols );
    notifyView( this, wxGRIDTABLE_NOTIFY_COLS_DELETED, int( aPos ), int( aNumCols ) );
    return true;
}


wxString wxGridStringTable::GetRowLabelValue( int aRow )
{
    if( aRow < int( m_rowLabels.size() ) && !m_rowLabels[aRow].IsEmpty() )
        return m_rowLabels[aRow];

    return wxGridTableBase::GetRowLabelValue( aRow );
}


wxString wxGridStringTable::GetColLabelValue( int aCol )
{
    if( aCol < int( m_colLabels.size() ) && !m_colLabels[aCol].IsEmpty() )
        return m_colLabels[aCol];

    return wxGridTableBase::GetColLabelValue( aCol );
}


void wxGridStringTable::SetRowLabelValue( int aRow, const wxString& aLabel )
{
    if( aRow < 0 )
        return;

    if( aRow >= int( m_rowLabels.size() ) )
        m_rowLabels.resize( aRow + 1 );

    m_rowLabels[aRow] = aLabel;
}


void wxGridStringTable::SetColLabelValue( int aCol, const wxString& aLabel )
{
    if( aCol < 0 )
        return;

    if( aCol >= int( m_colLabels.size() ) )
        m_colLabels.resize( aCol + 1 );

    m_colLabels[aCol] = aLabel;
}


// Renderers --------------------------------------------------------------------------------

void wxGridCellRenderer::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                               const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    // The background of the cell
    wxColour background;

    if( aIsSelected )
        background = aGrid.palette().color( aGrid.hasFocus() ? QPalette::Active : QPalette::Inactive,
                                            QPalette::Highlight );
    else
        background = aAttr.GetBackgroundColour();

    aDC.SetBrush( wxBrush( background ) );
    aDC.SetPen( *wxTRANSPARENT_PEN );
    aDC.DrawRectangle( aRect );
}


void wxGridCellStringRenderer::SetTextColoursAndFont( const wxGrid& aGrid,
                                                      const wxGridCellAttr& aAttr, wxDC& aDC,
                                                      bool aIsSelected )
{
    if( aIsSelected )
        aDC.SetTextForeground( aGrid.palette().color( QPalette::HighlightedText ) );
    else if( !aGrid.isEnabled() )
        aDC.SetTextForeground( aGrid.palette().color( QPalette::Disabled, QPalette::Text ) );
    else
        aDC.SetTextForeground( aAttr.GetTextColour() );

    aDC.SetFont( aAttr.GetFont() );
}


wxSize wxGridCellStringRenderer::DoGetBestSize( const wxGridCellAttr& aAttr, wxDC& aDC,
                                                const wxString& aText )
{
    aDC.SetFont( aAttr.GetFont() );
    wxSize size = aDC.GetMultiLineTextExtent( aText );

    // The margins of the cells of wxWidgets
    return wxSize( size.x + 6, size.y + 4 );
}


wxSize wxGridCellStringRenderer::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                              int aRow, int aCol )
{
    return DoGetBestSize( aAttr, aDC, aGrid.GetCellValue( aRow, aCol ) );
}


void wxGridCellStringRenderer::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                     const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, aIsSelected );

    SetTextColoursAndFont( aGrid, aAttr, aDC, aIsSelected );

    int hAlign, vAlign;
    aAttr.GetAlignment( &hAlign, &vAlign );

    wxRect rect = aRect;
    rect.Deflate( 2, 1 );
    aDC.DrawLabel( aGrid.GetCellValue( aRow, aCol ), rect, hAlign | vAlign );
}


void wxGridCellAutoWrapStringRenderer::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                             const wxRect& aRect, int aRow, int aCol,
                                             bool aIsSelected )
{
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, aIsSelected );
    SetTextColoursAndFont( aGrid, aAttr, aDC, aIsSelected );

    QPainter* painter = aDC.GetPainter();
    painter->save();
    painter->setFont( aAttr.GetFont() );
    painter->setPen( QColor( aDC.GetTextForeground() ) );
    painter->drawText( QRect( aRect.x + 2, aRect.y + 1, aRect.width - 4, aRect.height - 2 ),
                       Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                       ToQString( aGrid.GetCellValue( aRow, aCol ) ) );
    painter->restore();
}


void wxGridCellBoolRenderer::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                   const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, aIsSelected );

    wxGridTableBase* table = aGrid.GetTable();
    bool             value = false;

    if( table && table->CanGetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ) )
        value = table->GetValueAsBool( aRow, aCol );
    else
        value = wxGridCellBoolEditor::IsTrueValue( aGrid.GetCellValue( aRow, aCol ) );

    wxSize size = wxRendererNative::Get().GetCheckBoxSize( &aGrid );
    int    hAlign, vAlign;
    aAttr.GetAlignment( &hAlign, &vAlign );

    int x = aRect.x + ( aRect.width - size.x ) / 2;

    if( hAlign == wxALIGN_LEFT )
        x = aRect.x + 2;
    else if( hAlign == wxALIGN_RIGHT )
        x = aRect.x + aRect.width - size.x - 2;

    wxRect box( x, aRect.y + ( aRect.height - size.y ) / 2, size.x, size.y );
    wxRendererNative::Get().DrawCheckBox( &aGrid, aDC, box, value ? wxCONTROL_CHECKED : 0 );
}


wxSize wxGridCellBoolRenderer::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                            int aRow, int aCol )
{
    wxSize size = wxRendererNative::Get().GetCheckBoxSize( &aGrid );
    return wxSize( size.x + 4, size.y + 4 );
}


// Editors ----------------------------------------------------------------------------------

wxGridCellEditor::wxGridCellEditor() :
        m_control( nullptr ),
        m_attr( nullptr )
{
}


wxGridCellEditor::~wxGridCellEditor()
{
    Destroy();
}


void wxGridCellEditor::Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler )
{
    if( m_control )
    {
        wxSetWindowIdCompat( m_control.data(), aId );

        if( aEventHandler )
            wxEvtHandlerCompat( m_control.data() )->PushEventHandlerCompat( aEventHandler );
    }
    else
    {
        delete aEventHandler;
    }
}


void wxGridCellEditor::Destroy()
{
    if( m_control )
    {
        // The handler pushed by Create()
        wxEvtHandlerCompat( m_control.data() )->PopEventHandlerCompat( true );
        m_control.data()->deleteLater();
    }

    m_control = nullptr;
}


void wxGridCellEditor::SetSize( const wxRect& aRect )
{
    if( m_control )
        m_control.data()->setGeometry( aRect.x, aRect.y, aRect.width, aRect.height );
}


void wxGridCellEditor::Show( bool aShow, wxGridCellAttr* aAttr )
{
    if( m_control )
        m_control.data()->setVisible( aShow );
}


wxGridCellEditorEvtHandler::wxGridCellEditorEvtHandler( wxGrid* aGrid, wxGridCellEditor* aEditor ) :
        m_grid( aGrid ),
        m_editor( aEditor ),
        m_inSetFocus( false )
{
    Bind( wxEVT_KILL_FOCUS, &wxGridCellEditorEvtHandler::OnKillFocus, this );
}


void wxGridCellEditorEvtHandler::OnKillFocus( wxFocusEvent& aEvent )
{
    aEvent.Skip();

    if( !m_inSetFocus )
        DismissEditor();
}


void wxGridCellEditorEvtHandler::DismissEditor()
{
    // Later: the editor can't be closed from the handlers of its control
    QPointer<wxGrid> grid( m_grid );

    QMetaObject::invokeMethod( m_grid,
                               [grid]()
                               {
                                   if( grid )
                                       grid->DisableCellEditControl();
                               },
                               Qt::QueuedConnection );
}


void wxGridCellTextEditor::Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler )
{
    wxTextCtrl* text = new wxTextCtrl( aParent, aId );
    text->setFrame( false );

    if( m_maxChars )
        text->SetMaxLength( m_maxChars );

    m_control = text;

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


void wxGridCellTextEditor::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    m_value = aGrid->GetTable()->GetValue( aRow, aCol );
    Text()->ChangeValue( m_value );
    Text()->SetInsertionPointEnd();
    Text()->SelectAll();
    Text()->SetFocus();
}


bool wxGridCellTextEditor::EndEdit( int aRow, int aCol, const wxGrid* aGrid,
                                    const wxString& aOldVal, wxString* aNewVal )
{
    wxString value = Text()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void wxGridCellTextEditor::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
    m_value.clear();
}


void wxGridCellTextEditor::Reset()
{
    Text()->ChangeValue( m_value );
}


void wxGridCellTextEditor::StartingKey( wxKeyEvent& aEvent )
{
    // The key typed starts the text
    int key = aEvent.GetKeyCode();

    if( key >= WXK_SPACE && key < WXK_DELETE )
        Text()->ChangeValue( wxString( wxUniChar( key ) ) );
}


wxString wxGridCellTextEditor::GetValue() const
{
    return Text()->GetValue();
}


wxGridCellChoiceEditor::wxGridCellChoiceEditor( const wxArrayString& aChoices, bool aAllowOthers ) :
        m_choices( aChoices ),
        m_allowOthers( aAllowOthers )
{
}


wxGridCellChoiceEditor::wxGridCellChoiceEditor( size_t aCount, const wxString aChoices[],
                                                bool aAllowOthers ) :
        m_allowOthers( aAllowOthers )
{
    for( size_t i = 0; i < aCount; ++i )
        m_choices.Add( aChoices[i] );
}


void wxGridCellChoiceEditor::Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler )
{
    wxComboBox* combo = new wxComboBox( aParent, aId, wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, m_choices,
                                        m_allowOthers ? 0 : wxCB_READONLY );
    combo->setEditable( m_allowOthers );
    m_control = combo;

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


void wxGridCellChoiceEditor::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    if( m_allowOthers )
        Combo()->ChangeValue( m_value );
    else
        Combo()->SetStringSelection( m_value );

    Combo()->SetFocus();
}


bool wxGridCellChoiceEditor::EndEdit( int aRow, int aCol, const wxGrid* aGrid,
                                      const wxString& aOldVal, wxString* aNewVal )
{
    wxString value = Combo()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void wxGridCellChoiceEditor::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void wxGridCellChoiceEditor::Reset()
{
    Combo()->ChangeValue( m_value );
}


wxGridCellEditor* wxGridCellChoiceEditor::Clone() const
{
    return new wxGridCellChoiceEditor( m_choices, m_allowOthers );
}


wxString wxGridCellChoiceEditor::GetValue() const
{
    return Combo()->GetValue();
}


static wxString s_boolTrue = wxS( "1" );
static wxString s_boolFalse = wxEmptyString;


bool wxGridCellBoolEditor::IsTrueValue( const wxString& aValue )
{
    return aValue == s_boolTrue || aValue == wxS( "1" ) || aValue.IsSameAs( wxS( "true" ), false );
}


void wxGridCellBoolEditor::UseStringValues( const wxString& aTrue, const wxString& aFalse )
{
    s_boolTrue = aTrue;
    s_boolFalse = aFalse;
}


void wxGridCellBoolEditor::Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler )
{
    m_control = new wxCheckBox( aParent, aId );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


void wxGridCellBoolEditor::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    wxGridTableBase* table = aGrid->GetTable();

    if( table->CanGetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ) )
        m_value = table->GetValueAsBool( aRow, aCol );
    else
        m_value = IsTrueValue( table->GetValue( aRow, aCol ) );

    // A click on the cell toggles the value
    static_cast<wxCheckBox*>( m_control.data() )->SetValue( !m_value );
    m_control.data()->setFocus();
}


bool wxGridCellBoolEditor::EndEdit( int aRow, int aCol, const wxGrid* aGrid,
                                    const wxString& aOldVal, wxString* aNewVal )
{
    bool value = static_cast<wxCheckBox*>( m_control.data() )->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = GetValue();

    return true;
}


void wxGridCellBoolEditor::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    wxGridTableBase* table = aGrid->GetTable();

    if( table->CanSetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ) )
        table->SetValueAsBool( aRow, aCol, m_value );
    else
        table->SetValue( aRow, aCol, GetValue() );
}


void wxGridCellBoolEditor::Reset()
{
    static_cast<wxCheckBox*>( m_control.data() )->SetValue( m_value );
}


wxString wxGridCellBoolEditor::GetValue() const
{
    return m_value ? s_boolTrue : s_boolFalse;
}


// The model and the delegate of the view ----------------------------------------------------

/**
 * The model of the view of a grid: the cells of its table.  The table is changed by the code
 * of KiCad (then told to the view by the messages of the table, or a refresh of the grid): the
 * model keeps the numbers of rows and columns the view knows.
 */
class KIWX_GRID_MODEL : public QAbstractTableModel
{
public:
    explicit KIWX_GRID_MODEL( wxGrid* aGrid ) :
            QAbstractTableModel( aGrid ),
            m_grid( aGrid ),
            m_rows( 0 ),
            m_cols( 0 ),
            m_colLabelAlignment( Qt::AlignCenter ),
            m_rowLabelAlignment( Qt::AlignCenter )
    {
    }

    int rowCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return aParent.isValid() ? 0 : m_rows;
    }

    int columnCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return aParent.isValid() ? 0 : m_cols;
    }

    QVariant data( const QModelIndex& aIndex, int aRole ) const override
    {
        wxGridTableBase* table = m_grid->GetTable();

        if( !table || !aIndex.isValid() )
            return QVariant();

        if( aRole == Qt::DisplayRole || aRole == Qt::EditRole || aRole == Qt::ToolTipRole )
        {
            if( aRole == Qt::ToolTipRole )
                return QVariant();

            return ToQString( table->GetValue( aIndex.row(), aIndex.column() ) );
        }

        return QVariant();
    }

    QVariant headerData( int aSection, Qt::Orientation aOrientation, int aRole ) const override
    {
        wxGridTableBase* table = m_grid->GetTable();

        if( !table )
            return QVariant();

        if( aRole == Qt::DisplayRole )
        {
            if( aOrientation == Qt::Horizontal )
                return ToQString( table->GetColLabelValue( aSection ) );
            else
                return ToQString( table->GetRowLabelValue( aSection ) );
        }

        if( aRole == Qt::TextAlignmentRole )
            return int( aOrientation == Qt::Horizontal ? m_colLabelAlignment : m_rowLabelAlignment );

        return QVariant();
    }

    Qt::ItemFlags flags( const QModelIndex& aIndex ) const override
    {
        if( !aIndex.isValid() )
            return Qt::NoItemFlags;

        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        if( m_grid->IsEditable() && !m_grid->IsReadOnly( aIndex.row(), aIndex.column() ) )
            flags |= Qt::ItemIsEditable;

        return flags;
    }

    /// The table was changed: the numbers of rows and columns, and the contents.
    void sync()
    {
        wxGridTableBase* table = m_grid->GetTable();
        int              rows = table ? table->GetNumberRows() : 0;
        int              cols = table ? table->GetNumberCols() : 0;

        if( rows != m_rows || cols != m_cols )
        {
            beginResetModel();
            m_rows = rows;
            m_cols = cols;
            endResetModel();
        }
        else if( m_rows > 0 && m_cols > 0 )
        {
            Q_EMIT dataChanged( index( 0, 0 ), index( m_rows - 1, m_cols - 1 ) );
            Q_EMIT headerDataChanged( Qt::Horizontal, 0, m_cols - 1 );
            Q_EMIT headerDataChanged( Qt::Vertical, 0, m_rows - 1 );
        }
    }

    void rowsInserted( int aPos, int aCount )
    {
        // Not more rows than the table (rows added to it before a sync of the model are
        // already counted)
        if( wxGridTableBase* table = m_grid->GetTable() )
            aCount = std::min( aCount, table->GetNumberRows() - m_rows );

        if( aCount <= 0 )
            return;

        aPos = std::clamp( aPos, 0, m_rows );
        beginInsertRows( QModelIndex(), aPos, aPos + aCount - 1 );
        m_rows += aCount;
        endInsertRows();
    }

    void rowsDeleted( int aPos, int aCount )
    {
        aCount = std::min( aCount, m_rows - aPos );

        if( aCount <= 0 )
            return;

        beginRemoveRows( QModelIndex(), aPos, aPos + aCount - 1 );
        m_rows -= aCount;
        endRemoveRows();
    }

    void colsInserted( int aPos, int aCount )
    {
        if( wxGridTableBase* table = m_grid->GetTable() )
            aCount = std::min( aCount, table->GetNumberCols() - m_cols );

        if( aCount <= 0 )
            return;

        aPos = std::clamp( aPos, 0, m_cols );
        beginInsertColumns( QModelIndex(), aPos, aPos + aCount - 1 );
        m_cols += aCount;
        endInsertColumns();
    }

    void colsDeleted( int aPos, int aCount )
    {
        aCount = std::min( aCount, m_cols - aPos );

        if( aCount <= 0 )
            return;

        beginRemoveColumns( QModelIndex(), aPos, aPos + aCount - 1 );
        m_cols -= aCount;
        endRemoveColumns();
    }

    void cellChanged( int aRow, int aCol )
    {
        QModelIndex idx = index( aRow, aCol );
        Q_EMIT dataChanged( idx, idx );
    }

    void setColLabelAlignment( Qt::Alignment aAlignment ) { m_colLabelAlignment = aAlignment; }
    void setRowLabelAlignment( Qt::Alignment aAlignment ) { m_rowLabelAlignment = aAlignment; }
    Qt::Alignment colLabelAlignment() const { return m_colLabelAlignment; }
    Qt::Alignment rowLabelAlignment() const { return m_rowLabelAlignment; }

    void reset()
    {
        beginResetModel();
        wxGridTableBase* table = m_grid->GetTable();
        m_rows = table ? table->GetNumberRows() : 0;
        m_cols = table ? table->GetNumberCols() : 0;
        endResetModel();
    }

private:
    wxGrid*       m_grid;
    int           m_rows;
    int           m_cols;
    Qt::Alignment m_colLabelAlignment;
    Qt::Alignment m_rowLabelAlignment;
};


/**
 * The delegate of the view of a grid: the cells are drawn by their renderers, edited by the
 * controls of their editors (created once, kept by the editors).
 */
class KIWX_GRID_DELEGATE : public QStyledItemDelegate
{
public:
    explicit KIWX_GRID_DELEGATE( wxGrid* aGrid ) :
            QStyledItemDelegate( aGrid ),
            m_grid( aGrid ),
            m_editor( nullptr ),
            m_row( -1 ),
            m_col( -1 )
    {
    }

    ~KIWX_GRID_DELEGATE() override
    {
        wxSafeDecRef( m_editor );
    }

    void paint( QPainter* aPainter, const QStyleOptionViewItem& aOption,
                const QModelIndex& aIndex ) const override
    {
        int             row = aIndex.row();
        int             col = aIndex.column();
        wxGridCellAttr* attr = m_grid->GetCellAttr( row, col );

        if( !attr )
            return;

        wxGridCellRenderer* renderer = attr->GetRenderer( m_grid, row, col );

        aPainter->save();
        wxDC   dc( aPainter, m_grid->viewport() );
        QRect  r = aOption.rect;
        wxRect rect( r.x(), r.y(), r.width(), r.height() );
        bool   selected = aOption.state & QStyle::State_Selected;

        dc.SetClippingRegion( rect );
        renderer->Draw( *m_grid, *attr, dc, rect, row, col, selected );
        aPainter->restore();

        // The cursor of the grid
        if( aOption.state & QStyle::State_HasFocus )
        {
            aPainter->save();
            QPen pen( m_grid->palette().color( QPalette::Text ) );
            pen.setWidth( 2 );
            aPainter->setPen( pen );
            aPainter->setBrush( Qt::NoBrush );
            aPainter->drawRect( r.adjusted( 1, 1, -1, -1 ) );
            aPainter->restore();
        }

        renderer->DecRef();
        attr->DecRef();
    }

    QSize sizeHint( const QStyleOptionViewItem& aOption, const QModelIndex& aIndex ) const override
    {
        int             row = aIndex.row();
        int             col = aIndex.column();
        wxGridCellAttr* attr = m_grid->GetCellAttr( row, col );

        if( !attr )
            return QStyledItemDelegate::sizeHint( aOption, aIndex );

        wxGridCellRenderer* renderer = attr->GetRenderer( m_grid, row, col );

        QPixmap  pixmap( 1, 1 );
        QPainter painter( &pixmap );
        wxDC     dc( &painter, m_grid->viewport() );
        wxSize   size = renderer->GetBestSize( *m_grid, *attr, dc, row, col );

        renderer->DecRef();
        attr->DecRef();
        return QSize( size.x, size.y );
    }

    QWidget* createEditor( QWidget* aParent, const QStyleOptionViewItem& aOption,
                           const QModelIndex& aIndex ) const override
    {
        int row = aIndex.row();
        int col = aIndex.column();

        wxGridCellAttr* attr = m_grid->GetCellAttr( row, col );

        if( !attr )
            return nullptr;

        wxGridCellEditor* editor = attr->GetEditor( m_grid, row, col );
        editor->SetCellAttr( attr );

        if( !editor->IsCreated() )
        {
            editor->Create( wxWindowFromQt( aParent ), wxID_ANY,
                            new wxGridCellEditorEvtHandler( m_grid, editor ) );

            wxGridEditorCreatedEvent created( m_grid->GetId(), row, col, editor->GetControl() );
            created.SetEventObject( m_grid );
            m_grid->ProcessEvent( created );
        }
        else if( editor->GetControlWidgetCompat()->parentWidget() != aParent )
        {
            editor->GetControlWidgetCompat()->setParent( aParent );
        }

        wxSafeDecRef( m_editor );
        m_editor = editor;
        m_row = row;
        m_col = col;
        m_oldValue = m_grid->GetCellValue( row, col );
        m_began = false;

        attr->DecRef();

        m_grid->SendGridEvent( wxEVT_GRID_EDITOR_SHOWN, row, col );
        m_grid->m_cellEditCtrlEnabled = true;
        editor->Show( true );
        return editor->GetControlWidgetCompat();
    }

    void destroyEditor( QWidget* aEditor, const QModelIndex& aIndex ) const override
    {
        // The control belongs to its editor (reused by the next editions)
        if( m_editor && aEditor == m_editor->GetControlWidgetCompat() )
        {
            aEditor->hide();
            m_grid->m_cellEditCtrlEnabled = false;

            // wxGrid::HideCellEditControl() sends no event (its callers do)
            if( !m_quietClose )
                m_grid->SendGridEvent( wxEVT_GRID_EDITOR_HIDDEN, m_row, m_col );

            m_grid->setFocus();
            return;
        }

        QStyledItemDelegate::destroyEditor( aEditor, aIndex );
    }

    void setEditorData( QWidget* aEditor, const QModelIndex& aIndex ) const override
    {
        // Once per edition (the view calls it again when the model changes)
        if( !m_editor || m_began )
            return;

        m_began = true;
        m_editor->BeginEdit( aIndex.row(), aIndex.column(), m_grid );
    }

    void setModelData( QWidget* aEditor, QAbstractItemModel* aModel,
                       const QModelIndex& aIndex ) const override
    {
        if( !m_editor )
            return;

        int      row = aIndex.row();
        int      col = aIndex.column();
        wxString newValue;

        if( !m_editor->EndEdit( row, col, m_grid, m_oldValue, &newValue ) )
            return;

        // The handlers can veto the change
        if( !m_grid->SendGridEvent( wxEVT_GRID_CELL_CHANGING, row, col ) )
        {
            m_editor->Reset();
            return;
        }

        m_editor->ApplyEdit( row, col, m_grid );
        m_grid->GetModelCompat()->cellChanged( row, col );

        if( !m_grid->SendGridEvent( wxEVT_GRID_CELL_CHANGED, row, col ) )
        {
            // Vetoed after the change: the old value comes back
            m_grid->GetTable()->SetValue( row, col, m_oldValue );
            m_grid->GetModelCompat()->cellChanged( row, col );
        }
    }

    void updateEditorGeometry( QWidget* aEditor, const QStyleOptionViewItem& aOption,
                               const QModelIndex& aIndex ) const override
    {
        QRect r = aOption.rect;

        if( m_editor && aEditor == m_editor->GetControlWidgetCompat() )
            m_editor->SetSize( wxRect( r.x(), r.y(), r.width(), r.height() ) );
        else
            aEditor->setGeometry( r );
    }

    wxGridCellEditor* currentEditor() const { return m_editor; }

    /// Close the editors without the events of the end of the edition.
    void setQuietClose( bool aQuiet ) { m_quietClose = aQuiet; }

private:
    wxGrid*                   m_grid;
    mutable wxGridCellEditor* m_editor;
    mutable int               m_row;
    mutable int               m_col;
    mutable wxString          m_oldValue;
    mutable bool              m_began = false;
    bool                      m_quietClose = false;
};


/// The headers of a grid: the labels are drawn by wxGrid::DrawColLabel() and DrawRowLabel().
class KIWX_GRID_HEADER : public QHeaderView
{
public:
    KIWX_GRID_HEADER( Qt::Orientation aOrientation, wxGrid* aGrid ) :
            QHeaderView( aOrientation, aGrid ),
            m_grid( aGrid )
    {
    }

    /// The painting of Qt (the native labels of wxGrid).
    void paintDefaultSection( QPainter* aPainter, const QRect& aRect, int aSection ) const
    {
        QHeaderView::paintSection( aPainter, aRect, aSection );
    }

protected:
    void paintSection( QPainter* aPainter, const QRect& aRect, int aSection ) const override
    {
        m_grid->updateLabelSizes();

        // wxGrid draws the labels in the unscrolled coordinates of the grid
        aPainter->save();

        if( orientation() == Qt::Horizontal )
            aPainter->translate( aRect.x() - m_grid->GetColLeft( aSection ), aRect.y() );
        else
            aPainter->translate( aRect.x(), aRect.y() - m_grid->GetRowTop( aSection ) );

        wxDC dc( aPainter, const_cast<KIWX_GRID_HEADER*>( this ) );
        dc.SetFont( font() );

        if( orientation() == Qt::Horizontal )
            m_grid->DrawColLabel( dc, aSection );
        else
            m_grid->DrawRowLabel( dc, aSection );

        aPainter->restore();
    }

private:
    wxGrid* m_grid;
};


/// The corner of a grid (the button of the corner of QTableView): wxGrid::DrawCornerLabel().
class KIWX_GRID_CORNER_FILTER : public QObject
{
public:
    explicit KIWX_GRID_CORNER_FILTER( wxGrid* aGrid ) : QObject( aGrid ), m_grid( aGrid ) {}

    bool eventFilter( QObject* aObject, QEvent* aEvent ) override
    {
        if( aEvent->type() != QEvent::Paint )
            return false;

        QWidget* corner = static_cast<QWidget*>( aObject );
        QPainter painter( corner );
        wxDC     dc( &painter, corner );
        m_grid->DrawCornerLabel( dc );
        return true;
    }

private:
    wxGrid* m_grid;
};


// The grid ---------------------------------------------------------------------------------

static Qt::Alignment qtAlignment( int aHAlign, int aVAlign )
{
    Qt::Alignment alignment;

    if( aHAlign == wxALIGN_RIGHT )
        alignment |= Qt::AlignRight;
    else if( aHAlign == wxALIGN_CENTER || aHAlign == wxALIGN_CENTER_HORIZONTAL )
        alignment |= Qt::AlignHCenter;
    else
        alignment |= Qt::AlignLeft;

    if( aVAlign == wxALIGN_BOTTOM )
        alignment |= Qt::AlignBottom;
    else if( aVAlign == wxALIGN_TOP )
        alignment |= Qt::AlignTop;
    else
        alignment |= Qt::AlignVCenter;

    return alignment;
}


wxGrid::wxGrid( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize, long aStyle,
                const wxString& aName ) :
        wxWindowCompat<QTableView>( aParent ),
        m_table( nullptr ),
        m_ownTable( false ),
        m_selectionMode( wxGridSelectCells ),
        m_editable( true ),
        m_batchCount( 0 )
{
    SetId( aId );

    m_defaultAttr = new wxGridCellAttr;
    m_defaultAttr->SetKind( wxGridCellAttr::Default );
    m_defaultAttr->SetDefAttr( m_defaultAttr );
    m_defaultAttr->SetAlignment( wxALIGN_LEFT, wxALIGN_CENTER_VERTICAL );

    m_model = new KIWX_GRID_MODEL( this );
    m_delegate = new KIWX_GRID_DELEGATE( this );
    setHorizontalHeader( new KIWX_GRID_HEADER( Qt::Horizontal, this ) );
    setVerticalHeader( new KIWX_GRID_HEADER( Qt::Vertical, this ) );
    setModel( m_model );
    setItemDelegate( m_delegate );

    // The best size of a grid is the size of its cells (DoGetBestSize()): the layouts know when
    // it changes, as the sizers of wxWidgets (AutoSize(), SetColSize()...)
    for( QHeaderView* header : { horizontalHeader(), verticalHeader() } )
    {
        QObject::connect( header, &QHeaderView::sectionResized, this,
                          [this]() { updateGeometry(); } );
        QObject::connect( header, &QHeaderView::sectionCountChanged, this,
                          [this]() { updateGeometry(); } );
    }

    m_gridWin = wxWindowFromQt( viewport() );
    m_colLabelWin = wxWindowFromQt( horizontalHeader() );
    m_rowLabelWin = wxWindowFromQt( verticalHeader() );

    if( QAbstractButton* corner = findChild<QAbstractButton*>( QString(),
                                                               Qt::FindDirectChildrenOnly ) )
    {
        corner->installEventFilter( new KIWX_GRID_CORNER_FILTER( this ) );
        m_cornerLabelWin = wxWindowFromQt( corner );
    }

    setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                     | QAbstractItemView::AnyKeyPressed | QAbstractItemView::SelectedClicked );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    verticalHeader()->setDefaultSectionSize( fontMetrics().height() + 8 );
    verticalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    horizontalHeader()->setHighlightSections( false );
    setContextMenuPolicy( Qt::CustomContextMenu );
    setWordWrap( false );

    // The events of the grid
    QObject::connect( this, &QAbstractItemView::clicked, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          SendGridEvent( wxEVT_GRID_CELL_LEFT_CLICK, aIndex.row(), aIndex.column() );
                      } );

    QObject::connect( this, &QAbstractItemView::doubleClicked, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          SendGridEvent( wxEVT_GRID_CELL_LEFT_DCLICK, aIndex.row(), aIndex.column() );
                      } );

    QObject::connect( this, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          QModelIndex index = indexAt( aPos );
                          SendGridEvent( wxEVT_GRID_CELL_RIGHT_CLICK, index.isValid() ? index.row() : -1,
                                         index.isValid() ? index.column() : -1 );
                      } );

    QObject::connect( horizontalHeader(), &QHeaderView::sectionClicked, this,
                      [this]( int aCol ) { SendGridEvent( wxEVT_GRID_LABEL_LEFT_CLICK, -1, aCol ); } );

    QObject::connect( verticalHeader(), &QHeaderView::sectionClicked, this,
                      [this]( int aRow ) { SendGridEvent( wxEVT_GRID_LABEL_LEFT_CLICK, aRow, -1 ); } );

    horizontalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );

    QObject::connect( horizontalHeader(), &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          SendGridEvent( wxEVT_GRID_LABEL_RIGHT_CLICK, -1,
                                         horizontalHeader()->logicalIndexAt( aPos ) );
                      } );

    QObject::connect( horizontalHeader(), &QHeaderView::sectionResized, this,
                      [this]( int aCol, int, int )
                      {
                          wxGridSizeEvent event( wxEVT_GRID_COL_SIZE, GetId(), aCol );
                          event.SetEventObject( this );
                          ProcessEvent( event );
                      } );

    QObject::connect( selectionModel(), &QItemSelectionModel::currentChanged, this,
                      [this]( const QModelIndex& aCurrent, const QModelIndex& )
                      {
                          m_currentCellCoords = aCurrent.isValid()
                                                        ? wxGridCellCoords( aCurrent.row(),
                                                                            aCurrent.column() )
                                                        : wxGridCellCoords();

                          if( aCurrent.isValid() )
                              SendGridEvent( wxEVT_GRID_SELECT_CELL, aCurrent.row(), aCurrent.column() );
                      } );

    QObject::connect( selectionModel(), &QItemSelectionModel::selectionChanged, this,
                      [this]( const QItemSelection& aSelected, const QItemSelection& )
                      {
                          for( const QItemSelectionRange& range : aSelected )
                          {
                              wxGridRangeSelectEvent event( wxEVT_GRID_RANGE_SELECTED, GetId(),
                                                            range.top(), range.left(),
                                                            range.bottom(), range.right() );
                              event.SetEventObject( this );
                              ProcessEvent( event );
                          }
                      } );
}


wxGrid::~wxGrid()
{
    // The editors keep their controls: they go with the view
    setModel( nullptr );

    if( m_table )
        m_table->SetView( nullptr );

    if( m_ownTable )
        delete m_table;

    m_defaultAttr->SetDefAttr( nullptr );
    m_defaultAttr->DecRef();
}


bool wxGrid::SendGridEvent( wxEventType aType, int aRow, int aCol )
{
    return SendEvent( aType, aRow, aCol ) != -1;
}


static int modifiersCompat()
{
    int                   modifiers = 0;
    Qt::KeyboardModifiers qtModifiers = QApplication::keyboardModifiers();

    if( qtModifiers & Qt::ShiftModifier )
        modifiers |= wxMOD_SHIFT;

    if( qtModifiers & Qt::ControlModifier )
        modifiers |= wxMOD_CONTROL;

    if( qtModifiers & Qt::AltModifier )
        modifiers |= wxMOD_ALT;

    return modifiers;
}


int wxGrid::SendEvent( wxEventType aType, int aRow, int aCol )
{
    QPoint pos = viewport()->mapFromGlobal( QCursor::pos() );

    wxGridEvent event( GetId(), aType, this, aRow, aCol, pos.x(), pos.y(), true,
                       modifiersCompat() );
    bool processed = ProcessEvent( event );

    if( !event.IsAllowed() )
        return -1;

    return processed ? 1 : 0;
}


int wxGrid::SendEvent( wxEventType aType, const wxString& aString )
{
    QPoint pos = viewport()->mapFromGlobal( QCursor::pos() );

    wxGridEvent event( GetId(), aType, this, m_currentCellCoords.GetRow(),
                       m_currentCellCoords.GetCol(), pos.x(), pos.y(), true, modifiersCompat() );
    event.SetString( aString );
    bool processed = ProcessEvent( event );

    if( !event.IsAllowed() )
        return -1;

    return processed ? 1 : 0;
}


bool wxGrid::CreateGrid( int aNumRows, int aNumCols, wxGridSelectionModes aSelectionMode )
{
    return SetTable( new wxGridStringTable( aNumRows, aNumCols ), true, aSelectionMode );
}


bool wxGrid::SetTable( wxGridTableBase* aTable, bool aTakeOwnership,
                       wxGridSelectionModes aSelectionMode )
{
    if( m_table )
    {
        m_table->SetView( nullptr );

        if( m_ownTable )
            delete m_table;
    }

    m_table = aTable;
    m_ownTable = aTakeOwnership;

    if( m_table )
        m_table->SetView( this );

    m_model->reset();
    SetSelectionMode( aSelectionMode );
    return true;
}


bool wxGrid::ProcessTableMessage( wxGridTableMessage& aMessage )
{
    switch( aMessage.GetId() )
    {
    case wxGRIDTABLE_NOTIFY_ROWS_INSERTED:
        m_model->rowsInserted( aMessage.GetCommandInt(), aMessage.GetCommandInt2() );
        return true;

    case wxGRIDTABLE_NOTIFY_ROWS_APPENDED:
        m_model->rowsInserted( m_model->rowCount(), aMessage.GetCommandInt() );
        return true;

    case wxGRIDTABLE_NOTIFY_ROWS_DELETED:
        m_model->rowsDeleted( aMessage.GetCommandInt(), aMessage.GetCommandInt2() );
        return true;

    case wxGRIDTABLE_NOTIFY_COLS_INSERTED:
        m_model->colsInserted( aMessage.GetCommandInt(), aMessage.GetCommandInt2() );
        return true;

    case wxGRIDTABLE_NOTIFY_COLS_APPENDED:
        m_model->colsInserted( m_model->columnCount(), aMessage.GetCommandInt() );
        return true;

    case wxGRIDTABLE_NOTIFY_COLS_DELETED:
        m_model->colsDeleted( aMessage.GetCommandInt(), aMessage.GetCommandInt2() );
        return true;

    default:
        return false;
    }
}


int wxGrid::GetNumberRows() const
{
    return m_table ? m_table->GetNumberRows() : 0;
}


int wxGrid::GetNumberCols() const
{
    return m_table ? m_table->GetNumberCols() : 0;
}


wxString wxGrid::GetCellValue( int aRow, int aCol ) const
{
    if( !m_table || aRow < 0 || aCol < 0 )
        return wxString();

    return m_table->GetValue( aRow, aCol );
}


void wxGrid::SetCellValue( int aRow, int aCol, const wxString& aValue )
{
    if( !m_table )
        return;

    m_table->SetValue( aRow, aCol, aValue );

    if( aRow < m_model->rowCount() && aCol < m_model->columnCount() )
        m_model->cellChanged( aRow, aCol );
}


bool wxGrid::AppendRows( int aNumRows, bool aUpdateLabels )
{
    return m_table && m_table->AppendRows( aNumRows );
}


bool wxGrid::InsertRows( int aPos, int aNumRows, bool aUpdateLabels )
{
    return m_table && m_table->InsertRows( aPos, aNumRows );
}


bool wxGrid::DeleteRows( int aPos, int aNumRows, bool aUpdateLabels )
{
    return m_table && m_table->DeleteRows( aPos, aNumRows );
}


bool wxGrid::AppendCols( int aNumCols, bool aUpdateLabels )
{
    return m_table && m_table->AppendCols( aNumCols );
}


bool wxGrid::InsertCols( int aPos, int aNumCols, bool aUpdateLabels )
{
    return m_table && m_table->InsertCols( aPos, aNumCols );
}


bool wxGrid::DeleteCols( int aPos, int aNumCols, bool aUpdateLabels )
{
    return m_table && m_table->DeleteCols( aPos, aNumCols );
}


void wxGrid::ClearGrid()
{
    if( m_table )
    {
        m_table->Clear();
        ForceRefresh();
    }
}


void wxGrid::SetColSize( int aCol, int aWidth )
{
    if( aWidth == wxGRID_AUTOSIZE )
        AutoSizeColumn( aCol );
    else
        setColumnWidth( aCol, aWidth );
}


int wxGrid::GetColSize( int aCol ) const
{
    return columnWidth( aCol );
}


void wxGrid::SetRowSize( int aRow, int aHeight )
{
    setRowHeight( aRow, aHeight );
}


int wxGrid::GetRowSize( int aRow ) const
{
    return rowHeight( aRow );
}


void wxGrid::SetDefaultColSize( int aWidth, bool aResizeExisting )
{
    horizontalHeader()->setDefaultSectionSize( aWidth );
}


void wxGrid::SetDefaultRowSize( int aHeight, bool aResizeExisting )
{
    verticalHeader()->setDefaultSectionSize( aHeight );
}


int wxGrid::GetDefaultRowSize() const
{
    return verticalHeader()->defaultSectionSize();
}


int wxGrid::GetDefaultColSize() const
{
    return horizontalHeader()->defaultSectionSize();
}


void wxGrid::SetColMinimalWidth( int aCol, int aWidth )
{
    if( columnWidth( aCol ) < aWidth )
        setColumnWidth( aCol, aWidth );
}


void wxGrid::AutoSizeColumn( int aCol, bool aSetAsMin )
{
    resizeColumnToContents( aCol );
}


void wxGrid::AutoSizeColumns( bool aSetAsMin )
{
    resizeColumnsToContents();
}


void wxGrid::AutoSizeRow( int aRow, bool aSetAsMin )
{
    resizeRowToContents( aRow );
}


void wxGrid::AutoSizeRows( bool aSetAsMin )
{
    resizeRowsToContents();
}


void wxGrid::AutoSize()
{
    resizeColumnsToContents();
    resizeRowsToContents();
}


void wxGrid::AutoSizeColLabelSize( int aCol )
{
    horizontalHeader()->resizeSection( aCol, std::max( columnWidth( aCol ),
                                                       horizontalHeader()->sectionSizeHint( aCol ) ) );
}


void wxGrid::SetColLabelValue( int aCol, const wxString& aLabel )
{
    if( m_table )
    {
        m_table->SetColLabelValue( aCol, aLabel );
        Q_EMIT m_model->headerDataChanged( Qt::Horizontal, aCol, aCol );
    }
}


wxString wxGrid::GetColLabelValue( int aCol ) const
{
    return m_table ? m_table->GetColLabelValue( aCol ) : wxString();
}


void wxGrid::SetRowLabelValue( int aRow, const wxString& aLabel )
{
    if( m_table )
    {
        m_table->SetRowLabelValue( aRow, aLabel );
        Q_EMIT m_model->headerDataChanged( Qt::Vertical, aRow, aRow );
    }
}


wxString wxGrid::GetRowLabelValue( int aRow ) const
{
    return m_table ? m_table->GetRowLabelValue( aRow ) : wxString();
}


void wxGrid::SetColLabelSize( int aHeight )
{
    horizontalHeader()->setVisible( aHeight != 0 );

    if( aHeight > 0 )
        horizontalHeader()->setFixedHeight( aHeight );
}


void wxGrid::updateLabelSizes()
{
    m_colLabelHeight = GetColLabelSize();
    m_rowLabelWidth = GetRowLabelSize();
}


int wxGrid::GetColLabelSize() const
{
    return horizontalHeader()->isVisible() ? horizontalHeader()->height() : 0;
}


void wxGrid::SetRowLabelSize( int aWidth )
{
    verticalHeader()->setVisible( aWidth != 0 );

    if( aWidth > 0 )
        verticalHeader()->setFixedWidth( aWidth );
}


int wxGrid::GetRowLabelSize() const
{
    return verticalHeader()->isHidden() ? 0 : verticalHeader()->width();
}


void wxGrid::SetColLabelAlignment( int aHAlign, int aVAlign )
{
    m_model->setColLabelAlignment( qtAlignment( aHAlign, aVAlign ) );
}


void wxGrid::SetRowLabelAlignment( int aHAlign, int aVAlign )
{
    m_model->setRowLabelAlignment( qtAlignment( aHAlign, aVAlign ) );
}


static void wxAlignment( Qt::Alignment aAlignment, int* aHAlign, int* aVAlign )
{
    if( aHAlign )
    {
        if( aAlignment & Qt::AlignRight )
            *aHAlign = wxALIGN_RIGHT;
        else if( aAlignment & Qt::AlignHCenter )
            *aHAlign = wxALIGN_CENTER;
        else
            *aHAlign = wxALIGN_LEFT;
    }

    if( aVAlign )
    {
        if( aAlignment & Qt::AlignBottom )
            *aVAlign = wxALIGN_BOTTOM;
        else if( aAlignment & Qt::AlignTop )
            *aVAlign = wxALIGN_TOP;
        else
            *aVAlign = wxALIGN_CENTER;
    }
}


void wxGrid::GetColLabelAlignment( int* aHAlign, int* aVAlign ) const
{
    wxAlignment( m_model->colLabelAlignment(), aHAlign, aVAlign );
}


void wxGrid::GetRowLabelAlignment( int* aHAlign, int* aVAlign ) const
{
    wxAlignment( m_model->rowLabelAlignment(), aHAlign, aVAlign );
}


int wxGrid::GetColLeft( int aCol ) const
{
    return horizontalHeader()->sectionPosition( aCol );
}


int wxGrid::GetColRight( int aCol ) const
{
    return GetColLeft( aCol ) + GetColWidth( aCol ) - 1;
}


int wxGrid::GetRowTop( int aRow ) const
{
    return verticalHeader()->sectionPosition( aRow );
}


int wxGrid::GetRowBottom( int aRow ) const
{
    return GetRowTop( aRow ) + GetRowHeight( aRow ) - 1;
}


void wxGrid::DrawTextRectangle( wxDC& aDC, const wxString& aText, const wxRect& aRect,
                                int aHAlign, int aVAlign, int aTextOrientation ) const
{
    if( aRect.width <= 0 || aRect.height <= 0 )
        return;

    QPainter* painter = aDC.GetPainter();

    painter->save();
    painter->setClipRect( aRect.x, aRect.y, aRect.width, aRect.height, Qt::IntersectClip );
    aDC.DrawLabel( aText, aRect, aHAlign | aVAlign );
    painter->restore();
}


void wxGrid::DrawTextRectangle( wxDC& aDC, const wxArrayString& aLines, const wxRect& aRect,
                                int aHAlign, int aVAlign, int aTextOrientation ) const
{
    DrawTextRectangle( aDC, wxJoin( aLines, '\n', '\0' ), aRect, aHAlign, aVAlign,
                       aTextOrientation );
}


wxSize wxGrid::DoGetBestSize() const
{
    // All the cells and the labels, as wxGrid::DoGetBestSize()
    int width = 2 * frameWidth() + horizontalHeader()->length() + GetRowLabelSize();
    int height = 2 * frameWidth() + verticalHeader()->length() + GetColLabelSize();
    return wxSize( width, height );
}


QSize wxGrid::sizeHint() const
{
    wxSize size = DoGetBestSize();

    if( size.x <= 0 || size.y <= 0 )
        return wxWindowCompat<QTableView>::sizeHint();

    // The large grids stay in the screen
    if( QScreen* screen = this->screen() )
    {
        QSize available = screen->availableSize() * 2 / 3;
        size.x = std::min( size.x, available.width() );
        size.y = std::min( size.y, available.height() );
    }

    return QSize( size.x, size.y );
}


void wxGrid::DrawColLabel( wxDC& aDC, int aCol )
{
    if( GetColWidth( aCol ) <= 0 || m_colLabelHeight <= 0 )
        return;

    QRect rect( GetColLeft( aCol ), 0, GetColWidth( aCol ), m_colLabelHeight );
    static_cast<KIWX_GRID_HEADER*>( horizontalHeader() )->paintDefaultSection( aDC.GetPainter(),
                                                                              rect, aCol );
}


void wxGrid::DrawRowLabel( wxDC& aDC, int aRow )
{
    if( GetRowHeight( aRow ) <= 0 || m_rowLabelWidth <= 0 )
        return;

    QRect rect( 0, GetRowTop( aRow ), m_rowLabelWidth, GetRowHeight( aRow ) );
    static_cast<KIWX_GRID_HEADER*>( verticalHeader() )->paintDefaultSection( aDC.GetPainter(),
                                                                            rect, aRow );
}


void wxGrid::DrawCornerLabel( wxDC& aDC )
{
    QPainter* painter = aDC.GetPainter();
    QRect     rect( 0, 0, m_rowLabelWidth, m_colLabelHeight );

    painter->fillRect( rect, horizontalHeader()->palette().color( QPalette::Button ) );
}


void wxGridHeaderLabelsRenderer::DrawBorder( const wxGrid& aGrid, wxDC& aDC, wxRect& aRect ) const
{
    QPainter* painter = aDC.GetPainter();
    painter->save();
    painter->setPen( aGrid.palette().color( QPalette::Mid ) );
    painter->drawLine( aRect.GetRight(), aRect.GetTop(), aRect.GetRight(), aRect.GetBottom() );
    painter->drawLine( aRect.GetLeft(), aRect.GetBottom(), aRect.GetRight(), aRect.GetBottom() );
    painter->restore();
}


void wxGridHeaderLabelsRenderer::DrawLabel( const wxGrid& aGrid, wxDC& aDC, const wxString& aValue,
                                            const wxRect& aRect, int aHAlign, int aVAlign,
                                            int aTextOrientation ) const
{
    aDC.SetTextForeground( aGrid.palette().color( QPalette::ButtonText ) );
    aDC.DrawLabel( aValue, aRect, aHAlign | aVAlign );
}


void wxGrid::SetLabelFont( const wxFont& aFont )
{
    horizontalHeader()->setFont( aFont );
    verticalHeader()->setFont( aFont );
}


wxFont wxGrid::GetLabelFont() const
{
    return horizontalHeader()->font();
}


void wxGrid::SetDefaultCellAlignment( int aHAlign, int aVAlign )
{
    m_defaultAttr->SetAlignment( aHAlign, aVAlign );
}


wxGridCellAttr* wxGrid::GetOrCreateCellAttr( int aRow, int aCol ) const
{
    // The attributes of the cell itself (created if needed), with a reference for the caller
    if( !m_table || !m_table->CanHaveAttributes() )
        return nullptr;

    wxGridCellAttr* attr = m_table->GetAttr( aRow, aCol, wxGridCellAttr::Cell );

    if( !attr )
    {
        attr = new wxGridCellAttr;
        attr->IncRef();
        m_table->SetAttr( attr, aRow, aCol );
    }

    return attr;
}


void wxGrid::SetCellAlignment( int aRow, int aCol, int aHAlign, int aVAlign )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetAlignment( aHAlign, aVAlign );
        attr->DecRef();
    }
}


void wxGrid::GetCellAlignment( int aRow, int aCol, int* aHAlign, int* aVAlign ) const
{
    wxGridCellAttr* attr = GetCellAttr( aRow, aCol );
    attr->GetAlignment( aHAlign, aVAlign );
    attr->DecRef();
}


void wxGrid::SetDefaultCellFont( const wxFont& aFont )
{
    m_defaultAttr->SetFont( aFont );
}


void wxGrid::SetDefaultCellTextColour( const wxColour& aColour )
{
    m_defaultAttr->SetTextColour( aColour );
}


void wxGrid::SetDefaultCellBackgroundColour( const wxColour& aColour )
{
    m_defaultAttr->SetBackgroundColour( aColour );
}


wxColour wxGrid::GetDefaultCellBackgroundColour() const
{
    return m_defaultAttr->GetBackgroundColour();
}


wxColour wxGrid::GetDefaultCellTextColour() const
{
    return m_defaultAttr->GetTextColour();
}


wxFont wxGrid::GetDefaultCellFont() const
{
    return m_defaultAttr->GetFont();
}


void wxGrid::SetCellFont( int aRow, int aCol, const wxFont& aFont )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetFont( aFont );
        attr->DecRef();
    }
}


void wxGrid::SetCellTextColour( int aRow, int aCol, const wxColour& aColour )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetTextColour( aColour );
        attr->DecRef();
    }
}


void wxGrid::SetCellBackgroundColour( int aRow, int aCol, const wxColour& aColour )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetBackgroundColour( aColour );
        attr->DecRef();
    }
}


wxColour wxGrid::GetCellBackgroundColour( int aRow, int aCol ) const
{
    wxGridCellAttr* attr = GetCellAttr( aRow, aCol );
    wxColour        colour = attr->GetBackgroundColour();
    attr->DecRef();
    return colour;
}


wxColour wxGrid::GetCellTextColour( int aRow, int aCol ) const
{
    wxGridCellAttr* attr = GetCellAttr( aRow, aCol );
    wxColour        colour = attr->GetTextColour();
    attr->DecRef();
    return colour;
}


void wxGrid::SetCellRenderer( int aRow, int aCol, wxGridCellRenderer* aRenderer )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetRenderer( aRenderer );
        attr->DecRef();
    }
}


void wxGrid::SetCellEditor( int aRow, int aCol, wxGridCellEditor* aEditor )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetEditor( aEditor );
        attr->DecRef();
    }
}


void wxGrid::SetDefaultRenderer( wxGridCellRenderer* aRenderer )
{
    m_defaultAttr->SetRenderer( aRenderer );
}


void wxGrid::SetDefaultEditor( wxGridCellEditor* aEditor )
{
    m_defaultAttr->SetEditor( aEditor );
}


wxGridCellRenderer* wxGrid::GetCellRenderer( int aRow, int aCol ) const
{
    wxGridCellAttr*     attr = GetCellAttr( aRow, aCol );
    wxGridCellRenderer* renderer = attr->GetRenderer( this, aRow, aCol );
    attr->DecRef();
    return renderer;
}


wxGridCellEditor* wxGrid::GetCellEditor( int aRow, int aCol ) const
{
    wxGridCellAttr*   attr = GetCellAttr( aRow, aCol );
    wxGridCellEditor* editor = attr->GetEditor( this, aRow, aCol );
    attr->DecRef();
    return editor;
}


wxGridCellRenderer* wxGrid::GetDefaultRenderer() const
{
    return m_defaultAttr->GetRenderer( nullptr, 0, 0 );
}


wxGridCellEditor* wxGrid::GetDefaultEditor() const
{
    return m_defaultAttr->GetEditor( nullptr, 0, 0 );
}


wxGridCellRenderer* wxGrid::GetDefaultRendererForCell( int aRow, int aCol ) const
{
    // The default renderer of the type of the cell
    wxString type = m_table ? m_table->GetTypeName( aRow, aCol ) : wxString( wxGRID_VALUE_STRING );

    if( type == wxGRID_VALUE_BOOL )
        return new wxGridCellBoolRenderer;

    if( type == wxGRID_VALUE_NUMBER )
        return new wxGridCellNumberRenderer;

    if( type == wxGRID_VALUE_FLOAT )
        return new wxGridCellFloatRenderer;

    return new wxGridCellStringRenderer;
}


wxGridCellEditor* wxGrid::GetDefaultEditorForCell( int aRow, int aCol ) const
{
    // The editors are kept by the default attributes (one per type), as with wxWidgets
    wxString type = m_table ? m_table->GetTypeName( aRow, aCol ) : wxString( wxGRID_VALUE_STRING );

    static thread_local std::map<std::pair<const wxGrid*, wxString>, wxGridCellEditor*> s_editors;
    wxGridCellEditor*& editor = s_editors[std::make_pair( this, type )];

    if( !editor )
    {
        if( type == wxGRID_VALUE_BOOL )
            editor = new wxGridCellBoolEditor;
        else if( type == wxGRID_VALUE_NUMBER )
            editor = new wxGridCellNumberEditor;
        else if( type == wxGRID_VALUE_FLOAT )
            editor = new wxGridCellFloatEditor;
        else
            editor = new wxGridCellTextEditor;

        // The editor of the grid stays as long as the grid
        QObject::connect( this, &QObject::destroyed, [key = std::make_pair( this, type )]()
                          {
                              auto it = s_editors.find( key );

                              if( it != s_editors.end() )
                              {
                                  wxSafeDecRef( it->second );
                                  s_editors.erase( it );
                              }
                          } );
    }

    editor->IncRef();
    return editor;
}


void wxGrid::SetColFormatBool( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer );
    attr->SetEditor( new wxGridCellBoolEditor );
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER_VERTICAL );
    SetColAttr( aCol, attr );
}


void wxGrid::SetColFormatNumber( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellNumberRenderer );
    attr->SetEditor( new wxGridCellNumberEditor );
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_CENTER_VERTICAL );
    SetColAttr( aCol, attr );
}


void wxGrid::SetColFormatFloat( int aCol, int aWidth, int aPrecision )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellFloatRenderer( aWidth, aPrecision ) );
    attr->SetEditor( new wxGridCellFloatEditor( aWidth, aPrecision ) );
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_CENTER_VERTICAL );
    SetColAttr( aCol, attr );
}


void wxGrid::SetColAttr( int aCol, wxGridCellAttr* aAttr )
{
    if( m_table && m_table->CanHaveAttributes() )
        m_table->SetColAttr( aAttr, aCol );
    else
        wxSafeDecRef( aAttr );

    viewport()->update();
}


void wxGrid::SetRowAttr( int aRow, wxGridCellAttr* aAttr )
{
    if( m_table && m_table->CanHaveAttributes() )
        m_table->SetRowAttr( aAttr, aRow );
    else
        wxSafeDecRef( aAttr );

    viewport()->update();
}


void wxGrid::SetAttr( int aRow, int aCol, wxGridCellAttr* aAttr )
{
    if( m_table && m_table->CanHaveAttributes() )
        m_table->SetAttr( aAttr, aRow, aCol );
    else
        wxSafeDecRef( aAttr );

    viewport()->update();
}


wxGridCellAttr* wxGrid::GetCellAttr( int aRow, int aCol ) const
{
    // The attributes of the table over the default ones
    wxGridCellAttr* attr = m_table ? m_table->GetAttr( aRow, aCol, wxGridCellAttr::Any ) : nullptr;

    if( !attr )
    {
        m_defaultAttr->IncRef();
        return m_defaultAttr;
    }

    // The attributes not set by the table come from the default ones
    if( attr->GetKind() != wxGridCellAttr::Merged )
    {
        wxGridCellAttr* merged = attr->Clone();
        attr->DecRef();
        attr = merged;
    }

    attr->SetDefAttr( m_defaultAttr );
    return attr;
}


void wxGrid::SetReadOnly( int aRow, int aCol, bool aReadOnly )
{
    if( wxGridCellAttr* attr = GetOrCreateCellAttr( aRow, aCol ) )
    {
        attr->SetReadOnly( aReadOnly );
        attr->DecRef();
    }
}


bool wxGrid::IsReadOnly( int aRow, int aCol ) const
{
    wxGridCellAttr* attr = GetCellAttr( aRow, aCol );
    bool            readOnly = attr->IsReadOnly();
    attr->DecRef();
    return readOnly;
}


void wxGrid::EnableEditing( bool aEdit )
{
    m_editable = aEdit;

    if( !aEdit )
        DisableCellEditControl();
}


void wxGrid::EndBatch()
{
    if( m_batchCount > 0 && --m_batchCount == 0 )
        ForceRefresh();
}


void wxGrid::ForceRefresh()
{
    if( m_batchCount > 0 )
        return;

    m_model->sync();
    viewport()->update();
}


void wxGrid::SetSelectionMode( wxGridSelectionModes aMode )
{
    m_selectionMode = aMode;

    switch( aMode )
    {
    case wxGridSelectRows:
        setSelectionBehavior( QAbstractItemView::SelectRows );
        break;

    case wxGridSelectColumns:
        setSelectionBehavior( QAbstractItemView::SelectColumns );
        break;

    default:
        setSelectionBehavior( QAbstractItemView::SelectItems );
        break;
    }

    setSelectionMode( aMode == wxGridSelectNone ? QAbstractItemView::NoSelection
                                                : QAbstractItemView::ExtendedSelection );
}


void wxGrid::SelectRow( int aRow, bool aAddToSelected )
{
    if( aRow < 0 || aRow >= m_model->rowCount() )
        return;

    QItemSelection selection( m_model->index( aRow, 0 ),
                              m_model->index( aRow, std::max( m_model->columnCount() - 1, 0 ) ) );
    selectionModel()->select( selection, aAddToSelected ? QItemSelectionModel::Select
                                                        : QItemSelectionModel::ClearAndSelect );
}


void wxGrid::SelectCol( int aCol, bool aAddToSelected )
{
    if( aCol < 0 || aCol >= m_model->columnCount() )
        return;

    QItemSelection selection( m_model->index( 0, aCol ),
                              m_model->index( std::max( m_model->rowCount() - 1, 0 ), aCol ) );
    selectionModel()->select( selection, aAddToSelected ? QItemSelectionModel::Select
                                                        : QItemSelectionModel::ClearAndSelect );
}


void wxGrid::SelectBlock( int aTopRow, int aLeftCol, int aBottomRow, int aRightCol,
                          bool aAddToSelected )
{
    QItemSelection selection( m_model->index( aTopRow, aLeftCol ),
                              m_model->index( aBottomRow, aRightCol ) );
    selectionModel()->select( selection, aAddToSelected ? QItemSelectionModel::Select
                                                        : QItemSelectionModel::ClearAndSelect );
}


void wxGrid::SelectAll()
{
    selectAll();
}


void wxGrid::ClearSelection()
{
    selectionModel()->clearSelection();
}


bool wxGrid::IsSelection() const
{
    return selectionModel()->hasSelection();
}


bool wxGrid::IsInSelection( int aRow, int aCol ) const
{
    return selectionModel()->isSelected( m_model->index( aRow, aCol ) );
}


wxArrayInt wxGrid::GetSelectedRows() const
{
    // The rows selected entirely
    wxArrayInt rows;

    for( const QModelIndex& index : selectionModel()->selectedRows() )
        rows.push_back( index.row() );

    std::sort( rows.begin(), rows.end() );
    return rows;
}


wxArrayInt wxGrid::GetSelectedCols() const
{
    wxArrayInt cols;

    for( const QModelIndex& index : selectionModel()->selectedColumns() )
        cols.push_back( index.column() );

    std::sort( cols.begin(), cols.end() );
    return cols;
}


wxGridCellCoordsArray wxGrid::GetSelectedCells() const
{
    // The cells selected individually (not as part of a block), as wxWidgets: none
    return wxGridCellCoordsArray();
}


wxGridCellCoordsArray wxGrid::GetSelectionBlockTopLeft() const
{
    wxGridCellCoordsArray cells;

    for( const QItemSelectionRange& range : selectionModel()->selection() )
        cells.emplace_back( range.top(), range.left() );

    return cells;
}


wxGridCellCoordsArray wxGrid::GetSelectionBlockBottomRight() const
{
    wxGridCellCoordsArray cells;

    for( const QItemSelectionRange& range : selectionModel()->selection() )
        cells.emplace_back( range.bottom(), range.right() );

    return cells;
}


wxGridBlocks wxGrid::GetSelectedBlocks() const
{
    wxGridBlocks blocks;

    for( const QItemSelectionRange& range : selectionModel()->selection() )
        blocks.emplace_back( range.top(), range.left(), range.bottom(), range.right() );

    return blocks;
}


void wxGrid::DeselectRow( int aRow )
{
    QItemSelection selection( m_model->index( aRow, 0 ),
                              m_model->index( aRow, std::max( m_model->columnCount() - 1, 0 ) ) );
    selectionModel()->select( selection, QItemSelectionModel::Deselect );
}


void wxGrid::DeselectCell( int aRow, int aCol )
{
    selectionModel()->select( m_model->index( aRow, aCol ), QItemSelectionModel::Deselect );
}


int wxGrid::GetGridCursorRow() const
{
    QModelIndex index = currentIndex();
    return index.isValid() ? index.row() : -1;
}


int wxGrid::GetGridCursorCol() const
{
    QModelIndex index = currentIndex();
    return index.isValid() ? index.column() : -1;
}


void wxGrid::SetGridCursor( int aRow, int aCol )
{
    QModelIndex index = m_model->index( aRow, aCol );

    if( index.isValid() )
        selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
}


void wxGrid::GoToCell( int aRow, int aCol )
{
    MakeCellVisible( aRow, aCol );
    SetGridCursor( aRow, aCol );
}


void wxGrid::MakeCellVisible( int aRow, int aCol )
{
    QModelIndex index = m_model->index( aRow, std::max( aCol, 0 ) );

    if( index.isValid() )
        scrollTo( index );
}


bool wxGrid::IsVisible( int aRow, int aCol, bool aWholeCellVisible ) const
{
    QRect rect = visualRect( m_model->index( aRow, aCol ) );
    return aWholeCellVisible ? viewport()->rect().contains( rect )
                             : viewport()->rect().intersects( rect );
}


static bool moveGridCursor( wxGrid* aGrid, int aDRow, int aDCol, bool aExpand )
{
    int row = aGrid->GetGridCursorRow() + aDRow;
    int col = aGrid->GetGridCursorCol() + aDCol;

    if( row < 0 || col < 0 || row >= aGrid->GetNumberRows() || col >= aGrid->GetNumberCols() )
        return false;

    aGrid->GoToCell( row, col );
    return true;
}


bool wxGrid::MoveCursorDown( bool aExpandSelection ) { return moveGridCursor( this, 1, 0, aExpandSelection ); }
bool wxGrid::MoveCursorUp( bool aExpandSelection ) { return moveGridCursor( this, -1, 0, aExpandSelection ); }
bool wxGrid::MoveCursorLeft( bool aExpandSelection ) { return moveGridCursor( this, 0, -1, aExpandSelection ); }
bool wxGrid::MoveCursorRight( bool aExpandSelection ) { return moveGridCursor( this, 0, 1, aExpandSelection ); }


bool wxGrid::IsCellEditControlEnabled() const
{
    return state() == QAbstractItemView::EditingState;
}


bool wxGrid::IsCurrentCellReadOnly() const
{
    return IsReadOnly( GetGridCursorRow(), GetGridCursorCol() );
}


bool wxGrid::CanEnableCellControl() const
{
    return m_editable && GetGridCursorRow() >= 0 && !IsCurrentCellReadOnly();
}


void wxGrid::EnableCellEditControl( bool aEnable )
{
    if( aEnable )
    {
        ShowCellEditControl();
    }
    else if( IsCellEditControlEnabled() )
    {
        // The end of the edition: the value is saved (wxGrid::DisableCellEditControl())
        if( wxGridCellEditor* editor = m_delegate->currentEditor() )
        {
            if( QWidget* control = editor->GetControlWidgetCompat() )
            {
                commitData( control );
                closeEditor( control, QAbstractItemDelegate::NoHint );
            }
        }
    }
}


void wxGrid::ShowCellEditControl()
{
    if( CanEnableCellControl() && !IsCellEditControlEnabled() )
        edit( currentIndex() );
}


void wxGrid::HideCellEditControl()
{
    // The editor is hidden without saving its value and without events: the callers end
    // the edition themselves (as wxGrid::HideCellEditControl())
    if( IsCellEditControlEnabled() )
    {
        if( wxGridCellEditor* editor = m_delegate->currentEditor() )
        {
            if( QWidget* control = editor->GetControlWidgetCompat() )
            {
                m_delegate->setQuietClose( true );
                closeEditor( control, QAbstractItemDelegate::NoHint );
                m_delegate->setQuietClose( false );
            }
        }
    }
}


void wxGrid::SaveEditControlValue()
{
    if( IsCellEditControlEnabled() )
    {
        if( wxGridCellEditor* editor = m_delegate->currentEditor() )
        {
            if( QWidget* control = editor->GetControlWidgetCompat() )
                commitData( control );
        }
    }
}


wxWindow* wxGrid::GetGridWindow() const
{
    return wxWindowFromQt( viewport() );
}


wxWindow* wxGrid::GetGridColLabelWindow() const
{
    return wxWindowFromQt( horizontalHeader() );
}


wxWindow* wxGrid::GetGridRowLabelWindow() const
{
    return wxWindowFromQt( verticalHeader() );
}


wxWindow* wxGrid::GetGridCornerLabelWindow() const
{
    return m_cornerLabelWin;
}


wxRect wxGrid::CellToRect( int aRow, int aCol ) const
{
    QRect rect = visualRect( m_model->index( aRow, aCol ) );
    return wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
}


int wxGrid::XToCol( int aX, bool aClipToMinMax ) const
{
    int col = columnAt( aX );

    if( col < 0 && aClipToMinMax )
        col = aX < 0 ? 0 : GetNumberCols() - 1;

    return col;
}


int wxGrid::YToRow( int aY, bool aClipToMinMax ) const
{
    int row = rowAt( aY );

    if( row < 0 && aClipToMinMax )
        row = aY < 0 ? 0 : GetNumberRows() - 1;

    return row;
}


wxGridCellCoords wxGrid::XYToCell( int aX, int aY ) const
{
    return wxGridCellCoords( YToRow( aY ), XToCol( aX ) );
}


void wxGrid::CalcScrolledPosition( int aX, int aY, int* aXX, int* aYY ) const
{
    if( aXX )
        *aXX = aX - horizontalScrollBar()->value();

    if( aYY )
        *aYY = aY - verticalScrollBar()->value();
}


void wxGrid::CalcUnscrolledPosition( int aX, int aY, int* aXX, int* aYY ) const
{
    if( aXX )
        *aXX = aX + horizontalScrollBar()->value();

    if( aYY )
        *aYY = aY + verticalScrollBar()->value();
}


void wxGrid::SetSortingColumn( int aCol, bool aAscending )
{
    m_sortCol = aCol;
    m_sortAscending = aAscending;

    QHeaderView* header = horizontalHeader();

    if( aCol == wxNOT_FOUND )
    {
        header->setSortIndicatorShown( false );
    }
    else
    {
        header->setSortIndicatorShown( true );
        header->setSortIndicator( aCol, aAscending ? Qt::AscendingOrder : Qt::DescendingOrder );
    }
}
