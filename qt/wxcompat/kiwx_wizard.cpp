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
 * @file kiwx_wizard.cpp
 * The wizards of wxWidgets on a dialog of Qt (see wx/kiwx_wizard.h).
 */

#include <wx/kiwx_wizard.h>

#include <algorithm>

#include <QBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QStackedWidget>

#include <qt_strings.h>
#include <wx/translation.h>


wxWizardPage::wxWizardPage( wxWizard* aParent, const wxBitmap& aBitmap ) :
        wxPanel( aParent )
{
    // The page is shown by the wizard, in the area of its pages
    hide();
}


wxWizard::wxWizard( wxWidgetArgCompat aParent, int aId, const wxString& aTitle,
                    const wxBitmap& aBitmap, const wxPoint& aPos, long aStyle ) :
        wxWindowCompat<QDialog>( aParent.get() )
{
    SetId( aId );
    setWindowTitle( ToQString( aTitle ) );

    QVBoxLayout* layout = new QVBoxLayout( this );

    m_pages = new QStackedWidget( this );
    layout->addWidget( m_pages, 1 );

    QFrame* line = new QFrame( this );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    layout->addWidget( line );

    // Back, Next (or Finish) and Cancel, as the wizards of wxWidgets
    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch( 1 );

    m_backButton = new QPushButton( ToQString( _( "< &Back" ) ), this );
    m_nextButton = new QPushButton( ToQString( _( "&Next >" ) ), this );
    m_cancelButton = new QPushButton( ToQString( _( "&Cancel" ) ), this );
    m_nextButton->setDefault( true );

    buttons->addWidget( m_backButton );
    buttons->addWidget( m_nextButton );
    buttons->addSpacing( 10 );
    buttons->addWidget( m_cancelButton );
    layout->addLayout( buttons );

    QObject::connect( m_backButton, &QPushButton::clicked, this, [this]() { goBack(); } );
    QObject::connect( m_nextButton, &QPushButton::clicked, this, [this]() { goForward(); } );
    QObject::connect( m_cancelButton, &QPushButton::clicked, this, [this]() { reject(); } );

    if( aPos != wxDefaultPosition )
        move( aPos.x, aPos.y );
}


void wxWizard::SetPageSize( const wxSize& aSize )
{
    m_pages->setMinimumSize( std::max( aSize.x, 0 ), std::max( aSize.y, 0 ) );
}


wxSize wxWizard::GetPageSize() const
{
    return wxSize( m_pages->width(), m_pages->height() );
}


void wxWizard::addPages( wxWizardPage* aPage )
{
    for( wxWizardPage* page = aPage; page; page = page->GetNext() )
    {
        if( m_pages->indexOf( page ) < 0 )
            m_pages->addWidget( page );
    }
}


bool wxWizard::RunWizard( wxWizardPage* aFirstPage )
{
    if( !aFirstPage )
        return false;

    addPages( aFirstPage );

    m_current = nullptr;
    ShowPage( aFirstPage, true );

    adjustSize();

    return exec() == QDialog::Accepted;
}


bool wxWizard::ShowPage( wxWizardPage* aPage, bool aGoingForward )
{
    if( m_current && aPage != m_current )
    {
        // The page changes: the page can veto it
        wxWizardEvent changing( wxEVT_WIZARD_PAGE_CHANGING, GetId(), aGoingForward, m_current );
        changing.SetEventObject( this );
        ProcessEvent( changing );

        if( !changing.IsAllowed() )
            return false;
    }

    if( !aPage )
        return false;

    addPages( aPage );

    m_current = aPage;
    m_pages->setCurrentWidget( aPage );
    aPage->show();
    wxTransferDataToWindowCompat( aPage );

    updateButtons();

    wxWizardEvent changed( wxEVT_WIZARD_PAGE_CHANGED, GetId(), aGoingForward, aPage );
    changed.SetEventObject( this );
    ProcessEvent( changed );

    wxWizardEvent shown( wxEVT_WIZARD_PAGE_SHOWN, GetId(), aGoingForward, aPage );
    shown.SetEventObject( this );
    ProcessEvent( shown );

    return true;
}


void wxWizard::goForward()
{
    if( !m_current )
        return;

    // The data of the page, as wxWidgets: validated, then taken
    if( !wxValidateCompat( m_current ) || !wxTransferDataFromWindowCompat( m_current ) )
        return;

    if( wxWizardPage* next = m_current->GetNext() )
    {
        ShowPage( next, true );
        return;
    }

    // The last page: the wizard is finished, unless vetoed
    wxWizardEvent changing( wxEVT_WIZARD_PAGE_CHANGING, GetId(), true, m_current );
    changing.SetEventObject( this );
    ProcessEvent( changing );

    if( !changing.IsAllowed() )
        return;

    wxWizardEvent finished( wxEVT_WIZARD_FINISHED, GetId(), true, m_current );
    finished.SetEventObject( this );
    ProcessEvent( finished );

    accept();
}


void wxWizard::goBack()
{
    if( m_current && m_current->GetPrev() )
        ShowPage( m_current->GetPrev(), false );
}


void wxWizard::reject()
{
    // Cancel (the button, Escape, the close button of the window), unless vetoed
    wxWizardEvent cancel( wxEVT_WIZARD_CANCEL, GetId(), false, m_current );
    cancel.SetEventObject( this );
    ProcessEvent( cancel );

    if( cancel.IsAllowed() )
        QDialog::reject();
}


void wxWizard::updateButtons()
{
    m_backButton->setEnabled( HasPrevPage( m_current ) );
    m_nextButton->setText( HasNextPage( m_current ) ? ToQString( _( "&Next >" ) )
                                                    : ToQString( _( "&Finish" ) ) );
}
