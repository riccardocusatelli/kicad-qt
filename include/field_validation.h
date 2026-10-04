/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file validators.h
 * @brief Custom text control validator definitions.
 */

#pragma once

/**
 * @file field_validation.h
 * @brief The rules of the values of the symbol and sheet fields (the data model; the
 *        FIELD_VALIDATOR of the dialogs applies them).
 */

#include <template_fieldnames.h>
#include <wx/string.h>


/// The characters a value of @a aFieldId cannot contain.
wxString FieldExcludedChars( FIELD_T aFieldId );

/// True if a value of @a aFieldId cannot be empty.
bool FieldCannotBeEmpty( FIELD_T aFieldId );

/**
 * Return the error message if @a aValue is invalid for @a aFieldId.
 * Returns an empty string when the value is valid.
 */
wxString GetFieldValidationErrorMessage( FIELD_T aFieldId, const wxString& aValue );
