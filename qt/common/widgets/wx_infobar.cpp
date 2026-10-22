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
 * @file wx_infobar.cpp
 * WX_INFOBAR of the Qt build.
 */

#include <widgets/wx_infobar.h>

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

#include <qt_strings.h>


WX_INFOBAR::WX_INFOBAR( wxWidgetArgCompat aParent ) :
        wxWindowCompat<QFrame>( aParent.get() ),
        m_showTime( 0 ),
        m_updateLock( false ),
        m_showTimer( [this]() { onTimer(); } ),
        m_type( MESSAGE_TYPE::GENERIC ),
        m_closeButton( nullptr ),
        m_callback( std::nullopt )
{
    setObjectName( QStringLiteral( "InfoBar" ) );
    setFrameShape( QFrame::StyledPanel );
    setAutoFillBackground( true );
    setBackgroundRole( QPalette::ToolTipBase );
    setForegroundRole( QPalette::ToolTipText );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 6, 3, 3, 3 );

    m_icon = new QLabel( this );
    m_text = new QLabel( this );
    m_text->setWordWrap( true );
    m_text->setTextInteractionFlags( Qt::TextSelectableByMouse );

    m_buttons = new QHBoxLayout();
    m_buttons->setSpacing( 6 );

    layout->addWidget( m_icon );
    layout->addWidget( m_text, 1 );
    layout->addLayout( m_buttons );

    hide();
}


WX_INFOBAR::~WX_INFOBAR()
{
}


void WX_INFOBAR::SetShowTime( int aTime )
{
    m_showTime = aTime;
}


void WX_INFOBAR::QueueShowMessage( const wxString& aMessage, int aFlags )
{
    QTimer::singleShot( 0, this,
                        [this, aMessage, aFlags]()
                        {
                            ShowMessage( aMessage, aFlags );
                        } );
}


void WX_INFOBAR::QueueDismiss()
{
    QTimer::singleShot( 0, this,
                        [this]()
                        {
                            Dismiss();
                        } );
}


void WX_INFOBAR::ShowMessageFor( const wxString& aMessage, int aTime, int aFlags,
                                 MESSAGE_TYPE aType )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_showTime = aTime;
    ShowMessage( aMessage, aFlags );

    m_type = aType;
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_updateLock = true;

    m_message = aMessage;
    m_message.Trim();

    QStyle::StandardPixmap icon = QStyle::SP_MessageBoxInformation;

    if( aFlags & wxICON_ERROR )
        icon = QStyle::SP_MessageBoxCritical;
    else if( aFlags & wxICON_WARNING )
        icon = QStyle::SP_MessageBoxWarning;
    else if( aFlags & wxICON_QUESTION )
        icon = QStyle::SP_MessageBoxQuestion;

    if( aFlags & wxICON_NONE )
    {
        m_icon->hide();
    }
    else
    {
        int size = style()->pixelMetric( QStyle::PM_SmallIconSize );
        m_icon->setPixmap( style()->standardIcon( icon ).pixmap( size, size ) );
        m_icon->show();
    }

    m_text->setText( ToQString( m_message ) );
    show();

    if( m_showTime > 0 )
        m_showTimer.StartOnce( m_showTime );

    m_type = MESSAGE_TYPE::GENERIC;
    m_updateLock = false;
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags, MESSAGE_TYPE aType )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    ShowMessage( aMessage, aFlags );

    m_type = aType;
}


void WX_INFOBAR::Dismiss()
{
    if( !isVisible() )
        return;

    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_updateLock = true;

    m_showTimer.Stop();
    hide();

    if( m_callback )
        (*m_callback)();

    m_updateLock = false;
}


void WX_INFOBAR::AddButton( QWidget* aButton )
{
    aButton->setParent( this );
    m_buttons->addWidget( aButton );
}


void WX_INFOBAR::AddButton( const wxString& aLabel, std::function<void()> aCallback )
{
    QToolButton* button = new QToolButton( this );
    button->setText( ToQString( aLabel ) );
    button->setAutoRaise( true );
    button->setCursor( Qt::PointingHandCursor );

    QObject::connect( button, &QToolButton::clicked, this,
                      [aCallback]()
                      {
                          if( aCallback )
                              aCallback();
                      } );

    AddButton( button );
}


void WX_INFOBAR::AddCloseButton( const wxString& aTooltip )
{
    m_closeButton = new QToolButton( this );
    m_closeButton->setIcon( style()->standardIcon( QStyle::SP_TitleBarCloseButton ) );
    m_closeButton->setToolTip( ToQString( aTooltip ) );
    m_closeButton->setAutoRaise( true );

    QObject::connect( m_closeButton, &QToolButton::clicked, this,
                      [this]()
                      {
                          onCloseButton();
                      } );

    m_buttons->addWidget( m_closeButton );
}


void WX_INFOBAR::RemoveAllButtons()
{
    while( QLayoutItem* item = m_buttons->takeAt( 0 ) )
    {
        delete item->widget();
        delete item;
    }

    m_closeButton = nullptr;
}


bool WX_INFOBAR::HasCloseButton() const
{
    return m_closeButton != nullptr;
}


void WX_INFOBAR::onCloseButton()
{
    Dismiss();
}


void WX_INFOBAR::onTimer()
{
    // Reset and clear the timer
    m_showTime = 0;

    Dismiss();
}


REPORTER& INFOBAR_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    m_message.reset( new wxString( aText ) );
    m_severity = aSeverity;
    m_messageSet = true;

    return *this;
}


bool INFOBAR_REPORTER::HasMessage() const
{
    return m_message && !m_message->IsEmpty();
}


void INFOBAR_REPORTER::Finalize()
{
    // Don't do anything if no message was ever given
    if( !m_infoBar || !m_messageSet )
        return;

    // Short circuit if the message is empty and it is already hidden
    if( !HasMessage() && !m_infoBar->isVisible() )
        return;

    int icon = wxICON_NONE;

    switch( m_severity )
    {
    case RPT_SEVERITY_UNDEFINED: icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_INFO:      icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_EXCLUSION: icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_ACTION:    icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_WARNING:   icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_ERROR:     icon = wxICON_ERROR;       break;
    case RPT_SEVERITY_IGNORE:    icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_DEBUG:     icon = wxICON_INFORMATION; break;
    }

    if( m_message->EndsWith( wxS( "\n" ) ) )
        *m_message = m_message->Left( m_message->Length() - 1 );

    if( HasMessage() )
        m_infoBar->QueueShowMessage( *m_message, icon );
    else
        m_infoBar->QueueDismiss();
}
