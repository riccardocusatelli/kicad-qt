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

/**
 * @file inspectable_rows.h
 *
 * The rows of the property panel for an item of the model.  This file is compiled into each
 * Qt KIFACE (not into libkiqt): each KIFACE has its own PROPERTY_MANAGER, with the properties
 * of its items.
 */

#include <vector>

#include <QString>

#include <eda_units.h>

#include "property_panel.h"

class EDA_ITEM;


/**
 * The properties shown in the editors that all @a aItems have, by group, with distances in
 * @a aUnits.  A value the items do not share is shown empty (PROPERTY_ROW::m_Mixed).
 */
std::vector<PROPERTY_ROW> PropertyRows( const std::vector<EDA_ITEM*>& aItems,
                                        const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits );

/**
 * Set a property of an item from a value as the property panel gives it.
 *
 * @param aKey the property (PROPERTY_ROW::m_Key).
 * @param aValue the text (a measure in @a aUnits, unless it says its unit), the label of a
 *               choice, or "1" / "0".
 * @return false if the value is not valid for the property; @a aError says why.
 */
bool SetPropertyValue( EDA_ITEM* aItem, const void* aKey, const QString& aValue,
                       const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits, QString& aError );
