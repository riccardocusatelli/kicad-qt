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

#include <widgets/properties_panel.h>
#include <bitmaps.h>
#include <i18n_utility.h>
#include <widgets/bitmap_button.h>
#include <wx/menu.h>
#include <wx/utils.h>
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

    void ScrollWindow( int aDx, int aDy, const wxRect* aRect = nullptr ) override
    {
        wxPropertyGrid::ScrollWindow( aDx, aDy, aRect );

        if( PROPERTIES_PANEL* panel = static_cast<PROPERTIES_PANEL*>( GetParent() ) )
            panel->positionCategoryButtons();
    }

    /// True while a wxPropertyGrid event (e.g. right-click) is being processed.
    bool IsProcessingWxPGEvent() const { return m_processedEvent != nullptr; }

#if wxUSE_STATUSBAR
    wxStatusBar* GetStatusBar() override { return nullptr; }
#endif
};


PROPERTIES_PANEL::PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_SuppressGridChangeEvents( 0 ),
        m_frame( aFrame ),
        m_splitter_key_proportion( -1 ),
        m_deferredRebuildTimer( this )
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
    m_grid->SetValidationFailureBehavior( wxPGVFBFlags::StayInProperty );
#else
    m_grid->SetValidationFailureBehavior( wxPG_VFB_STAY_IN_PROPERTY );
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

    // Actual edits are allowed or vetoed per-property based on whether or not it's
    // something where the label/key should be editable (user fields, custom properties, ...)
    m_grid->MakeColumnEditable( 0 );

    Bind( wxEVT_PG_ITEM_EXPANDED,
          [&]( wxPropertyGridEvent& )
          {
              positionCategoryButtons();
          } );

    Bind( wxEVT_PG_ITEM_COLLAPSED,
          [&]( wxPropertyGridEvent& )
          {
              positionCategoryButtons();
          } );

    Bind( wxEVT_PG_LABEL_EDIT_BEGIN, &PROPERTIES_PANEL::onLabelEditBegin, this );
    Bind( wxEVT_PG_LABEL_EDIT_ENDING, &PROPERTIES_PANEL::onLabelEditEnding, this );
    Bind( wxEVT_PG_RIGHT_CLICK, &PROPERTIES_PANEL::onRightClick, this );

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
              positionCategoryButtons();
          } );

    Bind( wxEVT_SIZE,
          [&]( wxSizeEvent& aEvent )
          {
              CallAfter( [this]()
                         {
                            RecalculateSplitterPos();
                            positionCategoryButtons();
                         } );
              aEvent.Skip();
          } );

    m_frame->Bind( EDA_LANG_CHANGED, &PROPERTIES_PANEL::OnLanguageChanged, this );

    Bind( wxEVT_TIMER,
          [this]( wxTimerEvent& )
          {
              // The selection may have changed or lost items since the deferral
              UpdateData();
          },
          m_deferredRebuildTimer.GetId() );
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


void PROPERTIES_PANEL::onLabelEditBegin( wxPropertyGridEvent& aEvent )
{
    wxPGProperty* pgProp = aEvent.GetProperty();

    if( !pgProp || !isKeyEditable( pgProp ) )
    {
        aEvent.Veto();
        return;
    }

    // Remember the original label so an invalid rename can be undone.
    m_editingOriginalLabel = pgProp->GetLabel();
}


void PROPERTIES_PANEL::onLabelEditEnding( wxPropertyGridEvent& aEvent )
{
    wxPGProperty* pgProp = aEvent.GetProperty();

    if( !pgProp || !isKeyEditable( pgProp ) )
        return;

    const wxString oldName = pgProp->GetBaseName();

    wxTextCtrl* labelEditor = m_grid->GetLabelEditor();
    wxString    newName;

    if( labelEditor )
        newName = labelEditor->GetValue();

    if( !m_pendingNewKey.IsEmpty() && oldName == m_pendingNewKey )
    {
        const wxString pendingKey = m_pendingNewKey;

        m_pendingNewKey.Clear();

        if( m_resolvingPendingKey )
        {
            if( newName.IsEmpty() || isKeyNameInUse( newName ) )
                onNewItemLeftBlank( pendingKey );
            else
                onKeyRenamed( oldName, newName );
        }
        else if( newName.IsEmpty() || isKeyNameInUse( newName ) )
        {
            CallAfter(
                    [this, pendingKey]()
                    {
                        onNewItemLeftBlank( pendingKey );
                    } );
        }
        else
        {
            CallAfter(
                    [this, oldName, newName]()
                    {
                        onKeyRenamed( oldName, newName );
                    } );
        }

        return;
    }

    if( newName == oldName )
        return;

    if( newName.IsEmpty() || isKeyNameInUse( newName ) )
    {
        const wxString originalLabel = m_editingOriginalLabel;

        CallAfter(
                [this, oldName, originalLabel, newName]()
                {
                    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
                    {
                        wxPGProperty* p = it.GetProperty();

                        if( p->GetBaseName() == oldName && p->GetLabel() == newName )
                        {
                            p->SetLabel( originalLabel );
                            m_grid->Refresh();
                            break;
                        }
                    }
                } );

        return;
    }

    CallAfter(
            [this, oldName, newName]()
            {
                onKeyRenamed( oldName, newName );
            } );
}


void PROPERTIES_PANEL::onRightClick( wxPropertyGridEvent& aEvent )
{
    wxPGProperty* pgProp = aEvent.GetProperty();

    if( !pgProp )
        return;

    m_contextMenuPropertyName = pgProp->GetBaseName();

    wxMenu* menu = new wxMenu;

    if( !buildContextMenu( *menu, pgProp ) )
    {
        delete menu;
        return;
    }

    // Defer showing the menu until the property event has finished
    const wxPoint pos = ScreenToClient( wxGetMousePosition() );
    CallAfter( [this, menu, pos]()
               {
                   PopupMenu( menu, pos );
                   delete menu;
               } );
}


void PROPERTIES_PANEL::settlePendingLabelEdit()
{
    if( !m_grid->GetLabelEditor() )
    {
        m_pendingNewKey.Clear();
        return;
    }

    const wxString newName = m_grid->GetLabelEditor()->GetValue();
    const wxString pendingKey = m_pendingNewKey;

    m_resolvingPendingKey = true;
    m_grid->EndLabelEdit( true );
    m_resolvingPendingKey = false;

    if( pendingKey.IsEmpty() )
        return;

    if( newName.IsEmpty() || isKeyNameInUse( newName ) )
        onNewItemLeftBlank( pendingKey );
    else if( newName != pendingKey )
        onKeyRenamed( pendingKey, newName );
}


void PROPERTIES_PANEL::beginLabelEdit( const wxString& aKey, bool aStartBlank )
{
    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
    {
        wxPGProperty* p = it.GetProperty();

        if( !p->IsCategory() && p->GetBaseName() == aKey )
        {
            m_grid->SelectProperty( p );
            m_grid->BeginLabelEdit( 0 );

            if( aStartBlank && m_grid->GetLabelEditor() )
                m_grid->GetLabelEditor()->SetValue( wxEmptyString );

            break;
        }
    }
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

        // wxPropertyGrid::PerformValidation() fails to return true when the event is vetoed
        // (though the code looks like it does it).  Anyway, for now make sure that we catch
        // it.
        m_ValueChangeVetoed = false;

        if( !m_grid->CommitChangesFromEditor() || m_ValueChangeVetoed )
            return;

        const bool forward = !aEvent.ShiftDown();

        auto isTabStop =
                []( wxPGProperty* aProp )
                {
                    return aProp && !aProp->IsCategory() && aProp->IsVisible() && aProp->IsEnabled()
                           && !aProp->HasFlag( PG_PROP_READONLY ) && aProp->GetEditorClass();
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
        // Let an open editor run it's own internal copy action
        if( !m_grid->IsEditorFocused() )
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
        // wxPropertyGrid::PerformValidation() fails to return true when the event is vetoed
        // (though the code looks like it does it).  Anyway, for now make sure that we catch
        // it.
        m_ValueChangeVetoed = false;

        if( !m_grid->CommitChangesFromEditor() || m_ValueChangeVetoed )
            return;

        CallAfter(
                [this]()
                {
                    if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
                        m_grid->SelectProperty( prop, true );
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


bool PROPERTIES_PANEL::deferRebuild()
{
    // wxPG defers property deletion while one of its events is being processed
    // (m_processedEvent != nullptr), and wxPropertyGridPageState::DoClear() then
    // leaves the old rows in place; re-appending the new set on top of them
    // duplicates every row.  This happens when e.g. the context menu (opened from
    // a grid right-click) removes a property.  Re-run once the event has unwound.
    if( static_cast<PROPERTIES_PANEL_GRID*>( m_grid )->IsProcessingWxPGEvent() )
    {
        hideCategoryButtons();
        m_deferredRebuildTimer.StartOnce( 1 );
        return true;
    }

    m_deferredRebuildTimer.Stop();
    return false;
}


bool PROPERTIES_PANEL::hasCategoryButton( const wxString& aGroupName ) const
{
    return std::ranges::any_of( m_categoryButtons,
                                [&]( const CATEGORY_BUTTON& aButton )
                                {
                                    return aButton.groupKey == aGroupName
                                           && ( aButton.forceCategory || aGroupName == _HKI( "Custom Properties" ) )
                                           && ( !aButton.enableFunc || aButton.enableFunc() );
                                } );
}


void PROPERTIES_PANEL::commitEditorIfSelected( TOOLKIT_PG_PROPERTY* aProp )
{
    if( aProp == m_grid->GetSelection() && m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();
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


void PROPERTIES_PANEL::addCategoryButton( const wxString& aGroupKey, const wxString& aTooltip, BITMAPS aBitmap,
                                          std::function<void()> aAction, std::function<bool()> aEnableFunc,
                                          bool aForceCategory )
{
    BITMAP_BUTTON* button = new BITMAP_BUTTON( m_grid, wxID_ANY );
    button->SetBitmap( KiBitmapBundle( aBitmap ) );
    button->SetToolTip( aTooltip );
    button->Hide();

    button->Bind( wxEVT_BUTTON,
                  [this, button]( wxCommandEvent& )
                  {
                      for( CATEGORY_BUTTON& entry : m_categoryButtons )
                      {
                          if( entry.button == button )
                          {
                              entry.action();
                              return;
                          }
                      }
                  } );

    m_categoryButtons.emplace_back( CATEGORY_BUTTON{ .button = button,
                                                     .groupKey = aGroupKey,
                                                     .action = std::move( aAction ),
                                                     .enableFunc = std::move( aEnableFunc ),
                                                     .forceCategory = aForceCategory } );
}


wxPGProperty* PROPERTIES_PANEL::categoryForGroup( const wxString& aGroupKey ) const
{
    const wxString caption = wxGetTranslation( aGroupKey );

    for( wxPropertyGridIterator it = m_grid->GetIterator( wxPG_ITERATE_VISIBLE ); !it.AtEnd(); it.Next() )
    {
        if( wxPGProperty* pgProp = it.GetProperty(); pgProp->IsCategory() && pgProp->GetLabel() == caption )
            return pgProp;
    }

    return nullptr;
}


void PROPERTIES_PANEL::updateCategoryButtons()
{
    positionCategoryButtons();
}


void PROPERTIES_PANEL::hideCategoryButtons()
{
    for( CATEGORY_BUTTON& entry : m_categoryButtons )
        entry.button->Hide();
}


void PROPERTIES_PANEL::positionCategoryButtons()
{
    const int rightEdgeBase = m_grid->GetClientSize().x - m_grid->FromDIP( 4 );
    int       nextRightEdge = rightEdgeBase;
    wxString  currentRow;
    bool      haveCurrentRow = false;

    for( CATEGORY_BUTTON& entry : m_categoryButtons )
    {
        if( bool sameRow = haveCurrentRow && currentRow == entry.groupKey; !sameRow )
        {
            currentRow = entry.groupKey;
            haveCurrentRow = true;
            nextRightEdge = rightEdgeBase;
        }

        wxPGProperty* category = categoryForGroup( entry.groupKey );
        bool          visible = false;
        int           rowY = 0;
        int           rowHeight = m_grid->GetRowHeight();

        if( category && ( !entry.enableFunc || entry.enableFunc() ) )
        {
            rowY = m_grid->CalcScrolledPosition( wxPoint( 0, category->GetY() ) ).y;
            visible = ( rowY + rowHeight > 0 ) && ( rowY < m_grid->GetClientSize().y );
        }

        if( !visible )
        {
            entry.button->Hide();
            continue;
        }

        const wxSize btnSize = entry.button->GetSize();
        const int    btnY = rowY + ( rowHeight - btnSize.y ) / 2;
        const int    btnX = nextRightEdge - btnSize.x;

        if( !entry.button->IsShown() )
            entry.button->Show();

        entry.button->SetPosition( wxPoint( std::max( 0, btnX ), std::max( 0, btnY ) ) );
        entry.button->Raise();

        nextRightEdge = btnX - m_grid->FromDIP( 2 );
    }
}
