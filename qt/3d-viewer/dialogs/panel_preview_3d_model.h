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
 * @file panel_preview_3d_model.h
 * The 3D preview of the models of a footprint (footprint properties, 3D model chooser), of the
 * Qt build: the layout of panel_preview_3d_model_base.fbp, with the widgets of Qt (see
 * 3d-viewer/dialogs/panel_preview_3d_model.h).
 */

#ifndef PANEL_PREVIEW_3D_MODEL_H
#define PANEL_PREVIEW_3D_MODEL_H

#include <dialogs/panel_preview_3d_model_base.h>

#include <memory>
#include <vector>

#include <tool/tools_holder.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <3d_viewer_id.h>
#include <3d_rendering/track_ball.h>
#include <3d_viewer/eda_3d_viewer_settings.h>
#include <eda_units.h>
#include <wx/event.h>

// Define min and max parameter values
#define MAX_SCALE          10000.0
#define MAX_ROTATION       180.0
#define MAX_OFFSET         1000.0

#define SCALE_INCREMENT_FINE       0.02
#define SCALE_INCREMENT            0.1

#define ROTATION_INCREMENT 90      // in degrees
#define ROTATION_INCREMENT_FINE 1  // in degrees

#define OFFSET_INCREMENT_MM        0.5
#define OFFSET_INCREMENT_MM_FINE   0.1

#define OFFSET_INCREMENT_MIL       25.0
#define OFFSET_INCREMENT_MIL_FINE  5.0

// Declared classes to create pointers
class WX_INFOBAR;
class S3D_CACHE;
class FILENAME_RESOLVER;
class EMBEDDED_FILES;
class BOARD;
class BOARD_ADAPTER;
class FOOTPRINT;
class FP_3DMODEL;
class PCB_BASE_FRAME;
#if defined(__linux__) || defined(__FreeBSD__)
class SPNAV_VIEWER_PLUGIN;
#else
class NL_FOOTPRINT_PROPERTIES_PLUGIN;
#endif


/// The panel is shown or hidden in its dialog (defined with the panel of the 3D models of the
/// footprints).
wxDECLARE_EVENT( wxCUSTOM_PANEL_SHOWN_EVENT, wxCommandEvent );


class PANEL_PREVIEW_3D_MODEL: public TOOLS_HOLDER, public PANEL_PREVIEW_3D_MODEL_BASE
{
public:
    PANEL_PREVIEW_3D_MODEL( TOOLKIT_WINDOW* aParent, PCB_BASE_FRAME* aFrame, FOOTPRINT* aFootprint,
                            std::vector<FP_3DMODEL>* aParentModelList );

    ~PANEL_PREVIEW_3D_MODEL() override;

    TOOLKIT_WINDOW* GetToolCanvas() const override { return m_previewPane; }

    BOARD_ADAPTER& GetAdapter() { return m_boardAdapter; }
    CAMERA& GetCurrentCamera() { return m_currentCamera; }

    /**
     * Set the currently selected index in the model list so that the scale/rotation/offset
     * controls can be updated.
     */
    void SetSelectedModel( int idx );

    /**
     * Copy shapes from the current shape list which are flagged for preview to the copy of
     * footprint that is on the preview dummy board.
     */
    void UpdateDummyFootprint( bool aRelaodRequired = true );

    void SetEmbeddedFilesDelegate( EMBEDDED_FILES* aDelegate );

    FOOTPRINT* GetDummyFootprint() const { return m_dummyFootprint; }

    /**
     * The panel is shown or hidden in its dialog (the wxCUSTOM_PANEL_SHOWN_EVENT of the
     * wxWidgets build): the 3D mouse moves the preview while it is shown.
     */
    void OnPanelShown( bool aShown );

    /// The panel is shown in a window shown (wxWindow::IsShownOnScreen()).
    bool IsShownOnScreen() const { return isVisible(); }

    /// The events sent to the panel (wxWindow::ProcessWindowEvent()): wxCUSTOM_PANEL_SHOWN_EVENT.
    bool ProcessWindowEvent( wxEvent& aEvent )
    {
        if( aEvent.GetEventType() != wxCUSTOM_PANEL_SHOWN_EVENT )
            return false;

        OnPanelShown( static_cast<wxCommandEvent&>( aEvent ).GetInt() != 0 );
        return true;
    }

private:
    void loadSettings();

    void updateOrientation( QWidget* aSource ) override;

    void onIncrementRot( QWidget* aSource ) override { doIncrementRotation( aSource, 1.0 ); }
    void onDecrementRot( QWidget* aSource ) override { doIncrementRotation( aSource, -1.0 ); }
    void onIncrementScale( QWidget* aSource ) override { doIncrementScale( aSource, 1.0 ); }
    void onDecrementScale( QWidget* aSource ) override { doIncrementScale( aSource, -1.0 ); }
    void onIncrementOffset( QWidget* aSource ) override { doIncrementOffset( aSource, 1.0 ); }
    void onDecrementOffset( QWidget* aSource ) override { doIncrementOffset( aSource, -1.0 ); }

    void onOpacitySlider( QWidget* aSource ) override;

    void doIncrementScale( QWidget* aSpinButton, double aSign );
    void doIncrementRotation( QWidget* aSpinButton, double aSign );
    void doIncrementOffset( QWidget* aSpinButton, double aSign );

    /// The wheel over the controls of the values: the values change by steps.
    void mouseWheelScale( TEXT_CTRL_EVAL* aCtrl, bool aShift, int aWheelRotation );
    void mouseWheelRot( TEXT_CTRL_EVAL* aCtrl, bool aShift, int aWheelRotation );
    void mouseWheelOffset( TEXT_CTRL_EVAL* aCtrl, bool aShift, int aWheelRotation );

    void onUnitsChanged( EDA_UNITS aUnits );

    wxString formatScaleValue( double aValue );
    wxString formatRotationValue( double aValue );
    wxString formatOffsetValue( double aValue );

    void View3DISO( QWidget* aSource ) override
    {
        m_currentCamera.ToggleProjection();
        m_previewPane->Refresh();
    }

    // turn ON or OFF options to show the board body. If OFF, solder paste, soldermask
    // and board body are hidden, to allows a good view of the 3D model and its pads.
    // Useful for 3D model placement
    void setBodyStyleView( QWidget* aSource ) override;

    void View3DLeft( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_LEFT );
    }

    void View3DFront( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_FRONT );
    }

    void View3DTop( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_TOP );
    }

    void View3DUpdate( QWidget* aSource ) override
    {
        m_previewPane->ReloadRequest();
        m_previewPane->Refresh();
    }

    void View3DSettings( QWidget* aSource ) override;

    void View3DRight( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_RIGHT );
    }

    void View3DBack( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_BACK );
    }

    void View3DBottom( QWidget* aSource ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_BOTTOM );
    }

    void onModify();

    void syncLocalEmbeddedFiles();

private:
    PCB_BASE_FRAME*          m_parentFrame;
    EDA_3D_CANVAS*           m_previewPane;
    WX_INFOBAR*              m_infobar;
    BOARD_ADAPTER            m_boardAdapter;
    CAMERA&                  m_currentCamera;
    TRACK_BALL               m_trackBallCamera;

    BOARD*                   m_dummyBoard;
    FOOTPRINT*               m_dummyFootprint;

    std::vector<FP_3DMODEL>* m_parentModelList;
    int                      m_selected;            /// Index into m_parentInfoList

    EDA_UNITS                m_userUnits;
    EMBEDDED_FILES*          m_localEmbeddedFiles = nullptr;
    int                      m_unitsListener = -1;  ///< The units listener of the frame

    /// The 3d viewer Render initial settings (must be saved and restored)
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS          m_initialRender;

#if defined(__linux__) || defined(__FreeBSD__)
    std::unique_ptr<SPNAV_VIEWER_PLUGIN> m_spaceMouse;
#else
    std::unique_ptr<NL_FOOTPRINT_PROPERTIES_PLUGIN>  m_spaceMouse;
#endif
};

#endif  // PANEL_PREVIEW_3D_MODEL_H
