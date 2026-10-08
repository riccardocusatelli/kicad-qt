/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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
 * @file notifications_list_qt.cpp
 * The window of the list of the notifications with Qt (see common/notifications_list.h and
 * common/notifications_list_wx.cpp).
 */

#include <functional>
#include <unordered_map>

#include <QDateTime>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include <kiplatform/ui.h>
#include <notifications_manager.h>

#include "../../common/notifications_list.h"

#include <qt_strings.h>


static long long g_last_closed_timer = 0;


namespace
{

/// A link of the panel of a notification.
QLabel* makeLink( QWidget* aParent, const wxString& aLabel )
{
    QLabel* link = new QLabel( QStringLiteral( "<a href=\"#\">%1</a>" )
                                       .arg( ToQString( aLabel ).toHtmlEscaped() ),
                               aParent );
    link->setTextFormat( Qt::RichText );
    link->setTextInteractionFlags( Qt::LinksAccessibleByMouse );
    return link;
}


/// A notification of the list: its title, its description, and the links to its details and
/// to dismiss it.
class NOTIFICATION_PANEL : public QFrame
{
public:
    NOTIFICATION_PANEL( QWidget* aParent, NOTIFICATIONS_MANAGER* aManager, NOTIFICATION* aNoti ) :
            QFrame( aParent ),
            m_notification( aNoti ),
            m_manager( aManager )
    {
        setFrameShape( QFrame::Box );
        setMinimumHeight( 75 );

        QColor fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );

        QPalette pal = palette();
        pal.setColor( QPalette::Window, bg );
        pal.setColor( QPalette::WindowText, fg );
        setPalette( pal );
        setAutoFillBackground( true );

        QVBoxLayout* mainSizer = new QVBoxLayout( this );
        mainSizer->setContentsMargins( 1, 1, 1, 1 );
        mainSizer->setSpacing( 1 );

        QLabel* stTitle = new QLabel( ToQString( aNoti->title ), this );
        QFont   bold = stTitle->font();
        bold.setBold( true );
        stTitle->setFont( bold );
        stTitle->setWordWrap( true );
        mainSizer->addWidget( stTitle );

        QLabel* stDescription = new QLabel( ToQString( aNoti->description ), this );
        stDescription->setWordWrap( true );
        mainSizer->addWidget( stDescription );

        QHBoxLayout* tailSizer = new QHBoxLayout();

        if( !aNoti->href.IsEmpty() )
        {
            QLabel* hlDetails = makeLink( this, _( "View Details" ) );
            tailSizer->addWidget( hlDetails );

            QObject::connect( hlDetails, &QLabel::linkActivated, this,
                              [this]( const QString& )
                              {
                                  onDetails();
                              } );
        }

        QLabel* hlDismiss = makeLink( this, _( "Dismiss" ) );
        tailSizer->addWidget( hlDismiss );
        tailSizer->addStretch( 1 );

        QObject::connect( hlDismiss, &QLabel::linkActivated, this,
                          [this]( const QString& )
                          {
                              onDismiss();
                          } );

        mainSizer->addLayout( tailSizer, 1 );
    }

private:
    void onDetails()
    {
        wxString url = m_notification->href;

        if( url.StartsWith( wxS( "kicad://" ) ) )
        {
            url.Replace( wxS( "kicad://" ), wxS( "" ) );

            if( url == wxS( "pcm" ) )
            {
                // TODO
            }
        }
        else
        {
            QDesktopServices::openUrl( QUrl( ToQString( url ) ) );
        }
    }

    void onDismiss()
    {
        NOTIFICATIONS_MANAGER* manager = m_manager;
        wxString               key = m_notification->key;

        // This will cause this panel to get deleted
        QMetaObject::invokeMethod( this,
                                   [manager, key]()
                                   {
                                       manager->Remove( key );
                                   },
                                   Qt::QueuedConnection );
    }

private:
    NOTIFICATION*          m_notification;
    NOTIFICATIONS_MANAGER* m_manager;
};


/// The list: a popup, closed (and deleted) by a click outside of it.
class NOTIFICATIONS_LIST_QT : public QFrame, public NOTIFICATIONS_LIST
{
public:
    NOTIFICATIONS_LIST_QT( NOTIFICATIONS_MANAGER* aManager, QWidget* aParent ) :
            QFrame( aParent, Qt::Popup ),
            m_manager( aManager )
    {
        setAttribute( Qt::WA_DeleteOnClose );
        setFrameShape( QFrame::Box );
        setWindowTitle( ToQString( _( "Notifications" ) ) );
        resize( 300, 150 );

        QVBoxLayout* bSizer1 = new QVBoxLayout( this );
        bSizer1->setContentsMargins( 0, 0, 0, 0 );

        QScrollArea* scrolledWindow = new QScrollArea( this );
        scrolledWindow->setWidgetResizable( true );
        scrolledWindow->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

        QColor fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );

        m_content = new QWidget( scrolledWindow );

        QPalette pal = m_content->palette();
        pal.setColor( QPalette::Window, bg );
        pal.setColor( QPalette::WindowText, fg );
        m_content->setPalette( pal );
        m_content->setAutoFillBackground( true );

        m_contentSizer = new QVBoxLayout( m_content );
        m_contentSizer->setContentsMargins( 2, 2, 2, 2 );

        m_noNotificationsText = new QLabel( ToQString( _( "There are no notifications available" ) ),
                                            m_content );
        m_noNotificationsText->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        m_contentSizer->addWidget( m_noNotificationsText, 1 );
        m_contentSizer->addStretch( 0 );

        scrolledWindow->setWidget( m_content );
        bSizer1->addWidget( scrolledWindow );
    }

    ~NOTIFICATIONS_LIST_QT() override
    {
        g_last_closed_timer = QDateTime::currentMSecsSinceEpoch();
        notifyClosed( m_manager, this );
    }

    void Add( NOTIFICATION* aNoti ) override
    {
        m_noNotificationsText->hide();

        NOTIFICATION_PANEL* panel = new NOTIFICATION_PANEL( m_content, m_manager, aNoti );

        // Before the stretch at the end
        m_contentSizer->insertWidget( m_contentSizer->count() - 1, panel );
        m_panelMap[aNoti] = panel;
    }

    void Remove( NOTIFICATION* aNoti ) override
    {
        auto it = m_panelMap.find( aNoti );

        if( it != m_panelMap.end() )
        {
            it->second->deleteLater();
            m_panelMap.erase( it );
        }

        if( m_panelMap.size() == 0 )
            m_noNotificationsText->show();
    }

private:
    NOTIFICATIONS_MANAGER*                                 m_manager;
    QWidget*                                               m_content;
    QVBoxLayout*                                           m_contentSizer;
    std::unordered_map<NOTIFICATION*, NOTIFICATION_PANEL*> m_panelMap;

    /// Text to be displayed when no notifications are present, this gets a Show/Hide call as
    /// needed.
    QLabel*                                                m_noNotificationsText;
};

}


void NOTIFICATIONS_MANAGER::ShowList( QWidget* aParent, wxPoint aPos )
{
    // Debounce clicking on the icon with a list already showing.  The click on the button
    // closes the list first, and then we'd open it again without this guard.
    if( QDateTime::currentMSecsSinceEpoch() - g_last_closed_timer < 300 )
    {
        g_last_closed_timer = 0;
        return;
    }

    NOTIFICATIONS_LIST_QT* list = new NOTIFICATIONS_LIST_QT( this, aParent );

    for( NOTIFICATION& job : m_notifications )
        list->Add( &job );

    m_shownDialogs.push_back( list );

    // correct the position: the bottom right corner at aPos
    list->move( aPos.x - list->width(), aPos.y - list->height() );
    list->show();
    KIPLATFORM::UI::ForceFocus( list );
}
