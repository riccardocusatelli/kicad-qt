/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

/**
 * @file paged_dialog_qt.cpp
 * PAGED_DIALOG with Qt (common/widgets/paged_dialog.cpp with wxWidgets): the dialogs of pages in
 * a tree (the preferences, the board setup...), on the WX_TREEBOOK of the compatibility layer.
 * The pages are the panels of KiCad, unchanged.
 */

#include <widgets/paged_dialog.h>
#include <wx/grid.h>

#include <algorithm>
#include <map>

#include <QApplication>
#include <QKeyEvent>
#include <QScreen>
#include <QTableWidget>

#include <QTextBlock>

#include <launch_ext.h>
#include <string_utils.h>
#include <paths.h>
#include <widgets/resettable_panel.h>
#include <widgets/ui_common.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_panel.h>
#include <widgets/wx_treebook.h>

#include <qt_strings.h>

// Maps from dialogTitle <-> pageTitle for keeping track of last-selected pages.
// This is not a simple page index because some dialogs have dynamic page sets.
static std::map<wxString, wxString> g_lastPage;
static std::map<wxString, wxString> g_lastParentPage;


PAGED_DIALOG::PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset,
                            bool aShowOpenFolder, const wxString& aAuxiliaryAction,
                            const wxSize& aInitialSize ) :
        DIALOG_SHIM( aParent, aTitle ),
        m_auxiliaryButton( nullptr ),
        m_resetButton( nullptr ),
        m_openPrefsDirButton( nullptr ),
        m_title( aTitle )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    setLayout( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    WX_PANEL* treebookPanel = new WX_PANEL( this );
    treebookPanel->SetBorders( false, false, false, true );
    QVBoxLayout* treebookSizer = new QVBoxLayout( treebookPanel );
    treebookSizer->setContentsMargins( 0, 0, 0, 2 );

    m_treebook = new WX_TREEBOOK( treebookPanel, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    m_treebook->GetTreeCtrl()->setFrameShape( QFrame::NoFrame );

    treebookSizer->addWidget( m_treebook, 1 );
    mainSizer->Add( treebookPanel, 1, wxEXPAND, 0 );

    m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    if( aShowReset )
    {
        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_buttonsSizer->Add( m_resetButton, 0, wxALIGN_CENTER_VERTICAL );
    }

    if( aShowOpenFolder )
    {
#ifdef __APPLE__
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Reveal Preferences in Finder" ) );
#else
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Open Preferences Directory" ) );
#endif
        m_buttonsSizer->Add( m_openPrefsDirButton, 0, wxALIGN_CENTER_VERTICAL );
    }

    if( !aAuxiliaryAction.IsEmpty() )
    {
        m_auxiliaryButton = new wxButton( this, wxID_ANY, aAuxiliaryAction );
        m_buttonsSizer->Add( m_auxiliaryButton, 0, wxALIGN_CENTER_VERTICAL );
    }

    m_buttonsSizer->AddStretchSpacer();

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer( this );
    sdbSizer->AddStandardButton( QDialogButtonBox::Ok, wxID_OK );
    sdbSizer->AddStandardButton( QDialogButtonBox::Cancel, wxID_CANCEL );

    m_buttonsSizer->Add( sdbSizer, 1 );
    mainSizer->Add( m_buttonsSizer, 0, wxEXPAND );

    SetupStandardButtons();

    // We normally save the dialog size and position based on its class-name.  This class
    // substitutes the title so that each distinctly-titled dialog can have its own saved
    // size and position.
    m_hash_key = TO_UTF8( aTitle );

    if( aInitialSize.x > 0 && aInitialSize.y > 0 )
        resize( aInitialSize.x, aInitialSize.y );

    if( m_auxiliaryButton )
        m_auxiliaryButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onAuxiliaryAction, this );

    if( m_resetButton )
        m_resetButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onResetButton, this );

    if( m_openPrefsDirButton )
        m_openPrefsDirButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onOpenPrefsDir, this );

    m_treebook->Bind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onCharHook, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::onPageChanged, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::onPageChanging, this );
}


void PAGED_DIALOG::finishInitialization()
{
    for( size_t i = 1; i < m_treebook->GetPageCount(); ++i )
        m_macHack.push_back( true );

    // Room for the scrollbar of the tree
    QTreeWidget* tree = m_treebook->GetTreeCtrl();
    tree->resizeColumnToContents( 0 );
    tree->setMinimumWidth( tree->columnWidth( 0 ) + 40 );
}


void PAGED_DIALOG::SetInitialPage( const wxString& aPage, const wxString& aParentPage )
{
    g_lastPage[ m_title ] = aPage;
    g_lastParentPage[ m_title ] = aParentPage;
}


PAGED_DIALOG::~PAGED_DIALOG()
{
    // Store the current parentPageTitle/pageTitle hierarchy so we can re-select it
    // next time.
    wxString lastPage = wxEmptyString;
    wxString lastParentPage = wxEmptyString;

    int selected = m_treebook->GetSelection();

    if( selected != wxNOT_FOUND )
    {
        lastPage = m_treebook->GetPageText( (unsigned) selected );

        int parent = m_treebook->GetPageParent( (unsigned) selected );

        if( parent != wxNOT_FOUND )
            lastParentPage = m_treebook->GetPageText( (unsigned) parent );
    }

    g_lastPage[ m_title ] = lastPage;
    g_lastParentPage[ m_title ] = lastParentPage;
}


bool PAGED_DIALOG::TransferDataToWindow()
{
    finishInitialization();

    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    // Search for a page matching the lastParentPageTitle/lastPageTitle hierarchy
    wxString lastPage = g_lastPage[ m_title ];
    wxString lastParentPage = g_lastParentPage[ m_title ];
    int lastPageIndex = wxNOT_FOUND;

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( m_treebook->GetPageText( i ) == lastPage )
        {
            if( lastParentPage.IsEmpty() )
            {
                lastPageIndex = i;
                break;
            }

            if( m_treebook->GetPageParent( i ) >= 0
                && m_treebook->GetPageText( (unsigned) m_treebook->GetPageParent( i ) ) == lastParentPage )
            {
                lastPageIndex = i;
                break;
            }
        }
    }

    lastPageIndex = std::max( 0, lastPageIndex );
    m_treebook->SetSelection( lastPageIndex );
    UpdateResetButton( lastPageIndex );

    return true;
}


bool PAGED_DIALOG::TransferDataFromWindow()
{
    return DIALOG_SHIM::TransferDataFromWindow();
}


PAGED_DIALOG* PAGED_DIALOG::GetDialog( wxWindow* aWindow )
{
    for( QWidget* parent = aWindow ? aWindow->GetHandle() : nullptr; parent;
         parent = parent->parentWidget() )
    {
        if( PAGED_DIALOG* parentDialog = dynamic_cast<PAGED_DIALOG*>( parent ) )
            return parentDialog;
    }

    return nullptr;
}


void PAGED_DIALOG::SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId,
                             int aRow, int aCol )
{
    SetError( aMessage, FindWindow( aPageName ), FindWindow( aCtrlId ), aRow, aCol );
}


void PAGED_DIALOG::SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl,
                             int aRow, int aCol )
{
    m_infoBar->ShowMessageFor( aMessage, 10000, wxICON_WARNING );

    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
    {
        textCtrl->SetSelection( -1, -1 );
        textCtrl->SetFocus();
        return;
    }

    if( QPlainTextEdit* text = qobject_cast<QPlainTextEdit*>( aCtrl ? aCtrl->GetHandle() : nullptr ) )
    {
        if( aRow > 0 )
        {
            QTextCursor cursor( text->document()->findBlockByNumber( aRow - 1 ) );
            cursor.movePosition( QTextCursor::Right, QTextCursor::MoveAnchor, std::max( aCol - 1, 0 ) );
            text->setTextCursor( cursor );
        }

        text->setFocus();
        return;
    }

    if( wxGrid* grid = dynamic_cast<wxGrid*>( aCtrl ) )
    {
        grid->SetFocus();
        grid->MakeCellVisible( aRow, aCol );
        grid->SetGridCursor( aRow, aCol );
        grid->EnableCellEditControl( true );
        grid->ShowCellEditControl();
        return;
    }
}


void PAGED_DIALOG::UpdateResetButton( int aPage )
{
    wxWindow* panel = m_treebook->ResolvePage( aPage );
    wxPanel* resettable = dynamic_cast<wxPanel*>( panel );

    // Enable the reset button only if the page is re-settable
    if( m_resetButton )
    {
        if( resettable && ( resettable->GetWindowStyle() & wxRESETTABLE ) )
        {
            wxString name = m_treebook->GetPageText( aPage );
            name.Replace( wxT( "&" ), wxT( "&&" ) );

            m_resetButton->SetLabel( wxString::Format( _( "Reset %s to Defaults" ), name ) );
            m_resetButton->SetToolTip( resettable->GetHelpTextAtPoint( wxPoint( -INT_MAX, INT_MAX ),
                                                                       wxHelpEvent::Origin_Unknown ) );
            m_resetButton->Enable( true );
        }
        else
        {
            m_resetButton->SetLabel( _( "Reset to Defaults" ) );
            m_resetButton->SetToolTip( wxString() );
            m_resetButton->Enable( false );
        }
    }
}


void PAGED_DIALOG::onCharHook( wxKeyEvent& aEvent )
{
    QWidget* focus = QApplication::focusWidget();

    // The keys of the editable controls and of the lists are theirs
    if( qobject_cast<QLineEdit*>( focus ) || qobject_cast<QPlainTextEdit*>( focus )
            || qobject_cast<QAbstractItemView*>( focus ) )
    {
        aEvent.Skip();
        return;
    }

    if( aEvent.GetKeyCode() == WXK_UP )
    {
        int page = m_treebook->GetSelection();

        if( page >= 1 )
        {
            wxWindow* prevPage = m_treebook->GetPage( page - 1 );

            if( !prevPage || prevPage->GetChildren().empty() )
                m_treebook->SetSelection( std::max( page - 2, 0 ) );
            else
                m_treebook->SetSelection( page - 1 );
        }

        m_treebook->GetTreeCtrl()->setFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        int page = m_treebook->GetSelection();

        m_treebook->SetSelection( std::min<int>( page + 1, m_treebook->GetPageCount() - 1 ) );

        m_treebook->GetTreeCtrl()->setFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else
    {
        aEvent.Skip();
    }
}


void PAGED_DIALOG::onPageChanged( wxBookCtrlEvent& event )
{
    int sel = event.GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    size_t page = static_cast<size_t>( sel );

    // Use the first sub-page when a tree level node is selected, but only if the node is
    // expanded.
    wxWindow* currentPage = m_treebook->GetCurrentPage();

    if( currentPage && currentPage->GetChildren().empty()
            && page + 1 < m_treebook->GetPageCount()
            && m_treebook->IsNodeExpanded( page ) )
    {
        m_treebook->ChangeSelection( ++page );
    }

    UpdateResetButton( page );

    // Make sure the dialog size is enough to fit the page content, within the screen
    QSize hint = sizeHint().expandedTo( QSize( 600, 500 ) ).boundedTo( QSize( 1500, 900 ) );

    if( QScreen* screen = this->screen() )
    {
        QRect area = screen->availableGeometry();
        hint = hint.boundedTo( QSize( std::max( 0, area.width() - 40 ),
                                      std::max( 0, area.height() - 80 ) ) );
    }

    setMinimumSize( hint );

    if( width() < hint.width() || height() < hint.height() )
        resize( size().expandedTo( hint ) );
}


void PAGED_DIALOG::onPageChanging( wxBookCtrlEvent& aEvent )
{
    int currentPage = aEvent.GetOldSelection();

    if( currentPage == wxNOT_FOUND )
        return;

    wxWindow* page = m_treebook->GetPage( currentPage );

    if( !page )
        return;

    // If there is a validation error on the current page, don't allow the page change.
    if( !wxValidateCompat( page ) || !wxTransferDataFromWindowCompat( page ) )
    {
        aEvent.Veto();
        return;
    }
}


void PAGED_DIALOG::onResetButton( wxCommandEvent& aEvent )
{
    int sel = m_treebook->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    // NB: dynamic_cast doesn't work over Kiway
    wxWindow* panel = m_treebook->ResolvePage( sel );

    if( wxPanel* resettable = dynamic_cast<wxPanel*>( panel ) )
    {
        wxCommandEvent resetCommand( wxEVT_COMMAND_BUTTON_CLICKED, ID_RESET_PANEL );
        resettable->ProcessWindowEvent( resetCommand );
    }
}


void PAGED_DIALOG::onOpenPrefsDir( wxCommandEvent& aEvent )
{
    wxString dir( PATHS::GetUserSettingsPath() );
    LaunchExternal( dir );
}
