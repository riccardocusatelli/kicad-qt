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
 * @file dialog_export_3d_image.cpp
 * DIALOG_EXPORT_3D_IMAGE of the Qt build (see 3d-viewer/dialogs/dialog_export_3d_image.cpp).
 */

#include "dialog_export_3d_image.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include <bitmaps/bitmap_types.h>
#include <qt_strings.h>


DIALOG_EXPORT_3D_IMAGE::DIALOG_EXPORT_3D_IMAGE( TOOLKIT_WINDOW* aParent, const wxSize& aCanvasSize,
                                                EDA_3D_VIEWER_SETTINGS::EXPORT_IMAGE_SETTINGS* aCfg ) :
        DIALOG_SHIM( aParent, _( "Export 3D View" ) ),
        m_cfg( aCfg ),
        m_originalSize( aCanvasSize ),
        m_width( aCfg && aCfg->width > 0 ? aCfg->width : aCanvasSize.GetWidth() ),
        m_height( aCfg && aCfg->height > 0 ? aCfg->height : aCanvasSize.GetHeight() ),
        m_xResolution( aCfg ? aCfg->x_resolution : 300.0 ),
        m_yResolution( aCfg ? aCfg->y_resolution : 300.0 ),
        m_lockAspectRatio( aCfg ? aCfg->lock_aspect_ratio : true ),
        m_sizeUnits( aCfg ? static_cast<SIZE_UNITS>( aCfg->size_units ) : SIZE_UNITS::PIXELS ),
        m_resolutionUnits( aCfg ? static_cast<RESOLUTION_UNITS>( aCfg->resolution_units )
                                : RESOLUTION_UNITS::PIXELS_PER_INCH )
{
    m_aspectRatio = static_cast<double>(m_width) / static_cast<double>(m_height);

    // State is persisted through EXPORT_IMAGE_SETTINGS, so opt out of the generic
    // control-state save and restore.
    OptOut( this );

    QVBoxLayout* mainSizer = new QVBoxLayout( this );

    auto header =
            [this]( const wxString& aText )
            {
                QLabel* label = new QLabel( ToQString( aText ), this );
                QFont   font = label->font();
                font.setBold( true );
                label->setFont( font );
                return label;
            };

    auto spin =
            [this]( double aMax, int aDigits )
            {
                QDoubleSpinBox* ctrl = new QDoubleSpinBox( this );
                ctrl->setRange( 1, aMax );
                ctrl->setDecimals( aDigits );
                ctrl->setKeyboardTracking( false );
                return ctrl;
            };

    // Image Size section
    mainSizer->addWidget( header( _( "Image Size" ) ) );

    QGridLayout* sizeGrid = new QGridLayout();
    sizeGrid->setColumnStretch( 1, 1 );

    // Width row
    sizeGrid->addWidget( new QLabel( ToQString( _( "Width:" ) ), this ), 0, 0 );
    m_spinWidth = spin( 50000, 0 );
    sizeGrid->addWidget( m_spinWidth, 0, 1 );

    // Lock button - spans the 2 rows
    m_lockButton = new QToolButton( this );
    m_lockButton->setIcon( KiBitmapBundle( m_lockAspectRatio ? BITMAPS::locked : BITMAPS::unlocked ) );
    sizeGrid->addWidget( m_lockButton, 0, 2, 2, 1, Qt::AlignCenter );

    // Size units choice
    m_choiceSizeUnits = new QComboBox( this );
    m_choiceSizeUnits->addItem( ToQString( _( "pixels" ) ) );
    m_choiceSizeUnits->addItem( ToQString( _( "%" ) ) );
    m_choiceSizeUnits->addItem( ToQString( _( "mm" ) ) );
    m_choiceSizeUnits->addItem( ToQString( _( "in" ) ) );
    m_choiceSizeUnits->setCurrentIndex( static_cast<int>( m_sizeUnits ) );
    sizeGrid->addWidget( m_choiceSizeUnits, 0, 3 );

    // Height row
    sizeGrid->addWidget( new QLabel( ToQString( _( "Height:" ) ), this ), 1, 0 );
    m_spinHeight = spin( 50000, 0 );
    sizeGrid->addWidget( m_spinHeight, 1, 1 );

    mainSizer->addLayout( sizeGrid );

    // Pixel size display
    m_pixelSizeLabel = new QLabel( this );
    QFont smaller = m_pixelSizeLabel->font();
    smaller.setPointSizeF( smaller.pointSizeF() * 0.9 );
    m_pixelSizeLabel->setFont( smaller );
    mainSizer->addWidget( m_pixelSizeLabel );

    // Separator
    QFrame* line = new QFrame( this );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    mainSizer->addWidget( line );

    // Resolution section
    mainSizer->addWidget( header( _( "Resolution" ) ) );

    QGridLayout* resGrid = new QGridLayout();
    resGrid->setColumnStretch( 1, 1 );

    // X Resolution row
    resGrid->addWidget( new QLabel( ToQString( _( "X resolution:" ) ), this ), 0, 0 );
    m_spinXResolution = spin( 10000, 3 );
    resGrid->addWidget( m_spinXResolution, 0, 1 );

    // Resolution units choice
    m_choiceResolutionUnits = new QComboBox( this );
    m_choiceResolutionUnits->addItem( ToQString( _( "pixels/in" ) ) );
    m_choiceResolutionUnits->addItem( ToQString( _( "pixels/mm" ) ) );
    m_choiceResolutionUnits->setCurrentIndex( static_cast<int>( m_resolutionUnits ) );
    resGrid->addWidget( m_choiceResolutionUnits, 0, 2 );

    // Y Resolution row
    resGrid->addWidget( new QLabel( ToQString( _( "Y resolution:" ) ), this ), 1, 0 );
    m_spinYResolution = spin( 10000, 3 );
    resGrid->addWidget( m_spinYResolution, 1, 1 );

    mainSizer->addLayout( resGrid );

    // Dialog buttons
    QDialogButtonBox* buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                      this );
    mainSizer->addWidget( buttons );
    connectStandardButton( buttons->button( QDialogButtonBox::Ok ), wxID_OK );
    connectStandardButton( buttons->button( QDialogButtonBox::Cancel ), wxID_CANCEL );

    // Set initial values AFTER creating all controls
    m_spinWidth->setValue( m_width );
    m_spinHeight->setValue( m_height );
    m_spinXResolution->setValue( m_xResolution );
    m_spinYResolution->setValue( m_yResolution );
    UpdatePixelSize();

    // Connect the events AFTER setting initial values
    QObject::connect( m_lockButton, &QToolButton::clicked, this, [this]() { OnLockToggle(); } );
    QObject::connect( m_spinWidth, &QDoubleSpinBox::valueChanged, this, [this]() { OnWidthChange(); } );
    QObject::connect( m_spinHeight, &QDoubleSpinBox::valueChanged, this, [this]() { OnHeightChange(); } );
    QObject::connect( m_spinXResolution, &QDoubleSpinBox::valueChanged, this, [this]() { UpdatePixelSize(); } );
    QObject::connect( m_spinYResolution, &QDoubleSpinBox::valueChanged, this, [this]() { UpdatePixelSize(); } );
    QObject::connect( m_choiceSizeUnits, &QComboBox::activated, this, [this]() { OnSizeUnitChange(); } );
    QObject::connect( m_choiceResolutionUnits, &QComboBox::activated, this,
                      [this]() { OnResolutionUnitChange(); } );

    adjustSize();
}


bool DIALOG_EXPORT_3D_IMAGE::TransferDataFromWindow()
{
    double width = m_spinWidth->value();
    double height = m_spinHeight->value();
    double xRes = m_spinXResolution->value();
    double yRes = m_spinYResolution->value();

    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4;
        yRes *= 25.4;
    }

    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, m_sizeUnits );
    m_width = pixelSize.GetWidth();
    m_height = pixelSize.GetHeight();

    m_xResolution = m_spinXResolution->value();
    m_yResolution = m_spinYResolution->value();

    if( m_cfg )
    {
        m_cfg->width = m_width;
        m_cfg->height = m_height;
        m_cfg->x_resolution = m_xResolution;
        m_cfg->y_resolution = m_yResolution;
        m_cfg->size_units = static_cast<int>( m_sizeUnits );
        m_cfg->resolution_units = static_cast<int>( m_resolutionUnits );
        m_cfg->lock_aspect_ratio = m_lockAspectRatio;
    }

    return true;
}


void DIALOG_EXPORT_3D_IMAGE::OnLockToggle()
{
    m_lockAspectRatio = !m_lockAspectRatio;

    if( m_lockAspectRatio )
    {
        m_lockButton->setIcon( KiBitmapBundle( BITMAPS::locked ) );
        UpdateAspectRatio();
    }
    else
    {
        m_lockButton->setIcon( KiBitmapBundle( BITMAPS::unlocked ) );
    }
}


void DIALOG_EXPORT_3D_IMAGE::OnWidthChange()
{
    if( m_lockAspectRatio )
    {
        double width = m_spinWidth->value();
        double height;

        if( m_sizeUnits == SIZE_UNITS::PERCENT )
            height = m_originalSize.GetWidth() * width / 100.0 / m_aspectRatio;
        else
            height = width / m_aspectRatio;

        // Ensure height is not less than 1
        if( height < 1 )
            height = 1;

        // Without the change event of the height (as SetValue() of wxWidgets)
        QSignalBlocker blocker( m_spinHeight );
        m_spinHeight->setValue( height );
    }

    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnHeightChange()
{
    if( m_lockAspectRatio )
    {
        double height = m_spinHeight->value();
        double width;

        if( m_sizeUnits == SIZE_UNITS::PERCENT )
            width = m_originalSize.GetHeight() * height / 100.0 * m_aspectRatio;
        else
            width = height * m_aspectRatio;

        // Ensure width is not less than 1
        if( width < 1 )
            width = 1;

        QSignalBlocker blocker( m_spinWidth );
        m_spinWidth->setValue( width );
    }

    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnSizeUnitChange()
{
    SIZE_UNITS oldUnits = m_sizeUnits;
    m_sizeUnits = static_cast<SIZE_UNITS>( m_choiceSizeUnits->currentIndex() );
    ConvertSizeUnits( oldUnits, m_sizeUnits );
    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnResolutionUnitChange()
{
    RESOLUTION_UNITS oldUnits = m_resolutionUnits;
    m_resolutionUnits = static_cast<RESOLUTION_UNITS>( m_choiceResolutionUnits->currentIndex() );
    ConvertResolutionUnits( oldUnits, m_resolutionUnits );
    UpdatePixelSize();
}


wxSize DIALOG_EXPORT_3D_IMAGE::GetPixelSize( double aWidth, double aHeight, double aXResolution,
                                             double aYResolution, SIZE_UNITS aSizeUnits ) const
{
    switch( aSizeUnits )
    {
    case SIZE_UNITS::PIXELS:
        return wxSize( static_cast<int>( aWidth ), static_cast<int>( aHeight ) );
    case SIZE_UNITS::PERCENT:
        return wxSize( static_cast<int>( aWidth * m_originalSize.GetWidth() / 100.0 ),
                       static_cast<int>( aHeight * m_originalSize.GetHeight() / 100.0 ) );
    case SIZE_UNITS::MM:
        return wxSize( static_cast<int>( aWidth * aXResolution / 25.4 ),
                       static_cast<int>( aHeight * aYResolution / 25.4 ) );
    case SIZE_UNITS::INCHES:
        return wxSize( static_cast<int>( aWidth * aXResolution ),
                       static_cast<int>( aHeight * aYResolution ) );
    default:
        break;
    }

    return wxSize( 10, 10 ); // Should not happen
}


void DIALOG_EXPORT_3D_IMAGE::UpdatePixelSize()
{
    double width = m_spinWidth->value();
    double height = m_spinHeight->value();
    double xRes = m_spinXResolution->value();
    double yRes = m_spinYResolution->value();

    // Convert resolution to standard pixels/inch
    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4; // Convert mm to inches
        yRes *= 25.4;
    }

    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, m_sizeUnits );

    m_pixelSizeLabel->setText( ToQString( wxString::Format( _( "%d × %d pixels" ),
                                                            pixelSize.GetWidth(),
                                                            pixelSize.GetHeight() ) ) );
}


void DIALOG_EXPORT_3D_IMAGE::UpdateAspectRatio()
{
    m_aspectRatio = m_spinWidth->value() / m_spinHeight->value();
}


void DIALOG_EXPORT_3D_IMAGE::ConvertSizeUnits( SIZE_UNITS aFromUnit, SIZE_UNITS aToUnit )
{
    if( aFromUnit == aToUnit )
        return;

    double width = m_spinWidth->value();
    double height = m_spinHeight->value();
    double xRes = m_spinXResolution->value();
    double yRes = m_spinYResolution->value();

    // Convert resolution to standard pixels/inch
    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4; // Convert mm to inches
        yRes *= 25.4;
    }

    // Convert to pixels first
    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, aFromUnit );

    QSignalBlocker blockWidth( m_spinWidth );
    QSignalBlocker blockHeight( m_spinHeight );

    // Convert from pixels to target unit
    switch( aToUnit )
    {
    case SIZE_UNITS::PIXELS:
        m_spinWidth->setValue( pixelSize.GetWidth() );
        m_spinHeight->setValue( pixelSize.GetHeight() );
        break;
    case SIZE_UNITS::PERCENT:
        m_spinWidth->setValue( pixelSize.GetWidth() * 100.0 / m_originalSize.GetWidth() );
        m_spinHeight->setValue( pixelSize.GetHeight() * 100.0 / m_originalSize.GetHeight() );
        break;
    case SIZE_UNITS::MM:
        m_spinWidth->setValue( pixelSize.GetWidth() * 25.4 / xRes );
        m_spinHeight->setValue( pixelSize.GetHeight() * 25.4 / yRes );
        break;
    case SIZE_UNITS::INCHES:
        m_spinWidth->setValue( pixelSize.GetWidth() / xRes );
        m_spinHeight->setValue( pixelSize.GetHeight() / yRes );
        break;
    }
}


void DIALOG_EXPORT_3D_IMAGE::ConvertResolutionUnits( RESOLUTION_UNITS aFromUnit,
                                                     RESOLUTION_UNITS aToUnit )
{
    if( aFromUnit == aToUnit )
        return;

    double xRes = m_spinXResolution->value();
    double yRes = m_spinYResolution->value();

    QSignalBlocker blockX( m_spinXResolution );
    QSignalBlocker blockY( m_spinYResolution );

    if( aFromUnit == RESOLUTION_UNITS::PIXELS_PER_INCH && aToUnit == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        // Convert from pixels/inch to pixels/mm
        m_spinXResolution->setValue( xRes / 25.4 );
        m_spinYResolution->setValue( yRes / 25.4 );
    }
    else if( aFromUnit == RESOLUTION_UNITS::PIXELS_PER_MM && aToUnit == RESOLUTION_UNITS::PIXELS_PER_INCH )
    {
        // Convert from pixels/mm to pixels/inch
        m_spinXResolution->setValue( xRes * 25.4 );
        m_spinYResolution->setValue( yRes * 25.4 );
    }
}
