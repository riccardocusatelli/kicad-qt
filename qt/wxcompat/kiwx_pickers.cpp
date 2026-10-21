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
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

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

wxCollapsiblePane::wxCollapsiblePane( wxWidgetArgCompat aParent, int aId, const wxString& aLabel,
                                      const wxPoint& aPos, const wxSize& aSize, long aStyle ) :
        wxWindowCompat<QWidget>( aParent )
{
    SetId( aId );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_button = new QToolButton( this );
    m_button->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    m_button->setAutoRaise( true );
    m_button->setCheckable( true );
    m_button->setArrowType( Qt::RightArrow );
    m_button->setText( ToQString( aLabel ) );
    layout->addWidget( m_button, 0, Qt::AlignLeft );

    m_pane = new wxPanel( this );
    m_pane->hide();
    layout->addWidget( m_pane, 1 );

    QObject::connect( m_button, &QToolButton::toggled, this,
                      [this]( bool aChecked )
                      {
                          Collapse( !aChecked );

                          wxCollapsiblePaneEvent event( this, GetId(), !aChecked );
                          ProcessEvent( event );
                      } );
}


wxWindow* wxCollapsiblePane::GetPane() const
{
    return m_pane;
}


void wxCollapsiblePane::Collapse( bool aCollapse )
{
    {
        QSignalBlocker blocker( m_button );
        m_button->setChecked( !aCollapse );
    }

    m_button->setArrowType( aCollapse ? Qt::RightArrow : Qt::DownArrow );
    m_pane->setVisible( !aCollapse );
    updateGeometry();
}


bool wxCollapsiblePane::IsCollapsed() const
{
    return !m_pane->isVisibleTo( const_cast<wxCollapsiblePane*>( this ) );
}


void wxCollapsiblePane::SetLabel( const wxString& aLabel )
{
    m_button->setText( ToQString( aLabel ) );
}


wxString wxCollapsiblePane::GetLabel() const
{
    return ToWxString( m_button->text() );
}


wxGenericDirCtrl::wxGenericDirCtrl( wxWidgetArgCompat aParent, int aId, const wxString& aDir,
                                    const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                    const wxString& aFilter, int aDefaultFilter,
                                    const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent ),
        m_filterChoice( nullptr )
{
    SetId( aId );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_model = new QFileSystemModel( this );
    m_model->setRootPath( QString() );
    m_model->setNameFilterDisables( false );
    m_model->setFilter( ( aStyle & wxDIRCTRL_DIR_ONLY )
                                ? QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives
                                : QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Drives );

    m_tree = new QTreeView( this );
    m_tree->setModel( m_model );
    m_tree->setHeaderHidden( true );

    // Only the names
    for( int col = 1; col < m_model->columnCount(); ++col )
        m_tree->hideColumn( col );

    layout->addWidget( m_tree, 1 );

    if( aStyle & wxDIRCTRL_SHOW_FILTERS )
    {
        m_filterChoice = new wxChoice( this );
        layout->addWidget( m_filterChoice );

        QObject::connect( m_filterChoice, &QComboBox::currentIndexChanged, this,
                          [this]( int aIndex )
                          {
                              if( aIndex >= 0 )
                                  applyFilter( aIndex );
                          } );
    }

    QObject::connect( m_tree->selectionModel(), &QItemSelectionModel::currentChanged, this,
                      [this]( const QModelIndex&, const QModelIndex& )
                      {
                          wxCommandEvent event( wxEVT_DIRCTRL_SELECTIONCHANGED, GetId() );
                          event.SetEventObject( this );
                          ProcessEvent( event );
                      } );

    QObject::connect( m_tree, &QTreeView::activated, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          if( m_model->isDir( aIndex ) )
                              return;

                          wxCommandEvent event( wxEVT_DIRCTRL_FILEACTIVATED, GetId() );
                          event.SetEventObject( this );
                          ProcessEvent( event );
                      } );

    if( !aFilter.IsEmpty() )
    {
        SetFilter( aFilter );
        SetFilterIndex( aDefaultFilter );
    }

    if( !aDir.IsEmpty() )
        SetPath( aDir );
}


wxString wxGenericDirCtrl::GetPath() const
{
    QModelIndex index = m_tree->currentIndex();

    if( !index.isValid() )
        return wxString();

    return ToWxString( QDir::toNativeSeparators( m_model->filePath( index ) ) );
}


void wxGenericDirCtrl::SetPath( const wxString& aPath )
{
    QModelIndex index = m_model->index( ToQString( aPath ) );

    if( !index.isValid() )
        return;

    // The parents are expanded, and a directory too (as wxWidgets)
    for( QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent() )
        m_tree->expand( parent );

    if( m_model->isDir( index ) )
        m_tree->expand( index );

    m_tree->setCurrentIndex( index );
    m_tree->scrollTo( index );
}


wxString wxGenericDirCtrl::GetFilePath() const
{
    QModelIndex index = m_tree->currentIndex();

    if( !index.isValid() || m_model->isDir( index ) )
        return wxString();

    return ToWxString( QDir::toNativeSeparators( m_model->filePath( index ) ) );
}


void wxGenericDirCtrl::UnselectAll()
{
    QSignalBlocker blocker( m_tree->selectionModel() );
    m_tree->selectionModel()->clear();
}


void wxGenericDirCtrl::ShowHidden( bool aShow )
{
    QDir::Filters filters = m_model->filter();
    m_model->setFilter( aShow ? ( filters | QDir::Hidden ) : ( filters & ~QDir::Hidden ) );
}


void wxGenericDirCtrl::SetFilter( const wxString& aFilter )
{
    m_filter = aFilter;
    m_patterns.clear();

    // "description|patterns|description|patterns...", or only the patterns
    QStringList parts = ToQString( aFilter ).split( '|' );
    QStringList descriptions;

    if( parts.size() == 1 )
    {
        descriptions.append( parts[0] );
        m_patterns.push_back( parts[0] );
    }
    else
    {
        for( int i = 0; i + 1 < parts.size(); i += 2 )
        {
            descriptions.append( parts[i] );
            m_patterns.push_back( parts[i + 1] );
        }
    }

    if( m_filterChoice )
    {
        QSignalBlocker blocker( m_filterChoice );
        m_filterChoice->clear();
        m_filterChoice->addItems( descriptions );
    }

    SetFilterIndex( 0 );
}


void wxGenericDirCtrl::SetFilterIndex( int aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<int>( m_patterns.size() ) )
        return;

    if( m_filterChoice )
        m_filterChoice->SetSelection( aIndex );

    applyFilter( aIndex );
}


int wxGenericDirCtrl::GetFilterIndex() const
{
    return m_filterIndex;
}


void wxGenericDirCtrl::ReCreateTree()
{
    applyFilter( m_filterIndex );
}


void wxGenericDirCtrl::applyFilter( int aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<int>( m_patterns.size() ) )
        return;

    m_filterIndex = aIndex;

    QStringList patterns;

    for( const QString& pattern : m_patterns[aIndex].split( ';', Qt::SkipEmptyParts ) )
    {
        QString trimmed = pattern.trimmed();

        // All the files: no name filter
        if( trimmed == QStringLiteral( "*" ) || trimmed == QStringLiteral( "*.*" ) )
        {
            patterns.clear();
            break;
        }

        patterns.append( trimmed );
    }

    m_model->setNameFilters( patterns );
}
