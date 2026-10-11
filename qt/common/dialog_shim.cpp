/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2023 CERN
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
 * @file dialog_shim.cpp
 * DIALOG_SHIM of the Qt build.
 */

#include <dialog_shim.h>

#include <QAbstractButton>
#include <QApplication>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>

#include <nlohmann/json.hpp>

#include <eda_base_frame.h>
#include <eda_draw_frame.h>
#include <kiway.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/common_settings_internals.h>
#include <widgets/unit_binder.h>
#include <core/ki_assert.h>

#include <qt_strings.h>
#include <ui_hooks.h>


/// The key of a dialog in the settings: its title, without the part in parentheses.
static std::string getDialogKeyFromTitle( const wxString& aTitle )
{
    std::string title = aTitle.ToStdString();
    size_t parenPos = title.rfind( '(' );

    if( parenPos != std::string::npos && parenPos > 0 )
    {
        size_t end = parenPos;

        while( end > 0 && title[end - 1] == ' ' )
            end--;

        return title.substr( 0, end );
    }

    return title;
}


DIALOG_SHIM::DIALOG_SHIM( QWidget* aParent, const wxString& aTitle ) :
        wxWindowCompat<QDialog>( aParent ),
        KIWAY_HOLDER( nullptr, KIWAY_HOLDER::DIALOG ),
        m_units( EDA_UNITS::MM ),
        m_initialFocusTarget( nullptr ),
        m_qmodal_loop( nullptr ),
        m_qmodal_parent( nullptr ),
        m_parentFrame( nullptr ),
        m_returnCode( wxID_CANCEL )
{
    SetTitle( aTitle );

    // The frame of the dialog: its parent, or the frame its parent is in
    for( QWidget* ancestor = aParent; ancestor && !m_parentFrame;
         ancestor = ancestor->parentWidget() )
    {
        m_parentFrame = dynamic_cast<EDA_BASE_FRAME*>( ancestor );
    }

    if( m_parentFrame )
    {
        m_units = m_parentFrame->GetUserUnits();

        if( m_parentFrame->HasKiway() )
            SetKiway( this, &m_parentFrame->Kiway() );
    }
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    // The child controls (and the UNIT_BINDERs that live alongside them) outlive this dialog's
    // member teardown, so sever their back-references now while m_unitBinders is still valid.
    for( const auto& [window, binder] : m_unitBinders )
        binder->DetachFromDialogShim();

    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    if( HasKiway() && Kiway().GetBlockingDialog() == this )
        Kiway().SetBlockingDialog( nullptr );
}


void DIALOG_SHIM::SetTitle( const wxString& aTitle )
{
    setWindowTitle( ToQString( aTitle ) );
}


wxString DIALOG_SHIM::GetTitle() const
{
    return ToWxString( windowTitle() );
}


void DIALOG_SHIM::OnModify()
{
    if( !GetTitle().StartsWith( wxS( "*" ) ) )
        SetTitle( wxS( "*" ) + GetTitle() );
}


void DIALOG_SHIM::ClearModify()
{
    if( GetTitle().StartsWith( wxS( "*" ) ) )
        SetTitle( GetTitle().AfterFirst( '*' ) );
}


void DIALOG_SHIM::finishDialogSettings()
{
    // The layout gives the minimal size of the dialog
    adjustSize();
    setMinimumSize( minimumSizeHint() );
}


std::string DIALOG_SHIM::dialogKey() const
{
    return m_hash_key.empty() ? getDialogKeyFromTitle( GetTitle() ) : m_hash_key;
}


void DIALOG_SHIM::saveGeometry()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( !settings )
        return;

    std::map<std::string, nlohmann::json>& dlgMap =
            settings->CsInternals().m_dialogControlValues[ dialogKey() ];

    // The size is in device independent pixels, as the wxWidgets build saves it
    nlohmann::json geom;
    geom[ "x" ] = pos().x();
    geom[ "y" ] = pos().y();
    geom[ "w" ] = width();
    geom[ "h" ] = height();
    dlgMap[ "__geometry" ] = geom;
}


void DIALOG_SHIM::restoreGeometry()
{
    QRect saved;

    if( COMMON_SETTINGS* settings = Pgm().GetCommonSettings() )
    {
        auto dlgIt = settings->CsInternals().m_dialogControlValues.find( dialogKey() );

        if( dlgIt != settings->CsInternals().m_dialogControlValues.end() )
        {
            auto geoIt = dlgIt->second.find( "__geometry" );

            if( geoIt != dlgIt->second.end() && geoIt->second.is_object() )
            {
                const nlohmann::json& g = geoIt->second;
                saved = QRect( g.value( "x", 0 ), g.value( "y", 0 ), g.value( "w", 500 ),
                               g.value( "h", 300 ) );
            }
        }
    }

    if( saved.width() > 0 && saved.height() > 0 )
    {
        resize( std::max( width(), saved.width() ), std::max( height(), saved.height() ) );
        move( saved.topLeft() );
    }
    else if( parentWidget() )
    {
        // Centered on the parent window
        QRect parentRect = parentWidget()->window()->frameGeometry();
        move( parentRect.center() - QPoint( width() / 2, height() / 2 ) );
    }

    // Re-center if the title bar would land on no display.
    QPoint grabPoint = pos() + QPoint( width() / 2, 15 );

    if( !QGuiApplication::screenAt( grabPoint ) )
    {
        QScreen* screen = parentWidget() ? parentWidget()->screen()
                                         : QGuiApplication::primaryScreen();
        move( screen->availableGeometry().center() - QPoint( width() / 2, height() / 2 ) );
    }
}


void DIALOG_SHIM::RegisterUpdateUI( std::function<void()> aUpdate )
{
    KIQT::RegisterUpdateUI( this, std::move( aUpdate ) );
}


void DIALOG_SHIM::showEvent( QShowEvent* aEvent )
{
    if( !aEvent->spontaneous() )
    {
        TransferDataToWindow();

        // The order of the keyboard focus set by the dialog
        for( size_t ii = 1; ii < m_tabOrder.size(); ++ii )
            QWidget::setTabOrder( m_tabOrder[ii - 1], m_tabOrder[ii] );

        restoreGeometry();

        if( m_initialFocusTarget )
        {
            QWidget* target = m_initialFocusTarget;

            QTimer::singleShot( 0, this,
                                [target]()
                                {
                                    target->setFocus( Qt::OtherFocusReason );
                                } );
        }
    }

    QDialog::showEvent( aEvent );
}


void DIALOG_SHIM::hideEvent( QHideEvent* aEvent )
{
    if( !aEvent->spontaneous() )
    {
        saveGeometry();

        // The canvas of the frame gets the keys again
        if( EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_parentFrame ) )
        {
            if( frame->GetCanvas() )
                frame->GetCanvas()->SetFocus();
        }
    }

    QDialog::hideEvent( aEvent );
}


int DIALOG_SHIM::ShowModal()
{
    m_returnCode = wxID_CANCEL;
    exec();

    return m_returnCode;
}


void DIALOG_SHIM::EndModal( int aRetCode )
{
    if( IsQuasiModal() )
    {
        EndQuasiModal( aRetCode );
        return;
    }

    // The values are checked and kept before the dialog closes
    if( aRetCode == wxID_OK && ( !Validate() || !TransferDataFromWindow() ) )
        return;

    m_returnCode = aRetCode;
    QDialog::done( aRetCode == wxID_OK ? QDialog::Accepted : QDialog::Rejected );
}


void DIALOG_SHIM::EndDialog( int aRetCode )
{
    if( IsQuasiModal() )
    {
        EndQuasiModal( aRetCode );
        return;
    }

    m_returnCode = aRetCode;
    QDialog::done( aRetCode == wxID_OK ? QDialog::Accepted : QDialog::Rejected );
}


void DIALOG_SHIM::done( int aResult )
{
    // QDialog::accept() and reject() (the Enter and Escape keys) give the codes of KiCad
    EndModal( aResult == QDialog::Accepted ? wxID_OK : wxID_CANCEL );
}


int DIALOG_SHIM::ShowQuasiModal()
{
    KI_ASSERT_MSG( !m_qmodal_loop, wxT( "Caller using ShowQuasiModal() twice on same window?" ) );

    // quasi-modal: disable only my parent window
    QWidget* parent = parentWidget() ? parentWidget()->window() : nullptr;

    if( parent )
    {
        parent->setEnabled( false );
        m_qmodal_parent = parent;
    }

    // The dialog itself must stay enabled
    setEnabled( true );

    m_returnCode = wxID_CANCEL;
    show();

    QEventLoop event_loop;
    m_qmodal_loop = &event_loop;
    event_loop.exec();
    m_qmodal_loop = nullptr;

    return m_returnCode;
}


void DIALOG_SHIM::EndQuasiModal( int retCode )
{
    if( !IsQuasiModal() )
        return;

    // The values are checked and kept before the dialog closes, as for a modal dialog
    if( retCode == wxID_OK && ( !Validate() || !TransferDataFromWindow() ) )
        return;

    m_returnCode = retCode;

    if( m_qmodal_parent )
    {
        m_qmodal_parent->setEnabled( true );
        m_qmodal_parent = nullptr;
    }

    m_qmodal_loop->exit();
    hide();
}


void DIALOG_SHIM::connectStandardButton( QPushButton* aButton, int aId )
{
    // The button is connected once (SetupStandardButtons() can be called after), and not if it
    // has a handler: the handler does it (standardButtonClicked())
    if( aButton->property( "kicadStandardButton" ).isValid()
            || aButton->property( "kicadStandardButtonId" ).isValid() )
    {
        return;
    }

    aButton->setProperty( "kicadStandardButton", aId );

    if( aId == wxID_APPLY )
    {
        QObject::connect( aButton, &QPushButton::clicked, this,
                          [this]()
                          {
                              // Dialogs that provide Apply buttons should make sure data is
                              // valid before allowing a transfer
                              if( Validate() )
                                  TransferDataFromWindow();
                          } );
    }
    else if( aId != wxID_HELP && aId != wxID_CONTEXT_HELP )
    {
        QObject::connect( aButton, &QPushButton::clicked, this,
                          [this, aId]()
                          {
                              EndModal( aId );
                          } );
    }
}


void DIALOG_SHIM::standardButtonClicked( QWidget* aSource )
{
    QVariant id = aSource ? aSource->property( "kicadStandardButtonId" ) : QVariant();

    if( !id.isValid() )
        return;

    if( id.toInt() == wxID_APPLY )
    {
        // Dialogs that provide Apply buttons should make sure data is valid before allowing a
        // transfer
        if( Validate() )
            TransferDataFromWindow();
    }
    else if( id.toInt() != wxID_HELP && id.toInt() != wxID_CONTEXT_HELP )
    {
        EndModal( id.toInt() );
    }
}


void DIALOG_SHIM::SetupStandardButtons( std::map<int, wxString> aLabels )
{
    for( QDialogButtonBox* box : findChildren<QDialogButtonBox*>() )
    {
        auto setupButton =
                [&]( QDialogButtonBox::StandardButton aButton, int aId, const wxString& aLabel )
                {
                    QPushButton* button = box->button( aButton );

                    if( !button )
                        return;

                    if( aLabels.count( aId ) > 0 )
                        button->setText( ToQString( aLabels[aId] ) );
                    else
                        button->setText( ToQString( aLabel ) );

                    connectStandardButton( button, aId );
                };

        // The buttons are handled here, not by the roles of the box
        QObject::disconnect( box, nullptr, this, nullptr );

        setupButton( QDialogButtonBox::Ok,     wxID_OK,     _( "&OK" ) );
        setupButton( QDialogButtonBox::Cancel, wxID_CANCEL, _( "&Cancel" ) );
        setupButton( QDialogButtonBox::Yes,    wxID_YES,    _( "&Yes" ) );
        setupButton( QDialogButtonBox::No,     wxID_NO,     _( "&No" ) );
        setupButton( QDialogButtonBox::Apply,  wxID_APPLY,  _( "&Apply" ) );
        setupButton( QDialogButtonBox::Save,   wxID_SAVE,   _( "&Save" ) );
        setupButton( QDialogButtonBox::Help,   wxID_HELP,   _( "&Help" ) );

        if( QPushButton* ok = box->button( QDialogButtonBox::Ok ) )
            ok->setDefault( true );
    }
}


void DIALOG_SHIM::OptOut( QWidget* aWindow )
{
    m_optOut.insert( aWindow );
}


void DIALOG_SHIM::RegisterUnitBinder( UNIT_BINDER* aUnitBinder, QWidget* aWindow )
{
    m_unitBinders[aWindow] = aUnitBinder;
}


void DIALOG_SHIM::UnregisterUnitBinder( UNIT_BINDER* aUnitBinder )
{
    for( auto it = m_unitBinders.begin(); it != m_unitBinders.end(); )
    {
        if( it->second == aUnitBinder )
            it = m_unitBinders.erase( it );
        else
            ++it;
    }
}
