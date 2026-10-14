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
 * @file kiwx_combo.cpp
 * The combo controls of the wxWidgets compatibility layer of the Qt build.
 */

#include <wx/kiwx_combo.h>

#include <QFrame>
#include <QHBoxLayout>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>


wxSize wxComboPopup::GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight )
{
    return wxSize( aMinWidth, aPrefHeight > 0 ? std::min( aPrefHeight, aMaxHeight ) : aMaxHeight );
}


void wxComboPopup::Dismiss()
{
    if( m_combo )
        m_combo->Dismiss();
}


wxComboCtrl::wxComboCtrl( QWidget* aParent, int aId, const wxString& aValue, const wxPoint& aPos,
                          const wxSize& aSize, long aStyle, const wxValidator& aValidator,
                          const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent ),
        m_popup( nullptr ),
        m_popupWindow( nullptr ),
        m_popupMinWidth( 0 ),
        m_popupMaxHeight( 400 )
{
    SetId( aId );

    m_text = new wxTextCtrl( this, wxID_ANY, aValue );
    m_text->setFrame( false );

    m_button = new QToolButton( this );
    m_button->setArrowType( Qt::DownArrow );
    m_button->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_button->setAutoRaise( true );
    m_button->setFocusPolicy( Qt::NoFocus );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( m_text, 1 );
    layout->addWidget( m_button, 0 );

    setFocusProxy( m_text );

    QObject::connect( m_button, &QToolButton::clicked, this, [this]() { OnButtonClick(); } );

    if( !aName.IsEmpty() )
        SetName( aName );
}


wxComboCtrl::~wxComboCtrl()
{
    delete m_popup;
}


void wxComboCtrl::DoSetPopupControl( wxComboPopup* aPopup )
{
    m_popup = aPopup;
}


void wxComboCtrl::SetPopupControl( wxComboPopup* aPopup )
{
    if( m_popup && m_popup != aPopup )
        delete m_popup;

    DoSetPopupControl( aPopup );

    if( m_popup )
    {
        m_popup->InitBase( this );
        m_popup->Init();
    }
}


QWidget* wxComboCtrl::GetPopupWindow() const
{
    return m_popupWindow;
}


bool wxComboCtrl::IsPopupShown() const
{
    return m_popupWindow && m_popupWindow->isVisible();
}


void wxComboCtrl::Popup()
{
    if( !m_popup )
        return;

    if( !m_popupWindow )
    {
        m_popupWindow = new QFrame( this, Qt::Popup );
        m_popupWindow->setFrameShape( QFrame::StyledPanel );
        QVBoxLayout* layout = new QVBoxLayout( m_popupWindow );
        layout->setContentsMargins( 1, 1, 1, 1 );
    }

    if( !m_popup->m_created )
    {
        m_popup->Create( m_popupWindow );
        m_popup->m_created = true;

        if( QWidget* control = m_popup->GetControl() )
            m_popupWindow->layout()->addWidget( control );
    }

    m_popup->SetStringValue( GetValue() );

    // Below the control, as wide as it, within the screen
    wxSize size = m_popup->GetAdjustedSize( std::max( width(), m_popupMinWidth ), -1,
                                            m_popupMaxHeight );
    QPoint pos = mapToGlobal( QPoint( 0, height() ) );

    if( QScreen* screen = this->screen() )
    {
        QRect area = screen->availableGeometry();

        if( pos.y() + size.y > area.bottom() )
            pos.setY( mapToGlobal( QPoint( 0, 0 ) ).y() - size.y );

        pos.setX( std::min( pos.x(), area.right() - size.x ) );
    }

    m_popupWindow->resize( size.x, size.y );
    m_popupWindow->move( pos );

    m_popup->OnPopup();
    m_popupWindow->show();

    if( QWidget* control = m_popup->GetControl() )
        control->setFocus();
}


void wxComboCtrl::Dismiss()
{
    if( !IsPopupShown() )
        return;

    m_popupWindow->hide();

    if( m_popup )
    {
        m_popup->OnDismiss();
        SetValue( m_popup->GetStringValue() );
    }

    m_text->setFocus();
}


void wxComboCtrl::OnButtonClick()
{
    if( IsPopupShown() )
        Dismiss();
    else
        Popup();
}


void wxComboCtrl::SetButtonPosition( int aWidth, int aHeight, int aSide, int aSpacingX )
{
    if( aWidth > 0 )
        m_button->setFixedWidth( aWidth );
}


void wxComboCtrl::SetButtonBitmaps( const wxBitmapBundle& aNormal, bool aPushButtonBg,
                                    const wxBitmapBundle& aPressed, const wxBitmapBundle& aHover,
                                    const wxBitmapBundle& aDisabled )
{
    m_button->setArrowType( Qt::NoArrow );
    m_button->setIcon( aNormal );
}


wxString wxComboCtrl::GetValue() const
{
    return m_text->GetValue();
}


void wxComboCtrl::SetValue( const wxString& aValue )
{
    m_text->SetValue( aValue );
}


void wxComboCtrl::ChangeValue( const wxString& aValue )
{
    m_text->ChangeValue( aValue );
}


void wxComboCtrl::SelectAll()
{
    m_text->SelectAll();
}


void wxComboCtrl::SetInsertionPointEnd()
{
    m_text->SetInsertionPointEnd();
}


void wxComboCtrl::SetSelection( long aFrom, long aTo )
{
    m_text->SetSelection( aFrom, aTo );
}


bool wxComboCtrl::SetHint( const wxString& aHint )
{
    return m_text->SetHint( aHint );
}


void wxComboCtrl::SetEditable( bool aEditable )
{
    m_text->SetEditable( aEditable );
}


bool wxComboCtrl::IsEditable() const
{
    return m_text->IsEditable();
}
