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

#include <wx/kiwx_propgrid.h>

#include <algorithm>

#include <QApplication>
#include <QComboBox>
#include <QFocusEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPalette>
#include <QTimer>
#include <QVBoxLayout>

#include <wx/kiwx_gui.h>


wxDEFINE_EVENT( wxEVT_PG_SELECTED, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_CHANGING, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_CHANGED, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_HIGHLIGHTED, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_RIGHT_CLICK, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_DOUBLE_CLICK, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_ITEM_COLLAPSED, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_ITEM_EXPANDED, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_LABEL_EDIT_BEGIN, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_LABEL_EDIT_ENDING, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_COL_BEGIN_DRAG, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_COL_DRAGGING, wxPropertyGridEvent );
wxDEFINE_EVENT( wxEVT_PG_COL_END_DRAG, wxPropertyGridEvent );


namespace
{

/// The role of the items that keeps their property.
const int PROPERTY_ROLE = Qt::UserRole + 1;

/// The column of the values.
const int VALUE_COLUMN = 1;

/// The column of the units (when the property has no cell of its own there).
const unsigned int UNITS_COLUMN = 2;


QColor toQColor( const wxColour& aColour )
{
    return QColor( aColour.Red(), aColour.Green(), aColour.Blue(), aColour.Alpha() );
}


wxColour toWxColour( const QColor& aColour )
{
    return wxColour( aColour.red(), aColour.green(), aColour.blue(), aColour.alpha() );
}


/// The property is shown as a check box.
bool isCheckBox( const wxPGProperty* aProperty )
{
    return aProperty->GetEditorClass() == wxPGEditor_CheckBox;
}

} // namespace


// Cells, choices, editors ------------------------------------------------------------------

wxPGCell::wxPGCell( const wxString& aText, const wxBitmapBundle& aBitmap, const wxColour& aFgCol,
                    const wxColour& aBgCol ) :
        m_text( aText ),
        m_hasText( true ),
        m_fgCol( aFgCol ),
        m_bgCol( aBgCol )
{
}


wxPGChoices::wxPGChoices( const wxArrayString& aLabels, const wxArrayInt& aValues )
{
    Add( aLabels, aValues );
}


void wxPGChoices::Add( const wxString& aLabel, int aValue )
{
    m_labels.Add( aLabel );
    m_values.push_back( aValue == INT_MAX ? (int) m_values.size() : aValue );
}


void wxPGChoices::Add( const wxArrayString& aLabels, const wxArrayInt& aValues )
{
    for( size_t ii = 0; ii < aLabels.size(); ++ii )
        Add( aLabels[ii], ii < aValues.size() ? aValues[ii] : INT_MAX );
}


void wxPGChoices::Clear()
{
    m_labels.Clear();
    m_values.clear();
}


int wxPGChoices::Index( const wxString& aLabel ) const
{
    for( size_t ii = 0; ii < m_labels.size(); ++ii )
    {
        if( m_labels[ii] == aLabel )
            return (int) ii;
    }

    return wxNOT_FOUND;
}


int wxPGChoices::Index( int aValue ) const
{
    for( size_t ii = 0; ii < m_values.size(); ++ii )
    {
        if( m_values[ii] == aValue )
            return (int) ii;
    }

    return wxNOT_FOUND;
}


wxPGEditor* wxPGEditorCompat( const wxString& aName )
{
    static wxPGTextCtrlEditor          textCtrl;
    static wxPGChoiceEditor            choice;
    static wxPGCheckBoxEditor          checkBox;
    static wxPGTextCtrlAndButtonEditor textCtrlAndButton;

    if( aName == wxS( "Choice" ) || aName == wxS( "ComboBox" ) )
        return &choice;

    if( aName == wxS( "CheckBox" ) )
        return &checkBox;

    if( aName == wxS( "TextCtrlAndButton" ) )
        return &textCtrlAndButton;

    return &textCtrl;
}


// Properties -------------------------------------------------------------------------------

wxPGProperty::wxPGProperty( const wxString& aLabel, const wxString& aName ) :
        m_label( aLabel == wxPG_LABEL ? wxString() : aLabel ),
        m_name( aName == wxPG_LABEL ? m_label : aName )
{
}


wxPGProperty::~wxPGProperty()
{
    if( m_state )
        m_state->DeletingCompat( this );

    DeleteChildren();
}


wxString wxPGProperty::ValueToString( wxVariant& aValue, int aArgFlags ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    return aValue.GetString();
}


bool wxPGProperty::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    aVariant = aText;
    return true;
}


bool wxPGProperty::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
{
    aVariant = (long) aNumber;
    return true;
}


void wxPGProperty::SetValue( wxVariant aValue, wxVariant* aChangedValue, int aFlags )
{
    m_value = aValue;
    OnSetValue();
    refreshCompat();
}


bool wxPGProperty::SetValueFromString( const wxString& aText, int aFlags )
{
    wxVariant value( m_value );

    if( !StringToValue( value, aText, aFlags ) )
        return false;

    SetValue( value );
    return true;
}


bool wxPGProperty::SetValueFromInt( long aValue, int aFlags )
{
    wxVariant value( m_value );

    if( !IntToValue( value, (int) aValue, aFlags ) )
        return false;

    SetValue( value );
    return true;
}


wxString wxPGProperty::GetValueAsString( int aArgFlags ) const
{
    wxVariant value( m_value );
    return ValueToString( value, aArgFlags );
}


void wxPGProperty::SetValueInEvent( const wxVariant& aValue ) const
{
    m_valueInEvent = aValue;
    m_hasValueInEvent = true;
}


bool wxPGProperty::TakeValueInEventCompat( wxVariant& aValue )
{
    if( !m_hasValueInEvent )
        return false;

    aValue = m_valueInEvent;
    m_hasValueInEvent = false;
    return true;
}


void wxPGProperty::SetLabel( const wxString& aLabel )
{
    m_label = aLabel;
    refreshCompat();
}


wxPGProperty* wxPGProperty::GetMainParent() const
{
    const wxPGProperty* property = this;

    while( property->m_parent && property->m_parent->m_parent && !property->m_parent->IsCategory() )
        property = property->m_parent;

    return const_cast<wxPGProperty*>( property );
}


unsigned int wxPGProperty::GetIndexInParent() const
{
    if( !m_parent )
        return 0;

    auto it = std::find( m_parent->m_children.begin(), m_parent->m_children.end(), this );
    return (unsigned int) ( it - m_parent->m_children.begin() );
}


bool wxPGProperty::HasVisibleChildren() const
{
    for( wxPGProperty* child : m_children )
    {
        if( !child->m_hidden )
            return true;
    }

    return false;
}


wxPGProperty* wxPGProperty::AppendChild( wxPGProperty* aChild )
{
    return InsertChild( -1, aChild );
}


wxPGProperty* wxPGProperty::InsertChild( int aIndex, wxPGProperty* aChild )
{
    if( !aChild )
        return nullptr;

    aChild->m_parent = this;

    if( aIndex < 0 || aIndex >= (int) m_children.size() )
        m_children.push_back( aChild );
    else
        m_children.insert( m_children.begin() + aIndex, aChild );

    aChild->SetStateCompat( m_state );

    if( m_state )
        m_state->ChangedCompat();

    return aChild;
}


void wxPGProperty::DeleteChildren()
{
    // The children are taken first: their destruction can't see them in the tree
    std::vector<wxPGProperty*> children;
    children.swap( m_children );

    for( wxPGProperty* child : children )
    {
        child->m_parent = nullptr;
        delete child;
    }

    if( m_state && !children.empty() )
        m_state->ChangedCompat();
}


bool wxPGProperty::RemoveChildCompat( wxPGProperty* aChild )
{
    auto it = std::find( m_children.begin(), m_children.end(), aChild );

    if( it == m_children.end() )
        return false;

    m_children.erase( it );
    aChild->m_parent = nullptr;
    return true;
}


void wxPGProperty::Enable( bool aEnable )
{
    m_enabled = aEnable;

    for( wxPGProperty* child : m_children )
        child->Enable( aEnable );

    refreshCompat();
}


bool wxPGProperty::IsVisible() const
{
    if( m_hidden )
        return false;

    for( const wxPGProperty* parent = m_parent; parent && parent->m_parent; parent = parent->m_parent )
    {
        if( parent->m_hidden || !parent->m_expanded )
            return false;
    }

    return true;
}


bool wxPGProperty::HasFlag( int aFlag ) const
{
    return false;
}


bool wxPGProperty::Hide( bool aHide, int aFlags )
{
    m_hidden = aHide;

    if( aFlags & wxPG_RECURSE )
    {
        for( wxPGProperty* child : m_children )
            child->Hide( aHide, aFlags );
    }

    if( m_state )
        m_state->ChangedCompat();

    return true;
}


wxPGCell& wxPGProperty::GetCell( unsigned int aColumn )
{
    return m_cells[aColumn];
}


const wxPGCell& wxPGProperty::GetCell( unsigned int aColumn ) const
{
    static const wxPGCell none;
    auto it = m_cells.find( aColumn );
    return it != m_cells.end() ? it->second : none;
}


void wxPGProperty::SetCell( int aColumn, const wxPGCell& aCell )
{
    m_cells[aColumn] = aCell;
    refreshCompat();
}


void wxPGProperty::SetCell( int aColumn, const wxString& aText )
{
    m_cells[aColumn].SetText( aText );
    refreshCompat();
}


void wxPGProperty::SetAttribute( const wxString& aName, wxVariant aValue )
{
    if( !DoSetAttribute( aName, aValue ) )
        m_attributes[aName] = aValue;

    refreshCompat();
}


wxVariant wxPGProperty::GetAttribute( const wxString& aName ) const
{
    auto it = m_attributes.find( aName );
    return it != m_attributes.end() ? it->second : wxVariant();
}


wxVariant wxPGProperty::GetAttribute( const wxString& aName, const wxVariant& aDefault ) const
{
    auto it = m_attributes.find( aName );
    return it != m_attributes.end() ? it->second : aDefault;
}


long wxPGProperty::GetAttributeAsLong( const wxString& aName, long aDefault ) const
{
    auto it = m_attributes.find( aName );
    return it != m_attributes.end() && !it->second.IsNull() ? it->second.GetLong() : aDefault;
}


void wxPGProperty::SetBackgroundColour( const wxColour& aColour, int aFlags )
{
    for( auto& [column, cell] : m_cells )
        cell.SetBgCol( aColour );

    GetCell( 0 ).SetBgCol( aColour );
    GetCell( VALUE_COLUMN ).SetBgCol( aColour );

    if( aFlags & wxPG_RECURSE )
    {
        for( wxPGProperty* child : m_children )
            child->SetBackgroundColour( aColour, aFlags );
    }

    refreshCompat();
}


void wxPGProperty::SetTextColour( const wxColour& aColour, int aFlags )
{
    for( auto& [column, cell] : m_cells )
        cell.SetFgCol( aColour );

    GetCell( 0 ).SetFgCol( aColour );
    GetCell( VALUE_COLUMN ).SetFgCol( aColour );

    if( aFlags & wxPG_RECURSE )
    {
        for( wxPGProperty* child : m_children )
            child->SetTextColour( aColour, aFlags );
    }

    refreshCompat();
}


wxPropertyGrid* wxPGProperty::GetGrid() const
{
    return m_state ? m_state->GetGridCompat() : nullptr;
}


void wxPGProperty::SetStateCompat( wxPropertyGridPageState* aState )
{
    m_state = aState;

    for( wxPGProperty* child : m_children )
        child->SetStateCompat( aState );
}


void wxPGProperty::refreshCompat()
{
    if( wxPropertyGrid* grid = GetGrid() )
        grid->RefreshItemCompat( this );
}


wxPropertyCategory::wxPropertyCategory( const wxString& aLabel, const wxString& aName ) :
        wxPGProperty( aLabel, aName )
{
}


wxStringProperty::wxStringProperty( const wxString& aLabel, const wxString& aName,
                                    const wxString& aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_value = aValue;
}


wxIntProperty::wxIntProperty( const wxString& aLabel, const wxString& aName, long aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_value = aValue;
}


wxString wxIntProperty::ValueToString( wxVariant& aValue, int aArgFlags ) const
{
    return aValue.IsNull() ? wxString() : wxString::Format( wxS( "%ld" ), aValue.GetLong() );
}


bool wxIntProperty::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    long value = 0;

    if( !wxString( aText ).Trim().Trim( false ).ToLong( &value ) )
        return false;

    aVariant = value;
    return true;
}


bool wxIntProperty::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
{
    aVariant = (long) aNumber;
    return true;
}


wxFloatProperty::wxFloatProperty( const wxString& aLabel, const wxString& aName, double aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_value = aValue;
}


wxString wxFloatProperty::ValueToString( wxVariant& aValue, int aArgFlags ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    long precision = GetAttributeAsLong( wxPG_FLOAT_PRECISION, -1 );

    if( precision >= 0 )
        return wxString::FromCDouble( aValue.GetDouble(), (int) precision );

    return wxString::Format( wxS( "%g" ), aValue.GetDouble() );
}


bool wxFloatProperty::StringToValue( wxVariant& aVariant, const wxString& aText,
                                     int aArgFlags ) const
{
    double value = 0.0;

    wxString text = wxString( aText ).Trim().Trim( false );

    if( !text.ToCDouble( &value ) && !text.ToDouble( &value ) )
        return false;

    aVariant = value;
    return true;
}


wxBoolProperty::wxBoolProperty( const wxString& aLabel, const wxString& aName, bool aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_value = aValue;
}


wxString wxBoolProperty::ValueToString( wxVariant& aValue, int aArgFlags ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    return aValue.GetBool() ? wxS( "True" ) : wxS( "False" );
}


bool wxBoolProperty::StringToValue( wxVariant& aVariant, const wxString& aText,
                                    int aArgFlags ) const
{
    wxString text = aText.Lower();
    aVariant = text == wxS( "true" ) || text == wxS( "1" ) || text == wxS( "yes" );
    return true;
}


bool wxBoolProperty::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
{
    aVariant = aNumber != 0;
    return true;
}


wxEnumProperty::wxEnumProperty( const wxString& aLabel, const wxString& aName,
                                const wxArrayString& aLabels, const wxArrayInt& aValues,
                                int aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_choices.Add( aLabels, aValues );
    m_value = (long) aValue;
}


wxEnumProperty::wxEnumProperty( const wxString& aLabel, const wxString& aName,
                                const wxPGChoices& aChoices, int aValue ) :
        wxPGProperty( aLabel, aName )
{
    m_choices = aChoices;
    m_value = (long) aValue;
}


wxString wxEnumProperty::ValueToString( wxVariant& aValue, int aArgFlags ) const
{
    if( aValue.IsNull() )
        return wxEmptyString;

    // The value of a choice, or its text
    if( aValue.GetType() == wxS( "string" ) )
        return aValue.GetString();

    int index = m_choices.Index( (int) aValue.GetLong() );
    return index != wxNOT_FOUND ? m_choices.GetLabel( index ) : wxString();
}


bool wxEnumProperty::StringToValue( wxVariant& aVariant, const wxString& aText,
                                    int aArgFlags ) const
{
    int index = m_choices.Index( aText );

    if( index == wxNOT_FOUND )
        return false;

    aVariant = (long) m_choices.GetValue( index );
    return true;
}


bool wxEnumProperty::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
{
    if( aNumber < 0 || aNumber >= (int) m_choices.GetCount() )
        return false;

    aVariant = (long) m_choices.GetValue( aNumber );
    return true;
}


int wxEnumProperty::GetChoiceSelection() const
{
    if( m_value.IsNull() )
        return wxNOT_FOUND;

    if( m_value.GetType() == wxS( "string" ) )
        return m_choices.Index( m_value.GetString() );

    return m_choices.Index( (int) m_value.GetLong() );
}


wxEditEnumProperty::wxEditEnumProperty( const wxString& aLabel, const wxString& aName,
                                        const wxArrayString& aLabels, const wxArrayInt& aValues,
                                        const wxString& aValue ) :
        wxStringProperty( aLabel, aName, aValue )
{
    m_choices.Add( aLabels, aValues );
}


// The state of a page ----------------------------------------------------------------------

wxPropertyGridPageState::wxPropertyGridPageState() :
        m_root( std::make_unique<wxPGProperty>( wxS( "<Root>" ), wxS( "<Root>" ) ) )
{
    m_root->SetStateCompat( this );
}


wxPropertyGridPageState::~wxPropertyGridPageState()
{
    if( m_grid )
        m_grid->DetachStateCompat( this );

    m_grid = nullptr;
    m_currentCategory = nullptr;
    m_root.reset();
}


int wxPropertyGridPageState::GetColumnWidth( unsigned int aColumn ) const
{
    return m_grid ? m_grid->columnWidth( (int) aColumn ) : 0;
}


unsigned int wxPropertyGridPageState::GetColumnCount() const
{
    return m_grid ? m_grid->GetColumnCount() : 2;
}


wxPGProperty* wxPropertyGridPageState::BaseGetPropertyByName( const wxString& aName ) const
{
    std::vector<wxPGProperty*> properties;
    CollectCompat( m_root.get(), wxPG_ITERATE_ALL, properties );

    for( wxPGProperty* property : properties )
    {
        if( property->GetName() == aName )
            return property;
    }

    return nullptr;
}


void wxPropertyGridPageState::CollectCompat( wxPGProperty* aParent, int aFlags,
                                             std::vector<wxPGProperty*>& aResult ) const
{
    for( unsigned int ii = 0; ii < aParent->GetChildCount(); ++ii )
    {
        wxPGProperty* child = aParent->Item( ii );

        if( child->IsHiddenCompat() && !( aFlags & wxPG_ITERATE_HIDDEN ) )
            continue;

        if( child->IsCategory() ? ( aFlags & wxPG_ITERATE_CATEGORIES )
                                : ( aFlags & wxPG_ITERATE_PROPERTIES ) )
        {
            aResult.push_back( child );
        }

        if( child->IsExpanded() || ( aFlags & wxPG_ITERATE_HIDDEN ) )
            CollectCompat( child, aFlags, aResult );
    }
}


void wxPropertyGridPageState::ChangedCompat()
{
    if( m_grid )
        m_grid->RebuildCompat();
}


void wxPropertyGridPageState::DeletingCompat( wxPGProperty* aProperty )
{
    if( aProperty == m_currentCategory )
        m_currentCategory = nullptr;

    if( m_grid )
        m_grid->ForgetPropertyCompat( aProperty );
}


// The interface of the grids and of the pages ----------------------------------------------

wxPGProperty* wxPGPropArgCls::GetPtr( const wxPropertyGridInterface* aInterface ) const
{
    if( m_property )
        return m_property;

    return aInterface->GetProperty( m_name );
}


wxPropertyGridIterator::wxPropertyGridIterator( wxPropertyGridPageState* aState, int aFlags,
                                                wxPGProperty* aStartProperty )
{
    if( !aState )
        return;

    aState->CollectCompat( aState->DoGetRoot(), aFlags, m_properties );

    if( aStartProperty )
    {
        auto it = std::find( m_properties.begin(), m_properties.end(), aStartProperty );
        m_index = it != m_properties.end() ? (int) ( it - m_properties.begin() )
                                           : (int) m_properties.size();
    }
}


wxPGProperty* wxPropertyGridInterface::Append( wxPGProperty* aProperty )
{
    wxPropertyGridPageState* state = GetStateCompat();

    if( !aProperty || !state )
        return aProperty;

    // As wxWidgets: the categories go to the root, the properties in the last category
    wxPGProperty* parent = state->DoGetRoot();

    if( aProperty->IsCategory() )
        state->SetCurrentCategoryCompat( aProperty );
    else if( state->GetCurrentCategoryCompat() )
        parent = state->GetCurrentCategoryCompat();

    return parent->AppendChild( aProperty );
}


wxPGProperty* wxPropertyGridInterface::AppendIn( wxPGPropArg aParent, wxPGProperty* aProperty )
{
    wxPGProperty* parent = aParent.GetPtr( this );

    if( !parent )
        return Append( aProperty );

    return parent->AppendChild( aProperty );
}


wxPGProperty* wxPropertyGridInterface::Insert( wxPGPropArg aPriorThis, wxPGProperty* aProperty )
{
    wxPGProperty* prior = aPriorThis.GetPtr( this );

    if( !prior || !prior->GetParent() )
        return Append( aProperty );

    return prior->GetParent()->InsertChild( (int) prior->GetIndexInParent(), aProperty );
}


wxPGProperty* wxPropertyGridInterface::Insert( wxPGPropArg aParent, int aIndex,
                                               wxPGProperty* aProperty )
{
    wxPGProperty* parent = aParent.GetPtr( this );

    if( !parent )
        parent = GetRoot();

    return parent ? parent->InsertChild( aIndex, aProperty ) : aProperty;
}


void wxPropertyGridInterface::Clear()
{
    if( wxPropertyGridPageState* state = GetStateCompat() )
    {
        state->SetCurrentCategoryCompat( nullptr );
        state->DoGetRoot()->DeleteChildren();
    }
}


void wxPropertyGridInterface::DeleteProperty( wxPGPropArg aProperty )
{
    delete RemoveProperty( aProperty );
}


wxPGProperty* wxPropertyGridInterface::RemoveProperty( wxPGPropArg aProperty )
{
    wxPGProperty* property = aProperty.GetPtr( this );

    if( !property || !property->GetParent() )
        return nullptr;

    wxPropertyGridPageState* state = property->GetStateCompat();

    // The property leaves the tree and its page (it isn't deleted)
    if( state )
        state->DeletingCompat( property );

    property->GetParent()->RemoveChildCompat( property );
    property->SetStateCompat( nullptr );

    if( state )
        state->ChangedCompat();

    return property;
}


bool wxPropertyGridInterface::HideProperty( wxPGPropArg aProperty, bool aHide, int aFlags )
{
    wxPGProperty* property = aProperty.GetPtr( this );
    return property ? property->Hide( aHide, aFlags ) : false;
}


bool wxPropertyGridInterface::EnableProperty( wxPGPropArg aProperty, bool aEnable )
{
    wxPGProperty* property = aProperty.GetPtr( this );

    if( !property )
        return false;

    property->Enable( aEnable );
    return true;
}


bool wxPropertyGridInterface::IsPropertyEnabled( wxPGPropArg aProperty ) const
{
    wxPGProperty* property = aProperty.GetPtr( this );
    return property && property->IsEnabled();
}


bool wxPropertyGridInterface::Expand( wxPGPropArg aProperty )
{
    wxPGProperty* property = aProperty.GetPtr( this );

    if( !property )
        return false;

    property->SetExpandedCompat( true );

    if( wxPropertyGridPageState* state = property->GetStateCompat() )
        state->ChangedCompat();

    return true;
}


bool wxPropertyGridInterface::Collapse( wxPGPropArg aProperty )
{
    wxPGProperty* property = aProperty.GetPtr( this );

    if( !property )
        return false;

    property->SetExpandedCompat( false );

    if( wxPropertyGridPageState* state = property->GetStateCompat() )
        state->ChangedCompat();

    return true;
}


bool wxPropertyGridInterface::ExpandAll( bool aExpand )
{
    wxPropertyGridPageState* state = GetStateCompat();

    if( !state )
        return false;

    std::vector<wxPGProperty*> properties;
    state->CollectCompat( state->DoGetRoot(), wxPG_ITERATE_ALL, properties );

    for( wxPGProperty* property : properties )
    {
        if( property->GetChildCount() )
            property->SetExpandedCompat( aExpand );
    }

    state->ChangedCompat();
    return true;
}


wxPGProperty* wxPropertyGridInterface::GetProperty( const wxString& aName ) const
{
    wxPropertyGridPageState* state = GetStateCompat();
    return state ? state->BaseGetPropertyByName( aName ) : nullptr;
}


wxPGProperty* wxPropertyGridInterface::GetPropertyByLabel( const wxString& aLabel ) const
{
    wxPropertyGridPageState* state = GetStateCompat();

    if( !state )
        return nullptr;

    std::vector<wxPGProperty*> properties;
    state->CollectCompat( state->DoGetRoot(), wxPG_ITERATE_ALL, properties );

    for( wxPGProperty* property : properties )
    {
        if( property->GetLabel() == aLabel )
            return property;
    }

    return nullptr;
}


wxPGProperty* wxPropertyGridInterface::GetFirst( int aFlags )
{
    return *GetIterator( aFlags );
}


wxPGProperty* wxPropertyGridInterface::GetRoot() const
{
    wxPropertyGridPageState* state = GetStateCompat();
    return state ? state->DoGetRoot() : nullptr;
}


wxPropertyGridIterator wxPropertyGridInterface::GetIterator( int aFlags, wxPGProperty* aFirst )
{
    return wxPropertyGridIterator( GetStateCompat(), aFlags, aFirst );
}


wxVariant wxPropertyGridInterface::GetPropertyValue( wxPGPropArg aProperty )
{
    wxPGProperty* property = aProperty.GetPtr( this );
    return property ? property->GetValue() : wxVariant();
}


wxString wxPropertyGridInterface::GetPropertyValueAsString( wxPGPropArg aProperty ) const
{
    wxPGProperty* property = aProperty.GetPtr( this );
    return property ? property->GetValueAsString() : wxString();
}


void wxPropertyGridInterface::SetPropertyValue( wxPGPropArg aProperty, wxVariant aValue )
{
    if( wxPGProperty* property = aProperty.GetPtr( this ) )
        property->SetValue( aValue );
}


void wxPropertyGridInterface::SetPropertyLabel( wxPGPropArg aProperty, const wxString& aLabel )
{
    if( wxPGProperty* property = aProperty.GetPtr( this ) )
        property->SetLabel( aLabel );
}


void wxPropertyGridInterface::SetPropertyCell( wxPGPropArg aProperty, int aColumn,
                                               const wxString& aText )
{
    if( wxPGProperty* property = aProperty.GetPtr( this ) )
        property->SetCell( aColumn, aText );
}


void wxPropertyGridInterface::SetPropertyAttribute( wxPGPropArg aProperty, const wxString& aName,
                                                    wxVariant aValue, long aFlags )
{
    if( wxPGProperty* property = aProperty.GetPtr( this ) )
        property->SetAttribute( aName, aValue );
}


wxString wxPropertyGridEvent::GetPropertyName() const
{
    return m_property ? m_property->GetName() : wxString();
}


wxVariant wxPropertyGridEvent::GetPropertyValue() const
{
    if( !m_value.IsNull() )
        return m_value;

    return m_property ? m_property->GetValue() : wxVariant();
}


// The grid ---------------------------------------------------------------------------------

wxPropertyGrid::wxPropertyGrid( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                                const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxWindowCompat<QTreeWidget>( aParent ),
        m_ownState( std::make_unique<wxPropertyGridPageState>() )
{
    SetId( aId );
    SetWindowStyleFlag( aStyle );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 || aSize.y > 0 )
        resize( std::max( aSize.x, 0 ), std::max( aSize.y, 0 ) );

    setColumnCount( 2 );
    setHeaderLabels( { ToQString( _( "Property" ) ), ToQString( _( "Value" ) ) } );
    setHeaderHidden( true );
    setRootIsDecorated( true );
    setUniformRowHeights( true );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    setAllColumnsShowFocus( true );
    header()->setStretchLastSection( false );
    header()->setSectionResizeMode( QHeaderView::Interactive );

    const QPalette& palette = this->palette();
    m_defaultCell.SetFgCol( toWxColour( palette.color( QPalette::Text ) ) );
    m_defaultCell.SetBgCol( toWxColour( palette.color( QPalette::Base ) ) );
    m_categoryCell.SetFgCol( toWxColour( palette.color( QPalette::Text ) ) );
    m_categoryCell.SetBgCol( toWxColour( palette.color( QPalette::AlternateBase ) ) );
    m_disabledTextColour = toWxColour( palette.color( QPalette::Disabled, QPalette::Text ) );

    // As wxWidgets: Enter edits the property, then goes to the next one
    AddActionTrigger( wxPG_ACTION_EDIT, WXK_RETURN );

    QObject::connect( this, &QTreeWidget::currentItemChanged, this,
                      [this]( QTreeWidgetItem* aCurrent, QTreeWidgetItem* )
                      {
                          if( m_rebuilding || m_selecting )
                              return;

                          select( PropertyOfItemCompat( aCurrent ), false, true );
                      } );

    QObject::connect( this, &QTreeWidget::itemChanged, this,
                      [this]( QTreeWidgetItem* aItem, int aColumn )
                      {
                          if( m_rebuilding || aColumn != VALUE_COLUMN )
                              return;

                          wxPGProperty* property = PropertyOfItemCompat( aItem );

                          if( property && isCheckBox( property ) )
                              toggleCheckBox( property );
                      } );

    QObject::connect( this, &QTreeWidget::itemExpanded, this,
                      [this]( QTreeWidgetItem* aItem )
                      {
                          if( m_rebuilding )
                              return;

                          if( wxPGProperty* property = PropertyOfItemCompat( aItem ) )
                          {
                              property->SetExpandedCompat( true );
                              sendEvent( wxEVT_PG_ITEM_EXPANDED, property );
                          }
                      } );

    QObject::connect( this, &QTreeWidget::itemCollapsed, this,
                      [this]( QTreeWidgetItem* aItem )
                      {
                          if( m_rebuilding )
                              return;

                          if( wxPGProperty* property = PropertyOfItemCompat( aItem ) )
                          {
                              property->SetExpandedCompat( false );
                              sendEvent( wxEVT_PG_ITEM_COLLAPSED, property );
                          }
                      } );

    QObject::connect( this, &QTreeWidget::itemDoubleClicked, this,
                      [this]( QTreeWidgetItem* aItem, int )
                      {
                          if( wxPGProperty* property = PropertyOfItemCompat( aItem ) )
                              sendEvent( wxEVT_PG_DOUBLE_CLICK, property );
                      } );

    // The keys of the grid (also the ones that the code gives to it, as wxWidgets): the arrows
    // go from a property to the other
    Bind( wxEVT_KEY_DOWN,
          [this]( wxKeyEvent& aEvent )
          {
              switch( aEvent.GetKeyCode() )
              {
              case WXK_UP:
              case WXK_NUMPAD_UP:
                  moveSelection( -1 );
                  break;

              case WXK_DOWN:
              case WXK_NUMPAD_DOWN:
                  moveSelection( 1 );
                  break;

              case WXK_RETURN:
              case WXK_NUMPAD_ENTER:
                  if( m_selected && isCheckBox( m_selected ) && m_selected->IsEnabled() )
                  {
                      QTreeWidgetItem* item = m_selected->GetItemCompat();

                      if( item )
                      {
                          item->setCheckState( VALUE_COLUMN, item->checkState( VALUE_COLUMN ) == Qt::Checked
                                                                     ? Qt::Unchecked
                                                                     : Qt::Checked );
                      }
                  }
                  else if( m_editor )
                  {
                      m_editor->SetFocus();
                  }
                  else
                  {
                      aEvent.Skip();
                  }

                  break;

              default:
                  aEvent.Skip();
                  break;
              }
          } );

    SetStateCompat( m_ownState.get() );
}


wxPropertyGrid::~wxPropertyGrid()
{
    destroyEditor();

    if( m_state )
        m_state->SetGridCompat( nullptr );

    m_state = nullptr;
    m_ownState.reset();
}


void wxPropertyGrid::SetStateCompat( wxPropertyGridPageState* aState )
{
    if( aState == m_state )
        return;

    destroyEditor();
    m_selected = nullptr;

    if( m_state )
        m_state->SetGridCompat( nullptr );

    m_state = aState;

    if( m_state )
        m_state->SetGridCompat( this );

    RebuildCompat();
}


void wxPropertyGrid::DetachStateCompat( wxPropertyGridPageState* aState )
{
    if( aState != m_state )
        return;

    destroyEditor();
    m_selected = nullptr;
    m_state = nullptr;

    m_rebuilding = true;
    clear();
    m_rebuilding = false;
    m_dirty = false;
}


wxPGProperty* wxPropertyGrid::PropertyOfItemCompat( QTreeWidgetItem* aItem )
{
    if( !aItem )
        return nullptr;

    return static_cast<wxPGProperty*>( aItem->data( 0, PROPERTY_ROLE ).value<void*>() );
}


void wxPropertyGrid::RebuildCompat()
{
    // The items are made again on the event loop, once for the changes of the code
    if( m_dirty )
        return;

    m_dirty = true;
    QTimer::singleShot( 0, this, [this]() { ensureBuilt(); } );
}


void wxPropertyGrid::ensureBuilt()
{
    if( !m_dirty || m_rebuilding )
        return;

    m_dirty = false;

    // The text being edited goes to its property first
    wxPGProperty* selected = m_selected;

    if( m_editor && selected )
        CommitChangesFromEditor();

    destroyEditor();

    m_rebuilding = true;

    // The properties are forgotten by the items (they may be deleted)
    for( QTreeWidgetItemIterator it( this ); *it; ++it )
    {
        if( wxPGProperty* property = PropertyOfItemCompat( *it ) )
            property->SetItemCompat( nullptr );
    }

    clear();

    if( m_state )
        buildItems( m_state->DoGetRoot(), invisibleRootItem() );

    m_rebuilding = false;

    // The selected property stays selected
    m_selected = nullptr;

    if( selected && selected->GetItemCompat() && !selected->GetItemCompat()->isHidden() )
    {
        m_selecting = true;
        setCurrentItem( selected->GetItemCompat() );
        m_selecting = false;
        m_selected = selected;
        createEditor();
    }

    applyColumnSizes();
}


void wxPropertyGrid::buildItems( wxPGProperty* aParent, QTreeWidgetItem* aParentItem )
{
    for( unsigned int ii = 0; ii < aParent->GetChildCount(); ++ii )
    {
        wxPGProperty*    property = aParent->Item( ii );
        QTreeWidgetItem* item = new QTreeWidgetItem( aParentItem );

        item->setData( 0, PROPERTY_ROLE, QVariant::fromValue<void*>( property ) );
        property->SetItemCompat( item );

        buildItems( property, item );
        updateItem( property );
    }
}


void wxPropertyGrid::updateItem( wxPGProperty* aProperty )
{
    QTreeWidgetItem* item = aProperty->GetItemCompat();

    if( !item )
        return;

    bool rebuilding = m_rebuilding;
    m_rebuilding = true;

    const int columns = columnCount();
    const bool category = aProperty->IsCategory();

    item->setText( 0, ToQString( aProperty->GetLabel() ) );

    if( category )
    {
        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        item->setFirstColumnSpanned( true );
    }
    else if( isCheckBox( aProperty ) )
    {
        item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
        item->setCheckState( VALUE_COLUMN, !aProperty->GetValue().IsNull()
                                                   && aProperty->GetValue().GetBool()
                                           ? Qt::Checked
                                           : Qt::Unchecked );
        item->setText( VALUE_COLUMN, QString() );
    }
    else
    {
        item->setText( VALUE_COLUMN, ToQString( aProperty->GetValueAsString() ) );
    }

    // The other columns: their cells, or the units in the third one
    for( int column = 2; column < columns; ++column )
    {
        wxString text;

        if( aProperty->HasCellCompat( column ) && aProperty->GetCell( column ).HasText() )
            text = aProperty->GetCell( column ).GetText();
        else if( column == (int) UNITS_COLUMN && !aProperty->GetAttribute( wxPG_ATTR_UNITS ).IsNull() )
            text = aProperty->GetAttribute( wxPG_ATTR_UNITS ).GetString();

        item->setText( column, ToQString( text ) );
    }

    // The colours of the cells
    const wxPGCell& defaultCell = category ? m_categoryCell : m_defaultCell;

    for( int column = 0; column < columns; ++column )
    {
        wxColour fg = defaultCell.GetFgCol();
        wxColour bg = defaultCell.GetBgCol();

        if( aProperty->HasCellCompat( column ) )
        {
            const wxPGCell& cell = aProperty->GetCell( column );

            if( cell.GetFgCol().IsOk() )
                fg = cell.GetFgCol();

            if( cell.GetBgCol().IsOk() )
                bg = cell.GetBgCol();
        }

        if( !aProperty->IsEnabled() && m_disabledTextColour.IsOk() )
            fg = m_disabledTextColour;

        item->setForeground( column, QBrush( toQColor( fg ) ) );
        item->setBackground( column, QBrush( toQColor( bg ) ) );
    }

    QString help = ToQString( aProperty->GetHelpString() );
    item->setToolTip( 0, help );

    item->setHidden( aProperty->IsHiddenCompat() );

    if( aProperty->GetChildCount() )
        item->setExpanded( aProperty->IsExpanded() );

    m_rebuilding = rebuilding;
}


void wxPropertyGrid::RefreshItemCompat( wxPGProperty* aProperty )
{
    if( m_dirty || !aProperty->GetItemCompat() )
        return;

    updateItem( aProperty );

    // The editor shows the new value, if the user didn't change it
    if( aProperty == m_selected && m_editor && !m_committing )
    {
        if( wxTextCtrl* text = dynamic_cast<wxTextCtrl*>( m_editor ) )
        {
            if( !m_editorModified && text->GetValue() == m_editorInitialText )
            {
                m_editorInitialText = aProperty->GetValueAsString();
                text->ChangeValue( m_editorInitialText );
            }
        }
        else if( wxChoice* choice = dynamic_cast<wxChoice*>( m_editor ) )
        {
            choice->SetSelection( aProperty->GetChoiceSelection() );
        }
    }
}


void wxPropertyGrid::ForgetPropertyCompat( wxPGProperty* aProperty )
{
    if( aProperty == m_selected )
    {
        // The property is deleted: its editor is not committed
        destroyEditor();
        m_selected = nullptr;
    }

    if( QTreeWidgetItem* item = aProperty->GetItemCompat() )
    {
        item->setData( 0, PROPERTY_ROLE, QVariant::fromValue<void*>( nullptr ) );
        aProperty->SetItemCompat( nullptr );
    }

    RebuildCompat();
}


void wxPropertyGrid::select( wxPGProperty* aProperty, bool aFocus, bool aSendEvent )
{
    if( aProperty == m_selected && ( m_editor || !aProperty ) )
    {
        if( aFocus && m_editor )
            m_editor->SetFocus();

        return;
    }

    if( m_editor && m_selected )
        CommitChangesFromEditor();

    destroyEditor();
    m_selected = aProperty;

    m_selecting = true;
    setCurrentItem( aProperty ? aProperty->GetItemCompat() : nullptr );
    m_selecting = false;

    createEditor();

    if( aFocus && m_editor )
        m_editor->SetFocus();

    if( aSendEvent )
        sendEvent( wxEVT_PG_SELECTED, aProperty );
}


bool wxPropertyGrid::SelectProperty( wxPGPropArg aProperty, bool aFocus )
{
    ensureBuilt();
    select( aProperty.GetPtr( this ), aFocus, false );
    return true;
}


bool wxPropertyGrid::DoSelectProperty( wxPGProperty* aProperty, unsigned int aFlags )
{
    ensureBuilt();
    select( aProperty, aFlags & wxPG_SEL_FOCUS, !( aFlags & wxPG_SEL_DONT_SEND_EVENT ) );
    return true;
}


bool wxPropertyGrid::ClearSelection( bool aValidation )
{
    ensureBuilt();
    select( nullptr, false, false );
    return true;
}


bool wxPropertyGrid::EnsureVisible( wxPGPropArg aProperty )
{
    ensureBuilt();

    wxPGProperty* property = aProperty.GetPtr( this );

    if( !property || !property->GetItemCompat() )
        return false;

    scrollToItem( property->GetItemCompat() );
    return true;
}


void wxPropertyGrid::createEditor()
{
    if( m_editor || !m_selected || m_selected->IsCategory() || !m_selected->IsEnabled()
        || !m_selected->GetItemCompat() || isCheckBox( m_selected ) )
    {
        return;
    }

    wxPGProperty* property = m_selected;

    if( property->GetEditorClass() == wxPGEditor_Choice && property->HasChoicesCompat() )
    {
        wxChoice* choice = new wxChoice( viewport(), wxID_ANY );
        choice->Append( property->GetChoices().GetLabels() );
        choice->SetSelection( property->GetChoiceSelection() );

        QObject::connect( static_cast<QComboBox*>( choice ), &QComboBox::activated, this,
                          [this]( int aIndex ) { editorChoice( aIndex ); } );

        m_editor = choice;
    }
    else
    {
        m_editorInitialText = property->GetValueAsString();

        wxTextCtrl* text = new wxTextCtrl( viewport(), wxID_ANY, m_editorInitialText );
        text->setFrame( false );

        QObject::connect( static_cast<QLineEdit*>( text ), &QLineEdit::textEdited, this,
                          [this]() { m_editorModified = true; } );

        m_editor = text;
    }

    m_editorModified = false;
    m_editor->GetHandle()->installEventFilter( this );
    setItemWidget( property->GetItemCompat(), VALUE_COLUMN, m_editor->GetHandle() );
}


void wxPropertyGrid::destroyEditor()
{
    if( !m_editor )
        return;

    QWidget* widget = m_editor->GetHandle();
    m_editor = nullptr;
    m_editorModified = false;

    widget->removeEventFilter( this );

    // The item widget is deleted later: it can be in one of its events
    if( m_selected && m_selected->GetItemCompat() )
    {
        QTreeWidgetItem* item = m_selected->GetItemCompat();

        widget->setParent( nullptr );
        widget->hide();
        setItemWidget( item, VALUE_COLUMN, nullptr );
    }
    else
    {
        widget->hide();
    }

    widget->deleteLater();
}


wxWindow* wxPropertyGrid::GetEditorControl() const
{
    return m_editor;
}


wxTextCtrl* wxPropertyGrid::GetEditorTextCtrl() const
{
    return dynamic_cast<wxTextCtrl*>( m_editor );
}


bool wxPropertyGrid::IsEditorFocused() const
{
    return m_editor && m_editor->GetHandle()->hasFocus();
}


void wxPropertyGrid::RefreshEditor()
{
    if( !m_editor || !m_selected )
        return;

    if( wxTextCtrl* text = dynamic_cast<wxTextCtrl*>( m_editor ) )
    {
        if( !m_editorModified )
        {
            m_editorInitialText = m_selected->GetValueAsString();
            text->ChangeValue( m_editorInitialText );
        }
    }
    else if( wxChoice* choice = dynamic_cast<wxChoice*>( m_editor ) )
    {
        choice->SetSelection( m_selected->GetChoiceSelection() );
    }
}


void wxPropertyGrid::RefreshProperty( wxPGProperty* aProperty )
{
    if( aProperty )
        RefreshItemCompat( aProperty );
}


bool wxPropertyGrid::CommitChangesFromEditor( wxUint32 aFlags )
{
    wxTextCtrl* text = dynamic_cast<wxTextCtrl*>( m_editor );

    if( !text || !m_selected || m_committing )
        return true;

    wxString value = text->GetValue();

    if( !m_editorModified && value == m_editorInitialText )
        return true;

    wxVariant variant( m_selected->GetValue() );

    if( !m_selected->StringToValue( variant, value, wxPG_EDITABLE_VALUE ) )
        return false;

    m_committing = true;
    bool changed = changeValue( m_selected, variant );
    m_committing = false;

    if( !changed )
        return false;

    // The editor shows the value of the property
    if( m_editor == text && m_selected )
    {
        m_editorInitialText = m_selected->GetValueAsString();

        if( m_editorInitialText != value )
            text->ChangeValue( m_editorInitialText );
    }

    m_editorModified = false;
    return true;
}


void wxPropertyGrid::editorChoice( int aIndex )
{
    if( !m_selected )
        return;

    wxVariant variant( m_selected->GetValue() );

    if( m_selected->IntToValue( variant, aIndex ) )
        changeValue( m_selected, variant );
    else
        RefreshEditor();
}


void wxPropertyGrid::toggleCheckBox( wxPGProperty* aProperty )
{
    QTreeWidgetItem* item = aProperty->GetItemCompat();

    if( !item )
        return;

    bool      checked = item->checkState( VALUE_COLUMN ) == Qt::Checked;
    wxVariant variant( aProperty->GetValue() );

    if( !aProperty->IsEnabled() || !aProperty->IntToValue( variant, checked ? 1 : 0 )
        || !changeValue( aProperty, variant ) )
    {
        // Refused: the check box shows the value of the property again
        updateItem( aProperty );
    }
}


bool wxPropertyGrid::changeValue( wxPGProperty* aProperty, wxVariant aValue )
{
    if( !sendEvent( wxEVT_PG_CHANGING, aProperty, &aValue ) )
        return false;

    aProperty->SetValue( aValue );
    sendEvent( wxEVT_PG_CHANGED, aProperty );
    return true;
}


bool wxPropertyGrid::sendEvent( wxEventType aType, wxPGProperty* aProperty, const wxVariant* aValue )
{
    wxPropertyGridEvent event( aType, GetId() );
    event.SetEventObject( this );
    event.SetProperty( aProperty );
    event.SetCanVeto( aType == wxEVT_PG_CHANGING );

    if( aValue )
        event.SetValueCompat( *aValue );

    // The events of a grid go to its manager (wxWidgets propagates the command events)
    if( !ProcessEvent( event ) && m_manager )
        m_manager->ProcessEvent( event );

    return !event.WasVetoed();
}


void wxPropertyGrid::moveSelection( int aDirection )
{
    ensureBuilt();

    QTreeWidgetItem* current = m_selected ? m_selected->GetItemCompat() : nullptr;
    QTreeWidgetItem* next = nullptr;

    if( !current )
        next = topLevelItemCount() ? topLevelItem( 0 ) : nullptr;
    else
        next = aDirection < 0 ? itemAbove( current ) : itemBelow( current );

    while( next && next->isHidden() )
        next = aDirection < 0 ? itemAbove( next ) : itemBelow( next );

    if( next )
        select( PropertyOfItemCompat( next ), true, true );
}


bool wxPropertyGrid::propertyEvent( QEvent* aEvent )
{
    wxPGProperty* property = m_selected;

    if( !property || !m_editor )
        return false;

    wxTextCtrl* text = dynamic_cast<wxTextCtrl*>( m_editor );
    bool        processed = false;
    int         keyCode = 0;

    switch( aEvent->type() )
    {
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    {
        // The focus goes to the popup of a choice: the editor keeps it
        if( aEvent->type() == QEvent::FocusOut
            && static_cast<QFocusEvent*>( aEvent )->reason() == Qt::PopupFocusReason )
        {
            return false;
        }

        wxFocusEvent event( aEvent->type() == QEvent::FocusIn ? wxEVT_SET_FOCUS : wxEVT_KILL_FOCUS,
                            m_editor->GetId() );
        event.SetEventObject( m_editor );
        property->OnEvent( this, m_editor, event );
        break;
    }

    case QEvent::KeyPress:
    {
        wxKeyEvent event( wxEVT_KEY_DOWN );
        event.SetId( m_editor->GetId() );
        event.SetEventObject( m_editor );
        wxFillKeyEventCompat( event, aEvent );
        processed = property->OnEvent( this, m_editor, event );
        keyCode = event.GetKeyCode();

        // Tab goes to the next property (back with Shift), as the simulator models do
        if( keyCode == WXK_TAB )
            keyCode = event.ShiftDown() ? WXK_UP : WXK_DOWN;

        break;
    }

    default:
        return false;
    }

    // A value given by the property from its event: the editor shows it
    wxVariant valueInEvent;

    if( property == m_selected && property->TakeValueInEventCompat( valueInEvent ) && text
        && m_editor == text )
    {
        wxString valueText = property->ValueToString( valueInEvent );

        if( valueText != text->GetValue() )
        {
            text->ChangeValue( valueText );
            m_editorModified = true;
        }
    }

    // The text goes to the property when the editor loses the focus
    if( aEvent->type() == QEvent::FocusOut )
    {
        if( property == m_selected )
            CommitChangesFromEditor();

        return false;
    }

    if( aEvent->type() != QEvent::KeyPress || processed )
        return processed;

    switch( keyCode )
    {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
    {
        CommitChangesFromEditor();

        auto it = m_actionTriggers.find( wxPG_ACTION_NEXT_PROPERTY );

        if( it != m_actionTriggers.end()
            && std::find( it->second.begin(), it->second.end(), keyCode ) != it->second.end() )
        {
            moveSelection( 1 );
        }

        return true;
    }

    case WXK_UP:
    case WXK_DOWN:
    case WXK_NUMPAD_UP:
    case WXK_NUMPAD_DOWN:
        // The arrows of a choice change its value
        if( !text )
            return false;

        CommitChangesFromEditor();
        moveSelection( keyCode == WXK_UP || keyCode == WXK_NUMPAD_UP ? -1 : 1 );
        return true;

    case WXK_ESCAPE:
        if( text && ( m_editorModified || text->GetValue() != m_editorInitialText ) )
        {
            text->ChangeValue( m_editorInitialText );
            m_editorModified = false;
            return true;
        }

        return false;

    default:
        return false;
    }
}


bool wxPropertyGrid::eventFilter( QObject* aWatched, QEvent* aEvent )
{
    if( m_editor && aWatched == m_editor->GetHandle() )
    {
        // The key events go to the property, not to Qt, when it processes them
        if( aEvent->type() == QEvent::ShortcutOverride )
        {
            QKeyEvent* key = static_cast<QKeyEvent*>( aEvent );

            if( key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter
                || key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab )
            {
                aEvent->accept();
                return true;
            }

            return false;
        }

        if( aEvent->type() == QEvent::KeyPress || aEvent->type() == QEvent::FocusIn
            || aEvent->type() == QEvent::FocusOut )
        {
            return propertyEvent( aEvent );
        }
    }

    return wxWindowCompat<QTreeWidget>::eventFilter( aWatched, aEvent );
}


void wxPropertyGrid::AddActionTrigger( int aAction, int aKeyCode, int aModifiers )
{
    std::vector<int>& keys = m_actionTriggers[aAction];

    if( std::find( keys.begin(), keys.end(), aKeyCode ) == keys.end() )
        keys.push_back( aKeyCode );
}


void wxPropertyGrid::ClearActionTriggers( int aAction )
{
    m_actionTriggers.erase( aAction );
}


void wxPropertyGrid::SetColumnCount( int aColumnCount )
{
    int oldCount = columnCount();

    setColumnCount( std::max( aColumnCount, 2 ) );

    // The titles of the new columns are empty (not the numbers of Qt), as in wxWidgets
    for( int col = oldCount; col < columnCount(); ++col )
        headerItem()->setText( col, QString() );

    m_proportions.resize( columnCount(), 0 );
    RebuildCompat();
}


void wxPropertyGrid::SetColumnTitleCompat( unsigned int aColumn, const wxString& aTitle )
{
    QTreeWidgetItem* header = headerItem();

    if( (int) aColumn < columnCount() )
        header->setText( (int) aColumn, ToQString( aTitle ) );
}


void wxPropertyGrid::SetColumnProportion( unsigned int aColumn, int aProportion )
{
    if( m_proportions.size() < (size_t) columnCount() )
        m_proportions.resize( columnCount(), 0 );

    if( aColumn < m_proportions.size() )
        m_proportions[aColumn] = aProportion;
}


void wxPropertyGrid::ResetColumnSizes( bool aEnableAutoResizing )
{
    applyColumnSizes();
}


void wxPropertyGrid::applyColumnSizes()
{
    const int columns = columnCount();
    const int width = viewport()->width();
    int       total = 0;

    for( int column = 0; column < columns; ++column )
        total += column < (int) m_proportions.size() ? m_proportions[column] : 0;

    // Without proportions, the columns share the width
    for( int column = 0; column < columns; ++column )
    {
        int proportion = column < (int) m_proportions.size() ? m_proportions[column] : 0;
        int size = total > 0 ? width * proportion / total : width / columns;

        if( total > 0 && proportion <= 0 )
            continue;

        setColumnWidth( column, std::max( size, 16 ) );
    }
}


wxSize wxPropertyGrid::FitColumns()
{
    ensureBuilt();

    int width = 0;

    for( int column = 0; column < columnCount(); ++column )
    {
        resizeColumnToContents( column );
        width += columnWidth( column );
    }

    return wxSize( width, sizeHint().height() );
}


void wxPropertyGrid::SetSplitterPosition( int aPosition, int aColumn )
{
    if( aColumn >= 0 && aColumn < columnCount() )
        setColumnWidth( aColumn, aPosition );
}


int wxPropertyGrid::GetSplitterPosition( unsigned int aSplitterIndex ) const
{
    int position = 0;

    for( unsigned int column = 0; column <= aSplitterIndex && (int) column < columnCount(); ++column )
        position += columnWidth( (int) column );

    return position;
}


void wxPropertyGrid::CenterSplitter( bool aEnableAutoResizing )
{
    setColumnWidth( 0, viewport()->width() / 2 );
}


void wxPropertyGrid::SetCellDisabledTextColour( const wxColour& aColour )
{
    m_disabledTextColour = aColour;
    RebuildCompat();
}


void wxPropertyGrid::resizeEvent( QResizeEvent* aEvent )
{
    wxWindowCompat<QTreeWidget>::resizeEvent( aEvent );
    applyColumnSizes();
}


// The pages and their manager --------------------------------------------------------------

wxPropertyGridPage::wxPropertyGridPage( wxPropertyGridManager* aManager, const wxString& aLabel ) :
        m_manager( aManager ),
        m_label( aLabel )
{
}


wxPropertyGrid* wxPropertyGridPage::GetGrid() const
{
    return m_manager ? m_manager->GetGrid() : nullptr;
}


wxPropertyGridManager::wxPropertyGridManager( wxWidgetArgCompat aParent, int aId,
                                              const wxPoint& aPos, const wxSize& aSize,
                                              long aStyle, const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent )
{
    SetId( aId );
    SetWindowStyleFlag( aStyle );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 || aSize.y > 0 )
        resize( std::max( aSize.x, 0 ), std::max( aSize.y, 0 ) );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, aStyle );
    m_grid->SetManagerCompat( this );
    layout->addWidget( m_grid );
}


wxPropertyGridManager::~wxPropertyGridManager()
{
    // The grid stops showing the pages before they are deleted
    m_grid->SetStateCompat( nullptr );
    m_pages.clear();
}


wxPropertyGridPageState* wxPropertyGridManager::GetStateCompat() const
{
    if( wxPropertyGridPage* page = GetCurrentPage() )
        return page;

    return m_grid->GetStateCompat();
}


wxPropertyGridPage* wxPropertyGridManager::AddPage( const wxString& aLabel,
                                                    const wxBitmapBundle& aBitmap,
                                                    wxPropertyGridPage* aPage )
{
    if( !aPage )
        aPage = new wxPropertyGridPage( this, aLabel );

    aPage->SetManagerCompat( this );
    m_pages.emplace_back( aPage );

    if( m_selectedPage < 0 )
        SelectPage( 0 );

    return aPage;
}


wxPropertyGridPage* wxPropertyGridManager::GetPage( unsigned int aIndex ) const
{
    return aIndex < m_pages.size() ? m_pages[aIndex].get() : nullptr;
}


wxPropertyGridPage* wxPropertyGridManager::GetPage( const wxString& aName ) const
{
    for( const std::unique_ptr<wxPropertyGridPage>& page : m_pages )
    {
        if( page->GetLabelCompat() == aName )
            return page.get();
    }

    return nullptr;
}


wxPropertyGridPage* wxPropertyGridManager::GetCurrentPage() const
{
    return GetPage( m_selectedPage < 0 ? m_pages.size() : (unsigned int) m_selectedPage );
}


void wxPropertyGridManager::SelectPage( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_pages.size() )
        return;

    m_selectedPage = aIndex;
    m_grid->SetStateCompat( m_pages[aIndex].get() );
}


void wxPropertyGridManager::SelectPage( wxPropertyGridPage* aPage )
{
    for( size_t ii = 0; ii < m_pages.size(); ++ii )
    {
        if( m_pages[ii].get() == aPage )
            SelectPage( (int) ii );
    }
}


bool wxPropertyGridManager::RemovePage( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_pages.size() )
        return false;

    if( aIndex == m_selectedPage )
        m_grid->SetStateCompat( nullptr );

    m_pages.erase( m_pages.begin() + aIndex );

    if( m_selectedPage >= (int) m_pages.size() )
        m_selectedPage = (int) m_pages.size() - 1;

    if( m_selectedPage >= 0 )
        m_grid->SetStateCompat( m_pages[m_selectedPage].get() );

    return true;
}


void wxPropertyGridManager::SetColumnCount( int aColumnCount, int aPage )
{
    m_columnCount = aColumnCount;
    m_grid->SetColumnCount( aColumnCount );
}


int wxPropertyGridManager::GetColumnCount( int aPage ) const
{
    return (int) m_grid->GetColumnCount();
}


void wxPropertyGridManager::SetColumnTitle( int aColumn, const wxString& aTitle )
{
    m_grid->SetColumnTitleCompat( (unsigned int) aColumn, aTitle );
}


void wxPropertyGridManager::ShowHeader( bool aShow )
{
    m_grid->setHeaderHidden( !aShow );
}


void wxPropertyGridManager::SetSplitterPosition( int aPosition, int aColumn )
{
    m_grid->SetSplitterPosition( aPosition, aColumn );
}


bool wxPropertyGridManager::SelectProperty( wxPGPropArg aProperty, bool aFocus )
{
    return m_grid->SelectProperty( aProperty, aFocus );
}


bool wxPropertyGridManager::ClearSelection( bool aValidation )
{
    return m_grid->ClearSelection( aValidation );
}


bool wxPropertyGridManager::CommitChangesFromEditor( wxUint32 aFlags )
{
    return m_grid->CommitChangesFromEditor( aFlags );
}
