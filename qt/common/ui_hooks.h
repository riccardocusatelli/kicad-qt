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
 * @file ui_hooks.h
 *
 * Hooks of the Qt widgets of KiCad: functions called on an event of a widget, and the update
 * UI functions (the state of the controls computed from the data, as the update UI events of
 * wxWidgets).  The code generated from the wxFormBuilder files (qt/tools/fbp2qt.py) uses them.
 */

#include <functional>

#include <kiqt.h>
#include <QEvent>

class QWidget;


namespace KIQT
{

/**
 * Call @a aHandler when @a aWidget receives an event of type @a aType.  The event is then
 * processed as usual.  The hook lives as long as the widget.
 */
KIQT_API void OnEvent( QWidget* aWidget, QEvent::Type aType,
                       std::function<void( QEvent* aEvent )> aHandler );

/**
 * Give the events of @a aWidget to @a aFilter before the widget: the events for which it
 * returns true are not processed by the widget.  The filter lives as long as the widget.
 */
KIQT_API void FilterEvents( QWidget* aWidget, std::function<bool( QEvent* aEvent )> aFilter );

/**
 * Register a function updating the state of controls (enabled, shown, label...) from the data.
 *
 * The functions of the shown windows are called after the input events of the user and when a
 * window is shown, so that the controls follow the data, as with the update UI events of
 * wxWidgets.  The function lives as long as @a aOwner.
 */
KIQT_API void RegisterUpdateUI( QWidget* aOwner, std::function<void()> aUpdate );

/// Run the update UI functions of the shown windows now.
KIQT_API void UpdateUINow();

/**
 * Give an id of KiCad (the ids of wxWidgets: ID_..., wxID_...) to a widget, for the code that
 * finds its controls by id (the controls of the toolbars).
 */
KIQT_API void SetWindowId( QWidget* aWidget, int aId );

/// The id of KiCad of a widget, or wxID_ANY (-1).
KIQT_API int GetWindowId( const QWidget* aWidget );

} // namespace KIQT
