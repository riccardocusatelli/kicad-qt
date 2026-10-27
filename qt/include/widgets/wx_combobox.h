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
 * @file wx_combobox.h
 * WX_COMBOBOX of the Qt build: a combo box whose items can have a text of their own in the list
 * and be separators (see include/widgets/wx_combobox.h, which draws its items with
 * wxOwnerDrawnComboBox: QComboBox has its separators).
 */

#ifndef WX_COMBOBOX_H
#define WX_COMBOBOX_H

#include <wx/combobox.h>

#include <qt_strings.h>


class WX_COMBOBOX : public wxComboBox
{
public:
    WX_COMBOBOX( wxWidgetArgCompat aParent, int aId = wxID_ANY,
                 const wxString& aValue = wxEmptyString, const wxPoint& aPos = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize, int n = 0,
                 const wxString choices[] = nullptr, long style = 0 ) :
            wxComboBox( aParent, aId, aValue, aPos, aSize, wxArrayString(), style )
    {
        for( int i = 0; i < n; ++i )
            Append( choices[i] );

        // The control shows the text of the chosen item, not the one of the list (before the
        // handlers of wxEVT_COMBOBOX, connected after the construction)
        QObject::connect( this, &QComboBox::activated, this,
                          [this]( int aIndex )
                          {
                              QVariant text = itemData( aIndex, TEXT_ROLE );

                              if( isEditable() && text.isValid() )
                                  setEditText( text.toString() );
                          } );
    }

    /// Append an item, shown in the list with @a aMenuText if not empty ("---": a separator).
    void Append( const wxString& aText, const wxString& aMenuText = wxEmptyString )
    {
        if( aText == wxT( "---" ) )
        {
            insertSeparator( count() );
            return;
        }

        addItem( ToQString( aMenuText.IsEmpty() ? aText : aMenuText ) );
        setItemData( count() - 1, ToQString( aText ), TEXT_ROLE );
    }

private:
    /// The role of the text of an item (the client data of wxChoice are in Qt::UserRole).
    static constexpr int TEXT_ROLE = Qt::UserRole + 1;
};

#endif // WX_COMBOBOX_H
