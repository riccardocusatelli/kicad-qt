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


/**
 * @file std_bitmap_button.cpp
 * STD_BITMAP_BUTTON of the Qt build (see common/widgets/std_bitmap_button.cpp).
 */

#include <widgets/std_bitmap_button.h>


STD_BITMAP_BUTTON::STD_BITMAP_BUTTON( wxWidgetArgCompat aParent, int aId,
                                      const QIcon& aDummyBitmap, const wxPoint& aPos,
                                      const wxSize& aSize, int aStyle ) :
        wxWindowCompat<QPushButton>( aParent.get() )
{
    SetId( aId );
    setFocusPolicy( Qt::TabFocus );
}


void STD_BITMAP_BUTTON::SetBitmap( const QIcon& aBmp )
{
    setIcon( aBmp );

    // The button as high as the standard buttons, at least square
    QList<QSize> sizes = aBmp.availableSizes();

    if( !sizes.isEmpty() )
        setIconSize( sizes.first() );

    setMinimumWidth( std::max( minimumSizeHint().height(), iconSize().width() + 8 ) );
}


bool STD_BITMAP_BUTTON::Enable( bool aEnable )
{
    bool changed = isEnabled() != aEnable;
    setEnabled( aEnable );
    return changed;
}
