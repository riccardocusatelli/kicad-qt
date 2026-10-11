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
#include <QFontMetrics>
#include <QStringList>
#include <QWidget>

#include <gal/color4d.h>
#include <wx/gdicmn.h>
#include <qt_strings.h>


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


/// The font of the docked panes (the GUI font, one point smaller on macOS, as
/// KIUI::GetDockedPaneFont() of the wxWidgets build).
inline QFont DockedPaneFont( const QWidget* aWidget )
{
#ifdef __APPLE__
    return InfoFont( aWidget );
#else
    return aWidget ? aWidget->font() : QFont();
#endif
}


/// The size in pixels of a size in dialog units (wxWindow::ConvertDialogToPixels()), for the
/// font of @a aWidget.
inline wxSize DialogUnitsToPixels( const QWidget* aWidget, const wxSize& aSize )
{
    QFontMetrics metrics( aWidget ? aWidget->font() : QFont() );

    return wxSize( aSize.x * metrics.averageCharWidth() / 4, aSize.y * metrics.height() / 8 );
}


/**
 * The filter of a QFileDialog for a wildcard of wxFileDialog ("Description (*.a)|*.a|...", as
 * built by AddFileExtListToFilter()): "Description (*.a);;...".
 */
inline QString FileDialogFilter( const wxString& aWildcard )
{
    QStringList parts = ToQString( aWildcard ).split( QLatin1Char( '|' ) );
    QStringList filters;

    for( int ii = 0; ii + 1 < parts.size(); ii += 2 )
    {
        QString description = parts[ii].trimmed();
        QString patterns = parts[ii + 1].split( QLatin1Char( ';' ), Qt::SkipEmptyParts )
                                        .join( QLatin1Char( ' ' ) );

        // The description has its patterns between parentheses for Qt
        if( !description.contains( QLatin1Char( '(' ) ) )
            description += QStringLiteral( " (" ) + patterns + QLatin1Char( ')' );
        else
            description = description.left( description.lastIndexOf( QLatin1Char( '(' ) ) ).trimmed()
                          + QStringLiteral( " (" ) + patterns + QLatin1Char( ')' );

        filters << description;
    }

    // A wildcard without description
    if( filters.isEmpty() && !aWildcard.IsEmpty() )
        filters << ToQString( aWildcard );

    return filters.join( QStringLiteral( ";;" ) );
}

} // namespace KIQT
