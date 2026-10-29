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
 * @file kiwx_grid.h
 * The grids of the wxWidgets compatibility layer of the Qt build (wxcompat): wxGrid is a
 * QTableView whose model is its wxGridTableBase, the attributes of the cells (wxGridCellAttr)
 * give the renderers (drawn with a wxDC on the QPainter of the view) and the editors (whose
 * controls are the editors of the view).  The tables, renderers and editors of KiCad derive from
 * these classes unchanged.
 */

#ifndef KIWX_GRID_H
#define KIWX_GRID_H

#include <map>
#include <memory>
#include <vector>

#include <QHeaderView>
#include <QPointer>
#include <QTableView>

#include <wx/arrstr.h>
#include <wx/kiwx_gui.h>

class wxGrid;
class wxGridTableBase;
class wxGridCellRenderer;
class wxGridCellEditor;
class KIWX_GRID_MODEL;
class KIWX_GRID_DELEGATE;

#define wxGridNameStr "grid"

#define wxGRID_VALUE_STRING     "string"
#define wxGRID_VALUE_BOOL       "bool"
#define wxGRID_VALUE_NUMBER     "long"
#define wxGRID_VALUE_FLOAT      "double"
#define wxGRID_VALUE_CHOICE     "choice"
#define wxGRID_VALUE_DATE       "date"
#define wxGRID_VALUE_TEXT       wxGRID_VALUE_STRING
#define wxGRID_VALUE_LONG       wxGRID_VALUE_NUMBER

#define wxGRID_AUTOSIZE  -1
#define wxGRID_DEFAULT_NUMBER_ROWS 10
#define wxGRID_DEFAULT_NUMBER_COLS 10


/// An object with a reference count (wxRefCounter, wxClientDataContainer).
class KIQT_API wxGridRefCounted
{
public:
    wxGridRefCounted() : m_refCount( 1 ) {}

    void IncRef() { m_refCount++; }
    void DecRef()
    {
        if( --m_refCount == 0 )
            delete this;
    }

    int GetRefCount() const { return m_refCount; }

protected:
    virtual ~wxGridRefCounted() = default;

private:
    int m_refCount;
};

template <typename T>
inline void wxSafeDecRef( T* aObject )
{
    if( aObject )
        aObject->DecRef();
}


/**
 * The attributes of cells (the colours, the font, the alignment, the read only state, the
 * renderer and the editor).  Reference counted: GetAttr() gives a reference to its caller.
 */
class KIQT_API wxGridCellAttr : public wxGridRefCounted
{
public:
    enum wxAttrKind
    {
        Any,
        Cell,
        Row,
        Col,
        Default,
        Merged
    };

    enum wxAttrReadMode
    {
        Unset = -1,
        ReadWrite,
        ReadOnly
    };

    wxGridCellAttr( wxGridCellAttr* aDefault = nullptr );

    wxGridCellAttr* Clone() const;
    void MergeWith( wxGridCellAttr* aOther );

    void SetTextColour( const wxColour& aColour ) { m_textColour = aColour; }
    void SetBackgroundColour( const wxColour& aColour ) { m_backColour = aColour; }
    void SetFont( const wxFont& aFont ) { m_font = aFont; m_hasFont = true; }
    void SetAlignment( int aHAlign, int aVAlign ) { m_hAlign = aHAlign; m_vAlign = aVAlign; }
    void SetSize( int aRows, int aCols ) {}
    void SetOverflow( bool aAllow = true ) { m_overflow = aAllow; }
    void SetReadOnly( bool aReadOnly = true ) { m_readMode = aReadOnly ? ReadOnly : ReadWrite; }

    /// The renderer of the cells (the attribute takes the reference).
    void SetRenderer( wxGridCellRenderer* aRenderer );

    /// The editor of the cells (the attribute takes the reference).
    void SetEditor( wxGridCellEditor* aEditor );

    void SetKind( wxAttrKind aKind ) { m_kind = aKind; }
    wxAttrKind GetKind() const { return m_kind; }

    bool HasTextColour() const { return m_textColour.IsOk(); }
    bool HasBackgroundColour() const { return m_backColour.IsOk(); }
    bool HasFont() const { return m_hasFont; }
    bool HasAlignment() const { return m_hAlign != -1 || m_vAlign != -1; }
    bool HasRenderer() const { return m_renderer != nullptr; }
    bool HasEditor() const { return m_editor != nullptr; }
    bool HasReadWriteMode() const { return m_readMode != Unset; }
    bool HasOverflowMode() const { return true; }

    wxColour GetTextColour() const;
    wxColour GetBackgroundColour() const;
    wxFont GetFont() const;
    void GetAlignment( int* aHAlign, int* aVAlign ) const;
    void GetNonDefaultAlignment( int* aHAlign, int* aVAlign ) const { GetAlignment( aHAlign, aVAlign ); }
    bool GetOverflow() const { return m_overflow; }

    /// The renderer (with a reference for the caller), the default one if none.
    wxGridCellRenderer* GetRenderer( const wxGrid* aGrid, int aRow, int aCol ) const;

    /// The editor (with a reference for the caller), the default one if none.
    wxGridCellEditor* GetEditor( const wxGrid* aGrid, int aRow, int aCol ) const;

    bool IsReadOnly() const { return m_readMode == ReadOnly; }

    void SetDefAttr( wxGridCellAttr* aDefault ) { m_defAttr = aDefault; }

protected:
    ~wxGridCellAttr() override;

private:
    wxGridCellAttr* m_defAttr;

    wxColour            m_textColour;
    wxColour            m_backColour;
    wxFont              m_font;
    bool                m_hasFont;
    int                 m_hAlign;
    int                 m_vAlign;
    bool                m_overflow;
    wxAttrReadMode      m_readMode;
    wxGridCellRenderer* m_renderer;
    wxGridCellEditor*   m_editor;
    wxAttrKind          m_kind;
};


using wxGridCellAttrPtr = wxObjectDataPtr<wxGridCellAttr>;
using wxGridCellRendererPtr = wxObjectDataPtr<wxGridCellRenderer>;
using wxGridCellEditorPtr = wxObjectDataPtr<wxGridCellEditor>;


/// The attributes of the cells, rows and columns of a table.
class KIQT_API wxGridCellAttrProvider
{
public:
    wxGridCellAttrProvider() = default;
    virtual ~wxGridCellAttrProvider();

    virtual wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) const;

    virtual void SetAttr( wxGridCellAttr* aAttr, int aRow, int aCol );
    virtual void SetRowAttr( wxGridCellAttr* aAttr, int aRow );
    virtual void SetColAttr( wxGridCellAttr* aAttr, int aCol );

    void UpdateAttrRows( size_t aPos, int aNumRows );
    void UpdateAttrCols( size_t aPos, int aNumCols );

private:
    std::map<std::pair<int, int>, wxGridCellAttr*> m_cellAttrs;
    std::map<int, wxGridCellAttr*>                 m_rowAttrs;
    std::map<int, wxGridCellAttr*>                 m_colAttrs;
};


// The messages of the tables to their grid
enum wxGridTableRequest
{
    wxGRIDTABLE_NOTIFY_ROWS_INSERTED = 2002,
    wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
    wxGRIDTABLE_NOTIFY_ROWS_DELETED,
    wxGRIDTABLE_NOTIFY_COLS_INSERTED,
    wxGRIDTABLE_NOTIFY_COLS_APPENDED,
    wxGRIDTABLE_NOTIFY_COLS_DELETED
};

class KIQT_API wxGridTableMessage
{
public:
    wxGridTableMessage( wxGridTableBase* aTable = nullptr, int aId = 0, int aInt1 = -1,
                        int aInt2 = -1 ) :
            m_table( aTable ), m_id( aId ), m_comInt1( aInt1 ), m_comInt2( aInt2 )
    {
    }

    wxGridTableBase* GetTableObject() const { return m_table; }
    int GetId() const { return m_id; }
    int GetCommandInt() const { return m_comInt1; }
    int GetCommandInt2() const { return m_comInt2; }

private:
    wxGridTableBase* m_table;
    int              m_id;
    int              m_comInt1;
    int              m_comInt2;
};


/**
 * The data of a grid (wxGridTableBase): the tables of KiCad derive from it.
 */
class KIQT_API wxGridTableBase
{
public:
    wxGridTableBase();
    virtual ~wxGridTableBase();

    // Owns objects through unique_ptr; MSVC emits the copy of a dllexport class eagerly.
    wxGridTableBase( const wxGridTableBase& ) = delete;
    wxGridTableBase& operator=( const wxGridTableBase& ) = delete;

    virtual int GetNumberRows() = 0;
    virtual int GetNumberCols() = 0;
    virtual bool IsEmptyCell( int aRow, int aCol ) { return GetValue( aRow, aCol ).IsEmpty(); }

    virtual wxString GetValue( int aRow, int aCol ) = 0;
    virtual void SetValue( int aRow, int aCol, const wxString& aValue ) = 0;

    virtual wxString GetTypeName( int aRow, int aCol ) { return wxGRID_VALUE_STRING; }
    virtual bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName );
    virtual bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName );

    virtual long GetValueAsLong( int aRow, int aCol );
    virtual double GetValueAsDouble( int aRow, int aCol );
    virtual bool GetValueAsBool( int aRow, int aCol );
    virtual void SetValueAsLong( int aRow, int aCol, long aValue );
    virtual void SetValueAsDouble( int aRow, int aCol, double aValue );
    virtual void SetValueAsBool( int aRow, int aCol, bool aValue );
    virtual void* GetValueAsCustom( int aRow, int aCol, const wxString& aTypeName ) { return nullptr; }
    virtual void SetValueAsCustom( int aRow, int aCol, const wxString& aTypeName, void* aValue ) {}

    virtual void SetView( wxGrid* aGrid ) { m_view = aGrid; }
    virtual wxGrid* GetView() const { return m_view; }

    virtual void Clear() {}
    virtual bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 ) { return false; }
    virtual bool AppendRows( size_t aNumRows = 1 ) { return false; }
    virtual bool DeleteRows( size_t aPos = 0, size_t aNumRows = 1 ) { return false; }
    virtual bool InsertCols( size_t aPos = 0, size_t aNumCols = 1 ) { return false; }
    virtual bool AppendCols( size_t aNumCols = 1 ) { return false; }
    virtual bool DeleteCols( size_t aPos = 0, size_t aNumCols = 1 ) { return false; }

    virtual wxString GetRowLabelValue( int aRow );
    virtual wxString GetColLabelValue( int aCol );
    virtual void SetRowLabelValue( int aRow, const wxString& aLabel ) {}
    virtual void SetColLabelValue( int aCol, const wxString& aLabel ) {}

    void SetAttrProvider( wxGridCellAttrProvider* aProvider );
    wxGridCellAttrProvider* GetAttrProvider() const { return m_attrProvider.get(); }

    virtual bool CanHaveAttributes();
    virtual wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind );
    virtual void SetAttr( wxGridCellAttr* aAttr, int aRow, int aCol );
    virtual void SetRowAttr( wxGridCellAttr* aAttr, int aRow );
    virtual void SetColAttr( wxGridCellAttr* aAttr, int aCol );

    int GetRowsCount() { return GetNumberRows(); }
    int GetColsCount() { return GetNumberCols(); }

private:
    wxGrid*                                 m_view;
    std::unique_ptr<wxGridCellAttrProvider> m_attrProvider;
};


/// A table of strings (the table of CreateGrid()).
class KIQT_API wxGridStringTable : public wxGridTableBase
{
public:
    wxGridStringTable( int aNumRows = 0, int aNumCols = 0 );

    int GetNumberRows() override { return int( m_data.size() ); }
    int GetNumberCols() override { return m_numCols; }
    wxString GetValue( int aRow, int aCol ) override;
    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    void Clear() override;
    bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 ) override;
    bool AppendRows( size_t aNumRows = 1 ) override;
    bool DeleteRows( size_t aPos = 0, size_t aNumRows = 1 ) override;
    bool InsertCols( size_t aPos = 0, size_t aNumCols = 1 ) override;
    bool AppendCols( size_t aNumCols = 1 ) override;
    bool DeleteCols( size_t aPos = 0, size_t aNumCols = 1 ) override;

    wxString GetRowLabelValue( int aRow ) override;
    wxString GetColLabelValue( int aCol ) override;
    void SetRowLabelValue( int aRow, const wxString& aLabel ) override;
    void SetColLabelValue( int aCol, const wxString& aLabel ) override;

private:
    std::vector<std::vector<wxString>> m_data;
    int                                m_numCols;
    std::vector<wxString>              m_rowLabels;
    std::vector<wxString>              m_colLabels;
};


/**
 * The drawing of cells (wxGridCellRenderer): Draw() draws a cell with a wxDC on the QPainter of
 * the grid.
 */
class KIQT_API wxGridCellRenderer : public wxGridRefCounted
{
public:
    virtual void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect,
                       int aRow, int aCol, bool aIsSelected );

    virtual wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                                int aCol ) = 0;
    virtual int GetBestHeight( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                               int aCol, int aWidth )
    {
        return GetBestSize( aGrid, aAttr, aDC, aRow, aCol ).y;
    }

    virtual int GetBestWidth( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                              int aCol, int aHeight )
    {
        return GetBestSize( aGrid, aAttr, aDC, aRow, aCol ).x;
    }

    virtual wxGridCellRenderer* Clone() const = 0;
};


class KIQT_API wxGridCellStringRenderer : public wxGridCellRenderer
{
public:
    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool aIsSelected ) override;
    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                        int aCol ) override;
    wxGridCellRenderer* Clone() const override { return new wxGridCellStringRenderer; }

protected:
    /// The colours of the text and of the background of the cell.
    void SetTextColoursAndFont( const wxGrid& aGrid, const wxGridCellAttr& aAttr, wxDC& aDC,
                                bool aIsSelected );

    wxSize DoGetBestSize( const wxGridCellAttr& aAttr, wxDC& aDC, const wxString& aText );
};


class KIQT_API wxGridCellAutoWrapStringRenderer : public wxGridCellStringRenderer
{
public:
    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool aIsSelected ) override;
    wxGridCellRenderer* Clone() const override { return new wxGridCellAutoWrapStringRenderer; }
};


class KIQT_API wxGridCellNumberRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellRenderer* Clone() const override { return new wxGridCellNumberRenderer; }
};


class KIQT_API wxGridCellFloatRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellFloatRenderer( int aWidth = -1, int aPrecision = -1 ) {}
    wxGridCellRenderer* Clone() const override { return new wxGridCellFloatRenderer; }
};


class KIQT_API wxGridCellBoolRenderer : public wxGridCellRenderer
{
public:
    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect, int aRow,
               int aCol, bool aIsSelected ) override;
    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, int aRow,
                        int aCol ) override;
    wxGridCellRenderer* Clone() const override { return new wxGridCellBoolRenderer; }
};


/// The drawing of the labels of a grid (the renderers of the headers of wxWidgets).
class KIQT_API wxGridHeaderLabelsRenderer
{
public:
    virtual ~wxGridHeaderLabelsRenderer() = default;

    virtual void DrawBorder( const wxGrid& aGrid, wxDC& aDC, wxRect& aRect ) const;
    virtual void DrawLabel( const wxGrid& aGrid, wxDC& aDC, const wxString& aValue,
                            const wxRect& aRect, int aHAlign, int aVAlign,
                            int aTextOrientation ) const;
};

class KIQT_API wxGridColumnHeaderRendererDefault : public wxGridHeaderLabelsRenderer {};
class KIQT_API wxGridRowHeaderRendererDefault : public wxGridHeaderLabelsRenderer {};
class KIQT_API wxGridCornerHeaderRendererDefault : public wxGridHeaderLabelsRenderer {};

using wxGridColumnHeaderRenderer = wxGridHeaderLabelsRenderer;
using wxGridRowHeaderRenderer = wxGridHeaderLabelsRenderer;
using wxGridCornerHeaderRenderer = wxGridHeaderLabelsRenderer;




/**
 * The control of an editor (wxGridCellEditor::m_control): a guarded pointer that the editors
 * of KiCad cast to their control classes.
 */
class wxControlPtrCompat : public QPointer<QWidget>
{
public:
    using QPointer<QWidget>::QPointer;

    wxControlPtrCompat& operator=( QWidget* aWidget )
    {
        QPointer<QWidget>::operator=( aWidget );
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_base_of_v<QWidget, T>
                                                      && !std::is_same_v<T, QWidget>>>
    explicit operator T*() const
    {
        return static_cast<T*>( data() );
    }

    /// The window of the control (m_control->Bind(), GetEventHandler()...).
    wxWindow* operator->() const { return wxWindowFromQt( data() ); }
};


/**
 * The handler pushed on the control of an editor while it is created (wxGridCellEditor::Create()):
 * the end of the edition when the control loses the focus.  Qt handles the keys of the editors
 * (Tab, Return, Escape).
 */
class KIQT_API wxGridCellEditorEvtHandler : public wxEvtHandler
{
public:
    wxGridCellEditorEvtHandler( wxGrid* aGrid, wxGridCellEditor* aEditor );

    void OnKillFocus( wxFocusEvent& aEvent );
    void OnKeyDown( wxKeyEvent& aEvent ) { aEvent.Skip(); }
    void OnChar( wxKeyEvent& aEvent ) { aEvent.Skip(); }

    void SetInSetFocus( bool aInSetFocus ) { m_inSetFocus = aInSetFocus; }
    bool IsInSetFocus() const { return m_inSetFocus; }

    /// End the edition (the value of the editor is saved).
    void DismissEditor();

private:
    wxGrid*           m_grid;
    wxGridCellEditor* m_editor;
    bool              m_inSetFocus;
};


/**
 * The edition of cells (wxGridCellEditor): its control, created once by Create(), is the editor
 * of the cell in the grid.
 */
class KIQT_API wxGridCellEditor : public wxGridRefCounted
{
public:
    wxGridCellEditor();

    bool IsCreated() const { return m_control != nullptr; }

    wxWindow* GetControl() const { return m_control ? wxWindowFromQt( m_control.data() ) : nullptr; }
    wxWindow* GetWindow() const { return GetControl(); }
    void SetControl( wxWidgetArgCompat aControl ) { m_control = aControl.get(); }

    /// The widget of the control.
    QWidget* GetControlWidgetCompat() const { return m_control.data(); }

    wxGridCellAttr* GetCellAttr() const { return m_attr; }
    void SetCellAttr( wxGridCellAttr* aAttr ) { m_attr = aAttr; }

    /**
     * Create the control of the editor, child of @a aParent.  The derived editors make their
     * control, then call this one: it pushes @a aEventHandler on the control.
     */
    virtual void Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler ) = 0;

    virtual void SetSize( const wxRect& aRect );
    virtual void Show( bool aShow, wxGridCellAttr* aAttr = nullptr );
    virtual void PaintBackground( wxDC& aDC, const wxRect& aRect, const wxGridCellAttr& aAttr ) {}

    /// Fill the control with the value of the cell.
    virtual void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) = 0;

    /// The new value of the cell (in @a aNewVal), if it changed.
    virtual bool EndEdit( int aRow, int aCol, const wxGrid* aGrid, const wxString& aOldVal,
                          wxString* aNewVal ) = 0;

    /// Store the value of the control in the cell.
    virtual void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) = 0;

    virtual void Reset() = 0;
    virtual bool IsAcceptedKey( wxKeyEvent& aEvent ) { return true; }
    virtual void StartingKey( wxKeyEvent& aEvent ) {}
    virtual void StartingClick() {}
    virtual void HandleReturn( wxKeyEvent& aEvent ) {}
    virtual void Destroy();
    virtual wxGridCellEditor* Clone() const = 0;
    virtual wxString GetValue() const = 0;

    virtual void SetValidator( const wxValidator& aValidator ) {}

protected:
    ~wxGridCellEditor() override;

    wxControlPtrCompat m_control;
    wxGridCellAttr*   m_attr;
};


class KIQT_API wxGridCellTextEditor : public wxGridCellEditor
{
public:
    explicit wxGridCellTextEditor( size_t aMaxChars = 0 ) : m_maxChars( aMaxChars ) {}

    void Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int aRow, int aCol, const wxGrid* aGrid, const wxString& aOldVal,
                  wxString* aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;
    void StartingKey( wxKeyEvent& aEvent ) override;
    wxGridCellEditor* Clone() const override { return new wxGridCellTextEditor( m_maxChars ); }
    wxString GetValue() const override;

    wxTextCtrl* Text() const { return static_cast<wxTextCtrl*>( m_control.data() ); }

protected:
    /// The creation of the text control (wxGridCellTextEditor::DoCreate()).
    void DoCreate( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler, long aStyle = 0 )
    {
        wxGridCellTextEditor::Create( aParent, aId, aEventHandler );
    }

    wxString m_value;
    size_t   m_maxChars;
};


class KIQT_API wxGridCellNumberEditor : public wxGridCellTextEditor
{
public:
    wxGridCellNumberEditor( int aMin = -1, int aMax = -1 ) {}
    wxGridCellEditor* Clone() const override { return new wxGridCellNumberEditor; }
};


class KIQT_API wxGridCellFloatEditor : public wxGridCellTextEditor
{
public:
    wxGridCellFloatEditor( int aWidth = -1, int aPrecision = -1 ) {}
    wxGridCellEditor* Clone() const override { return new wxGridCellFloatEditor; }
};


class KIQT_API wxGridCellChoiceEditor : public wxGridCellEditor
{
public:
    wxGridCellChoiceEditor( const wxArrayString& aChoices = wxArrayString(),
                            bool aAllowOthers = false );
    wxGridCellChoiceEditor( size_t aCount, const wxString aChoices[], bool aAllowOthers = false );

    void Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int aRow, int aCol, const wxGrid* aGrid, const wxString& aOldVal,
                  wxString* aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;
    wxGridCellEditor* Clone() const override;
    wxString GetValue() const override;

    wxComboBox* Combo() const { return static_cast<wxComboBox*>( m_control.data() ); }

protected:
    wxArrayString m_choices;
    bool          m_allowOthers;
    wxString      m_value;
};


class KIQT_API wxGridCellBoolEditor : public wxGridCellEditor
{
public:
    void Create( wxWindow* aParent, int aId, wxEvtHandler* aEventHandler ) override;
    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int aRow, int aCol, const wxGrid* aGrid, const wxString& aOldVal,
                  wxString* aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;
    wxGridCellEditor* Clone() const override { return new wxGridCellBoolEditor; }
    wxString GetValue() const override;

    static bool IsTrueValue( const wxString& aValue );
    static void UseStringValues( const wxString& aTrue = wxS( "1" ), const wxString& aFalse = wxEmptyString );

protected:
    bool m_value = false;
};


/// The coordinates of a cell.
class KIQT_API wxGridCellCoords
{
public:
    wxGridCellCoords( int aRow = -1, int aCol = -1 ) : m_row( aRow ), m_col( aCol ) {}

    int GetRow() const { return m_row; }
    int GetCol() const { return m_col; }
    void SetRow( int aRow ) { m_row = aRow; }
    void SetCol( int aCol ) { m_col = aCol; }

    bool operator==( const wxGridCellCoords& aOther ) const
    {
        return m_row == aOther.m_row && m_col == aOther.m_col;
    }

    bool operator!=( const wxGridCellCoords& aOther ) const { return !( *this == aOther ); }

private:
    int m_row;
    int m_col;
};

/// An array of cells (wxGridCellCoordsArray of wxWidgets).
class wxGridCellCoordsArray : public std::vector<wxGridCellCoords>
{
public:
    using std::vector<wxGridCellCoords>::vector;

    size_t GetCount() const { return size(); }
    size_t Count() const { return size(); }
    bool   IsEmpty() const { return empty(); }
    void   Add( const wxGridCellCoords& aCoords ) { push_back( aCoords ); }
    void   Clear() { clear(); }

    wxGridCellCoords&       Item( size_t aIndex ) { return ( *this )[aIndex]; }
    const wxGridCellCoords& Item( size_t aIndex ) const { return ( *this )[aIndex]; }
};


/// A block of selected cells.
class KIQT_API wxGridBlockCoords
{
public:
    wxGridBlockCoords( int aTop = -1, int aLeft = -1, int aBottom = -1, int aRight = -1 ) :
            m_top( aTop ), m_left( aLeft ), m_bottom( aBottom ), m_right( aRight )
    {
    }

    int GetTopRow() const { return m_top; }
    int GetLeftCol() const { return m_left; }
    int GetBottomRow() const { return m_bottom; }
    int GetRightCol() const { return m_right; }

private:
    int m_top, m_left, m_bottom, m_right;
};

using wxGridBlocks = std::vector<wxGridBlockCoords>;


/**
 * A grid (wxGrid): a QTableView whose model is its table.
 */
class KIQT_API wxGrid : public wxWindowCompat<QTableView>
{
public:
    enum wxGridSelectionModes
    {
        wxGridSelectCells = 0,
        wxGridSelectRows = 1,
        wxGridSelectColumns = 2,
        wxGridSelectRowsOrColumns = wxGridSelectRows | wxGridSelectColumns,
        wxGridSelectNone = 4
    };

    enum TabBehaviour
    {
        Tab_Stop,
        Tab_Wrap,
        Tab_Leave
    };

    wxGrid( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY, const wxPoint& aPos = wxDefaultPosition,
            const wxSize& aSize = wxDefaultSize, long aStyle = 0,
            const wxString& aName = wxGridNameStr );
    ~wxGrid() override;

    // The table
    bool CreateGrid( int aNumRows, int aNumCols,
                     wxGridSelectionModes aSelectionMode = wxGridSelectCells );
    bool SetTable( wxGridTableBase* aTable, bool aTakeOwnership = false,
                   wxGridSelectionModes aSelectionMode = wxGridSelectCells );
    wxGridTableBase* GetTable() const { return m_table; }
    bool ProcessTableMessage( wxGridTableMessage& aMessage );

    int GetNumberRows() const;
    int GetNumberCols() const;

    wxString GetCellValue( int aRow, int aCol ) const;
    void SetCellValue( int aRow, int aCol, const wxString& aValue );
    wxString GetCellValue( const wxGridCellCoords& aCoords ) const { return GetCellValue( aCoords.GetRow(), aCoords.GetCol() ); }

    bool AppendRows( int aNumRows = 1, bool aUpdateLabels = true );
    bool InsertRows( int aPos = 0, int aNumRows = 1, bool aUpdateLabels = true );
    bool DeleteRows( int aPos = 0, int aNumRows = 1, bool aUpdateLabels = true );
    bool AppendCols( int aNumCols = 1, bool aUpdateLabels = true );
    bool InsertCols( int aPos = 0, int aNumCols = 1, bool aUpdateLabels = true );
    bool DeleteCols( int aPos = 0, int aNumCols = 1, bool aUpdateLabels = true );
    void ClearGrid();

    // The sizes and labels
    void SetColSize( int aCol, int aWidth );
    int GetColSize( int aCol ) const;
    void SetRowSize( int aRow, int aHeight );
    int GetRowSize( int aRow ) const;
    void SetDefaultColSize( int aWidth, bool aResizeExisting = false );
    void SetDefaultRowSize( int aHeight, bool aResizeExisting = false );
    int GetDefaultRowSize() const;
    int GetDefaultColSize() const;
    void SetColMinimalWidth( int aCol, int aWidth );
    void SetColMinimalAcceptableWidth( int aWidth ) { m_colMinimalAcceptableWidthCompat = aWidth; }
    int GetColMinimalAcceptableWidth() const { return m_colMinimalAcceptableWidthCompat; }
    void SetRowMinimalHeight( int aRow, int aHeight ) {}
    void AutoSizeColumn( int aCol, bool aSetAsMin = true );
    void AutoSizeColumns( bool aSetAsMin = true );
    void AutoSizeRow( int aRow, bool aSetAsMin = true );
    void AutoSizeRows( bool aSetAsMin = true );
    void AutoSize();
    void AutoSizeColLabelSize( int aCol );

    void SetColLabelValue( int aCol, const wxString& aLabel );
    wxString GetColLabelValue( int aCol ) const;
    void SetRowLabelValue( int aRow, const wxString& aLabel );
    wxString GetRowLabelValue( int aRow ) const;
    void SetColLabelSize( int aHeight );
    int GetColLabelSize() const;
    void SetRowLabelSize( int aWidth );
    int GetRowLabelSize() const;
    void HideColLabels() { SetColLabelSize( 0 ); }
    void HideRowLabels() { SetRowLabelSize( 0 ); }
    void SetColLabelAlignment( int aHAlign, int aVAlign );
    void SetRowLabelAlignment( int aHAlign, int aVAlign );
    void SetLabelFont( const wxFont& aFont );
    wxFont GetLabelFont() const;
    void SetLabelBackgroundColour( const wxColour& ) {}
    wxColour GetLabelBackgroundColour() const { return wxColour( palette().color( QPalette::Button ) ); }
    void SetLabelTextColour( const wxColour& ) {}
    void SetUseNativeColLabels( bool = true ) {}
    void UseNativeColHeader( bool = true ) {}
    void SetColLabelTextOrientation( int ) {}

    void HideCol( int aCol ) { setColumnHidden( aCol, true ); }
    void ShowCol( int aCol ) { setColumnHidden( aCol, false ); }
    bool IsColShown( int aCol ) const { return !isColumnHidden( aCol ); }
    void HideRow( int aRow ) { setRowHidden( aRow, true ); }
    void ShowRow( int aRow ) { setRowHidden( aRow, false ); }
    bool IsRowShown( int aRow ) const { return !isRowHidden( aRow ); }

    // The spans of the cells (one cell covering aRows x aCols cells)
    void SetCellSize( int aRow, int aCol, int aRows, int aCols )
    {
        setSpan( aRow, aCol, std::max( aRows, 1 ), std::max( aCols, 1 ) );
    }

    void GetCellSize( int aRow, int aCol, int* aRows, int* aCols ) const
    {
        *aRows = rowSpan( aRow, aCol );
        *aCols = columnSpan( aRow, aCol );
    }

    // The attributes of the cells
    void SetDefaultCellAlignment( int aHAlign, int aVAlign );
    void SetCellAlignment( int aRow, int aCol, int aHAlign, int aVAlign );
    void GetCellAlignment( int aRow, int aCol, int* aHAlign, int* aVAlign ) const;
    void SetDefaultCellFont( const wxFont& aFont );
    void SetDefaultCellTextColour( const wxColour& aColour );
    void SetDefaultCellBackgroundColour( const wxColour& aColour );
    wxColour GetDefaultCellBackgroundColour() const;
    wxColour GetDefaultCellTextColour() const;
    wxFont GetDefaultCellFont() const;
    void SetCellFont( int aRow, int aCol, const wxFont& aFont );
    void SetCellTextColour( int aRow, int aCol, const wxColour& aColour );
    void SetCellBackgroundColour( int aRow, int aCol, const wxColour& aColour );
    wxColour GetCellBackgroundColour( int aRow, int aCol ) const;
    wxColour GetCellTextColour( int aRow, int aCol ) const;
    void SetCellRenderer( int aRow, int aCol, wxGridCellRenderer* aRenderer );
    void SetCellEditor( int aRow, int aCol, wxGridCellEditor* aEditor );
    void SetDefaultRenderer( wxGridCellRenderer* aRenderer );
    void SetDefaultEditor( wxGridCellEditor* aEditor );
    wxGridCellRenderer* GetCellRenderer( int aRow, int aCol ) const;
    wxGridCellEditor* GetCellEditor( int aRow, int aCol ) const;
    wxGridCellRenderer* GetDefaultRenderer() const;
    wxGridCellEditor* GetDefaultEditor() const;
    wxGridCellRenderer* GetDefaultRendererForCell( int aRow, int aCol ) const;
    wxGridCellEditor* GetDefaultEditorForCell( int aRow, int aCol ) const;
    void SetColFormatBool( int aCol );
    void SetColFormatNumber( int aCol );
    void SetColFormatFloat( int aCol, int aWidth = -1, int aPrecision = -1 );
    void SetColFormatCustom( int aCol, const wxString& aTypeName ) {}
    void SetColAttr( int aCol, wxGridCellAttr* aAttr );
    void SetRowAttr( int aRow, wxGridCellAttr* aAttr );

    // The order of the columns moved by the user (the visual indexes of the header)

    int GetColPos( int aCol ) const { return horizontalHeader()->visualIndex( aCol ); }
    int GetColAt( int aPos ) const { return horizontalHeader()->logicalIndex( aPos ); }

    void SetColPos( int aCol, int aPos )
    {
        horizontalHeader()->moveSection( horizontalHeader()->visualIndex( aCol ), aPos );
    }

    void ResetColPos()
    {
        for( int col = 0; col < horizontalHeader()->count(); ++col )
            SetColPos( col, col );
    }
    void SetAttr( int aRow, int aCol, wxGridCellAttr* aAttr );

    /// The attributes of a cell, merged with the default ones (with a reference for the caller).
    wxGridCellAttr* GetCellAttr( int aRow, int aCol ) const;
    wxGridCellAttr* GetOrCreateCellAttr( int aRow, int aCol ) const;

    wxGridCellAttrPtr GetOrCreateCellAttrPtr( int aRow, int aCol ) const
    {
        return wxGridCellAttrPtr( GetOrCreateCellAttr( aRow, aCol ) );
    }
    wxGridCellAttr* GetDefaultCellAttr() const { return m_defaultAttr; }

    void SetReadOnly( int aRow, int aCol, bool aReadOnly = true );
    bool IsReadOnly( int aRow, int aCol ) const;
    void EnableEditing( bool aEdit );
    bool IsEditable() const { return m_editable; }

    // The appearance
    void EnableGridLines( bool aEnable = true ) { setShowGrid( aEnable ); }
    bool GridLinesEnabled() const { return showGrid(); }
    void SetGridLineColour( const wxColour& ) {}
    void EnableDragRowSize( bool = true ) {}
    void EnableDragColSize( bool = true ) {}
    void EnableDragGridSize( bool = true ) {}
    void DisableDragGridSize() {}
    void EnableDragColMove( bool = true ) {}
    void EnableDragRowMove( bool = true ) {}
    bool CanDragRowMove() const { return false; }
    void EnableDragCell( bool = true ) {}
    void DisableDragColSize() {}
    void DisableDragRowSize() {}
    void DisableColResize( int aCol ) {}
    void EnableScrolling( bool aHorizontal, bool aVertical ) {}
    void DisableRowResize( int aRow ) {}
    void SetMargins( int, int ) {}
    void SetScrollRate( int, int ) {}
    void SetTabBehaviour( TabBehaviour ) {}
    void SetSelectionBackground( const wxColour& ) {}
    void SetSelectionForeground( const wxColour& ) {}
    void SetCellHighlightPenWidth( int ) {}
    void SetCellHighlightROPenWidth( int ) {}

    /// The column the rows are sorted by (an indicator in its label; the table sorts them).
    void SetSortingColumn( int aCol, bool aAscending = true );
    void UnsetSortingColumn() { SetSortingColumn( wxNOT_FOUND ); }
    int GetSortingColumn() const { return m_sortCol; }
    bool IsSortingBy( int aCol ) const { return m_sortCol != wxNOT_FOUND && m_sortCol == aCol; }
    bool IsSortOrderAscending() const { return m_sortAscending; }

    void BeginBatch() { m_batchCount++; }
    void EndBatch();
    int GetBatchCount() const { return m_batchCount; }
    void ForceRefresh();

    /// Draw a block of cells again (the grid is drawn again).
    void RefreshBlock( int aTopRow, int aLeftCol, int aBottomRow, int aRightCol ) { ForceRefresh(); }
    void Refresh( bool aEraseBackground = true, const wxRect* aRect = nullptr ) { ForceRefresh(); }

    // The selection and the cursor
    void SetSelectionMode( wxGridSelectionModes aMode );
    wxGridSelectionModes GetSelectionMode() const { return m_selectionMode; }
    void SelectRow( int aRow, bool aAddToSelected = false );
    void SelectCol( int aCol, bool aAddToSelected = false );
    void SelectBlock( int aTopRow, int aLeftCol, int aBottomRow, int aRightCol,
                      bool aAddToSelected = false );
    void SelectAll();
    void ClearSelection();
    bool IsSelection() const;
    bool IsInSelection( int aRow, int aCol ) const;
    wxArrayInt GetSelectedRows() const;
    wxArrayInt GetSelectedCols() const;
    wxGridCellCoordsArray GetSelectedCells() const;
    wxGridCellCoordsArray GetSelectionBlockTopLeft() const;
    wxGridCellCoordsArray GetSelectionBlockBottomRight() const;
    wxGridBlocks GetSelectedBlocks() const;
    void DeselectRow( int aRow );
    void DeselectCell( int aRow, int aCol );

    int GetGridCursorRow() const;
    int GetGridCursorCol() const;
    wxGridCellCoords GetGridCursorCoords() const { return wxGridCellCoords( GetGridCursorRow(), GetGridCursorCol() ); }
    void SetGridCursor( int aRow, int aCol );
    void SetGridCursor( const wxGridCellCoords& aCoords ) { SetGridCursor( aCoords.GetRow(), aCoords.GetCol() ); }
    void GoToCell( int aRow, int aCol );
    void MakeCellVisible( int aRow, int aCol );
    bool IsVisible( int aRow, int aCol, bool aWholeCellVisible = true ) const;
    bool MoveCursorDown( bool aExpandSelection );
    bool MoveCursorUp( bool aExpandSelection );
    bool MoveCursorLeft( bool aExpandSelection );
    bool MoveCursorRight( bool aExpandSelection );

    // The edition of the cells
    bool IsCellEditControlEnabled() const;
    bool IsCellEditControlShown() const { return IsCellEditControlEnabled(); }
    bool IsCurrentCellReadOnly() const;
    void EnableCellEditControl( bool aEnable = true );
    void DisableCellEditControl() { EnableCellEditControl( false ); }
    bool CanEnableCellControl() const;
    void ShowCellEditControl();
    void HideCellEditControl();
    void SaveEditControlValue();

    // The windows of the grid
    wxWindow* GetGridWindow() const;
    wxWindow* GetGridColLabelWindow() const;
    wxWindow* GetGridRowLabelWindow() const;
    wxWindow* GetGridCornerLabelWindow() const;

    // The geometry
    wxRect CellToRect( int aRow, int aCol ) const;
    int XToCol( int aX, bool aClipToMinMax = false ) const;
    int YToRow( int aY, bool aClipToMinMax = false ) const;
    wxGridCellCoords XYToCell( int aX, int aY ) const;
    void CalcUnscrolledPosition( int aX, int aY, int* aXX, int* aYY ) const;
    void CalcScrolledPosition( int aX, int aY, int* aXX, int* aYY ) const;

    wxPoint CalcUnscrolledPosition( const wxPoint& aPt ) const
    {
        wxPoint pt;
        CalcUnscrolledPosition( aPt.x, aPt.y, &pt.x, &pt.y );
        return pt;
    }

    wxPoint CalcScrolledPosition( const wxPoint& aPt ) const
    {
        wxPoint pt;
        CalcScrolledPosition( aPt.x, aPt.y, &pt.x, &pt.y );
        return pt;
    }

    /// Send a grid event to the handlers of the grid; return false if it was vetoed.
    bool SendGridEvent( wxEventType aType, int aRow, int aCol );

    // The geometry of the columns and rows (unscrolled)
    int GetColLeft( int aCol ) const;
    int GetColRight( int aCol ) const;
    int GetColWidth( int aCol ) const { return GetColSize( aCol ); }
    int GetRowTop( int aRow ) const;
    int GetRowBottom( int aRow ) const;
    int GetRowHeight( int aRow ) const { return GetRowSize( aRow ); }

    void GetColLabelAlignment( int* aHAlign, int* aVAlign ) const;
    void GetRowLabelAlignment( int* aHAlign, int* aVAlign ) const;
    int GetColLabelTextOrientation() const { return wxHORIZONTAL; }
    void SetDefaultCellOverflow( bool aAllow ) { m_defaultAttr->SetOverflow( aAllow ); }

    /// Draw a text (its lines) in a rectangle of the grid.
    void DrawTextRectangle( wxDC& aDC, const wxString& aText, const wxRect& aRect,
                            int aHAlign = wxALIGN_LEFT, int aVAlign = wxALIGN_TOP,
                            int aTextOrientation = wxHORIZONTAL ) const;
    void DrawTextRectangle( wxDC& aDC, const wxArrayString& aLines, const wxRect& aRect,
                            int aHAlign = wxALIGN_LEFT, int aVAlign = wxALIGN_TOP,
                            int aTextOrientation = wxHORIZONTAL ) const;

    /// The best size of the grid (its size hint).
    virtual wxSize DoGetBestSize() const;

    /// Draw the labels (the headers of the view call them).
    virtual void DrawColLabel( wxDC& aDC, int aCol );
    virtual void DrawRowLabel( wxDC& aDC, int aRow );
    virtual void DrawCornerLabel( wxDC& aDC );

    QSize sizeHint() const override;

    /// As wxWidgets, the grid does not shrink below its best size in a sizer (its minimum size is
    /// its best size, unless set).
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    /**
     * Send a grid event about the current cell.
     *
     * @return -1 if vetoed, 1 if processed, 0 if not processed (as wxGrid::SendEvent()).
     */
    int SendEvent( wxEventType aType, int aRow, int aCol );
    int SendEvent( wxEventType aType, const wxString& aString = wxEmptyString );

    // The state of wxGrid that the derived classes of KiCad use
    bool             m_waitForSlowClick = false;
    bool             m_nativeColumnLabels = false;
    bool             m_cellEditCtrlEnabled = false;
    int              m_colLabelHeight = 0;
    int              m_rowLabelWidth = 0;
    wxGridCellCoords m_currentCellCoords;
    wxWindow*        m_gridWin = nullptr;
    wxWindow*        m_colLabelWin = nullptr;
    wxWindow*        m_rowLabelWin = nullptr;
    wxWindow*        m_cornerLabelWin = nullptr;

public:

    /// The model of the view (for the delegate and the tables).
    KIWX_GRID_MODEL* GetModelCompat() const { return m_model; }

private:
    friend class KIWX_GRID_DELEGATE;
    friend class KIWX_GRID_MODEL;
    friend class KIWX_GRID_HEADER;

    /// The sizes of the labels (m_colLabelHeight, m_rowLabelWidth).
    void updateLabelSizes();

    QItemSelectionModel::SelectionFlags selectionFlags() const;

    wxGridTableBase*     m_table;
    bool                 m_ownTable;
    KIWX_GRID_MODEL*     m_model;
    KIWX_GRID_DELEGATE*  m_delegate;
    wxGridCellAttr*      m_defaultAttr;
    wxGridSelectionModes m_selectionMode;
    bool                 m_editable;
    int                  m_batchCount;
    int                  m_sortCol = wxNOT_FOUND;
    bool                 m_sortAscending = true;
    int                  m_colMinimalAcceptableWidthCompat = 0;
};

using wxGridSelectionModes = wxGrid::wxGridSelectionModes;


/// No refresh of a grid while the locker lives (wxGridUpdateLocker).
class wxGridUpdateLocker
{
public:
    explicit wxGridUpdateLocker( wxGrid* aGrid = nullptr ) { Create( aGrid ); }

    ~wxGridUpdateLocker()
    {
        if( m_grid )
            m_grid->EndBatch();
    }

    void Create( wxGrid* aGrid )
    {
        m_grid = aGrid;

        if( m_grid )
            m_grid->BeginBatch();
    }

    wxGridUpdateLocker( const wxGridUpdateLocker& ) = delete;
    wxGridUpdateLocker& operator=( const wxGridUpdateLocker& ) = delete;

private:
    wxGrid* m_grid = nullptr;
};

#endif // KIWX_GRID_H
