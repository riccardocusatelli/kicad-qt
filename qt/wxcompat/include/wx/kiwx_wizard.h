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

#pragma once

/**
 * @file kiwx_wizard.h
 * The wizards of wxWidgets (wxWizard) on a dialog of Qt: the pages (panels) are chained, the
 * dialog shows one of them with the buttons to go back, forward (or finish) and to cancel, and
 * sends the events of the wizard (the cancel and the page changes can be vetoed).
 */

#include <kiqt.h>

#include <QDialog>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_events.h>
#include <wx/kiwx_gdi.h>
#include <wx/kiwx_gui.h>
#include <wx/kiwx_window.h>
#include <wx/toplevel.h>

class QPushButton;
class QStackedWidget;
class wxWizard;


/// A page of a wizard: a panel, with the previous and the next pages.
class KIQT_API wxWizardPage : public wxPanel
{
public:
    explicit wxWizardPage( wxWizard* aParent, const wxBitmap& aBitmap = wxNullBitmap );

    virtual wxWizardPage* GetPrev() const = 0;
    virtual wxWizardPage* GetNext() const = 0;
};


/// A page of a wizard which knows its previous and next pages.
class KIQT_API wxWizardPageSimple : public wxWizardPage
{
public:
    explicit wxWizardPageSimple( wxWizard* aParent, wxWizardPage* aPrev = nullptr,
                                 wxWizardPage* aNext = nullptr,
                                 const wxBitmap& aBitmap = wxNullBitmap ) :
            wxWizardPage( aParent, aBitmap ),
            m_prev( aPrev ),
            m_next( aNext )
    {
    }

    wxWizardPage* GetPrev() const override { return m_prev; }
    wxWizardPage* GetNext() const override { return m_next; }

    void SetPrev( wxWizardPage* aPrev ) { m_prev = aPrev; }
    void SetNext( wxWizardPage* aNext ) { m_next = aNext; }

    /// Chain this page and the next one; returns the next one (to chain the pages in a row).
    wxWizardPageSimple& Chain( wxWizardPageSimple* aNext )
    {
        m_next = aNext;
        aNext->SetPrev( this );
        return *aNext;
    }

    static void Chain( wxWizardPageSimple* aFirst, wxWizardPageSimple* aSecond )
    {
        aFirst->Chain( aSecond );
    }

private:
    wxWizardPage* m_prev;
    wxWizardPage* m_next;
};


/// An event of a wizard: the page, and the direction of a page change.
class wxWizardEvent : public wxNotifyEvent
{
public:
    explicit wxWizardEvent( wxEventType aType = wxEVT_NULL, int aId = 0, bool aDirection = true,
                            wxWizardPage* aPage = nullptr ) :
            wxNotifyEvent( aType, aId ),
            m_direction( aDirection ),
            m_page( aPage )
    {
    }

    /// True going forward, false going back.
    bool GetDirection() const { return m_direction; }
    wxWizardPage* GetPage() const { return m_page; }

private:
    bool          m_direction;
    wxWizardPage* m_page;
};


/// A wizard: a dialog showing its pages one at a time.
class KIQT_API wxWizard : public wxWindowCompat<QDialog>
{
public:
    explicit wxWizard( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                       const wxString& aTitle = wxEmptyString,
                       const wxBitmap& aBitmap = wxNullBitmap,
                       const wxPoint& aPos = wxDefaultPosition,
                       long aStyle = wxDEFAULT_DIALOG_STYLE );

    /// Show the wizard from a page; true if the user finished it, false if he cancelled it.
    bool RunWizard( wxWizardPage* aFirstPage );

    wxWizardPage* GetCurrentPage() const { return m_current; }

    /// Show a page; false if it is vetoed (wxEVT_WIZARD_PAGE_CHANGING).
    bool ShowPage( wxWizardPage* aPage, bool aGoingForward = true );

    bool HasNextPage( wxWizardPage* aPage ) const { return aPage && aPage->GetNext(); }
    bool HasPrevPage( wxWizardPage* aPage ) const { return aPage && aPage->GetPrev(); }

    /// The minimum size of the area of the pages.
    void SetPageSize( const wxSize& aSize );
    wxSize GetPageSize() const;

    void SetBorder( int ) {}

protected:
    void reject() override;

private:
    /// Add a page, and the pages chained to it, to the area of the pages.
    void addPages( wxWizardPage* aPage );

    void goForward();
    void goBack();

    /// The buttons follow the page (back, next or finish).
    void updateButtons();

    QStackedWidget* m_pages = nullptr;
    QPushButton*    m_backButton = nullptr;
    QPushButton*    m_nextButton = nullptr;
    QPushButton*    m_cancelButton = nullptr;
    wxWizardPage*   m_current = nullptr;
};
