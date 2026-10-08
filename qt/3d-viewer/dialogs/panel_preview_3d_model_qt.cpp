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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file panel_preview_3d_model_qt.cpp
 * PANEL_PREVIEW_3D_MODEL of the Qt build (see 3d-viewer/dialogs/panel_preview_3d_model.cpp).
 */

#include <3d_rendering/opengl/render_3d_opengl.h> // Must be included before any GL header

#include <cmath>

#include <QSignalBlocker>
#include <QWheelEvent>

#include <dialogs/panel_preview_3d_model.h>
#include <dialogs/panel_preview_3d_model_values.h>
#include <dialogs/dialog_unit_entry.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/eda_3d_actions.h>
#include <tools/eda_3d_controller.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <common_ogl/ogl_attr_list.h>
#include <dialog_shim.h>
#include <footprint.h>
#include <lset.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <toolkit/gui_services.h>
#include <widgets/wx_infobar.h>
#include <widgets/std_bitmap_button.h>
#include <eda_3d_viewer_settings.h>
#include <board_design_settings.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>
#include <pcb_base_frame.h>
#include <qt_ui.h>
#include <ui_hooks.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <3d_spacenav/spnav_viewer_plugin.h>
#else
#include <3d_navlib/nl_footprint_properties_plugin.h>
#endif


PANEL_PREVIEW_3D_MODEL::PANEL_PREVIEW_3D_MODEL( TOOLKIT_WINDOW* aParent, PCB_BASE_FRAME* aFrame,
                                                FOOTPRINT* aFootprint,
                                                std::vector<FP_3DMODEL>* aParentModelList ) :
        PANEL_PREVIEW_3D_MODEL_BASE( aParent ),
        m_parentFrame( aFrame ),
        m_previewPane( nullptr ),
        m_infobar( nullptr ),
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    m_userUnits = m_parentFrame->GetUserUnits();

    m_dummyBoard = new BOARD();

    m_dummyBoard->SetProject( &aFrame->Prj(), true );
    m_dummyBoard->SetEmbeddedFilesDelegate( aFrame->GetBoard() );

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    BOARD_DESIGN_SETTINGS parent_bds = aFrame->GetDesignSettings();
    BOARD_DESIGN_SETTINGS& dummy_bds = m_dummyBoard->GetDesignSettings();
    dummy_bds.SetBoardThickness( parent_bds.GetBoardThickness() );
    dummy_bds.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );
    BOARD_STACKUP& dummy_board_stackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    dummy_board_stackup.RemoveAll();
    dummy_board_stackup.BuildDefaultStackupList( &dummy_bds, 2 );

    m_selected = -1;

    m_previewLabel->setFont( KIQT::InfoFont( m_previewLabel ) );

    // Set the bitmap of 3D view buttons:
    m_bpvTop->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_top ) );
    m_bpvFront->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_front ) );
    m_bpvBack->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_back ) );
    m_bpvLeft->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_left ) );
    m_bpvRight->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_right ) );
    m_bpvBottom->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_bottom ) );
    m_bpvISO->SetBitmap( KiBitmapBundle( BITMAPS::ortho ) );
    m_bpvBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::show_board_body ) );
    m_bpUpdate->SetBitmap( KiBitmapBundle( BITMAPS::reload ) );
    m_bpSettings->SetBitmap( KiBitmapBundle( BITMAPS::options_3drender ) );

    for( TEXT_CTRL_EVAL* rotCtrl : { xrot, yrot, zrot } )
    {
        rotCtrl->SetCustomEval(
                [this]( TEXT_CTRL_EVAL* aCtrl )
                {
                    double value = PreviewRotationFromString( PreviewEvaluateText( aCtrl->GetValue() ) );
                    aCtrl->SetValue( formatRotationValue( value ) );
                } );
    }

    for( TEXT_CTRL_EVAL* scaleCtrl : { xscale, yscale, zscale } )
    {
        scaleCtrl->SetCustomEval(
                [this]( TEXT_CTRL_EVAL* aCtrl )
                {
                    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                              PreviewEvaluateText( aCtrl->GetValue() ) );
                    aCtrl->SetValue( formatScaleValue( value ) );
                } );
    }

    // The wheel over a control changes its value by steps (the wheel events of the controls)
    auto wheelFilter =
            [this]( TEXT_CTRL_EVAL* aCtrl,
                    void ( PANEL_PREVIEW_3D_MODEL::*aHandler )( TEXT_CTRL_EVAL*, bool, int ) )
            {
                KIQT::FilterEvents( aCtrl,
                        [this, aCtrl, aHandler]( QEvent* aEvent )
                        {
                            if( aEvent->type() != QEvent::Wheel )
                                return false;

                            QWheelEvent* wheel = static_cast<QWheelEvent*>( aEvent );
                            ( this->*aHandler )( aCtrl, wheel->modifiers() & Qt::ShiftModifier,
                                                 wheel->angleDelta().y() );
                            return true;
                        } );
            };

    for( TEXT_CTRL_EVAL* ctrl : { xscale, yscale, zscale } )
        wheelFilter( ctrl, &PANEL_PREVIEW_3D_MODEL::mouseWheelScale );

    for( TEXT_CTRL_EVAL* ctrl : { xrot, yrot, zrot } )
        wheelFilter( ctrl, &PANEL_PREVIEW_3D_MODEL::mouseWheelRot );

    for( TEXT_CTRL_EVAL* ctrl : { xoff, yoff, zoff } )
        wheelFilter( ctrl, &PANEL_PREVIEW_3D_MODEL::mouseWheelOffset );

    m_parentModelList = aParentModelList;

    m_dummyFootprint = new FOOTPRINT( *aFootprint );
    m_dummyFootprint->SetParentGroup( nullptr );

    // Ensure the footprint is shown like in Fp editor: rot 0, not flipped
    // to avoid mistakes when setting the3D shape position/rotation
    if( m_dummyFootprint->IsFlipped() )
        m_dummyFootprint->Flip( m_dummyFootprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    m_dummyFootprint->SetOrientation( ANGLE_0 );

    m_dummyBoard->Add( m_dummyFootprint );

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                       m_boardAdapter, m_currentCamera,
                                       PROJECT_PCB::Get3DCacheManager( &aFrame->Prj() ) );

    try
    {
#if defined(__linux__) || defined(__FreeBSD__)
        m_spaceMouse = std::make_unique<SPNAV_VIEWER_PLUGIN>( m_previewPane );
#else
        m_spaceMouse = std::make_unique<NL_FOOTPRINT_PROPERTIES_PLUGIN>( m_previewPane );
#endif
        m_spaceMouse->SetFocus( true );
    }
    catch( const std::exception& e )
    {
        KI_LOG_TRACE( wxT( "KI_TRACE_NAVLIB" ), wxS( "%s" ), e.what() );
    }
    catch( ... )
    {
        KI_LOG_TRACE( wxT( "KI_TRACE_NAVLIB" ),
                      wxT( "Unknown exception during SpaceMouse initialization" ) );
    }

    m_boardAdapter.SetBoard( m_dummyBoard );
    m_boardAdapter.m_IsBoardView = false;

    // Force display 3D models, regardless the 3D viewer options.
    m_boardAdapter.m_IsPreviewer = true;

    loadSettings();

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_dummyBoard, nullptr, nullptr, nullptr, this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_previewPane->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    m_infobar = new WX_INFOBAR( this );
    m_previewPane->SetInfoBar( m_infobar );

    m_SizerPanelView->addWidget( m_infobar );
    m_SizerPanelView->addWidget( m_previewPane, 1 );

    m_infobar->hide();

    // The offsets follow the units of the frame
    m_unitsListener = aFrame->AddUnitsChangedListener(
            [this]( EDA_BASE_FRAME* aChangedFrame )
            {
                onUnitsChanged( aChangedFrame->GetUserUnits() );
            } );
}


PANEL_PREVIEW_3D_MODEL::~PANEL_PREVIEW_3D_MODEL()
{
    if( m_parentFrame && m_unitsListener >= 0 )
        m_parentFrame->RemoveUnitsChangedListener( m_unitsListener );

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // Restore the 3D viewer Render settings, that can be modified by the panel tools
    if( m_boardAdapter.m_Cfg )
        m_boardAdapter.m_Cfg->m_Render = m_initialRender;

    delete m_dummyBoard;
    delete m_previewPane;
}


void PANEL_PREVIEW_3D_MODEL::loadSettings()
{
    KI_CHECK_RET( m_previewPane, wxT( "Cannot load settings to null canvas" ) );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        // Save the 3D viewer render settings, to restore it after closing the preview
        m_initialRender = cfg->m_Render;

        m_boardAdapter.m_Cfg = cfg;

        m_previewPane->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_previewPane->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_previewPane->SetProjectionMode( cfg->m_Camera.projection_mode );
    }
}


wxString PANEL_PREVIEW_3D_MODEL::formatScaleValue( double aValue )
{
    return wxString::Format( wxT( "%.4f" ), aValue );
}


wxString PANEL_PREVIEW_3D_MODEL::formatRotationValue( double aValue )
{
    // Sigh.  Did we really need differentiated +/- 0.0?
    if( aValue == -0.0 )
        aValue = 0.0;

    return wxString::Format( wxT( "%.2f%s" ), aValue, EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
}


wxString PANEL_PREVIEW_3D_MODEL::formatOffsetValue( double aValue )
{
    // Convert from internal units (mm) to user units
    if( m_userUnits == EDA_UNITS::INCH )
        aValue /= 25.4;
    else if( m_userUnits == EDA_UNITS::MILS )
        aValue /= 25.4 / 1e3;

    return wxString::Format( wxT( "%.6f%s" ), aValue, EDA_UNIT_UTILS::GetText( m_userUnits ) );
}


void PANEL_PREVIEW_3D_MODEL::SetSelectedModel( int idx )
{
    // The opacity is shown without its change event (as SetValue() of wxWidgets)
    QSignalBlocker blocker( m_opacity );

    if( m_parentModelList && idx >= 0 && idx < (int) m_parentModelList->size() )
    {
        m_selected = idx;
        const FP_3DMODEL& modelInfo = m_parentModelList->at( (unsigned) m_selected );

        // Use ChangeValue() instead of SetValue().  It's not the user making the change, so we
        // don't want to generate change events.
        xscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.x ) );
        yscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.y ) );
        zscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.z ) );

        // Rotation is stored in the file as positive-is-CW, but we use positive-is-CCW in the GUI
        // to match the rest of KiCad
        xrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.x ) );
        yrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.y ) );
        zrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.z ) );

        xoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.x ) );
        yoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.y ) );
        zoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.z ) );

        m_opacity->setValue( modelInfo.m_Opacity * 100.0 );
    }
    else
    {
        m_selected = -1;

        for( TEXT_CTRL_EVAL* ctrl : { xscale, yscale, zscale, xrot, yrot, zrot, xoff, yoff, zoff } )
            ctrl->ChangeValue( wxEmptyString );

        m_opacity->setValue( 100 );
    }
}


void PANEL_PREVIEW_3D_MODEL::updateOrientation( QWidget* aSource )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        auto unscaled =
                []( TEXT_CTRL_EVAL* aCtrl )
                {
                    return EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                      PreviewEvaluateText( aCtrl->GetValue() ) );
                };

        auto offset =
                [this]( TEXT_CTRL_EVAL* aCtrl )
                {
                    return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                      PreviewEvaluateText( aCtrl->GetValue() ) )
                           / pcbIUScale.IU_PER_MM;
                };

        modelInfo->m_Scale.x = unscaled( xscale );
        modelInfo->m_Scale.y = unscaled( yscale );
        modelInfo->m_Scale.z = unscaled( zscale );

        // Rotation is stored in the file as positive-is-CW, but we use positive-is-CCW in the GUI
        // to match the rest of KiCad
        modelInfo->m_Rotation.x = -PreviewRotationFromString( PreviewEvaluateText( xrot->GetValue() ) );
        modelInfo->m_Rotation.y = -PreviewRotationFromString( PreviewEvaluateText( yrot->GetValue() ) );
        modelInfo->m_Rotation.z = -PreviewRotationFromString( PreviewEvaluateText( zrot->GetValue() ) );

        modelInfo->m_Offset.x = offset( xoff );
        modelInfo->m_Offset.y = offset( yoff );
        modelInfo->m_Offset.z = offset( zoff );

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
        onModify();
    }
}


void PANEL_PREVIEW_3D_MODEL::onOpacitySlider( QWidget* aSource )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Opacity = m_opacity->value() / 100.0;

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
        onModify();
    }
}


void PANEL_PREVIEW_3D_MODEL::setBodyStyleView( QWidget* aSource )
{
    // turn ON or OFF options to show the board body if OFF, solder paste, soldermask
    // and board body are hidden, to allows a good view of the 3D model and its pads.
    EDA_3D_VIEWER_SETTINGS* cfg = m_boardAdapter.m_Cfg;

    if( !cfg )
        return;

    cfg->m_Render.preview_show_board_body = !cfg->m_Render.preview_show_board_body;

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::View3DSettings( QWidget* aSource )
{
    BOARD_DESIGN_SETTINGS bds = m_dummyBoard->GetDesignSettings();
    int                   thickness = bds.GetBoardThickness();

    WX_UNIT_ENTRY_DIALOG dlg( m_parentFrame, _( "3D Preview Options" ), _( "Board thickness:" ),
                              thickness );

    if( dlg.ShowModal() != wxID_OK )
        return;

    bds.SetBoardThickness( dlg.GetValue() );

    BOARD_STACKUP& boardStackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    boardStackup.RemoveAll();
    boardStackup.BuildDefaultStackupList( &bds, 2 );

    UpdateDummyFootprint( true );

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::doIncrementScale( QWidget* aSpinButton, double aSign )
{
    TEXT_CTRL_EVAL* textCtrl = xscale;

    if( aSpinButton == m_spinYscale )
        textCtrl = yscale;
    else if( aSpinButton == m_spinZscale )
        textCtrl = zscale;

    double step = SCALE_INCREMENT;

    if( TOOLKIT::IsKeyDown( WXK_SHIFT ) )
        step = SCALE_INCREMENT_FINE;

    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                              textCtrl->GetValue() );

    value += ( step * aSign );
    value = std::max( 1/MAX_SCALE, value );
    value = std::min( value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementRotation( QWidget* aSpinButton, double aSign )
{
    TEXT_CTRL_EVAL* textCtrl = xrot;

    if( aSpinButton == m_spinYrot )
        textCtrl = yrot;
    else if( aSpinButton == m_spinZrot )
        textCtrl = zrot;

    double step = ROTATION_INCREMENT;

    if( TOOLKIT::IsKeyDown( WXK_SHIFT ) )
        step = ROTATION_INCREMENT_FINE;

    double value = PreviewRotationFromString( textCtrl->GetValue() );

    value = PreviewNormalizeRotation( value + step * aSign );

    textCtrl->SetValue( formatRotationValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementOffset( QWidget* aSpinButton, double aSign )
{
    TEXT_CTRL_EVAL* textCtrl = xoff;

    if( aSpinButton == m_spinYoffset )
        textCtrl = yoff;
    else if( aSpinButton == m_spinZoffset )
        textCtrl = zoff;

    bool   fine = TOOLKIT::IsKeyDown( WXK_SHIFT );
    double step_mm = fine ? OFFSET_INCREMENT_MM_FINE : OFFSET_INCREMENT_MM;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCH )
        step_mm = 25.4 * ( fine ? OFFSET_INCREMENT_MIL_FINE : OFFSET_INCREMENT_MIL ) / 1000;

    double value_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                 textCtrl->GetValue() )
                      / pcbIUScale.IU_PER_MM;

    value_mm += ( step_mm * aSign );
    value_mm = std::max( -MAX_OFFSET, value_mm );
    value_mm = std::min( value_mm, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::mouseWheelScale( TEXT_CTRL_EVAL* aCtrl, bool aShift,
                                              int aWheelRotation )
{
    double step = aShift ? SCALE_INCREMENT_FINE : SCALE_INCREMENT;

    if( aWheelRotation >= 0 )
        step = -step;

    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                              aCtrl->GetValue() );

    value += step;
    value = std::max( 1/MAX_SCALE, value );
    value = std::min( value, MAX_SCALE );

    aCtrl->SetValue( formatScaleValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::mouseWheelRot( TEXT_CTRL_EVAL* aCtrl, bool aShift, int aWheelRotation )
{
    double step = aShift ? ROTATION_INCREMENT_FINE : ROTATION_INCREMENT;

    if( aWheelRotation >= 0 )
        step = -step;

    double value = PreviewRotationFromString( aCtrl->GetValue() );

    value = PreviewNormalizeRotation( value + step );

    aCtrl->SetValue( formatRotationValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::mouseWheelOffset( TEXT_CTRL_EVAL* aCtrl, bool aShift,
                                               int aWheelRotation )
{
    double step_mm = aShift ? OFFSET_INCREMENT_MM_FINE : OFFSET_INCREMENT_MM;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCH )
        step_mm = 25.4 * ( aShift ? OFFSET_INCREMENT_MIL_FINE : OFFSET_INCREMENT_MIL ) / 1000.0;

    if( aWheelRotation >= 0 )
        step_mm = -step_mm;

    double value_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                 aCtrl->GetValue() )
                      / pcbIUScale.IU_PER_MM;

    value_mm += step_mm;
    value_mm = std::max( -MAX_OFFSET, value_mm );
    value_mm = std::min( value_mm, MAX_OFFSET );

    aCtrl->SetValue( formatOffsetValue( value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::onUnitsChanged( EDA_UNITS aUnits )
{
    auto offset_mm =
            [this]( TEXT_CTRL_EVAL* aCtrl )
            {
                return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                  aCtrl->GetValue() )
                       / pcbIUScale.IU_PER_MM;
            };

    double xoff_mm = offset_mm( xoff );
    double yoff_mm = offset_mm( yoff );
    double zoff_mm = offset_mm( zoff );

    m_userUnits = aUnits;

    xoff->SetValue( formatOffsetValue( xoff_mm ) );
    yoff->SetValue( formatOffsetValue( yoff_mm ) );
    zoff->SetValue( formatOffsetValue( zoff_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::OnPanelShown( bool aShown )
{
    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aShown );
}


void PANEL_PREVIEW_3D_MODEL::UpdateDummyFootprint( bool aReloadRequired )
{
    m_dummyFootprint->Models().clear();

    for( FP_3DMODEL& model : *m_parentModelList )
    {
        if( model.m_Show )
            m_dummyFootprint->Models().push_back( model );
    }

    syncLocalEmbeddedFiles();

    if( aReloadRequired )
        m_previewPane->ReloadRequest();

    m_previewPane->Request_refresh();
}


void PANEL_PREVIEW_3D_MODEL::SetEmbeddedFilesDelegate( EMBEDDED_FILES* aDelegate )
{
    m_localEmbeddedFiles = aDelegate;
    syncLocalEmbeddedFiles();
}


void PANEL_PREVIEW_3D_MODEL::syncLocalEmbeddedFiles()
{
    m_dummyFootprint->ClearEmbeddedFiles();

    if( m_localEmbeddedFiles )
    {
        for( const auto& [name, file] : m_localEmbeddedFiles->EmbeddedFileMap() )
            m_dummyFootprint->AddFile( new EMBEDDED_FILES::EMBEDDED_FILE( *file ) );
    }
}


void PANEL_PREVIEW_3D_MODEL::onModify()
{
    KIWAY_HOLDER* kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( TOOLKIT::GetTopLevelParent( this ) );

    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::DIALOG )
        static_cast<DIALOG_SHIM*>( kiwayHolder )->OnModify();
}
