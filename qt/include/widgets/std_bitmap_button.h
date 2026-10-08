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
 * @file std_bitmap_button.h
 * STD_BITMAP_BUTTON of the Qt build: a standard button showing an icon.
 */

#ifndef STD_BITMAP_BUTTON_H
#define STD_BITMAP_BUTTON_H

#include <QIcon>
#include <QPushButton>


/**
 * A bitmap button widget that behaves like a standard dialog button, except with an icon
 * instead of a label.
 */
class STD_BITMAP_BUTTON : public QPushButton
{
public:
    STD_BITMAP_BUTTON( QWidget* aParent );

    ~STD_BITMAP_BUTTON() override = default;

    void SetBitmap( const QIcon& aBmp );

    bool Enable( bool aEnable = true );
};

#endif /*STD_BITMAP_BUTTON_H*/
