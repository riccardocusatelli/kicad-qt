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
 * @file dialog_export_3d_image.h
 * The dialog of the size of an exported image of the 3D viewer, of the Qt build (see
 * 3d-viewer/dialogs/dialog_export_3d_image.h).
 */

#pragma once

#include "dialog_shim.h"
#include <3d_viewer/eda_3d_viewer_frame.h> // for EDA_3D_VIEWER_EXPORT_FORMAT
#include <3d_viewer/eda_3d_viewer_settings.h>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QToolButton;

enum class SIZE_UNITS
{
    PIXELS,
    PERCENT,
    MM,
    INCHES
};

enum class RESOLUTION_UNITS
{
    PIXELS_PER_INCH,
    PIXELS_PER_MM
};

class DIALOG_EXPORT_3D_IMAGE : public DIALOG_SHIM
{
public:
    DIALOG_EXPORT_3D_IMAGE( TOOLKIT_WINDOW* aParent, const wxSize& aCanvasSize,
                            EDA_3D_VIEWER_SETTINGS::EXPORT_IMAGE_SETTINGS* aCfg );

    wxSize GetSize() const { return wxSize( m_width, m_height ); }
    double GetXResolution() const { return m_xResolution; }
    double GetYResolution() const { return m_yResolution; }

private:
    bool TransferDataFromWindow() override;

    void OnLockToggle();
    void OnWidthChange();
    void OnHeightChange();
    void OnSizeUnitChange();
    void OnResolutionUnitChange();

    wxSize GetPixelSize( double aWidth, double aHeight, double aXResolution, double aYResolution,
                         SIZE_UNITS aSizeUnits ) const;
    void UpdatePixelSize();
    void UpdateAspectRatio();
    void ConvertSizeUnits( SIZE_UNITS aFromUnit, SIZE_UNITS aToUnit );
    void ConvertResolutionUnits( RESOLUTION_UNITS aFromUnit, RESOLUTION_UNITS aToUnit );

private:
    EDA_3D_VIEWER_SETTINGS::EXPORT_IMAGE_SETTINGS* m_cfg;
    wxSize m_originalSize;
    int m_width;
    int m_height;
    double m_xResolution;
    double m_yResolution;
    double m_aspectRatio;
    bool m_lockAspectRatio;

    SIZE_UNITS m_sizeUnits;
    RESOLUTION_UNITS m_resolutionUnits;

    QDoubleSpinBox* m_spinWidth;
    QDoubleSpinBox* m_spinHeight;
    QDoubleSpinBox* m_spinXResolution;
    QDoubleSpinBox* m_spinYResolution;
    QComboBox* m_choiceSizeUnits;
    QComboBox* m_choiceResolutionUnits;
    QToolButton* m_lockButton;
    QLabel* m_pixelSizeLabel;
};
