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
#include <QWidget>

#include "kiqt.h"

class GAL_CANVAS;


/**
 * A document of the Qt shell (a board, a schematic), made by the KIFACE of its editor
 * (KIFACE_QT_DOCUMENT_FACTORY).
 */
class KIQT_API QT_DOCUMENT : public QWidget
{
    Q_OBJECT

public:
    using QWidget::QWidget;

    /// Load a file; on failure, @a aError says why.
    virtual bool Open( const QString& aFileName, QString& aError ) = 0;

    /// The file shown.
    virtual QString FileName() const = 0;

    /// The canvas of the document.
    virtual GAL_CANVAS* Canvas() const = 0;

    /// Show everything.
    virtual void ZoomToFit() = 0;

signals:
    /// The cursor moved (the text shows its position, in the units of the document).
    void cursorPositionChanged( const QString& aText );

    /// A message for the status bar.
    void statusMessage( const QString& aText );
};


/// The factory a KIFACE returns for KIFACE_QT_DOCUMENT_FACTORY.
typedef QT_DOCUMENT* ( *QT_DOCUMENT_FACTORY )( QWidget* aParent );
