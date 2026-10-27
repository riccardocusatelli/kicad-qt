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
 * @file kiwx_propgrid.h
 * The property grid of the wxWidgets compatibility layer (wxPropertyGrid, wxPropertyGridManager,
 * wxPGProperty and the properties of wxWidgets) on a QTreeWidget.
 *
 * The properties make a tree (the categories, their properties), kept by the state of a page
 * (wxPropertyGridPageState).  The grid shows the properties of a state as the items of the tree:
 * the label, the value and the cells of the other columns (the units in the third one, as
 * wxWidgets).  The selected property is edited in the value column, with a control of the layer
 * (a wxTextCtrl, a wxChoice for the choices) given to the events of the property (OnEvent());
 * the booleans are check boxes.  The changes are committed as with wxWidgets: StringToValue() or
 * IntToValue(), then SetValue(), with wxEVT_PG_CHANGING and wxEVT_PG_CHANGED.
 */

#ifndef KIWX_PROPGRID_H
#define KIWX_PROPGRID_H

#include <kiqt.h>

#include <climits>
#include <map>
#include <memory>
#include <vector>

#include <QTreeWidget>

#include <wx/arrstr.h>
#include <wx/variant.h>
#include <wx/bmpbndl.h>
#include <wx/kiwx_gui.h>

class QTreeWidgetItem;

class wxPGProperty;
class wxPropertyGrid;
class wxPropertyGridInterface;
class wxPropertyGridManager;
class wxPropertyGridPageState;
class wxTextCtrl;


// The styles of the grid and of the manager
#define wxPG_AUTO_SORT                  0x00000010
#define wxPG_HIDE_CATEGORIES            0x00000020
#define wxPG_ALPHABETIC_MODE            ( wxPG_HIDE_CATEGORIES | wxPG_AUTO_SORT )
#define wxPG_BOLD_MODIFIED              0x00000040
#define wxPG_SPLITTER_AUTO_CENTER       0x00000080
#define wxPG_TOOLTIPS                   0x00000100
#define wxPG_HIDE_MARGIN                0x00000200
#define wxPG_STATIC_SPLITTER            0x00000400
#define wxPG_STATIC_LAYOUT              ( wxPG_HIDE_MARGIN | wxPG_STATIC_SPLITTER )
#define wxPG_LIMITED_EDITING            0x00000800
#define wxPG_TOOLBAR                    0x00001000
#define wxPG_DESCRIPTION                0x00002000
#define wxPG_NO_INTERNAL_BORDER         0x00004000
#define wxPG_WINDOW_STYLE_MASK          0x0000FFF0
#define wxPG_DEFAULT_STYLE              0
#define wxPGMAN_DEFAULT_STYLE           0

// The extra styles
#define wxPG_EX_INIT_NOCAT              0x00001000
#define wxPG_EX_NO_FLAT_TOOLBAR         0x00002000
#define wxPG_EX_MODE_BUTTONS            0x00008000
#define wxPG_EX_HELP_AS_TOOLTIPS        0x00010000
#define wxPG_EX_NATIVE_DOUBLE_BUFFERING 0x00080000
#define wxPG_EX_AUTO_UNSPECIFIED_VALUES 0x00200000
#define wxPG_EX_WRITEONLY_BUILTIN_ATTRIBUTES 0x00400000
#define wxPG_EX_HIDE_PAGE_BUTTONS       0x01000000
#define wxPG_EX_MULTIPLE_SELECTION      0x02000000
#define wxPG_EX_ENABLE_TLP_TRACKING     0x04000000
#define wxPG_EX_NO_TOOLBAR_DIVIDER      0x08000000
#define wxPG_EX_TOOLBAR_SEPARATOR       0x10000000
#define wxPG_EX_ALWAYS_ALLOW_FOCUS      0x00001000

// The flags of the iterations: the properties, the categories, the hidden ones (and the ones of
// the collapsed categories)
#define wxPG_ITERATE_PROPERTIES         0x0001
#define wxPG_ITERATE_CATEGORIES         0x0002
#define wxPG_ITERATE_HIDDEN             0x0004
#define wxPG_ITERATE_FIXED_CHILDREN     0x0008
#define wxPG_ITERATE_ALL_PARENTS        ( wxPG_ITERATE_PROPERTIES | wxPG_ITERATE_CATEGORIES )
#define wxPG_ITERATE_ALL_PARENTS_RECURSIVELY wxPG_ITERATE_ALL_PARENTS
#define wxPG_ITERATE_VISIBLE            ( wxPG_ITERATE_PROPERTIES | wxPG_ITERATE_CATEGORIES )
#define wxPG_ITERATE_ALL                ( wxPG_ITERATE_VISIBLE | wxPG_ITERATE_HIDDEN )
#define wxPG_ITERATE_NORMAL             ( wxPG_ITERATE_PROPERTIES | wxPG_ITERATE_HIDDEN )
#define wxPG_ITERATE_DEFAULT            wxPG_ITERATE_NORMAL

// The flags of the selections, of the hiding of the properties, of the values
#define wxPG_SEL_FOCUS                  0x0001
#define wxPG_SEL_FORCE                  0x0002
#define wxPG_SEL_NONVISIBLE             0x0004
#define wxPG_SEL_NOVALIDATE             0x0008
#define wxPG_SEL_DELETING               0x0010
#define wxPG_SEL_SETUNSPEC              0x0020
#define wxPG_SEL_DIALOGVAL              0x0040
#define wxPG_SEL_DONT_SEND_EVENT        0x0080
#define wxPG_SEL_NO_REFRESH             0x0100
#define wxPG_DONT_RECURSE               0x0000
#define wxPG_RECURSE                    0x0020
#define wxPG_FULL_VALUE                 0x0001
#define wxPG_REPORT_ERROR               0x0002
#define wxPG_PROPERTY_SPECIFIC          0x0004
#define wxPG_EDITABLE_VALUE             0x0008
#define wxPG_COMPOSITE_FRAGMENT         0x0010
#define wxPG_UNEDITABLE_COMPOSITE_FRAGMENT 0x0020
#define wxPG_VALUE_IS_CURRENT           0x0040
#define wxPG_PROGRAMMATIC_VALUE         0x0080
#define wxPG_SETVAL_REFRESH_EDITOR      0x0001
#define wxPG_SETVAL_AGGREGATED          0x0002
#define wxPG_SETVAL_FROM_PARENT         0x0004
#define wxPG_SETVAL_BY_USER             0x0008

// The attributes of the properties
#define wxPG_ATTR_UNITS                 wxS( "Units" )
#define wxPG_ATTR_HINT                  wxS( "Hint" )
#define wxPG_ATTR_INLINE_HELP           wxS( "InlineHelp" )
#define wxPG_ATTR_MIN                   wxS( "Min" )
#define wxPG_ATTR_MAX                   wxS( "Max" )
#define wxPG_ATTR_AUTOCOMPLETE          wxS( "AutoComplete" )
#define wxPG_BOOL_USE_CHECKBOX          wxS( "UseCheckbox" )
#define wxPG_BOOL_USE_DOUBLE_CLICK_CYCLING wxS( "UseDClickCycling" )
#define wxPG_FLOAT_PRECISION            wxS( "Precision" )
#define wxPG_STRING_PASSWORD            wxS( "Password" )
#define wxPG_UINT_BASE                  wxS( "Base" )
#define wxPG_UINT_PREFIX                wxS( "Prefix" )

/// The name of a property is its label (the name given to the constructors of the properties).
#define wxPG_LABEL wxString( wxS( "@!" ) )

/// The actions of the keys of the grid.
enum wxPG_KEYBOARD_ACTIONS
{
    wxPG_ACTION_INVALID = 0,
    wxPG_ACTION_NEXT_PROPERTY,
    wxPG_ACTION_PREV_PROPERTY,
    wxPG_ACTION_EXPAND_PROPERTY,
    wxPG_ACTION_COLLAPSE_PROPERTY,
    wxPG_ACTION_CANCEL_EDIT,
    wxPG_ACTION_EDIT,
    wxPG_ACTION_PRESS_BUTTON,
    wxPG_ACTION_MAX
};

/// The behaviors of the validation failures.
enum wxPG_VALIDATION_FAILURE_BEHAVIOR_FLAGS
{
    wxPG_VFB_STAY_IN_PROPERTY = 0x01,
    wxPG_VFB_BEEP = 0x02,
    wxPG_VFB_MARK_CELL = 0x04,
    wxPG_VFB_SHOW_MESSAGE = 0x08,
    wxPG_VFB_SHOW_MESSAGEBOX = 0x10,
    wxPG_VFB_SHOW_MESSAGE_ON_STATUSBAR = 0x20,
    wxPG_VFB_DEFAULT = wxPG_VFB_MARK_CELL | wxPG_VFB_SHOW_MESSAGEBOX,
    wxPG_VFB_UNDEFINED = 0x80
};


/// The events of the grid (they go to the grid, then to its manager).
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_SELECTED, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_CHANGING, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_CHANGED, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_HIGHLIGHTED, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_RIGHT_CLICK, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_DOUBLE_CLICK, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_ITEM_COLLAPSED, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_ITEM_EXPANDED, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_LABEL_EDIT_BEGIN, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_LABEL_EDIT_ENDING, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_COL_BEGIN_DRAG, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_COL_DRAGGING, wxPropertyGridEvent );
wxDECLARE_EXPORTED_EVENT( KIQT_API, wxEVT_PG_COL_END_DRAG, wxPropertyGridEvent );


/// A cell of a property: its text and colours (wxPGCell).
class KIQT_API wxPGCell
{
public:
    wxPGCell() = default;
    wxPGCell( const wxString& aText, const wxBitmapBundle& aBitmap = wxBitmapBundle(),
              const wxColour& aFgCol = wxNullColour, const wxColour& aBgCol = wxNullColour );

    void SetText( const wxString& aText ) { m_text = aText; m_hasText = true; }
    const wxString& GetText() const { return m_text; }
    bool HasText() const { return m_hasText; }

    void SetFgCol( const wxColour& aColour ) { m_fgCol = aColour; }
    const wxColour& GetFgCol() const { return m_fgCol; }
    void SetBgCol( const wxColour& aColour ) { m_bgCol = aColour; }
    const wxColour& GetBgCol() const { return m_bgCol; }
    void SetFont( const wxFont& aFont ) { m_font = aFont; }
    const wxFont& GetFont() const { return m_font; }

private:
    wxString m_text;
    bool     m_hasText = false;
    wxColour m_fgCol;
    wxColour m_bgCol;
    wxFont   m_font;
};


/// The choices of a property: labels and values (wxPGChoices).
class KIQT_API wxPGChoices
{
public:
    wxPGChoices() = default;
    wxPGChoices( const wxArrayString& aLabels, const wxArrayInt& aValues = wxArrayInt() );

    void Add( const wxString& aLabel, int aValue = INT_MAX );
    void Add( const wxArrayString& aLabels, const wxArrayInt& aValues = wxArrayInt() );
    void Clear();

    unsigned int GetCount() const { return (unsigned int) m_labels.size(); }
    bool IsOk() const { return true; }
    const wxString& GetLabel( unsigned int aIndex ) const { return m_labels[aIndex]; }
    int GetValue( unsigned int aIndex ) const { return m_values[aIndex]; }
    wxArrayString GetLabels() const { return m_labels; }

    /// The index of a label (wxNOT_FOUND if none).
    int Index( const wxString& aLabel ) const;

    /// The index of a value (wxNOT_FOUND if none).
    int Index( int aValue ) const;

private:
    wxArrayString    m_labels;
    std::vector<int> m_values;
};


/// The editors of the properties (the classes of wxWidgets, the controls are the grid's).
class KIQT_API wxPGEditor : public wxObject
{
public:
    ~wxPGEditor() override = default;
    virtual wxString GetName() const = 0;
};

class KIQT_API wxPGTextCtrlEditor : public wxPGEditor
{
public:
    wxString GetName() const override { return wxS( "TextCtrl" ); }
};

class KIQT_API wxPGChoiceEditor : public wxPGEditor
{
public:
    wxString GetName() const override { return wxS( "Choice" ); }
};

class KIQT_API wxPGCheckBoxEditor : public wxPGEditor
{
public:
    wxString GetName() const override { return wxS( "CheckBox" ); }
};

class KIQT_API wxPGTextCtrlAndButtonEditor : public wxPGTextCtrlEditor
{
public:
    wxString GetName() const override { return wxS( "TextCtrlAndButton" ); }
};

/// The editors of wxWidgets (wxPGEditor_TextCtrl...).
KIQT_API wxPGEditor* wxPGEditorCompat( const wxString& aName );

#define wxPGEditor_TextCtrl          wxPGEditorCompat( wxS( "TextCtrl" ) )
#define wxPGEditor_Choice            wxPGEditorCompat( wxS( "Choice" ) )
#define wxPGEditor_CheckBox          wxPGEditorCompat( wxS( "CheckBox" ) )
#define wxPGEditor_TextCtrlAndButton wxPGEditorCompat( wxS( "TextCtrlAndButton" ) )


/// The information of a failed validation (wxPGValidationInfo).
class KIQT_API wxPGValidationInfo
{
public:
    const wxString& GetFailureMessage() const { return m_message; }
    void SetFailureMessage( const wxString& aMessage ) { m_message = aMessage; }
    int GetFailureBehavior() const { return m_behavior; }
    void SetFailureBehavior( int aBehavior ) { m_behavior = aBehavior; }

private:
    wxString m_message;
    int      m_behavior = wxPG_VFB_DEFAULT;
};


/**
 * A property (wxPGProperty): its label and name, its value (a wxVariant), its children, the
 * cells of its columns and its attributes.  The conversions of the value are the virtual
 * methods of wxWidgets.
 */
class KIQT_API wxPGProperty : public wxObject
{
public:
    explicit wxPGProperty( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL );
    ~wxPGProperty() override;

    wxPGProperty( const wxPGProperty& ) = delete;
    wxPGProperty& operator=( const wxPGProperty& ) = delete;

    // The conversions of the value

    /// The text of a value (the value of the property if @a aValue is null).
    virtual wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const;
    virtual bool StringToValue( wxVariant& aVariant, const wxString& aText,
                                int aArgFlags = 0 ) const;
    virtual bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const;

    /// The value was set (SetValue()).
    virtual void OnSetValue() {}

    /// An event of the editor of the property: true if processed.
    virtual bool OnEvent( wxPropertyGrid* aPropGrid, wxWindow* aPrimary, wxEvent& aEvent )
    {
        return false;
    }

    virtual wxValidator* DoGetValidator() const { return nullptr; }
    virtual const wxPGEditor* DoGetEditorClass() const { return wxPGEditor_TextCtrl; }

    /// The index of the choice of the value (choice properties), wxNOT_FOUND if none.
    virtual int GetChoiceSelection() const { return wxNOT_FOUND; }

    virtual bool DoSetAttribute( const wxString& aName, wxVariant& aValue ) { return false; }

    // The value

    wxVariant GetValue() const { return m_value; }
    void SetValue( wxVariant aValue, wxVariant* aChangedValue = nullptr,
                   int aFlags = wxPG_SETVAL_REFRESH_EDITOR );
    bool SetValueFromString( const wxString& aText, int aFlags = wxPG_PROGRAMMATIC_VALUE );
    bool SetValueFromInt( long aValue, int aFlags = 0 );
    wxString GetValueAsString( int aArgFlags = 0 ) const;
    wxString GetDisplayedString() const { return GetValueAsString( 0 ); }

    /// Set the value from an event of the editor (it is committed, the editor shows it).
    void SetValueInEvent( const wxVariant& aValue ) const;

    bool IsValueUnspecified() const { return m_value.IsNull(); }
    void SetValueToUnspecified() { m_value.MakeNull(); }

    // The label, the name

    const wxString& GetLabel() const { return m_label; }
    void SetLabel( const wxString& aLabel );
    const wxString& GetName() const { return m_name; }
    const wxString& GetBaseName() const { return m_name; }
    void SetName( const wxString& aName ) { m_name = aName; }

    void SetHelpString( const wxString& aHelp ) { m_help = aHelp; }
    const wxString& GetHelpString() const { return m_help; }

    // The tree

    virtual bool IsCategory() const { return false; }
    bool IsRoot() const { return m_parent == nullptr; }
    wxPGProperty* GetParent() const { return m_parent; }
    wxPGProperty* GetMainParent() const;
    unsigned int GetChildCount() const { return (unsigned int) m_children.size(); }
    wxPGProperty* Item( unsigned int aIndex ) const { return m_children[aIndex]; }
    unsigned int GetIndexInParent() const;
    bool HasVisibleChildren() const;

    /// Add a child (the property takes it).
    wxPGProperty* AppendChild( wxPGProperty* aChild );
    wxPGProperty* InsertChild( int aIndex, wxPGProperty* aChild );

    /// Delete the children.
    void DeleteChildren();

    /// Take a child out of the children (it isn't deleted).
    bool RemoveChildCompat( wxPGProperty* aChild );

    // The state

    bool IsEnabled() const { return m_enabled; }
    void Enable( bool aEnable = true );
    bool IsVisible() const;
    bool IsVisibleCompat() const { return !m_hidden; }
    bool HasFlag( int aFlag ) const;
    bool Hide( bool aHide, int aFlags = wxPG_RECURSE );
    bool IsExpanded() const { return m_expanded; }
    bool IsExpandedCompat() const { return m_expanded; }
    void SetExpandedCompat( bool aExpanded ) { m_expanded = aExpanded; }
    bool IsHiddenCompat() const { return m_hidden; }
    void SetHiddenCompat( bool aHidden ) { m_hidden = aHidden; }

    // The cells, the attributes, the colours

    wxPGCell& GetCell( unsigned int aColumn );
    const wxPGCell& GetCell( unsigned int aColumn ) const;
    bool HasCellCompat( unsigned int aColumn ) const { return m_cells.count( aColumn ) > 0; }
    void SetCell( int aColumn, const wxPGCell& aCell );
    void SetCell( int aColumn, const wxString& aText );

    void SetAttribute( const wxString& aName, wxVariant aValue );
    wxVariant GetAttribute( const wxString& aName ) const;
    wxVariant GetAttribute( const wxString& aName, const wxVariant& aDefault ) const;
    long GetAttributeAsLong( const wxString& aName, long aDefault ) const;

    void SetBackgroundColour( const wxColour& aColour, int aFlags = wxPG_RECURSE );
    void SetTextColour( const wxColour& aColour, int aFlags = wxPG_RECURSE );

    void SetEditor( const wxPGEditor* aEditor ) { m_editor = aEditor; }
    void SetEditor( const wxString& aEditor ) { m_editor = wxPGEditorCompat( aEditor ); }
    const wxPGEditor* GetEditorClass() const { return m_editor ? m_editor : DoGetEditorClass(); }

    void SetClientData( void* aData ) { m_clientData = aData; }
    void* GetClientData() const { return m_clientData; }

    const wxPGChoices& GetChoices() const { return m_choices; }
    void SetChoices( const wxPGChoices& aChoices ) { m_choices = aChoices; }
    bool HasChoicesCompat() const { return m_choices.GetCount() > 0; }

    /// The grid that shows the property (nullptr if none).
    wxPropertyGrid* GetGrid() const;

    /// The state of the page of the property (set by the page when it takes the property).
    void SetStateCompat( wxPropertyGridPageState* aState );
    wxPropertyGridPageState* GetStateCompat() const { return m_state; }

    /// The item of the grid that shows the property.
    QTreeWidgetItem* GetItemCompat() const { return m_item; }
    void SetItemCompat( QTreeWidgetItem* aItem ) { m_item = aItem; }

    /// The value that an event of the editor sets (SetValueInEvent()).
    bool TakeValueInEventCompat( wxVariant& aValue );

protected:
    /// The grid shows the changes of the property.
    void refreshCompat();

    wxVariant   m_value;
    wxPGChoices m_choices;

private:
    friend class wxPropertyGridPageState;

    wxString                         m_label;
    wxString                         m_name;
    wxString                         m_help;
    wxPGProperty*                    m_parent = nullptr;
    std::vector<wxPGProperty*>       m_children;
    std::map<unsigned int, wxPGCell> m_cells;
    std::map<wxString, wxVariant>    m_attributes;
    bool                             m_enabled = true;
    bool                             m_hidden = false;
    bool                             m_expanded = true;
    const wxPGEditor*                m_editor = nullptr;
    void*                            m_clientData = nullptr;
    wxPropertyGridPageState*         m_state = nullptr;
    QTreeWidgetItem*                 m_item = nullptr;

    mutable wxVariant m_valueInEvent;
    mutable bool      m_hasValueInEvent = false;
};


/// A category of properties (wxPropertyCategory).
class KIQT_API wxPropertyCategory : public wxPGProperty
{
public:
    explicit wxPropertyCategory( const wxString& aLabel = wxPG_LABEL,
                                 const wxString& aName = wxPG_LABEL );

    bool IsCategory() const override { return true; }
    wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const override
    {
        return wxEmptyString;
    }
};


/// A text property (wxStringProperty).
class KIQT_API wxStringProperty : public wxPGProperty
{
public:
    explicit wxStringProperty( const wxString& aLabel = wxPG_LABEL,
                               const wxString& aName = wxPG_LABEL,
                               const wxString& aValue = wxEmptyString );
};


/// A text property, with a button (wxLongStringProperty): edited as a text here.
class KIQT_API wxLongStringProperty : public wxStringProperty
{
public:
    using wxStringProperty::wxStringProperty;
};


/// An integer property (wxIntProperty).
class KIQT_API wxIntProperty : public wxPGProperty
{
public:
    explicit wxIntProperty( const wxString& aLabel = wxPG_LABEL,
                            const wxString& aName = wxPG_LABEL, long aValue = 0 );

    wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const override;
    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override;
    bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const override;
};


/// A unsigned integer property (wxUIntProperty).
class KIQT_API wxUIntProperty : public wxIntProperty
{
public:
    explicit wxUIntProperty( const wxString& aLabel = wxPG_LABEL,
                             const wxString& aName = wxPG_LABEL, unsigned long aValue = 0 ) :
            wxIntProperty( aLabel, aName, (long) aValue )
    {
    }
};


/// A floating point property (wxFloatProperty).
class KIQT_API wxFloatProperty : public wxPGProperty
{
public:
    explicit wxFloatProperty( const wxString& aLabel = wxPG_LABEL,
                              const wxString& aName = wxPG_LABEL, double aValue = 0.0 );

    wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const override;
    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override;
};


/// A boolean property (wxBoolProperty), a check box.
class KIQT_API wxBoolProperty : public wxPGProperty
{
public:
    explicit wxBoolProperty( const wxString& aLabel = wxPG_LABEL,
                             const wxString& aName = wxPG_LABEL, bool aValue = false );

    wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const override;
    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override;
    bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const override;
    const wxPGEditor* DoGetEditorClass() const override { return wxPGEditor_CheckBox; }
};


/// A property with choices, its value the index of the choice (wxEnumProperty).
class KIQT_API wxEnumProperty : public wxPGProperty
{
public:
    wxEnumProperty( const wxString& aLabel, const wxString& aName, const wxArrayString& aLabels,
                    const wxArrayInt& aValues = wxArrayInt(), int aValue = 0 );
    wxEnumProperty( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
                    const wxPGChoices& aChoices = wxPGChoices(), int aValue = 0 );

    wxString ValueToString( wxVariant& aValue, int aArgFlags = 0 ) const override;
    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override;
    bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const override;
    int GetChoiceSelection() const override;
    const wxPGEditor* DoGetEditorClass() const override { return wxPGEditor_Choice; }

    size_t GetItemCount() const { return m_choices.GetCount(); }
};


/// A text property with choices (wxEditEnumProperty): edited as a text here.
class KIQT_API wxEditEnumProperty : public wxStringProperty
{
public:
    wxEditEnumProperty( const wxString& aLabel, const wxString& aName,
                        const wxArrayString& aLabels, const wxArrayInt& aValues = wxArrayInt(),
                        const wxString& aValue = wxEmptyString );
};


/**
 * The properties of a page (wxPropertyGridPageState): the root of their tree, and the grid that
 * shows them (nullptr if none).
 */
class KIQT_API wxPropertyGridPageState
{
public:
    wxPropertyGridPageState();
    virtual ~wxPropertyGridPageState();

    wxPropertyGridPageState( const wxPropertyGridPageState& ) = delete;
    wxPropertyGridPageState& operator=( const wxPropertyGridPageState& ) = delete;

    wxPGProperty* DoGetRoot() const { return m_root.get(); }

    /// The width of a column of the grid that shows the page.
    int GetColumnWidth( unsigned int aColumn ) const;
    unsigned int GetColumnCount() const;

    /// The property of a name (nullptr if none).
    wxPGProperty* BaseGetPropertyByName( const wxString& aName ) const;

    /// The properties in the order of the grid, for the flags of an iteration.
    void CollectCompat( wxPGProperty* aParent, int aFlags,
                        std::vector<wxPGProperty*>& aResult ) const;

    /// The grid that shows the page (set by the grid).
    wxPropertyGrid* GetGridCompat() const { return m_grid; }
    void SetGridCompat( wxPropertyGrid* aGrid ) { m_grid = aGrid; }

    /// The properties changed: the grid shows them again.
    void ChangedCompat();

    /// A property is deleted (the grid forgets it).
    void DeletingCompat( wxPGProperty* aProperty );

    /// The category of the properties appended to the root (the last category appended).
    wxPGProperty* GetCurrentCategoryCompat() const { return m_currentCategory; }
    void SetCurrentCategoryCompat( wxPGProperty* aCategory ) { m_currentCategory = aCategory; }

private:
    std::unique_ptr<wxPGProperty> m_root;
    wxPropertyGrid*               m_grid = nullptr;
    wxPGProperty*                 m_currentCategory = nullptr;
};


/**
 * An argument that names a property: the property or its name (wxPGPropArg).
 */
class KIQT_API wxPGPropArgCls
{
public:
    wxPGPropArgCls( const wxPGProperty* aProperty ) : m_property( const_cast<wxPGProperty*>( aProperty ) ) {}
    wxPGPropArgCls( const wxString& aName ) : m_name( aName ) {}
    wxPGPropArgCls( const char* aName ) : m_name( aName ) {}
    wxPGPropArgCls( const wchar_t* aName ) : m_name( aName ) {}

    wxPGProperty* GetPtr( const wxPropertyGridInterface* aInterface ) const;

private:
    wxPGProperty* m_property = nullptr;
    wxString      m_name;
};

using wxPGPropArg = const wxPGPropArgCls&;


/// An iteration over the properties of a page (wxPropertyGridIterator).
class KIQT_API wxPropertyGridIterator
{
public:
    wxPropertyGridIterator() = default;
    wxPropertyGridIterator( wxPropertyGridPageState* aState, int aFlags = wxPG_ITERATE_DEFAULT,
                            wxPGProperty* aStartProperty = nullptr );

    void Next( bool aIterateChildren = true ) { m_index++; }
    void Prev() { m_index--; }
    bool AtEnd() const { return m_index < 0 || m_index >= (int) m_properties.size(); }
    wxPGProperty* GetProperty() const { return AtEnd() ? nullptr : m_properties[m_index]; }

    wxPGProperty* operator*() const { return GetProperty(); }
    wxPropertyGridIterator& operator++() { Next(); return *this; }
    wxPropertyGridIterator operator++( int ) { wxPropertyGridIterator it = *this; Next(); return it; }
    wxPropertyGridIterator& operator--() { Prev(); return *this; }

private:
    std::vector<wxPGProperty*> m_properties;
    int                        m_index = 0;
};

using wxPropertyGridConstIterator = wxPropertyGridIterator;


/**
 * The functions of the grids and of the pages on their properties (wxPropertyGridInterface).
 */
class KIQT_API wxPropertyGridInterface
{
public:
    virtual ~wxPropertyGridInterface() = default;

    /// Add a property at the end of the root (in the last category, as wxWidgets).
    wxPGProperty* Append( wxPGProperty* aProperty );
    wxPGProperty* AppendIn( wxPGPropArg aParent, wxPGProperty* aProperty );

    /// Insert a property before another one, in its parent.
    wxPGProperty* Insert( wxPGPropArg aPriorThis, wxPGProperty* aProperty );
    wxPGProperty* Insert( wxPGPropArg aParent, int aIndex, wxPGProperty* aProperty );

    void Clear();
    void DeleteProperty( wxPGPropArg aProperty );
    wxPGProperty* RemoveProperty( wxPGPropArg aProperty );

    bool HideProperty( wxPGPropArg aProperty, bool aHide = true, int aFlags = wxPG_RECURSE );
    bool EnableProperty( wxPGPropArg aProperty, bool aEnable = true );
    bool DisableProperty( wxPGPropArg aProperty ) { return EnableProperty( aProperty, false ); }
    bool IsPropertyEnabled( wxPGPropArg aProperty ) const;

    bool Expand( wxPGPropArg aProperty );
    bool Collapse( wxPGPropArg aProperty );
    bool ExpandAll( bool aExpand = true );
    bool CollapseAll() { return ExpandAll( false ); }

    wxPGProperty* GetProperty( const wxString& aName ) const;
    wxPGProperty* GetPropertyByName( const wxString& aName ) const { return GetProperty( aName ); }
    wxPGProperty* GetPropertyByLabel( const wxString& aLabel ) const;
    wxPGProperty* GetFirst( int aFlags = wxPG_ITERATE_ALL );
    wxPGProperty* GetRoot() const;

    wxPropertyGridIterator GetIterator( int aFlags = wxPG_ITERATE_DEFAULT,
                                        wxPGProperty* aFirst = nullptr );

    wxVariant GetPropertyValue( wxPGPropArg aProperty );
    wxString GetPropertyValueAsString( wxPGPropArg aProperty ) const;
    void SetPropertyValue( wxPGPropArg aProperty, wxVariant aValue );
    void SetPropertyLabel( wxPGPropArg aProperty, const wxString& aLabel );
    void SetPropertyCell( wxPGPropArg aProperty, int aColumn, const wxString& aText );
    void SetPropertyAttribute( wxPGPropArg aProperty, const wxString& aName, wxVariant aValue,
                               long aFlags = 0 );

    /// The state of the properties.
    virtual wxPropertyGridPageState* GetStateCompat() const = 0;
};


/// An event of the grid (wxPropertyGridEvent).
class KIQT_API wxPropertyGridEvent : public wxCommandEvent
{
public:
    explicit wxPropertyGridEvent( wxEventType aType = wxEVT_NULL, int aId = 0 ) :
            wxCommandEvent( aType, aId )
    {
    }

    wxPGProperty* GetProperty() const { return m_property; }
    void SetProperty( wxPGProperty* aProperty ) { m_property = aProperty; }
    wxString GetPropertyName() const;
    wxVariant GetPropertyValue() const;
    wxVariant GetValue() const { return m_value; }
    void SetValueCompat( const wxVariant& aValue ) { m_value = aValue; }
    unsigned int GetColumn() const { return m_column; }
    void SetColumn( unsigned int aColumn ) { m_column = aColumn; }

    bool CanVeto() const { return m_canVeto; }
    void SetCanVeto( bool aCanVeto ) { m_canVeto = aCanVeto; }
    void Veto( bool aVeto = true ) { m_vetoed = aVeto; }
    bool WasVetoed() const { return m_vetoed; }
    void SetValidationFailureBehavior( int aFlags ) {}

private:
    wxPGProperty* m_property = nullptr;
    wxVariant     m_value;
    unsigned int  m_column = 1;
    bool          m_canVeto = false;
    bool          m_vetoed = false;
};

#define wxPropertyGridEventHandler( func ) ( &func )


/**
 * A property grid (wxPropertyGrid) on a QTreeWidget: the properties of a page (its own, or the
 * page of its manager), one per item.  The selected property is edited in the value column.
 */
class KIQT_API wxPropertyGrid : public wxWindowCompat<QTreeWidget>, public wxPropertyGridInterface
{
public:
    explicit wxPropertyGrid( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                             const wxPoint& aPos = wxDefaultPosition,
                             const wxSize& aSize = wxDefaultSize,
                             long aStyle = wxPG_DEFAULT_STYLE,
                             const wxString& aName = wxS( "wxPropertyGrid" ) );
    ~wxPropertyGrid() override;

    wxPropertyGridPageState* GetStateCompat() const override { return m_state; }
    wxPropertyGridPageState* GetState() const { return m_state; }

    /// Show the properties of another page (the pages of a manager).
    void SetStateCompat( wxPropertyGridPageState* aState );

    wxPropertyGrid* GetGrid() { return this; }

    // The selection and the editor

    wxPGProperty* GetSelection() const { return m_selected; }
    bool SelectProperty( wxPGPropArg aProperty, bool aFocus = false );
    bool DoSelectProperty( wxPGProperty* aProperty, unsigned int aFlags = 0 );
    bool ClearSelection( bool aValidation = false );
    bool EnsureVisible( wxPGPropArg aProperty );

    wxWindow* GetEditorControl() const;
    wxTextCtrl* GetEditorTextCtrl() const;
    wxWindow* GetPrimaryEditor() const { return GetEditorControl(); }

    /// Commit the text of the editor to the property (false if the property refuses it).
    bool CommitChangesFromEditor( wxUint32 aFlags = 0 );
    void EditorsValueWasModified() { m_editorModified = true; }
    void EditorsValueWasNotModified() { m_editorModified = false; }
    bool IsEditorsValueModified() const { return m_editorModified; }
    void RefreshEditor();
    void RefreshProperty( wxPGProperty* aProperty );
    bool IsEditorFocused() const;

    // The keys

    void DedicateKey( int aKeyCode ) {}
    void AddActionTrigger( int aAction, int aKeyCode, int aModifiers = 0 );
    void ClearActionTriggers( int aAction );

    // The columns

    void SetColumnCount( int aColumnCount );
    unsigned int GetColumnCount() const { return (unsigned int) columnCount(); }
    void SetColumnProportion( unsigned int aColumn, int aProportion );
    void ResetColumnSizes( bool aEnableAutoResizing = false );
    wxSize FitColumns();
    void SetSplitterPosition( int aPosition, int aColumn = 0 );
    int GetSplitterPosition( unsigned int aSplitterIndex = 0 ) const;
    void CenterSplitter( bool aEnableAutoResizing = false );
    void SetColumnTitleCompat( unsigned int aColumn, const wxString& aTitle );

    // The colours

    void SetCellDisabledTextColour( const wxColour& aColour );
    void SetCellBackgroundColour( const wxColour& aColour ) {}
    void SetCellTextColour( const wxColour& aColour ) {}
    void SetCaptionBackgroundColour( const wxColour& aColour ) {}
    void SetCaptionTextColour( const wxColour& aColour ) {}
    void SetMarginColour( const wxColour& aColour ) {}
    void SetLineColour( const wxColour& aColour ) {}
    void SetEmptySpaceColour( const wxColour& aColour ) {}
    void SetSelectionBackgroundColour( const wxColour& aColour ) {}
    void SetSelectionTextColour( const wxColour& aColour ) {}
    const wxPGCell& GetPropertyDefaultCell() const { return m_defaultCell; }
    const wxPGCell& GetCategoryDefaultCell() const { return m_categoryCell; }

    void SetExtraStyle( long aExtraStyle ) {}
    void SetVerticalSpacing( int aSpacing ) {}
    void MakeColumnEditable( unsigned int aColumn, bool aEditable = true ) {}

    // Internal: the items of the properties

    /// Show the properties of the page again (their tree changed).
    void RebuildCompat();

    /// Show a property again (its value, its cells changed).
    void RefreshItemCompat( wxPGProperty* aProperty );

    /// A property is deleted.
    void ForgetPropertyCompat( wxPGProperty* aProperty );

    /// The property of an item (nullptr if none).
    static wxPGProperty* PropertyOfItemCompat( QTreeWidgetItem* aItem );

    /// The page of the grid is deleted.
    void DetachStateCompat( wxPropertyGridPageState* aState );

    /// The manager of the grid (its events go to the manager too).
    void SetManagerCompat( wxPropertyGridManager* aManager ) { m_manager = aManager; }

protected:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override;
    void resizeEvent( QResizeEvent* aEvent ) override;

private:
    void ensureBuilt();
    void select( wxPGProperty* aProperty, bool aFocus, bool aSendEvent );
    void applyColumnSizes();
    void buildItems( wxPGProperty* aParent, QTreeWidgetItem* aParentItem );
    void updateItem( wxPGProperty* aProperty );
    void createEditor();
    void destroyEditor();
    void editorChoice( int aIndex );
    void toggleCheckBox( wxPGProperty* aProperty );
    void moveSelection( int aDirection );
    bool sendEvent( wxEventType aType, wxPGProperty* aProperty, const wxVariant* aValue = nullptr );
    bool changeValue( wxPGProperty* aProperty, wxVariant aValue );
    bool propertyEvent( QEvent* aEvent );

    std::unique_ptr<wxPropertyGridPageState> m_ownState;
    wxPropertyGridPageState*                 m_state = nullptr;
    wxPropertyGridManager*                   m_manager = nullptr;
    bool                                     m_dirty = false;
    bool                                     m_selecting = false;
    wxPGProperty*                            m_selected = nullptr;
    wxWindow*                                m_editor = nullptr;
    wxString                                 m_editorInitialText;
    bool                                     m_editorModified = false;
    bool                                     m_rebuilding = false;
    bool                                     m_committing = false;
    wxPGCell                                 m_defaultCell;
    wxPGCell                                 m_categoryCell;
    wxColour                                 m_disabledTextColour;
    std::vector<int>                         m_proportions;
    std::map<int, std::vector<int>>          m_actionTriggers;
};


/// A page of a manager (wxPropertyGridPage): its properties are shown by the grid of the manager.
class KIQT_API wxPropertyGridPage : public wxEvtHandler,
                                    public wxPropertyGridInterface,
                                    public wxPropertyGridPageState
{
public:
    explicit wxPropertyGridPage( wxPropertyGridManager* aManager = nullptr,
                                 const wxString& aLabel = wxEmptyString );
    ~wxPropertyGridPage() override = default;

    wxPropertyGridPageState* GetStateCompat() const override
    {
        return const_cast<wxPropertyGridPage*>( this );
    }

    wxPropertyGrid* GetGrid() const;
    const wxString& GetLabelCompat() const { return m_label; }
    void SetManagerCompat( wxPropertyGridManager* aManager ) { m_manager = aManager; }

private:
    wxPropertyGridManager* m_manager;
    wxString               m_label;
};


/**
 * A manager of pages of properties (wxPropertyGridManager): its grid shows the selected page,
 * with the header of its columns.
 */
class KIQT_API wxPropertyGridManager : public wxWindowCompat<QWidget>,
                                       public wxPropertyGridInterface
{
public:
    explicit wxPropertyGridManager( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                                    const wxPoint& aPos = wxDefaultPosition,
                                    const wxSize& aSize = wxDefaultSize,
                                    long aStyle = wxPGMAN_DEFAULT_STYLE,
                                    const wxString& aName = wxS( "wxPropertyGridManager" ) );
    ~wxPropertyGridManager() override;

    wxPropertyGridPageState* GetStateCompat() const override;

    wxPropertyGrid* GetGrid() const { return m_grid; }

    // The pages

    wxPropertyGridPage* AddPage( const wxString& aLabel = wxEmptyString,
                                 const wxBitmapBundle& aBitmap = wxBitmapBundle(),
                                 wxPropertyGridPage* aPage = nullptr );
    wxPropertyGridPage* GetPage( unsigned int aIndex ) const;
    wxPropertyGridPage* GetPage( const wxString& aName ) const;
    wxPropertyGridPage* GetCurrentPage() const;
    size_t GetPageCount() const { return m_pages.size(); }
    int GetSelectedPage() const { return m_selectedPage; }
    void SelectPage( int aIndex );
    void SelectPage( wxPropertyGridPage* aPage );
    bool RemovePage( int aIndex );

    // The columns

    void SetColumnCount( int aColumnCount, int aPage = -1 );
    int GetColumnCount( int aPage = -1 ) const;
    void SetColumnTitle( int aColumn, const wxString& aTitle );
    void ShowHeader( bool aShow = true );
    void SetDescription( const wxString& aLabel, const wxString& aContent ) {}
    void SetDescBoxHeight( int aHeight, bool aRefresh = true ) {}
    void SetSplitterLeft( bool aSubProps = false, bool aAllPages = true ) {}
    void SetSplitterPosition( int aPosition, int aColumn = 0 );
    void SetExtraStyle( long aExtraStyle ) {}

    bool SelectProperty( wxPGPropArg aProperty, bool aFocus = false );
    bool ClearSelection( bool aValidation = false );
    bool CommitChangesFromEditor( wxUint32 aFlags = 0 );

private:
    wxPropertyGrid*                                  m_grid;
    std::vector<std::unique_ptr<wxPropertyGridPage>> m_pages;
    int                                              m_selectedPage = -1;
    int                                              m_columnCount = 2;
};

#endif // KIWX_PROPGRID_H
