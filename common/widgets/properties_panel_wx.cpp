/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "properties_panel.h"
#include <properties/pg_properties.h>
#include <eda_base_frame.h>
#include <pgm_base.h>
#include <pgm_gui.h>
#include <properties/pg_cell_renderer.h>
#include <widgets/ui_common.h>

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/propgrid/advprops.h>
#include <core/ki_assert.h>


// This is provided by wx >3.3.0
#if !wxCHECK_VERSION( 3, 3, 0 )
extern APIIMPORT wxPGGlobalVarsClass* wxPGGlobalVars;
#endif


class PROPERTIES_PANEL_GRID : public wxPropertyGrid
{
public:
    PROPERTIES_PANEL_GRID( wxWindow* aParent ) :
            wxPropertyGrid( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_TOOLTIPS )
    {
    }

#if wxUSE_STATUSBAR
    wxStatusBar* GetStatusBar() override { return nullptr; }
#endif
};


PROPERTIES_PANEL::PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_SuppressGridChangeEvents( 0 ),
        m_frame( aFrame ),
        m_splitter_key_proportion( -1 )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

#if !wxCHECK_VERSION( 3, 3, 0 )
    // on some platforms wxPGGlobalVars is initialized automatically,
    // but others need an explicit init
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();
#endif

    // See https://gitlab.com/kicad/code/kicad/-/issues/12297
    // and https://github.com/wxWidgets/wxWidgets/issues/11787
    if( wxPGGlobalVars->m_mapEditorClasses.empty() )
    {
        wxPGEditor_TextCtrl = nullptr;
        wxPGEditor_Choice = nullptr;
        wxPGEditor_ComboBox = nullptr;
        wxPGEditor_TextCtrlAndButton = nullptr;
        wxPGEditor_CheckBox = nullptr;
        wxPGEditor_ChoiceAndButton = nullptr;
        wxPGEditor_SpinCtrl = nullptr;
        wxPGEditor_DatePickerCtrl = nullptr;
    }

    if( !PgmGui().m_PropertyGridInitialized )
    {
        delete wxPGGlobalVars->m_defaultRenderer;
        wxPGGlobalVars->m_defaultRenderer = new PG_CELL_RENDERER();
        PgmGui().m_PropertyGridInitialized = true;
    }

    m_caption = new wxStaticText( this, wxID_ANY, _( "No objects selected" ) );
    mainSizer->Add( m_caption, 0, wxALL | wxEXPAND, 5 );

    m_grid = new PROPERTIES_PANEL_GRID( this );
    m_grid->SetUnspecifiedValueAppearance( wxPGCell( wxT( "<...>" ) ) );

#if wxCHECK_VERSION( 3, 3, 0 )
    m_grid->SetValidationFailureBehavior( wxPGVFBFlags::MarkCell );
#else
    m_grid->SetValidationFailureBehavior( wxPG_VFB_MARK_CELL );
#endif

#if wxCHECK_VERSION( 3, 3, 0 )
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_RETURN );
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_NUMPAD_ENTER );
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_DOWN );
    m_grid->AddActionTrigger( wxPGKeyboardAction::PrevProperty, WXK_UP );
    m_grid->AddActionTrigger( wxPGKeyboardAction::Edit, WXK_SPACE );
#else
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_NUMPAD_ENTER );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_DOWN );
    m_grid->AddActionTrigger( wxPG_ACTION_PREV_PROPERTY, WXK_UP );
    m_grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_SPACE );
#endif

    m_grid->DedicateKey( WXK_RETURN );
    m_grid->DedicateKey( WXK_NUMPAD_ENTER );
    m_grid->DedicateKey( WXK_DOWN );
    m_grid->DedicateKey( WXK_UP );
    mainSizer->Add( m_grid, 1, wxEXPAND, 5 );

    m_grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

#ifdef __WXGTK__
    // Needed for dark mode, on wx 3.0 at least.
    m_grid->SetCaptionTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
#endif

    SetFont( KIUI::GetDockedPaneFont( this ) );

    SetSizer( mainSizer );
    Layout();

    m_grid->CenterSplitter();

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PROPERTIES_PANEL::onCharHook ), nullptr, this );

    Bind( wxEVT_PG_CHANGED,
          [this]( wxPropertyGridEvent& aEvent )
          {
              valueChanged( aEvent.GetPropertyName(), aEvent.GetPropertyValue() );
              aEvent.Skip();
          } );

    Bind( wxEVT_PG_CHANGING,
          [this]( wxPropertyGridEvent& aEvent )
          {
              if( !valueChanging( aEvent.GetPropertyName(), aEvent.GetPropertyValue() ) )
                  aEvent.Veto();
              else
                  aEvent.Skip();
          } );
    Connect( wxEVT_SHOW, wxShowEventHandler( PROPERTIES_PANEL::onShow ), nullptr, this );

    Bind( wxEVT_PG_COL_END_DRAG,
          [&]( wxPropertyGridEvent& )
          {
              m_splitter_key_proportion = static_cast<float>( m_grid->GetSplitterPosition() ) / m_grid->GetSize().x;
          } );

    Bind( wxEVT_SIZE,
          [&]( wxSizeEvent& aEvent )
          {
              CallAfter( [this]()
                         {
                            RecalculateSplitterPos();
                         } );
              aEvent.Skip();
          } );

    m_frame->Bind( EDA_LANG_CHANGED, &PROPERTIES_PANEL::OnLanguageChanged, this );
}


PROPERTIES_PANEL::~PROPERTIES_PANEL()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &PROPERTIES_PANEL::OnLanguageChanged, this );
}


void PROPERTIES_PANEL::OnLanguageChanged( wxCommandEvent& aEvent )
{
    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();
    m_displayed.clear();    // no ownership of pointers

    UpdateData();

    aEvent.Skip();
}


void PROPERTIES_PANEL::onShow( wxShowEvent& aEvent )
{
    if( aEvent.IsShown() )
        UpdateData();

    aEvent.Skip();
}


void PROPERTIES_PANEL::onCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB
        && ( aEvent.GetModifiers() == wxMOD_NONE || aEvent.GetModifiers() == wxMOD_SHIFT ) )
    {
        // wxPropertyGrid hard-codes Tab to focus the current editor and then navigate out of
        // the grid, so it never steps between properties.  Intercept Tab here to commit any
        // pending edit and move selection to the next (or previous, with Shift) editable
        // property, wrapping around at the ends.
        m_grid->CommitChangesFromEditor();

        const bool forward = !aEvent.ShiftDown();

        auto isTabStop =
                []( wxPGProperty* aProp )
                {
                    return aProp && !aProp->IsCategory() && aProp->IsVisible() && aProp->IsEnabled()
                           && !aProp->HasFlag( wxPG_PROP_READONLY ) && aProp->GetEditorClass();
                };

        auto findTarget =
                [&]( wxPropertyGridIterator aIt )
                {
                    while( !aIt.AtEnd() )
                    {
                        if( isTabStop( aIt.GetProperty() ) )
                            return aIt.GetProperty();

                        if( forward )
                            aIt.Next();
                        else
                            aIt.Prev();
                    }

                    return static_cast<wxPGProperty*>( nullptr );
                };

        wxPGProperty* current = m_grid->GetSelectedProperty();
        wxPGProperty* target = nullptr;

        if( current )
        {
            wxPropertyGridIterator it = m_grid->GetIterator( wxPG_ITERATE_VISIBLE, current );

            if( forward )
                it.Next();
            else
                it.Prev();

            target = findTarget( it );
        }

        // Wrap around (or pick a starting property if nothing is selected)
        if( !target )
        {
            wxPropertyGridIterator it = m_grid->GetIterator( wxPG_ITERATE_VISIBLE,
                                                             forward ? wxTOP : wxBOTTOM );
            target = findTarget( it );
        }

        if( target )
            m_grid->SelectProperty( target, true );
        else
            aEvent.Skip();

        return;
    }

    if( aEvent.GetKeyCode() == 'C' && aEvent.GetModifiers() == wxMOD_CONTROL )
    {
        if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
        {
            if( wxTheClipboard->Open() )
            {
                wxTheClipboard->SetData( new wxTextDataObject( prop->GetValueAsString() ) );
                wxTheClipboard->Close();
                return;
            }
        }
    }

    if( aEvent.GetKeyCode() == WXK_SPACE )
    {
        if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
        {
            if( prop->GetValueType() == wxT( "bool" ) )
            {
                m_grid->SetPropertyValue( prop, !prop->GetValue().GetBool() );
                return;
            }
        }
    }

    if( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER
        || aEvent.GetKeyCode() == WXK_DOWN || aEvent.GetKeyCode() == WXK_UP )
    {
        m_grid->CommitChangesFromEditor();

        CallAfter( [this]()
                   {
                       m_grid->SelectProperty( m_grid->GetSelectedProperty(), true );
                   } );
    }

    aEvent.Skip();
}


void PROPERTIES_PANEL::RecalculateSplitterPos()
{
    if( m_splitter_key_proportion < 0 )
        m_grid->CenterSplitter();
    else
        m_grid->SetSplitterPosition( m_splitter_key_proportion * m_grid->GetSize().x );
}


void PROPERTIES_PANEL::SetSplitterProportion( float aProportion )
{
    m_splitter_key_proportion = aProportion;
    RecalculateSplitterPos();
}


void PROPERTIES_PANEL::setCaption( const wxString& aCaption )
{
    m_caption->SetLabel( aCaption );
}


void PROPERTIES_PANEL::gridReset()
{
    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();
    m_displayed.clear();
}


std::vector<TOOLKIT_PG_PROPERTY*> PROPERTIES_PANEL::gridProperties() const
{
    std::vector<TOOLKIT_PG_PROPERTY*> properties;

    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
        properties.push_back( it.GetProperty() );

    return properties;
}


void PROPERTIES_PANEL::gridAppendCategory( const wxString& aCaption )
{
    m_grid->Append( new wxPropertyCategory( aCaption ) );
}


void PROPERTIES_PANEL::gridAppend( TOOLKIT_PG_PROPERTY* aProperty )
{
    m_grid->Append( aProperty );
}
