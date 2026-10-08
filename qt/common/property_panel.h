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
#include <QStringList>
#include <QTreeWidget>

#include "kiqt.h"


/// A property of the selected items, as shown and edited.
struct PROPERTY_ROW
{
    /// How the value is edited
    enum class EDITOR
    {
        NONE,       ///< Read only
        TEXT,       ///< A text, a number or a measure with its unit
        CHECK,      ///< Yes or no
        CHOICE      ///< One of m_Choices
    };

    QString     m_Group;
    QString     m_Name;
    QString     m_Value;            ///< Empty when the items have different values
    bool        m_Mixed = false;    ///< The selected items have different values
    EDITOR      m_Editor = EDITOR::NONE;
    QStringList m_Choices;
    const void* m_Key = nullptr;    ///< The property, for the model that made the row
};


/**
 * The properties of the selected items, by group.  The values that can be changed are edited
 * in place: a text (Enter applies it), a check box, a list of choices.
 */
class KIQT_API PROPERTY_PANEL : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PROPERTY_PANEL( QWidget* aParent = nullptr );

    /// Show these properties, with a title (the type of the item); no rows for no selection.
    void ShowProperties( const QString& aTitle, const std::vector<PROPERTY_ROW>& aRows );

Q_SIGNALS:
    /**
     * The user changed a value.
     * @param aKey the property (PROPERTY_ROW::m_Key).
     * @param aValue the text, the label of the choice, or "1" / "0" for a check box.
     */
    void valueEdited( const void* aKey, const QString& aValue );

private:
    void onItemChanged( QTreeWidgetItem* aItem, int aColumn );

    std::vector<PROPERTY_ROW> m_rows;
    bool                      m_filling = false;
};
