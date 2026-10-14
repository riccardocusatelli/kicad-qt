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

#include <wx/kiwx_statusbar.h>

#include <algorithm>

#include <QLabel>

#include <qt_strings.h>


wxStatusBar::wxStatusBar( wxWidgetArgCompat aParent, int aId, long aStyle, const wxString& aName ) :
        wxWindowCompat<QStatusBar>( aParent )
{
    SetId( aId );
    SetName( aName );
    setSizeGripEnabled( ( aStyle & wxSTB_SIZEGRIP ) != 0 );
    SetFieldsCount( 1 );
}


void wxStatusBar::SetFieldsCount( int aNumber, const int* aWidths )
{
    aNumber = std::max( aNumber, 1 );

    while( int( m_fields.size() ) > aNumber )
    {
        removeWidget( m_fields.back().m_label );
        delete m_fields.back().m_label;
        m_fields.pop_back();
    }

    while( int( m_fields.size() ) < aNumber )
    {
        FIELD field;
        field.m_label = new QLabel( this );
        field.m_label->setTextFormat( Qt::PlainText );
        addWidget( field.m_label, 1 );
        m_fields.push_back( field );
    }

    if( aWidths )
        SetStatusWidths( aNumber, aWidths );
}


void wxStatusBar::SetStatusWidths( int aNumber, const int* aWidths )
{
    for( int i = 0; i < aNumber && i < int( m_fields.size() ); ++i )
    {
        QLabel* label = m_fields[i].m_label;
        int     width = aWidths ? aWidths[i] : -1;

        // The widgets are added again with their stretch (QStatusBar has no setter)
        removeWidget( label );

        if( width >= 0 )
        {
            label->setFixedWidth( width );
            insertWidget( i, label, 0 );
        }
        else
        {
            label->setMinimumWidth( 0 );
            label->setMaximumWidth( QWIDGETSIZE_MAX );
            insertWidget( i, label, -width );
        }

        label->show();
    }
}


void wxStatusBar::SetStatusText( const wxString& aText, int aField )
{
    if( aField >= 0 && aField < int( m_fields.size() ) )
        m_fields[aField].m_label->setText( ToQString( aText ) );
}


wxString wxStatusBar::GetStatusText( int aField ) const
{
    if( aField >= 0 && aField < int( m_fields.size() ) )
        return ToWxString( m_fields[aField].m_label->text() );

    return wxEmptyString;
}


void wxStatusBar::PushStatusText( const wxString& aText, int aField )
{
    if( aField < 0 || aField >= int( m_fields.size() ) )
        return;

    m_fields[aField].m_stack.push_back( GetStatusText( aField ) );
    SetStatusText( aText, aField );
}


void wxStatusBar::PopStatusText( int aField )
{
    if( aField < 0 || aField >= int( m_fields.size() ) || m_fields[aField].m_stack.empty() )
        return;

    SetStatusText( m_fields[aField].m_stack.back(), aField );
    m_fields[aField].m_stack.pop_back();
}


bool wxStatusBar::GetFieldRect( int aField, wxRect& aRect ) const
{
    if( aField < 0 || aField >= int( m_fields.size() ) )
        return false;

    QRect rect = m_fields[aField].m_label->geometry();
    aRect = wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
    return true;
}
