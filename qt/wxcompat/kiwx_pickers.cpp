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

#include <wx/kiwx_pickers.h>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QToolButton>
#include <QUrl>

#include <qt_strings.h>


// wxHyperlinkCtrl --------------------------------------------------------------------------

wxHyperlinkCtrl::wxHyperlinkCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aLabel,
                                  const wxString& aUrl, const wxPoint& aPos, const wxSize& aSize,
                                  long aStyle ) :
        wxWindowCompat<QLabel>( aParent ),
        m_label( aLabel ),
        m_url( aUrl ),
        m_visited( false )
{
    SetId( aId );
    setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    setOpenExternalLinks( false );

    if( aStyle & wxHL_ALIGN_RIGHT )
        setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    else if( aStyle & wxHL_ALIGN_LEFT )
        setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    update();

    // The event of the link; if no handler processes it, the URL opens (as wxWidgets)
    QObject::connect( this, &QLabel::linkActivated, this,
                      [this]( const QString& )
                      {
                          m_visited = true;

                          wxHyperlinkEvent event( m_url, GetId() );
                          event.SetEventObject( this );

                          if( !ProcessEvent( event ) && !m_url.IsEmpty() )
                              QDesktopServices::openUrl( QUrl( ToQString( m_url ) ) );
                      } );
}


void wxHyperlinkCtrl::SetURL( const wxString& aUrl )
{
    m_url = aUrl;
    update();
}


void wxHyperlinkCtrl::SetLabel( const wxString& aLabel )
{
    m_label = aLabel;
    update();
}


void wxHyperlinkCtrl::update()
{
    setText( QStringLiteral( "<a href=\"%1\">%2</a>" )
                     .arg( ToQString( m_url ).toHtmlEscaped(),
                           ToQString( m_label ).toHtmlEscaped() ) );
    setToolTip( ToQString( m_url ) );
}


// The pickers ------------------------------------------------------------------------------

wxFileDirPickerCtrlBase::wxFileDirPickerCtrlBase( wxWidgetArgCompat aParent, int aId,
                                                  const wxString& aPath, const wxString& aMessage,
                                                  long aStyle, wxEventType aEventType ) :
        wxWindowCompat<QWidget>( aParent.get() ),
        m_message( aMessage ),
        m_pickerStyle( aStyle ),
        m_eventType( aEventType )
{
    SetId( aId );

    m_text = new wxTextCtrl( this, wxID_ANY, aPath );
    m_button = new QToolButton( this );
    m_button->setText( QStringLiteral( "…" ) );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_text, 1 );
    layout->addWidget( m_button, 0 );

    setFocusProxy( m_text );

    QObject::connect( m_button, &QToolButton::clicked, this,
                      [this]()
                      {
                          wxString path = browse();

                          if( !path.IsEmpty() )
                          {
                              m_text->ChangeValue( path );
                              sendChanged();
                          }
                      } );

    QObject::connect( m_text, &QLineEdit::editingFinished, this, [this]() { sendChanged(); } );
}


wxString wxFileDirPickerCtrlBase::GetPath() const
{
    return m_text->GetValue();
}


void wxFileDirPickerCtrlBase::SetPath( const wxString& aPath )
{
    m_text->ChangeValue( aPath );
}


void wxFileDirPickerCtrlBase::sendChanged()
{
    wxFileDirPickerEvent event( m_eventType, GetId(), GetPath() );
    event.SetEventObject( this );
    ProcessEvent( event );
}


wxFilePickerCtrl::wxFilePickerCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aPath,
                                    const wxString& aMessage, const wxString& aWildcard,
                                    const wxPoint& aPos, const wxSize& aSize, long aStyle ) :
        wxFileDirPickerCtrlBase( aParent, aId, aPath, aMessage, aStyle, wxEVT_FILEPICKER_CHANGED ),
        m_wildcard( aWildcard )
{
}


wxString wxFilePickerCtrl::browse()
{
    wxFileName current( GetPath() );
    wxString   dir = current.GetPath().IsEmpty() ? m_initialDir : current.GetPath();
    long       style = ( m_pickerStyle & wxFLP_SAVE ) ? wxFD_SAVE : wxFD_OPEN;

    if( m_pickerStyle & wxFLP_OVERWRITE_PROMPT )
        style |= wxFD_OVERWRITE_PROMPT;

    if( m_pickerStyle & wxFLP_FILE_MUST_EXIST )
        style |= wxFD_FILE_MUST_EXIST;

    wxFileDialog dialog( this, m_message, dir, current.GetFullName(), m_wildcard, style );
    return dialog.ShowModal() == wxID_OK ? dialog.GetPath() : wxString();
}


wxDirPickerCtrl::wxDirPickerCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aPath,
                                  const wxString& aMessage, const wxPoint& aPos,
                                  const wxSize& aSize, long aStyle ) :
        wxFileDirPickerCtrlBase( aParent, aId, aPath, aMessage, aStyle, wxEVT_DIRPICKER_CHANGED )
{
}


wxString wxDirPickerCtrl::browse()
{
    wxString    dir = GetPath().IsEmpty() ? m_initialDir : GetPath();
    wxDirDialog dialog( this, m_message, dir );
    return dialog.ShowModal() == wxID_OK ? dialog.GetPath() : wxString();
}
