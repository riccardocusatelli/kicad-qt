/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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

/**
 * @file appearance_controls_3D_wx.cpp
 * The widgets of the appearance panel of the 3D viewer, on wxWidgets.
 */

#include <../3d_rendering/opengl/render_3d_opengl.h> // Must be included before any GL header

#include <dialogs/appearance_controls_3D.h>

#include <bitmaps.h>
#include <settings/color_settings.h>
#include <confirm.h>
#include <pgm_base.h>
#include <dpi_scaling_common.h>
#include <eda_list_dialog.h>
#include <pcb_display_options.h>
#include <eda_3d_viewer_frame.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <board.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/eda_3d_actions.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <widgets/wx_infobar.h>
#include <widgets/grid_bitmap_toggle.h>
#include <dialogs/eda_view_switcher.h>
#include <wx/bmpbuttn.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/checkbox.h>

#include <algorithm>
#include <core/ki_assert.h>


APPEARANCE_CONTROLS_3D::APPEARANCE_CONTROLS_3D( EDA_3D_VIEWER_FRAME* aParent, wxWindow* aFocusOwner ) :
        APPEARANCE_CONTROLS_3D_BASE( aParent ),
        m_frame( aParent ),
        m_focusOwner( aFocusOwner ),
        m_lastSelectedViewport( nullptr )
{
    DPI_SCALING_COMMON dpi( nullptr, m_frame );

    int screenHeight  = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    m_pointSize       = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ).GetPointSize();

    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );
    SetBorders( true, false, false, false );

    m_layersOuterSizer = new wxBoxSizer( wxVERTICAL );
    m_windowLayers->SetSizer( m_layersOuterSizer );
    m_windowLayers->SetScrollRate( 0, 5 );
    m_windowLayers->Bind( wxEVT_SET_FOCUS, &APPEARANCE_CONTROLS_3D::OnSetFocus, this );

    m_envOuterSizer = new wxBoxSizer( wxVERTICAL );

    wxFont infoFont = KIUI::GetInfoFont( this );
    m_panelLayers->SetFont( infoFont );
    m_windowLayers->SetFont( infoFont );
    m_presetsLabel->SetFont( infoFont );
    m_viewportsLabel->SetFont( infoFont );

    // Create display options
    m_cbUseBoardStackupColors = new wxCheckBox( m_panelLayers, wxID_ANY,
                                                _( "Use board stackup colors" ) );
    m_cbUseBoardStackupColors->SetFont( infoFont );

    m_cbUseBoardStackupColors->Bind( wxEVT_CHECKBOX,
            [this]( wxCommandEvent& aEvent )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_UseStackupColors = aEvent.IsChecked();

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_cbUseBoardEditorCopperColors = new wxCheckBox( m_panelLayers, wxID_ANY,
                                                     _( "Use PCB editor copper colors" ) );
    m_cbUseBoardEditorCopperColors->SetFont( infoFont );
    m_cbUseBoardEditorCopperColors->SetToolTip( _( "Use the board editor layer colors for copper "
                                                   "layers (realtime renderer only)" ) );

    m_cbUseBoardEditorCopperColors->Bind( wxEVT_CHECKBOX,
            [this]( wxCommandEvent& aEvent )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_Render.use_board_editor_copper_colors = aEvent.IsChecked();

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_panelLayersSizer->Add( m_cbUseBoardStackupColors, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5 );
    m_panelLayersSizer->Add( m_cbUseBoardEditorCopperColors, 0,
                             wxEXPAND | wxALL, 5 );

    m_cbLayerPresets->SetToolTip( wxString::Format( _( "Save and restore color and visibility combinations.\n"
                                                       "Use %s+Tab to activate selector.\n"
                                                       "Successive Tabs while holding %s down will cycle through "
                                                       "presets in the popup." ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ),
                                                    KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbViewports->SetToolTip( wxString::Format( _( "Save and restore camera position and zoom.\n"
                                                    "Use %s+Tab to activate selector.\n"
                                                    "Successive Tabs while holding %s down will cycle through "
                                                    "viewports in the popup." ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ),
                                                 KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    if( screenHeight <= 900 && m_pointSize >= FromDIP( KIUI::c_IndicatorSizeDIP ) )
        m_pointSize = m_pointSize * 8 / 10;

    m_cbLayerPresets->Bind( wxEVT_CHOICE, &APPEARANCE_CONTROLS_3D::onLayerPresetChanged, this );

    m_toggleGridRenderer = new GRID_BITMAP_TOGGLE_RENDERER(
            KiBitmapBundle( BITMAPS::visibility ), KiBitmapBundle( BITMAPS::visibility_off ) );

    m_frame->Bind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS_3D::OnLanguageChanged, this );
}


APPEARANCE_CONTROLS_3D::~APPEARANCE_CONTROLS_3D()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &APPEARANCE_CONTROLS_3D::OnLanguageChanged, this );
}


wxSize APPEARANCE_CONTROLS_3D::GetBestSize() const
{
    DPI_SCALING_COMMON dpi( nullptr, m_frame );
    wxSize      size( 220 * dpi.GetScaleFactor(), 480 * dpi.GetScaleFactor() );
    return size;
}


void APPEARANCE_CONTROLS_3D::OnSetFocus( wxFocusEvent& aEvent )
{
#ifdef __WXMSW__
    // In wxMSW, buttons won't process events unless they have focus, so we'll let it take the
    // focus and give it back to the parent in the button event handler.
    if( wxBitmapButton* btn = dynamic_cast<wxBitmapButton*>( aEvent.GetEventObject() ) )
    {
        wxCommandEvent evt( wxEVT_BUTTON );
        wxPostEvent( btn, evt );
    }
#endif

    passOnFocus();
    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::OnSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::rebuildControls()
{
    Freeze();

    rebuildLayers();
    m_cbUseBoardStackupColors->SetLabel( _( "Use board stackup colors" ) );
    rebuildLayerPresetsWidget( true );
    rebuildViewportsWidget();

    Thaw();
    Refresh();
}


void APPEARANCE_CONTROLS_3D::OnLanguageChanged( wxCommandEvent& aEvent )
{
    rebuildControls();

    aEvent.Skip();
}


void APPEARANCE_CONTROLS_3D::OnDarkModeToggle()
{
    // This is essentially a list of hacks because DarkMode isn't yet implemented inside
    // wxWidgets.
    //
    // The individual wxPanels, COLOR_SWATCHes and GRID_CELL_COLOR_RENDERERs should really be
    // overriding some virtual method or responding to some wxWidgets event so that the parent
    // doesn't have to know what it contains.  But, that's not where we are, so... :shrug:

    m_layerPanelColour = m_panelLayers->GetBackgroundColour().ChangeLightness( 110 );

    m_windowLayers->SetBackgroundColour( m_layerPanelColour );

    for( wxSizerItem* child : m_layersOuterSizer->GetChildren() )
    {
        if( child && child->GetWindow() )
            child->GetWindow()->SetBackgroundColour( m_layerPanelColour );
    }
}


void APPEARANCE_CONTROLS_3D::ApplyViewport( const wxString& aViewportName )
{
    int idx = m_cbViewports->FindString( aViewportName );

    if( idx >= 0 && idx < (int)m_cbViewports->GetCount() - 3 /* separator */ )
    {
        m_cbViewports->SetSelection( idx );
        m_lastSelectedViewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( idx ) );
    }
    else
    {
        m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 ); // separator
        m_lastSelectedViewport = nullptr;
    }

    if( m_lastSelectedViewport )
        doApplyViewport( *m_lastSelectedViewport );
}


void APPEARANCE_CONTROLS_3D::rebuildLayers()
{
    int swatchWidth = m_windowLayers->ConvertDialogToPixels( wxSize( 8, 0 ) ).x;

    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();
    std::map<int, COLOR4D>    defaultColors = m_frame->GetAdapter().GetDefaultColors();
    LSET                      enabled = m_frame->GetBoard()->GetEnabledLayers();

    m_layerSettings.clear();
    m_layersOuterSizer->Clear( true );
    m_layersOuterSizer->AddSpacer( 5 );

    m_envOuterSizer->Clear( true );

    auto appendLayer =
            [&]( const std::unique_ptr<APPEARANCE_SETTING_3D>& aSetting )
            {
                wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
                int         layer = aSetting->m_Id;

                aSetting->m_Visible = visibleLayers.test( layer );

                if( colors.count( layer ) )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_windowLayers, colors[ layer ], layer,
                                                             COLOR4D::WHITE, defaultColors[ layer ],
                                                             SWATCH_SMALL );
                    swatch->SetToolTip( _( "Left double click or middle click to change color" ) );

                    swatch->SetReadOnlyCallback(
                            [this]()
                            {
                                WX_INFOBAR* infobar = m_frame->GetInfoBar();

                                infobar->RemoveAllButtons();
                                infobar->AddCloseButton();

                                infobar->ShowMessageFor( _( "Uncheck 'Use board stackup colors' to "
                                                            "allow color editing." ),
                                                         10000, wxICON_INFORMATION );
                            } );

                    sizer->Add( swatch, 0,  wxALIGN_CENTER_VERTICAL, 0 );
                    aSetting->m_Ctl_color = swatch;

                    swatch->Bind( COLOR_SWATCH_CHANGED,
                            [this]( wxCommandEvent& event )
                            {
                                auto swatch = static_cast<COLOR_SWATCH*>( event.GetEventObject() );
                                onColorSwatchChanged( swatch );

                                passOnFocus();
                            } );
                }
                else
                {
                    sizer->AddSpacer( swatchWidth  );
                }

                sizer->AddSpacer( 5 );

                wxString     layerName = aSetting->GetLabel();
                PCB_LAYER_ID boardLayer = Map3DLayerToPCBLayer( layer );

                if( boardLayer != UNDEFINED_LAYER )
                    layerName = m_frame->GetBoard()->GetLayerName( boardLayer );

                wxStaticText* label = new wxStaticText( m_windowLayers, layer, layerName );
                label->Wrap( -1 );
                label->SetToolTip( aSetting->GetTooltip() );

                if( layer == LAYER_3D_BACKGROUND_TOP || layer == LAYER_3D_BACKGROUND_BOTTOM )
                {
                    sizer->AddSpacer( swatchWidth );
                }
                else
                {
                    BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( m_windowLayers, layer,
                                                                    KiBitmapBundle( BITMAPS::visibility ),
                                                                    KiBitmapBundle( BITMAPS::visibility_off ),
                                                                    aSetting->m_Visible );

                    btn_visible->Bind( TOGGLE_CHANGED,
                            [this]( wxCommandEvent& aEvent )
                            {
                                int id = static_cast<wxWindow*>( aEvent.GetEventObject() )->GetId();
                                bool isVisible = aEvent.GetInt();
                                OnLayerVisibilityChanged( id, isVisible );

                                passOnFocus();
                            } );

                    wxString tip;
                    tip.Printf( _( "Show or hide %s" ), aSetting->GetLabel().Lower() );
                    btn_visible->SetToolTip( tip );

                    aSetting->m_Ctl_visibility = btn_visible;
                    sizer->Add( btn_visible, 0, wxALIGN_CENTER_VERTICAL, 0 );
                }

                sizer->AddSpacer( 5 );
                sizer->Add( label, 0, wxALIGN_CENTER_VERTICAL, 0 );

                m_layersOuterSizer->Add( sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );
                m_layersOuterSizer->AddSpacer( 2 );
            };

    for( const APPEARANCE_SETTING_3D& s_setting : s_layerSettings )
    {
        m_layerSettings.emplace_back( std::make_unique<APPEARANCE_SETTING_3D>( s_setting ) );
        std::unique_ptr<APPEARANCE_SETTING_3D>& setting = m_layerSettings.back();

        if( setting->m_Spacer )
        {
            m_layersOuterSizer->AddSpacer( m_pointSize );
        }
        else if( setting->m_Id >= LAYER_3D_USER_1 && setting->m_Id <= LAYER_3D_USER_45 )
        {
            if( enabled.test( Map3DLayerToPCBLayer( setting->m_Id ) ) )
                appendLayer( setting );
        }
        else
        {
            appendLayer( setting );
        }

        m_layerSettingsMap[setting->m_Id] = setting.get();
    }

    m_sizerOuter->Layout();
}


void APPEARANCE_CONTROLS_3D::UpdateLayerCtls()
{
    EDA_3D_VIEWER_SETTINGS*   cfg = m_frame->GetAdapter().m_Cfg;
    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

    for( std::unique_ptr<APPEARANCE_SETTING_3D>& setting : m_layerSettings )
    {
        if( setting->m_Spacer )
            continue;

        if( setting->m_Ctl_visibility )
            setting->m_Ctl_visibility->SetValue( visibleLayers.test( setting->m_Id ) );

        if( setting->m_Ctl_color )
        {
            setting->m_Ctl_color->SetSwatchColor( colors[ setting->m_Id ], false );

            // if cfg->m_UseStackupColors is set, board colors cannot be modified locally, but
            // other colors can be
            if( alg::contains( s_inStackupColors, setting->m_Id ) && cfg )
                setting->m_Ctl_color->SetReadOnly( cfg->m_UseStackupColors );
        }
    }

    if( cfg )
    {
        m_cbUseBoardStackupColors->SetValue( cfg->m_UseStackupColors );
        m_cbUseBoardEditorCopperColors->SetValue( cfg->m_Render.use_board_editor_copper_colors );
    }
}


void APPEARANCE_CONTROLS_3D::rebuildLayerPresetsWidget( bool aReset )
{
    m_presetsLabel->SetLabel( wxString::Format( _( "Presets (%s+Tab):" ),
                                                KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) );

    m_cbLayerPresets->Clear();

    // Build the layers preset list.

    m_cbLayerPresets->Append( _( "Follow PCB Editor" ) );
    m_cbLayerPresets->Append( _( "Follow PCB Plot Settings" ) );

    if( !m_frame->GetAdapter().m_Cfg->m_LayerPresets.empty() )
        m_cbLayerPresets->Append( wxT( "---" ) );

    for( const LAYER_PRESET_3D& preset : m_frame->GetAdapter().m_Cfg->m_LayerPresets )
        m_cbLayerPresets->Append( preset.name );

    m_cbLayerPresets->Append( wxT( "---" ) );
    m_cbLayerPresets->Append( _( "Save preset..." ) );
    m_cbLayerPresets->Append( _( "Delete preset..." ) );

    if( aReset )
        updateLayerPresetWidget( m_frame->GetAdapter().m_Cfg->m_CurrentPreset );
}


void APPEARANCE_CONTROLS_3D::syncLayerPresetSelection()
{
    m_frame->GetAdapter().m_Cfg->m_CurrentPreset = wxEmptyString;

    std::vector<LAYER_PRESET_3D>& presets = m_frame->GetAdapter().m_Cfg->m_LayerPresets;
    std::bitset<LAYER_3D_END>     visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>        colors = m_frame->GetAdapter().GetLayerColors();

    auto it = std::find_if(
            presets.begin(), presets.end(),
            [&]( const LAYER_PRESET_3D& aPreset )
            {
                if( aPreset.name.Lower() == _( "legacy colors" ) && m_cbUseBoardStackupColors->GetValue() )
                    return false;

                for( int layer = LAYER_3D_BOARD; layer < LAYER_3D_END; ++layer )
                {
                    if( aPreset.layers.test( layer ) != visibleLayers.test( layer ) )
                        return false;
                }

                for( int layer : { LAYER_FP_REFERENCES, LAYER_FP_VALUES, LAYER_FP_TEXT } )
                {
                    if( aPreset.layers.test( layer ) != visibleLayers.test( layer ) )
                        return false;
                }

                for( int layer = LAYER_3D_START + 1; layer < LAYER_3D_END; ++layer )
                {
                    auto it1 = aPreset.colors.find( layer );
                    auto it2 = colors.find( layer );

                    if( it1 != aPreset.colors.end() && it2 != colors.end() && *it1 != *it2 )
                        return false;
                }

                return true;
            } );

    if( it != presets.end() )
    {
        m_frame->GetAdapter().m_Cfg->m_CurrentPreset = it->name;
        m_cbLayerPresets->SetStringSelection( it->name );
    }
    else
    {
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS_3D::updateLayerPresetWidget( const wxString& aName )
{
    if( aName == FOLLOW_PCB )
        m_cbLayerPresets->SetSelection( 0 );
    else if( aName == FOLLOW_PLOT_SETTINGS )
        m_cbLayerPresets->SetSelection( 1 );
    else if( !m_cbLayerPresets->SetStringSelection( aName ) )
        m_cbLayerPresets->SetSelection( m_cbLayerPresets->GetCount() - 3 ); // separator
}


void APPEARANCE_CONTROLS_3D::onLayerPresetChanged( wxCommandEvent& aEvent )
{
    EDA_3D_VIEWER_SETTINGS*   cfg = m_frame->GetAdapter().m_Cfg;
    int                       count = m_cbLayerPresets->GetCount();
    int                       index = m_cbLayerPresets->GetSelection();
    wxString                  name;

    auto resetSelection =
            [&]()
            {
                updateLayerPresetWidget( cfg->m_CurrentPreset );
            };

    if( index == 0 )
    {
        doApplyLayerPreset( FOLLOW_PCB );
    }
    else if( index == 1 )
    {
        doApplyLayerPreset( FOLLOW_PLOT_SETTINGS );
    }
    else if( index == count - 2 )
    {
        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Layer preset name:" ),
                               _( "Save Layer Preset" ) );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
        std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

        name = dlg.GetValue();

        if( LAYER_PRESET_3D* preset = cfg->FindPreset( name ) )
        {
            if( !IsOK( wxGetTopLevelParent( this ), _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            preset->layers = visibleLayers;
            preset->colors = colors;
            m_cbLayerPresets->SetSelection( m_cbLayerPresets->FindString( name ) );
        }
        else
        {
            cfg->m_LayerPresets.emplace_back( name, visibleLayers, colors );
            rebuildLayerPresetsWidget( false );
            m_cbLayerPresets->SetStringSelection( name );
        }

        cfg->m_CurrentPreset = name;
        m_presetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( LAYER_PRESET_3D& preset : cfg->m_LayerPresets )
        {
            wxArrayString item;
            item.Add( preset.name );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            name = dlg.GetTextSelection();

            if( m_cbLayerPresets->FindString( name ) != wxNOT_FOUND )
                m_cbLayerPresets->Delete( m_cbLayerPresets->FindString( name ) );

            std::erase_if( cfg->m_LayerPresets,
                    [name]( const LAYER_PRESET_3D& preset )
                    {
                        return preset.name == name;
                    } );

            if( cfg->m_CurrentPreset == name )
                cfg->m_CurrentPreset = wxEmptyString;

            if( m_presetMRU.Index( name ) >= 0 )
                m_presetMRU.Remove( name );
        }

        resetSelection();
        return;
    }
    else if( m_cbLayerPresets->GetString( index ) == wxT( "---" ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else
    {
        doApplyLayerPreset( m_cbLayerPresets->GetStringSelection() );
    }

    // Move to front of MRU list
    if( m_presetMRU.Index( name ) != wxNOT_FOUND )
        m_presetMRU.Remove( name );

    m_presetMRU.Insert( name, 0 );

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::rebuildViewportsWidget()
{
    m_viewportsLabel->SetLabel( wxString::Format( _( "Viewports (%s+Tab):" ),
                                                  KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) );

    m_cbViewports->Clear();

    for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        m_cbViewports->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbViewports->Append( wxT( "---" ) );
    m_cbViewports->Append( _( "Save viewport..." ) );
    m_cbViewports->Append( _( "Delete viewport..." ) );

    m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );
    m_lastSelectedViewport = nullptr;
}


void APPEARANCE_CONTROLS_3D::onViewportChanged( wxCommandEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        KI_CHECK( viewport, /* void */ );

        doApplyViewport( *viewport );

        if( !viewport->name.IsEmpty() )
        {
            if( m_viewportMRU.Index( viewport->name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewport->name );

            m_viewportMRU.Insert( viewport->name, 0 );
        }
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "Viewport name:" ),
                               _( "Save Viewport" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            if( m_lastSelectedViewport )
                m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
            else
                m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );

            return;
        }

        name = dlg.GetValue();
        bool exists = m_viewports.count( name );

        if( !exists )
        {
            m_viewports[name] = VIEWPORT3D( name, m_frame->GetCurrentCamera().GetViewMatrix() );

            index = m_cbViewports->Insert( name, index-1, static_cast<void*>( &m_viewports[name] ) );
        }
        else
        {
            m_viewports[name].matrix = m_frame->GetCurrentCamera().GetViewMatrix();
            index = m_cbViewports->FindString( name );

            if( m_viewportMRU.Index( name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( name );
        }

        m_cbViewports->SetSelection( index );
        m_viewportMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete an existing viewport
        wxArrayString headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Viewports" ) );

        for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        {
            wxArrayString item;
            item.Add( pair.first );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Viewport" ), headers, items );
        dlg.SetListLabel( _( "Select viewport:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString viewportName = dlg.GetTextSelection();
            int idx = m_cbViewports->FindString( viewportName );

            if( idx != wxNOT_FOUND )
            {
                m_viewports.erase( viewportName );
                m_cbViewports->Delete( idx );
            }

            if( m_viewportMRU.Index( viewportName ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewportName );
        }

        if( m_lastSelectedViewport )
            m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
        else
            m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );

        return;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::onUpdateViewportsCb( wxUpdateUIEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        KI_CHECK( viewport, /* void */ );

        if( m_frame->GetCurrentCamera().GetViewMatrix() != viewport->matrix )
            m_cbViewports->SetSelection( -1 );
    }
}


void APPEARANCE_CONTROLS_3D::passOnFocus()
{
    m_focusOwner->SetFocus();
}
