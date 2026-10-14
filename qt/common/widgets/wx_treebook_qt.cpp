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
 * @file wx_treebook_qt.cpp
 * WX_TREEBOOK with Qt (common/widgets/wx_treebook.cpp with wxWidgets).
 */

#include <widgets/wx_treebook.h>

#include <QShowEvent>
#include <QVBoxLayout>


/**
 * A page made when it is first shown: the panels of the paged dialogs are many, most of them
 * are never shown.
 */
class LAZY_PAGE : public wxPanel
{
public:
    LAZY_PAGE( QWidget* aParent, std::function<QWidget*( QWidget* aParent )> aLazyCtor ) :
            wxPanel( aParent, wxID_ANY ),
            m_lazyCtor( std::move( aLazyCtor ) ),
            m_contents( nullptr )
    {
        m_layout = new QVBoxLayout( this );
        m_layout->setContentsMargins( 0, 0, 0, 0 );
    }

    QWidget* Resolve()
    {
        if( !m_contents )
        {
            m_contents = m_lazyCtor( this );
            m_layout->addWidget( m_contents, 1 );
            m_layout->activate();

            wxTransferDataToWindowCompat( m_contents );
        }

        return m_contents;
    }

    // The transfers of the page are the ones of its contents, once made
    bool TransferDataToWindow() override
    {
        return m_contents ? wxTransferDataToWindowCompat( m_contents ) : true;
    }

    bool TransferDataFromWindow() override
    {
        return m_contents ? wxTransferDataFromWindowCompat( m_contents ) : true;
    }

    bool Validate() override
    {
        return m_contents ? wxValidateCompat( m_contents ) : true;
    }

    long GetContentsStyle() const
    {
        wxPanel* panel = dynamic_cast<wxPanel*>( m_contents );
        return panel ? panel->GetWindowStyle() : 0;
    }

protected:
    void showEvent( QShowEvent* aEvent ) override
    {
        Resolve();
        wxPanel::showEvent( aEvent );
    }

private:
    std::function<QWidget*( QWidget* aParent )> m_lazyCtor;

    QVBoxLayout* m_layout;
    QWidget*     m_contents;
};


WX_TREEBOOK::WX_TREEBOOK( QWidget* parent, int id, const wxPoint& pos, const wxSize& size,
                          long style, const wxString& name ) :
        wxTreebook( parent, id, pos, size, style, name )
{
}


bool WX_TREEBOOK::AddLazyPage( std::function<QWidget*( QWidget* aParent )> aLazyCtor,
                               const wxString& text, bool bSelect, int imageId )
{
    return AddPage( new LAZY_PAGE( this, std::move( aLazyCtor ) ), text, bSelect, imageId );
}


bool WX_TREEBOOK::AddLazySubPage( std::function<QWidget*( QWidget* aParent )> aLazyCtor,
                                  const wxString& text, bool bSelect, int imageId )
{
    return AddSubPage( new LAZY_PAGE( this, std::move( aLazyCtor ) ), text, bSelect, imageId );
}


QWidget* WX_TREEBOOK::ResolvePage( size_t aPage )
{
    wxWindow* page = GetPage( aPage );

    if( LAZY_PAGE* lazyPage = dynamic_cast<LAZY_PAGE*>( page ) )
        return lazyPage->Resolve();

    return page ? page->GetHandle() : nullptr;
}
