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


#include <widgets/spin_button.h>

#include <QLineEdit>
#include <QStyle>
#include <QStyleOptionSpinBox>


SPIN_BUTTON::SPIN_BUTTON( QWidget* aParent ) :
        QAbstractSpinBox( aParent )
{
    // Only the arrows: no text
    setButtonSymbols( QAbstractSpinBox::UpDownArrows );
    setReadOnly( true );
    lineEdit()->hide();
    setFocusPolicy( Qt::NoFocus );
}


void SPIN_BUTTON::stepBy( int aSteps )
{
    for( int ii = 0; ii < std::abs( aSteps ); ++ii )
    {
        if( aSteps > 0 && m_up )
            m_up();
        else if( aSteps < 0 && m_down )
            m_down();
    }
}


QAbstractSpinBox::StepEnabled SPIN_BUTTON::stepEnabled() const
{
    return StepUpEnabled | StepDownEnabled;
}


QSize SPIN_BUTTON::sizeHint() const
{
    // The width of the arrows of a spin box
    QStyleOptionSpinBox option;
    initStyleOption( &option );

    QRect up = style()->subControlRect( QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, this );
    QSize size = QAbstractSpinBox::sizeHint();

    return QSize( std::max( up.width() + 4, 16 ), size.height() );
}


QSize SPIN_BUTTON::minimumSizeHint() const
{
    return sizeHint();
}
