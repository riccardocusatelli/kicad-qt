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

#pragma once

#include <QString>
#include <QStringList>
#include <wx/string.h>

/// wxString (kiwx) to QString.
inline QString ToQString( const wxString& aStr )
{
    return QString::fromStdWString( aStr.ToStdWstring() );
}

/// QString to wxString (kiwx).
inline wxString ToWxString( const QString& aStr )
{
    return wxString( aStr.toStdWString() );
}

/**
 * The filters of a Qt file dialog from a wildcard of wxWidgets ("Description (*.a; *.b)|*.a;*.b|...",
 * the wildcards of wildcards_and_files_ext.h): "Description (*.a *.b)", one by filter.
 */
inline QStringList ToQtFileFilters( const wxString& aWildcard )
{
    QStringList parts = ToQString( aWildcard ).split( QLatin1Char( '|' ) );
    QStringList filters;

    for( int ii = 0; ii + 1 < parts.size(); ii += 2 )
    {
        QString description = parts[ii];
        int     open = description.lastIndexOf( QLatin1Char( '(' ) );

        if( open > 0 )
            description = description.left( open ).trimmed();

        QStringList patterns;

        for( QString pattern : parts[ii + 1].split( QLatin1Char( ';' ), Qt::SkipEmptyParts ) )
            patterns.append( pattern.trimmed() );

        filters.append( description + QStringLiteral( " (" ) + patterns.join( QLatin1Char( ' ' ) )
                        + QLatin1Char( ')' ) );
    }

    return filters;
}
