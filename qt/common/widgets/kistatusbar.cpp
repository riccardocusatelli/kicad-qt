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
 * @file kistatusbar.cpp
 * KISTATUSBAR of the Qt build (see common/widgets/kistatusbar.cpp).
 */

#include <widgets/kistatusbar.h>

#include <ranges>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <fmt/format.h>
#include <wx/tokenzr.h>

#include <background_jobs_monitor.h>
#include <bitmaps.h>
#include <notifications_manager.h>
#include <pgm_gui.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <widgets/bitmap_button.h>
#include <widgets/wx_html_report_box.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <ui_hooks.h>


namespace
{

/// The dialog of the load messages: the messages, and a button to clear them.
class STATUSBAR_WARNING_REPORTER_DIALOG : public QDialog
{
public:
    STATUSBAR_WARNING_REPORTER_DIALOG( QWidget* aParent, KISTATUSBAR* aStatusBar ) :
            QDialog( aParent ),
            m_statusBar( aStatusBar )
    {
        setWindowTitle( ToQString( _( "Messages" ) ) );

        QVBoxLayout* layout = new QVBoxLayout( this );

        m_Reporter = new WX_HTML_REPORT_BOX( this );
        m_Reporter->setMinimumSize( 640, 360 );
        layout->addWidget( m_Reporter, 1 );

        QDialogButtonBox* buttons = new QDialogButtonBox( QDialogButtonBox::Ok, this );
        QPushButton*      clearButton = buttons->addButton( ToQString( _( "Clear" ) ),
                                                            QDialogButtonBox::ResetRole );

        QObject::connect( buttons, &QDialogButtonBox::accepted, this, &QDialog::accept );
        QObject::connect( clearButton, &QPushButton::clicked, this,
                          [this]()
                          {
                              if( m_statusBar )
                                  m_statusBar->ClearWarningMessages();

                              done( wxID_CLEAR );
                          } );

        layout->addWidget( buttons );
    }

    WX_HTML_REPORT_BOX* m_Reporter;

private:
    KISTATUSBAR* m_statusBar;
};

}


KISTATUSBAR::KISTATUSBAR( int aNumberFields, QWidget* parent, wxWindowID id, STYLE_FLAGS aFlags ) :
        QStatusBar( parent ),
        m_backgroundStopButton( nullptr ),
        m_notificationsButton( nullptr ),
        m_warningButton( nullptr ),
        m_normalFieldsCount( aNumberFields ),
        m_styleFlags( aFlags )
{
    if( id != wxID_ANY )
        KIQT::SetWindowId( this, id );

    setSizeGripEnabled( true );

    // The fields of the frame, then the controls of the background jobs, of the messages and of
    // the notifications
    for( int ii = 0; ii < aNumberFields; ++ii )
    {
        QLabel* field = new QLabel( this );
        field->setTextFormat( Qt::PlainText );
        addPermanentWidget( field, ii == 0 ? 1 : 0 );
        m_fields.push_back( field );
    }

    m_backgroundTxt = new QLabel( this );
    m_backgroundTxt->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_backgroundTxt->setTextFormat( Qt::PlainText );
    addPermanentWidget( m_backgroundTxt, 1 );

    m_backgroundProgressBar = new QProgressBar( this );
    m_backgroundProgressBar->setRange( 0, 100 );
    m_backgroundProgressBar->setTextVisible( false );
    m_backgroundProgressBar->setFixedWidth( 75 );
    addPermanentWidget( m_backgroundProgressBar );

    // A click on the progress bar shows the list of the jobs
    KIQT::FilterEvents( m_backgroundProgressBar,
                        [this]( QEvent* aEvent )
                        {
                            if( aEvent->type() == QEvent::MouseButtonPress )
                            {
                                onBackgroundProgressClick();
                                return true;
                            }

                            return false;
                        } );

    if( m_styleFlags & CANCEL_BUTTON )
    {
        m_backgroundStopButton = new QToolButton( this );
        m_backgroundStopButton->setText( QStringLiteral( "X" ) );
        m_backgroundStopButton->setAutoRaise( true );
        addPermanentWidget( m_backgroundStopButton );
    }

    if( m_styleFlags & WARNING_ICON )
    {
        m_warningButton = new BITMAP_BUTTON( this );
        m_warningButton->SetPadding( 0 );
        m_warningButton->SetBitmap( KiBitmapBundle( BITMAPS::small_warning ) );
        m_warningButton->SetBitmapCentered( true );
        m_warningButton->setToolTip( ToQString( _( "View load messages" ) ) );
        m_warningButton->hide();
        addPermanentWidget( m_warningButton );

        QObject::connect( m_warningButton, &QAbstractButton::clicked, this,
                          [this]()
                          {
                              onLoadWarningsIconClick();
                          } );
    }

    if( m_styleFlags & NOTIFICATION_ICON )
    {
        m_notificationsButton = new BITMAP_BUTTON( this );
        m_notificationsButton->SetPadding( 0 );
        m_notificationsButton->SetBitmap( KiBitmapBundle( BITMAPS::notifications ) );
        m_notificationsButton->SetShowBadge( true );
        m_notificationsButton->SetBitmapCentered( true );
        addPermanentWidget( m_notificationsButton );

        QObject::connect( m_notificationsButton, &QAbstractButton::clicked, this,
                          [this]()
                          {
                              onNotificationsIconClick();
                          } );
    }

    HideBackgroundProgressBar();
}


KISTATUSBAR::~KISTATUSBAR()
{
}


void KISTATUSBAR::SetStatusText( const wxString& aText, int aField )
{
    if( aField >= 0 && aField < (int) m_fields.size() )
        m_fields[aField]->setText( ToQString( aText ) );
}


wxString KISTATUSBAR::GetStatusText( int aField ) const
{
    if( aField >= 0 && aField < (int) m_fields.size() )
        return ToWxString( m_fields[aField]->text() );

    return wxEmptyString;
}


void KISTATUSBAR::SetStatusWidths( int aSize, const int* aWidths )
{
    // Only the fields of the frame: the other ones follow their controls
    for( int ii = 0; ii < aSize && ii < (int) m_fields.size(); ++ii )
    {
        QLabel* field = m_fields[ii];
        int     width = aWidths[ii];

        removeWidget( field );

        if( width >= 0 )
        {
            field->setFixedWidth( width );
            insertPermanentWidget( ii, field, 0 );
        }
        else
        {
            field->setMinimumWidth( 0 );
            field->setMaximumWidth( QWIDGETSIZE_MAX );
            insertPermanentWidget( ii, field, -width );
        }

        field->setVisible( width != 0 );
    }
}


void KISTATUSBAR::onNotificationsIconClick()
{
    KI_CHECK( m_notificationsButton, /* void */ );

    QPoint pos = m_notificationsButton->mapToGlobal(
            QPoint( m_notificationsButton->width(), 0 ) );

    PgmGui().GetNotificationsManager().ShowList( this, wxPoint( pos.x(), pos.y() ) );
}


void KISTATUSBAR::onBackgroundProgressClick()
{
    QPoint pos = m_backgroundProgressBar->mapToGlobal(
            QPoint( m_backgroundProgressBar->width(), 0 ) );

    PgmGui().GetBackgroundJobMonitor().ShowList( this, wxPoint( pos.x(), pos.y() ) );
}


void KISTATUSBAR::resizeEvent( QResizeEvent* aEvent )
{
    QStatusBar::resizeEvent( aEvent );
    updateBackgroundText();
}


void KISTATUSBAR::ShowBackgroundProgressBar( bool aCancellable )
{
    m_backgroundProgressBar->show();
    m_backgroundTxt->show();

    if( m_backgroundStopButton )
        m_backgroundStopButton->setVisible( aCancellable );
}


void KISTATUSBAR::HideBackgroundProgressBar()
{
    // The controls of the background jobs only take room while a job is running
    m_backgroundProgressBar->hide();
    m_backgroundTxt->hide();

    if( m_backgroundStopButton )
        m_backgroundStopButton->hide();
}


void KISTATUSBAR::SetBackgroundProgress( int aAmount )
{
    int range = m_backgroundProgressBar->maximum();

    if( aAmount > range )
        aAmount = range;

    m_backgroundProgressBar->setValue( aAmount );
}


void KISTATUSBAR::SetBackgroundProgressMax( int aAmount )
{
    m_backgroundProgressBar->setRange( 0, aAmount );
}


void KISTATUSBAR::SetBackgroundStatusText( const wxString& aTxt )
{
    m_backgroundRawText = aTxt;
    updateBackgroundText();

    // When there are multiple normal fields, the last normal field (typically used for
    // file watcher status on Windows) can visually overlap with the background job label
    // since both have variable width. Save and clear that field when showing background
    // text, and restore it when the background text is cleared.
    if( m_normalFieldsCount > 1 )
    {
        int      adjacentField = m_normalFieldsCount - 1;
        wxString currentText = GetStatusText( adjacentField );

        if( !aTxt.empty() )
        {
            if( !currentText.empty() )
                m_savedStatusText = currentText;

            SetStatusText( wxEmptyString, adjacentField );
        }
        else if( !m_savedStatusText.empty() )
        {
            SetStatusText( m_savedStatusText, adjacentField );
            m_savedStatusText.clear();
        }
    }
}


void KISTATUSBAR::updateBackgroundText()
{
    QString text = ToQString( m_backgroundRawText );
    int     width = m_backgroundTxt->width();

    if( !text.isEmpty() && width > 4 )
    {
        QFontMetrics metrics( m_backgroundTxt->font() );
        int          margin = metrics.horizontalAdvance( QStringLiteral( "XX" ) );

        text = metrics.elidedText( text, Qt::ElideRight, std::max( 0, width - margin ) );
    }

    m_backgroundTxt->setText( text );
}


void KISTATUSBAR::SetNotificationCount( int aCount )
{
    KI_CHECK( m_notificationsButton, /* void */ );
    wxString cnt = "";

    if( aCount > 0 )
        cnt = fmt::format( "{}", aCount );

    m_notificationsButton->SetBadgeText( cnt );
}


void KISTATUSBAR::AddWarningMessages( const wxString& aSource, const wxString& aMessages )
{
    {
        std::lock_guard<std::mutex> lock( m_warningMutex );

        wxStringTokenizer tokenizer( aMessages, wxS( "\n" ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            LOAD_MESSAGE msg;
            msg.message = tokenizer.GetNextToken();
            msg.severity = RPT_SEVERITY_WARNING;  // Default to warning for font substitutions
            m_warningMessages[aSource].push_back( msg );
        }
    }

    updateWarningUI();
}


void KISTATUSBAR::AddWarningMessages( const wxString& aSource, const std::vector<LOAD_MESSAGE>& aMessages )
{
    KI_LOG_TRACE( traceLibraries, "KISTATUSBAR::AddWarningMessages: this=%p, count=%zu",
                this, aMessages.size() );

    if( aMessages.empty() )
        return;

    {
        std::lock_guard<std::mutex> lock( m_warningMutex );
        m_warningMessages[aSource].insert( m_warningMessages[aSource].end(), aMessages.begin(),
                                           aMessages.end() );
    }

    // Update UI on main thread (not after the status bar was deleted)
    QMetaObject::invokeMethod( this, [this]() { updateWarningUI(); }, Qt::QueuedConnection );
}


size_t KISTATUSBAR::GetLoadWarningCount() const
{
    std::lock_guard<std::mutex> lock( m_warningMutex );

    size_t count = 0;

    for( const auto& [source, messages] : m_warningMessages )
        count += messages.size();

    return count;
}


void KISTATUSBAR::updateWarningUI()
{
    if( !m_warningButton )
        return;

    size_t messageCount = GetLoadWarningCount();

    m_warningButton->setVisible( messageCount > 0 );
    m_warningButton->SetShowBadge( messageCount > 0 );

    if( messageCount > 0 )
    {
        m_warningButton->setToolTip( ToQString( wxString::Format( _( "View %zu message(s)" ),
                                                                  messageCount ) ) );

        // Show count badge on the warning button
        wxString badgeText = messageCount > 99
                ? wxString( "99+" )
                : wxString::Format( wxS( "%zu" ), messageCount );
        m_warningButton->SetBadgeText( badgeText );
    }
    else
    {
        m_warningButton->SetBadgeText( wxEmptyString );
        m_warningButton->setToolTip( ToQString( _( "View messages" ) ) );
    }
}


void KISTATUSBAR::ClearWarningMessages( const wxString& aSource )
{
    {
        std::lock_guard<std::mutex> lock( m_warningMutex );

        if( aSource.IsEmpty() )
            m_warningMessages.clear();
        else if( auto it = m_warningMessages.find( aSource ); it != m_warningMessages.end() )
            m_warningMessages.erase( it );
    }

    updateWarningUI();
}


void KISTATUSBAR::onLoadWarningsIconClick()
{
    // Copy messages under lock to avoid holding lock during modal dialog
    std::unordered_map<wxString, std::vector<LOAD_MESSAGE>> messages;
    {
        std::lock_guard<std::mutex> lock( m_warningMutex );
        messages = m_warningMessages;
    }

    if( messages.empty() )
        return;

    STATUSBAR_WARNING_REPORTER_DIALOG dlg( parentWidget(), this );

    for( const std::vector<LOAD_MESSAGE>& source : std::views::values( messages ) )
    {
        for( const LOAD_MESSAGE& msg : source )
            dlg.m_Reporter->Report( msg.message, msg.severity );
    }

    dlg.m_Reporter->Flush();
    dlg.exec();
}
