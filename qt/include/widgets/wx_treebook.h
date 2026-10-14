/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

/**
 * @file wx_treebook.h
 * WX_TREEBOOK with Qt (include/widgets/wx_treebook.h with wxWidgets): the book of pages in a
 * tree of the compatibility layer (wxTreebook), with the pages made when they are first shown.
 */

#ifndef WX_TREEBOOK_H
#define WX_TREEBOOK_H

#include <functional>

#include <wx/kiwx_gui.h>


class KIQT_API WX_TREEBOOK : public wxTreebook
{
public:
    WX_TREEBOOK( QWidget* parent, int id, const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize, long style = 0,
                 const wxString& name = wxEmptyString );

    /// A page made by @a aLazyCtor when it is first shown.
    bool AddLazyPage( std::function<QWidget*( QWidget* aParent )> aLazyCtor,
                      const wxString& text, bool bSelect = false, int imageId = -1 );

    bool AddLazySubPage( std::function<QWidget*( QWidget* aParent )> aLazyCtor,
                         const wxString& text, bool bSelect = false, int imageId = -1 );

    /// The window of a page (made now if it is a lazy page).
    QWidget* ResolvePage( size_t aPage );
};

#endif // WX_TREEBOOK_H
