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

#include <vector>

#include <QString>
#include <QTreeWidget>

#include "kiqt.h"


/// A property of the selected item, as shown.
struct PROPERTY_ROW
{
    QString m_Group;
    QString m_Name;
    QString m_Value;
};


/// The properties of the selected items, by group (read only).
class KIQT_API PROPERTY_PANEL : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PROPERTY_PANEL( QWidget* aParent = nullptr );

    /// Show these properties, with a title (the type of the item); no rows for no selection.
    void ShowProperties( const QString& aTitle, const std::vector<PROPERTY_ROW>& aRows );
};
