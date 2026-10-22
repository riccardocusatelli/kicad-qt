/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file datectrl.h
 * The date picker of the wxWidgets compatibility layer (wxDatePickerCtrl): a QDateEdit with a
 * calendar popup.
 */

#pragma once

#include <wx/datetime.h>
#include <wx/kiwx_window.h>

#include <QDateEdit>


#define wxDP_SPIN         0x01
#define wxDP_DROPDOWN     0x02
#define wxDP_DEFAULT      0x03
#define wxDP_ALLOWNONE    0x04
#define wxDP_SHOWCENTURY  0x08


class KIQT_API wxDatePickerCtrl : public wxWindowCompat<QDateEdit>
{
public:
    explicit wxDatePickerCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                               const wxDateTime& aDate = wxDefaultDateTime,
                               const wxPoint& aPos = wxDefaultPosition,
                               const wxSize& aSize = wxDefaultSize,
                               long aStyle = wxDP_DEFAULT | wxDP_SHOWCENTURY );

    /// The date (an invalid wxDateTime for no date shows the current one).
    void SetValue( const wxDateTime& aDate );
    wxDateTime GetValue() const;
};
