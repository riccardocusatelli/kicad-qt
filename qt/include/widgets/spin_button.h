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
 * @file spin_button.h
 * The arrows of a spin control without its text (the wxSpinButton of the wxFormBuilder files),
 * on Qt.
 */

#pragma once

#include <functional>

#include <QAbstractSpinBox>

#include <kiqt.h>


/**
 * The up and down arrows of a spin box, without text: a click on an arrow (or the wheel, or
 * the arrow keys) calls the up or down callback.
 */
class KIQT_API SPIN_BUTTON : public QAbstractSpinBox
{
public:
    SPIN_BUTTON( QWidget* aParent );

    /// The function called by the up arrow.
    void SetUpCallback( std::function<void()> aCallback ) { m_up = std::move( aCallback ); }

    /// The function called by the down arrow.
    void SetDownCallback( std::function<void()> aCallback ) { m_down = std::move( aCallback ); }

    void stepBy( int aSteps ) override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    StepEnabled stepEnabled() const override;

private:
    std::function<void()> m_up;
    std::function<void()> m_down;
};
