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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file eda_view_switcher.cpp
 * EDA_VIEW_SWITCHER of the Qt build (see common/dialogs/eda_view_switcher.cpp).
 */

#include <dialogs/eda_view_switcher.h>

#include <QKeyEvent>

#include <toolkit/gui_services.h>


EDA_VIEW_SWITCHER::EDA_VIEW_SWITCHER( TOOLKIT_WINDOW* aParent, const wxArrayString& aItems,
                                      wxKeyCode aCtrlKey ) :
        EDA_VIEW_SWITCHER_BASE( aParent ),
        m_ctrlKey( aCtrlKey )
{
    for( const wxString& item : aItems )
        m_listBox->addItem( ToQString( item ) );

    // The first Tab (the one opening the switcher) goes to the second view
    m_listBox->setCurrentRow( std::min( 0, m_listBox->count() - 1 ) );

    // The list as large as its views
    int width = m_listBox->sizeHintForColumn( 0 ) + 2 * m_listBox->frameWidth() + 10;
    int height = m_listBox->sizeHintForRow( 0 ) * m_listBox->count()
                 + 2 * m_listBox->frameWidth() + 10;

    m_listBox->setMinimumSize( width, height );
    m_listBox->setFocus();
    adjustSize();
}


void EDA_VIEW_SWITCHER::advance( bool aBackward )
{
    int count = m_listBox->count();

    if( count == 0 )
        return;

    int idx = m_listBox->currentRow() + ( aBackward ? -1 : 1 );

    m_listBox->setCurrentRow( ( idx + count ) % count );
}


bool EDA_VIEW_SWITCHER::event( QEvent* aEvent )
{
    switch( aEvent->type() )
    {
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    {
        QKeyEvent* event = static_cast<QKeyEvent*>( aEvent );

        if( event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab )
        {
            if( aEvent->type() == QEvent::KeyPress )
            {
                bool backward = event->key() == Qt::Key_Backtab
                                || ( TOOLKIT::IsKeyDown( WXK_SHIFT ) && m_ctrlKey != WXK_SHIFT );

                advance( backward );
            }

            aEvent->accept();
            return true;
        }

        if( event->key() == Qt::Key_Escape && aEvent->type() == QEvent::KeyPress )
        {
            EndModal( wxID_CANCEL );
            return true;
        }

        break;
    }

    case QEvent::KeyRelease:
        // The switch key is released: the view is chosen
        if( !TOOLKIT::IsKeyDown( m_ctrlKey ) )
        {
            EndModal( wxID_OK );
            return true;
        }

        break;

    default:
        break;
    }

    return EDA_VIEW_SWITCHER_BASE::event( aEvent );
}
