/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

#include <dialogs/dialog_move_exact.h>

#include <limits>

#include <QPalette>

#include <math/box2.h>
#include <math/util.h>      // for KiROUND
#include <pcb_base_frame.h>
#include <trigo.h>
#include <core/ki_assert.h>

#include <qt_strings.h>


DIALOG_MOVE_EXACT::DIALOG_MOVE_EXACT( PCB_BASE_FRAME *aParent, VECTOR2I& aTranslate,
                                      EDA_ANGLE& aRotate, ROTATION_ANCHOR& aAnchor,
                                      const BOX2I& aBbox ) :
    DIALOG_MOVE_EXACT_BASE( aParent ),
    m_translation( aTranslate ),
    m_rotation( aRotate ),
    m_rotationAnchor( aAnchor ),
    m_bbox( aBbox ),
    m_moveX( aParent, m_xLabel, m_xEntry, m_xUnit ),
    m_moveY( aParent, m_yLabel, m_yEntry, m_yUnit ),
    m_rotate( aParent, m_rotLabel, m_rotEntry, m_rotUnit ),
    m_stateX( 0.0 ),
    m_stateY( 0.0 ),
    m_stateRadius( 0.0 )
{
    m_tabOrder = {
        m_xEntry,
        m_yEntry,
        m_rotEntry,
        m_anchorOptions,
        m_stdButtonsOK,
        m_stdButtonsCancel
    };

    // Configure display origin transforms
    m_moveX.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
    m_moveY.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );

    m_menuIDs.push_back( aAnchor );
    m_menuIDs.push_back( ROTATE_AROUND_USER_ORIGIN );

    if( aParent->IsType( FRAME_PCB_EDITOR ) )
        m_menuIDs.push_back( ROTATE_AROUND_AUX_ORIGIN );

    // DIALOG_SHIM needs a title- and anchor-options-specific hash_key so we don't save/restore
    // state between usage cases.
    m_hash_key = GetTitle().ToStdString();

    if( aAnchor == ROTATE_AROUND_SEL_CENTER )
        m_hash_key += "|with_selection";

    if( aParent->IsType( FRAME_PCB_EDITOR ) )
        m_hash_key += "|pcb_editor";

    buildRotationAnchorMenu();
    m_rotate.SetUnits( EDA_UNITS::DEGREES );

    SetupStandardButtons();

    finishDialogSettings();
}


void DIALOG_MOVE_EXACT::buildRotationAnchorMenu()
{
    m_anchorOptions->clear();

    for( const ROTATION_ANCHOR& anchorID : m_menuIDs )
    {
        switch( anchorID )
        {
        case ROTATE_AROUND_ITEM_ANCHOR:
            m_anchorOptions->addItem( ToQString( _( "Rotate around item anchor" ) ) );
            break;

        case ROTATE_AROUND_SEL_CENTER:
            m_anchorOptions->addItem( ToQString( _( "Rotate around selection center" ) ) );
            break;

        case ROTATE_AROUND_USER_ORIGIN:
            m_anchorOptions->addItem( ToQString( _( "Rotate around local coordinates origin" ) ) );
            break;

        case ROTATE_AROUND_AUX_ORIGIN:
            m_anchorOptions->addItem( ToQString( _( "Rotate around drill/place origin" ) ) );
            break;
        }
    }

    m_anchorOptions->setCurrentIndex( 0 );
}


void DIALOG_MOVE_EXACT::ToPolarDeg( double x, double y, double& r, EDA_ANGLE& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0) ? EDA_ANGLE( VECTOR2D( x, y ) ) : ANGLE_0;
}


bool DIALOG_MOVE_EXACT::GetTranslationInIU( VECTOR2D& val, bool polar )
{
    if( polar )
    {
        const double    r = m_moveX.GetDoubleValue();
        const EDA_ANGLE q = m_moveY.GetAngleValue();

        val.x = r * q.Cos();
        val.y = r * q.Sin();
    }
    else
    {
        // direct read
        val.x = m_moveX.GetDoubleValue();
        val.y = m_moveY.GetDoubleValue();
    }

    // no validation to do here, but in future, you could return false here
    return true;
}


void DIALOG_MOVE_EXACT::OnPolarChanged( QWidget* aSource )
{
    bool   newPolar = m_polarCoords->isChecked();
    double moveX = m_moveX.GetDoubleValue();
    double moveY = m_moveY.GetDoubleValue();
    updateDialogControls( newPolar );

    if( newPolar )
    {
        if( moveX != m_stateX || moveY != m_stateY )
        {
            m_stateX = moveX;
            m_stateY = moveY;
            ToPolarDeg( m_stateX, m_stateY, m_stateRadius, m_stateTheta );

            m_moveX.SetDoubleValue( m_stateRadius );
            m_stateRadius = m_moveX.GetDoubleValue();
            m_moveY.SetAngleValue( m_stateTheta );
            m_stateTheta = m_moveY.GetAngleValue();
        }
        else
        {
            m_moveX.SetDoubleValue( m_stateRadius );
            m_moveY.SetAngleValue( m_stateTheta );
        }
    }
    else
    {
        if( moveX != m_stateRadius || moveY != m_stateTheta.AsDegrees() )
        {
            m_stateRadius = moveX;
            m_stateTheta = EDA_ANGLE( moveY, DEGREES_T );
            m_stateX = m_stateRadius * m_stateTheta.Cos();
            m_stateY = m_stateRadius * m_stateTheta.Sin();

            m_moveX.SetDoubleValue( m_stateX );
            m_stateX = m_moveX.GetDoubleValue();
            m_moveY.SetDoubleValue( m_stateY );
            m_stateY = m_moveY.GetDoubleValue();
        }
        else
        {
            m_moveX.SetDoubleValue( m_stateX );
            m_moveY.SetDoubleValue( m_stateY );
        }
    }
}


void DIALOG_MOVE_EXACT::updateDialogControls( bool aPolar )
{
    if( aPolar )
    {
        m_moveX.SetLabel( _( "Distance:" ) );     // Polar radius
        m_moveY.SetLabel( _( "Angle:" ) );        // Polar theta or angle
        m_moveY.SetUnits( EDA_UNITS::DEGREES );
    }
    else
    {
        m_moveX.SetLabel( _( "Move X:" ) );
        m_moveY.SetLabel( _( "Move Y:" ) );
        m_moveY.SetUnits( GetUserUnits() );
    }
}


void DIALOG_MOVE_EXACT::OnClear( QWidget* aSource )
{
    if( aSource == m_clearX )
        m_moveX.SetValue( 0 );
    else if( aSource == m_clearY )
        m_moveY.SetValue( 0 );
    else if( aSource == m_clearRot )
        m_rotate.SetAngleValue( ANGLE_0 );

    // Keep m_stdButtonsOK focused to allow enter key activate the OK button
    m_stdButtonsOK->setFocus();
}


bool DIALOG_MOVE_EXACT::TransferDataToWindow()
{
    updateDialogControls( m_polarCoords->isChecked() );

    // Force the evaluation when setting previous values
    m_moveX.RequireEval();
    m_moveY.RequireEval();
    m_rotate.RequireEval();

    return true;
}


bool DIALOG_MOVE_EXACT::TransferDataFromWindow()
{
    // for the output, we only deliver a Cartesian vector
    VECTOR2D translation;
    bool     ok = GetTranslationInIU( translation, m_polarCoords->isChecked() );
    m_translation.x = KiROUND( translation.x );
    m_translation.y = KiROUND( translation.y );
    m_rotation = m_rotate.GetAngleValue();

    const int anchorSelection = m_anchorOptions->currentIndex();
    KI_CHECK_MSG( anchorSelection >= 0 && anchorSelection < static_cast<int>( m_menuIDs.size() ),
                  false,
                  wxString::Format( "Invalid rotation anchor selection: %d", anchorSelection ) );

    m_rotationAnchor = m_menuIDs[anchorSelection];

    return ok;
}


void DIALOG_MOVE_EXACT::OnTextFocusLost( QWidget* aSource )
{
    if( QLineEdit* entry = qobject_cast<QLineEdit*>( aSource ) )
    {
        if( entry->text().isEmpty() )
            entry->setText( QStringLiteral( "0" ) );
    }
}


void DIALOG_MOVE_EXACT::OnTextChanged( QWidget* aSource )
{
    double delta_x = m_moveX.GetDoubleValue();
    double delta_y = m_moveY.GetDoubleValue();
    double max_border = std::numeric_limits<int>::max() * M_SQRT1_2;

    bool invalid = m_bbox.GetLeft() + delta_x < -max_border
                   || m_bbox.GetRight() + delta_x > max_border
                   || m_bbox.GetTop() + delta_y < -max_border
                   || m_bbox.GetBottom() + delta_y > max_border;

    QString tooltip;
    QPalette palette = QLineEdit().palette();

    if( invalid )
    {
        tooltip = ToQString( _( "Invalid movement values.  Movement would place selection "
                                "outside of the maximum board area." ) );
        palette.setColor( QPalette::Text, Qt::red );
    }

    for( QLineEdit* entry : { m_xEntry, m_yEntry } )
    {
        entry->setToolTip( tooltip );
        entry->setPalette( palette );
    }

    m_stdButtonsOK->setEnabled( !invalid );
}
