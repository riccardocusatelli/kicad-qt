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
 * @file qt_ui.h
 * Small helpers of the user interface of the Qt build: the colors of KiCad (COLOR4D) and the
 * fonts of the panels (as KIUI::GetInfoFont() of the wxWidgets build).
 */

#pragma once

#include <QColor>
#include <QFont>
#include <QWidget>

#include <gal/color4d.h>


/// The QColor of a color of KiCad.
inline QColor ToQColor( const KIGFX::COLOR4D& aColor )
{
    return QColor( aColor.Red8(), aColor.Green8(), aColor.Blue8(), aColor.Alpha8() );
}


/// The color of KiCad of a QColor.
inline KIGFX::COLOR4D FromQColor( const QColor& aColor )
{
    return KIGFX::COLOR4D( aColor.redF(), aColor.greenF(), aColor.blueF(), aColor.alphaF() );
}


namespace KIQT
{

/// The font of the information texts of a widget (one point smaller than its font).
inline QFont InfoFont( const QWidget* aWidget )
{
    QFont font = aWidget ? aWidget->font() : QFont();

    if( font.pointSizeF() > 0 )
        font.setPointSizeF( font.pointSizeF() - 1 );

    return font;
}

} // namespace KIQT
