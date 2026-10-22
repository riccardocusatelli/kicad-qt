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
 * @file hotkey_cycle_popup.cpp
 * HOTKEY_CYCLE_POPUP of the Qt build (see common/dialogs/hotkey_cycle_popup.cpp).
 */

#include <dialogs/hotkey_cycle_popup.h>

#include <algorithm>

#include <QTimer>

#include <class_draw_panel_gal.h>
#include <eda_draw_frame.h>


#define SHOW_TIME_MS 500


HOTKEY_CYCLE_POPUP::HOTKEY_CYCLE_POPUP( EDA_DRAW_FRAME* aParent ) :
        EDA_VIEW_SWITCHER_BASE( aParent ),
        m_drawFrame( aParent )
{
    // A popup that doesn't take the keys from the canvas
    setWindowFlags( Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus );
    setAttribute( Qt::WA_ShowWithoutActivating );
    m_listBox->setFocusPolicy( Qt::NoFocus );

    m_showTimer = new QTimer( this );
    m_showTimer->setSingleShot( true );

    QObject::connect( m_showTimer, &QTimer::timeout, this,
                      [this]()
                      {
                          hide();

                          if( EDA_DRAW_PANEL_GAL* canvas = m_drawFrame->GetCanvas() )
                              canvas->SetFocus();
                      } );
}


void HOTKEY_CYCLE_POPUP::Popup( const wxString& aTitle, const wxArrayString& aItems,
                                int aSelection )
{
    m_stTitle->setText( ToQString( aTitle ) );
    m_listBox->clear();

    for( const wxString& item : aItems )
        m_listBox->addItem( ToQString( item ) );

    m_listBox->setCurrentRow( std::min( aSelection, m_listBox->count() - 1 ) );

    // The list as large as its items
    int width = std::max( m_listBox->sizeHintForColumn( 0 ),
                          m_stTitle->fontMetrics().horizontalAdvance( m_stTitle->text() ) );
    int height = m_listBox->sizeHintForRow( 0 ) * m_listBox->count();

    m_listBox->setMinimumSize( width + 2 * m_listBox->frameWidth() + 10,
                               height + 2 * m_listBox->frameWidth() + 10 );
    adjustSize();

    m_showTimer->start( SHOW_TIME_MS );

    if( !isVisible() )
    {
        // Centred on the frame
        QWidget* frame = parentWidget();

        if( frame )
            move( frame->geometry().center() - rect().center() );

        show();
    }
}
