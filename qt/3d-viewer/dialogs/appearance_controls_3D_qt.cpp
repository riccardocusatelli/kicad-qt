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


/**
 * @file appearance_controls_3D_qt.cpp
 * The widgets of the appearance panel of the 3D viewer, on Qt (see
 * 3d-viewer/dialogs/appearance_controls_3D_wx.cpp).
 */

#include <../3d_rendering/opengl/render_3d_opengl.h> // Must be included before any GL header

#include <dialogs/appearance_controls_3D.h>

#include <QCheckBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <bitmaps.h>
#include <confirm.h>
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_frame.h>
#include <eda_list_dialog.h>
#include <hotkeys_basic.h>
#include <board.h>
#include <tools/eda_3d_actions.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <widgets/ui_common.h>
#include <widgets/wx_infobar.h>

#include <algorithm>
#include <core/ki_assert.h>
#include <qt_strings.h>
#include <qt_ui.h>


APPEARANCE_CONTROLS_3D::APPEARANCE_CONTROLS_3D( EDA_3D_VIEWER_FRAME* aParent,
                                                TOOLKIT_WINDOW* aFocusOwner ) :
        APPEARANCE_CONTROLS_3D_BASE( aParent ),
        m_frame( aParent ),
        m_focusOwner( aFocusOwner ),
        m_lastSelectedViewport( nullptr )
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int      screenHeight = screen ? screen->size().height() : 1080;
    m_pointSize       = font().pointSize();

    SetBorders( true, false, false, false );

    // The rows of the layers, in the scrolled area
    m_layersWidget = new QWidget( m_windowLayers );
    m_layersOuterSizer = new QVBoxLayout( m_layersWidget );
    m_layersOuterSizer->setContentsMargins( 0, 0, 0, 0 );
    m_layersOuterSizer->setSpacing( 0 );
    m_windowLayers->setWidget( m_layersWidget );

    QFont infoFont = KIQT::InfoFont( this );
    m_panelLayers->setFont( infoFont );
    m_windowLayers->setFont( infoFont );
    m_presetsLabel->setFont( infoFont );
    m_viewportsLabel->setFont( infoFont );

    // Create display options
    m_cbUseBoardStackupColors = new QCheckBox( ToQString( _( "Use board stackup colors" ) ),
                                               m_panelLayers );
    m_cbUseBoardStackupColors->setFont( infoFont );

    QObject::connect( m_cbUseBoardStackupColors, &QCheckBox::clicked, this,
            [this]( bool aChecked )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_UseStackupColors = aChecked;

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_cbUseBoardEditorCopperColors = new QCheckBox( ToQString( _( "Use PCB editor copper colors" ) ),
                                                    m_panelLayers );
    m_cbUseBoardEditorCopperColors->setFont( infoFont );
    m_cbUseBoardEditorCopperColors->setToolTip( ToQString( _( "Use the board editor layer colors for copper "
                                                              "layers (realtime renderer only)" ) ) );

    QObject::connect( m_cbUseBoardEditorCopperColors, &QCheckBox::clicked, this,
            [this]( bool aChecked )
            {
                EDA_3D_VIEWER_SETTINGS* cfg = m_frame->GetAdapter().m_Cfg;
                cfg->m_Render.use_board_editor_copper_colors = aChecked;

                UpdateLayerCtls();
                m_frame->NewDisplay( true );
            } );

    m_panelLayersSizer->addWidget( m_cbUseBoardStackupColors );
    m_panelLayersSizer->addWidget( m_cbUseBoardEditorCopperColors );

    m_cbLayerPresets->setToolTip( ToQString( wxString::Format( _( "Save and restore color and visibility combinations.\n"
                                                                  "Use %s+Tab to activate selector.\n"
                                                                  "Successive Tabs while holding %s down will cycle through "
                                                                  "presets in the popup." ),
                                                               KeyNameFromKeyCode( PRESET_SWITCH_KEY ),
                                                               KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) ) );

    m_cbViewports->setToolTip( ToQString( wxString::Format( _( "Save and restore camera position and zoom.\n"
                                                               "Use %s+Tab to activate selector.\n"
                                                               "Successive Tabs while holding %s down will cycle through "
                                                               "viewports in the popup." ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) ) );

    if( screenHeight <= 900 && m_pointSize >= KIUI::c_IndicatorSizeDIP )
        m_pointSize = m_pointSize * 8 / 10;

    OnDarkModeToggle();
}


APPEARANCE_CONTROLS_3D::~APPEARANCE_CONTROLS_3D()
{
}


wxSize APPEARANCE_CONTROLS_3D::GetBestSize() const
{
    return wxSize( 220, 480 );
}


void APPEARANCE_CONTROLS_3D::OnSetFocus( QWidget* aSource )
{
    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::rebuildControls()
{
    setUpdatesEnabled( false );

    rebuildLayers();
    m_cbUseBoardStackupColors->setText( ToQString( _( "Use board stackup colors" ) ) );
    m_cbUseBoardEditorCopperColors->setText( ToQString( _( "Use PCB editor copper colors" ) ) );
    rebuildLayerPresetsWidget( true );
    rebuildViewportsWidget();

    setUpdatesEnabled( true );
}


void APPEARANCE_CONTROLS_3D::OnLanguageChanged()
{
    rebuildControls();
}


void APPEARANCE_CONTROLS_3D::OnDarkModeToggle()
{
    // The rows of the layers on a slightly lighter background than the panel
    QColor layerPanelColour = m_panelLayers->palette().color( QPalette::Window ).lighter( 110 );

    QPalette layersPalette = m_layersWidget->palette();
    layersPalette.setColor( QPalette::Window, layerPanelColour );
    m_layersWidget->setPalette( layersPalette );
    m_layersWidget->setAutoFillBackground( true );

    for( std::unique_ptr<APPEARANCE_SETTING_3D>& setting : m_layerSettings )
    {
        if( setting->m_Ctl_color )
            setting->m_Ctl_color->OnDarkModeToggle();
    }
}


void APPEARANCE_CONTROLS_3D::ApplyViewport( const wxString& aViewportName )
{
    int idx = m_cbViewports->findText( ToQString( aViewportName ) );

    if( idx >= 0 && idx < m_cbViewports->count() - 3 /* separator */ )
    {
        m_cbViewports->setCurrentIndex( idx );
        m_lastSelectedViewport = viewportAt( idx );
    }
    else
    {
        m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 ); // separator
        m_lastSelectedViewport = nullptr;
    }

    if( m_lastSelectedViewport )
        doApplyViewport( *m_lastSelectedViewport );
}


VIEWPORT3D* APPEARANCE_CONTROLS_3D::viewportAt( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= m_cbViewports->count() )
        return nullptr;

    return static_cast<VIEWPORT3D*>( m_cbViewports->itemData( aIndex ).value<void*>() );
}


void APPEARANCE_CONTROLS_3D::rebuildLayers()
{
    int swatchWidth = COLOR_SWATCH::DialogUnitsToPixels( m_windowLayers, wxSize( 8, 0 ) ).x;

    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();
    std::map<int, COLOR4D>    defaultColors = m_frame->GetAdapter().GetDefaultColors();
    LSET                      enabled = m_frame->GetBoard()->GetEnabledLayers();
    COLOR4D                   background = FromQColor( m_layersWidget->palette().color( QPalette::Window ) );

    m_layerSettings.clear();
    m_layerSettingsMap.clear();

    // The rows of the previous layers go
    while( QLayoutItem* item = m_layersOuterSizer->takeAt( 0 ) )
    {
        if( QWidget* widget = item->widget() )
            delete widget;

        delete item;
    }

    m_layersOuterSizer->addSpacing( 5 );

    auto appendLayer =
            [&]( const std::unique_ptr<APPEARANCE_SETTING_3D>& aSetting )
            {
                QWidget*     row = new QWidget( m_layersWidget );
                QHBoxLayout* sizer = new QHBoxLayout( row );
                int          layer = aSetting->m_Id;

                sizer->setContentsMargins( 5, 0, 5, 0 );
                sizer->setSpacing( 0 );

                aSetting->m_Visible = visibleLayers.test( layer );

                if( colors.count( layer ) )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( row, colors[ layer ], layer,
                                                             COLOR4D::WHITE, defaultColors[ layer ],
                                                             SWATCH_SMALL );
                    swatch->setToolTip( ToQString( _( "Left double click or middle click to change color" ) ) );
                    swatch->SetSwatchBackground( background );

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

                    swatch->SetChangedCallback(
                            [this, swatch]()
                            {
                                onColorSwatchChanged( swatch );

                                passOnFocus();
                            } );

                    sizer->addWidget( swatch, 0, Qt::AlignVCenter );
                    aSetting->m_Ctl_color = swatch;
                }
                else
                {
                    sizer->addSpacing( swatchWidth );
                }

                sizer->addSpacing( 5 );

                wxString     layerName = aSetting->GetLabel();
                PCB_LAYER_ID boardLayer = Map3DLayerToPCBLayer( layer );

                if( boardLayer != UNDEFINED_LAYER )
                    layerName = m_frame->GetBoard()->GetLayerName( boardLayer );

                QLabel* label = new QLabel( ToQString( layerName ), row );
                label->setToolTip( ToQString( aSetting->GetTooltip() ) );

                if( layer == LAYER_3D_BACKGROUND_TOP || layer == LAYER_3D_BACKGROUND_BOTTOM )
                {
                    sizer->addSpacing( swatchWidth );
                }
                else
                {
                    BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( row, layer,
                                                                    KiBitmapBundle( BITMAPS::visibility ),
                                                                    KiBitmapBundle( BITMAPS::visibility_off ),
                                                                    aSetting->m_Visible );

                    btn_visible->SetToggleCallback(
                            [this, layer]( bool aVisible )
                            {
                                OnLayerVisibilityChanged( layer, aVisible );

                                passOnFocus();
                            } );

                    wxString tip;
                    tip.Printf( _( "Show or hide %s" ), aSetting->GetLabel().Lower() );
                    btn_visible->setToolTip( ToQString( tip ) );

                    aSetting->m_Ctl_visibility = btn_visible;
                    sizer->addWidget( btn_visible, 0, Qt::AlignVCenter );
                }

                sizer->addSpacing( 5 );
                sizer->addWidget( label, 1, Qt::AlignVCenter );

                m_layersOuterSizer->addWidget( row );
                m_layersOuterSizer->addSpacing( 2 );
            };

    for( const APPEARANCE_SETTING_3D& s_setting : s_layerSettings )
    {
        m_layerSettings.emplace_back( std::make_unique<APPEARANCE_SETTING_3D>( s_setting ) );
        std::unique_ptr<APPEARANCE_SETTING_3D>& setting = m_layerSettings.back();

        if( setting->m_Spacer )
        {
            m_layersOuterSizer->addSpacing( m_pointSize );
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

    m_layersOuterSizer->addStretch( 1 );
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
        m_cbUseBoardStackupColors->setChecked( cfg->m_UseStackupColors );
        m_cbUseBoardEditorCopperColors->setChecked( cfg->m_Render.use_board_editor_copper_colors );
    }
}


void APPEARANCE_CONTROLS_3D::rebuildLayerPresetsWidget( bool aReset )
{
    m_presetsLabel->setText( ToQString( wxString::Format( _( "Presets (%s+Tab):" ),
                                                          KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) ) );

    QSignalBlocker blocker( m_cbLayerPresets );

    m_cbLayerPresets->clear();

    // Build the layers preset list.

    m_cbLayerPresets->addItem( ToQString( _( "Follow PCB Editor" ) ) );
    m_cbLayerPresets->addItem( ToQString( _( "Follow PCB Plot Settings" ) ) );

    if( !m_frame->GetAdapter().m_Cfg->m_LayerPresets.empty() )
        m_cbLayerPresets->addItem( QStringLiteral( "---" ) );

    for( const LAYER_PRESET_3D& preset : m_frame->GetAdapter().m_Cfg->m_LayerPresets )
        m_cbLayerPresets->addItem( ToQString( preset.name ) );

    m_cbLayerPresets->addItem( QStringLiteral( "---" ) );
    m_cbLayerPresets->addItem( ToQString( _( "Save preset..." ) ) );
    m_cbLayerPresets->addItem( ToQString( _( "Delete preset..." ) ) );

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
                if( aPreset.name.Lower() == _( "legacy colors" ) && m_cbUseBoardStackupColors->isChecked() )
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
        m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->findText( ToQString( it->name ) ) );
    }
    else
    {
        m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->count() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS_3D::updateLayerPresetWidget( const wxString& aName )
{
    if( aName == FOLLOW_PCB )
    {
        m_cbLayerPresets->setCurrentIndex( 0 );
    }
    else if( aName == FOLLOW_PLOT_SETTINGS )
    {
        m_cbLayerPresets->setCurrentIndex( 1 );
    }
    else
    {
        int idx = m_cbLayerPresets->findText( ToQString( aName ) );

        if( idx >= 0 )
            m_cbLayerPresets->setCurrentIndex( idx );
        else
            m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->count() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS_3D::onLayerPresetChanged( QWidget* aSource )
{
    EDA_3D_VIEWER_SETTINGS*   cfg = m_frame->GetAdapter().m_Cfg;
    int                       count = m_cbLayerPresets->count();
    int                       index = m_cbLayerPresets->currentIndex();
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
        bool    ok = false;
        QString text = QInputDialog::getText( window(), ToQString( _( "Save Layer Preset" ) ),
                                              ToQString( _( "Layer preset name:" ) ),
                                              QLineEdit::Normal, QString(), &ok );

        if( !ok )
        {
            resetSelection();
            return;
        }

        std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
        std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

        name = ToWxString( text );

        if( LAYER_PRESET_3D* preset = cfg->FindPreset( name ) )
        {
            if( !IsOK( window(), _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            preset->layers = visibleLayers;
            preset->colors = colors;
            m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->findText( text ) );
        }
        else
        {
            cfg->m_LayerPresets.emplace_back( name, visibleLayers, colors );
            rebuildLayerPresetsWidget( false );
            m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->findText( text ) );
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

            int idx = m_cbLayerPresets->findText( ToQString( name ) );

            if( idx >= 0 )
                m_cbLayerPresets->removeItem( idx );

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
    else if( m_cbLayerPresets->itemText( index ) == QStringLiteral( "---" ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else
    {
        doApplyLayerPreset( ToWxString( m_cbLayerPresets->currentText() ) );
    }

    // Move to front of MRU list
    if( m_presetMRU.Index( name ) != wxNOT_FOUND )
        m_presetMRU.Remove( name );

    m_presetMRU.Insert( name, 0 );

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::rebuildViewportsWidget()
{
    m_viewportsLabel->setText( ToQString( wxString::Format( _( "Viewports (%s+Tab):" ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) ) );

    QSignalBlocker blocker( m_cbViewports );

    m_cbViewports->clear();

    for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        m_cbViewports->addItem( ToQString( pair.first ), QVariant::fromValue( static_cast<void*>( &pair.second ) ) );

    m_cbViewports->addItem( QStringLiteral( "---" ) );
    m_cbViewports->addItem( ToQString( _( "Save viewport..." ) ) );
    m_cbViewports->addItem( ToQString( _( "Delete viewport..." ) ) );

    m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 );
    m_lastSelectedViewport = nullptr;
}


void APPEARANCE_CONTROLS_3D::onViewportChanged( QWidget* aSource )
{
    int count = m_cbViewports->count();
    int index = m_cbViewports->currentIndex();

    auto resetSelection =
            [&]()
            {
                if( m_lastSelectedViewport )
                    m_cbViewports->setCurrentIndex( m_cbViewports->findText( ToQString( m_lastSelectedViewport->name ) ) );
                else
                    m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 );
            };

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = viewportAt( index );

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
        bool    ok = false;
        QString text = QInputDialog::getText( window(), ToQString( _( "Save Viewport" ) ),
                                              ToQString( _( "Viewport name:" ) ),
                                              QLineEdit::Normal, QString(), &ok );

        if( !ok )
        {
            resetSelection();
            return;
        }

        wxString name = ToWxString( text );
        bool     exists = m_viewports.count( name );

        if( !exists )
        {
            m_viewports[name] = VIEWPORT3D( name, m_frame->GetCurrentCamera().GetViewMatrix() );

            index = index - 1;
            m_cbViewports->insertItem( index, text, QVariant::fromValue( static_cast<void*>( &m_viewports[name] ) ) );
        }
        else
        {
            m_viewports[name].matrix = m_frame->GetCurrentCamera().GetViewMatrix();
            index = m_cbViewports->findText( text );

            if( m_viewportMRU.Index( name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( name );
        }

        m_cbViewports->setCurrentIndex( index );
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
            int idx = m_cbViewports->findText( ToQString( viewportName ) );

            if( idx >= 0 )
            {
                if( m_lastSelectedViewport == viewportAt( idx ) )
                    m_lastSelectedViewport = nullptr;

                m_cbViewports->removeItem( idx );
                m_viewports.erase( viewportName );
            }

            if( m_viewportMRU.Index( viewportName ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewportName );
        }

        resetSelection();
        return;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS_3D::onUpdateViewportsCb( QWidget* aSource )
{
    int count = m_cbViewports->count();
    int index = m_cbViewports->currentIndex();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = viewportAt( index );

        KI_CHECK( viewport, /* void */ );

        if( m_frame->GetCurrentCamera().GetViewMatrix() != viewport->matrix )
            m_cbViewports->setCurrentIndex( -1 );
    }
}


void APPEARANCE_CONTROLS_3D::passOnFocus()
{
    if( m_focusOwner )
        m_focusOwner->setFocus();
}
