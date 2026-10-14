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

#include <wx/kiwx_filedlg.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <qt_strings.h>


wxString wxWildcardToQtFiltersCompat( const wxString& aWildcard )
{
    QStringList parts = ToQString( aWildcard ).split( '|' );
    QStringList filters;

    // A single pattern, or pairs of a description and its patterns
    if( parts.size() == 1 )
    {
        QString patterns = parts.front().replace( ';', ' ' );
        return ToWxString( patterns + QStringLiteral( " (" ) + patterns + QStringLiteral( ")" ) );
    }

    for( qsizetype i = 0; i + 1 < parts.size(); i += 2 )
    {
        QString patterns = parts[i + 1];
        patterns.replace( ';', ' ' );
        filters.push_back( parts[i].trimmed() + QStringLiteral( " (" ) + patterns
                           + QStringLiteral( ")" ) );
    }

    return ToWxString( filters.join( QStringLiteral( ";;" ) ) );
}


// The controls added to the file dialogs -------------------------------------------------

void wxFileDialogCustomControl::Show( bool aShow )
{
    m_widget->setVisible( aShow );
}


void wxFileDialogCustomControl::Enable( bool aEnable )
{
    m_widget->setEnabled( aEnable );
}


bool wxFileDialogCheckBox::GetValue() const
{
    return static_cast<QCheckBox*>( m_widget )->isChecked();
}


void wxFileDialogCheckBox::SetValue( bool aValue )
{
    static_cast<QCheckBox*>( m_widget )->setChecked( aValue );
}


bool wxFileDialogRadioButton::GetValue() const
{
    return static_cast<QRadioButton*>( m_widget )->isChecked();
}


void wxFileDialogRadioButton::SetValue( bool aValue )
{
    static_cast<QRadioButton*>( m_widget )->setChecked( aValue );
}


int wxFileDialogChoice::GetSelection() const
{
    return static_cast<QComboBox*>( m_widget )->currentIndex();
}


void wxFileDialogChoice::SetSelection( int aIndex )
{
    static_cast<QComboBox*>( m_widget )->setCurrentIndex( aIndex );
}


wxString wxFileDialogTextCtrl::GetValue() const
{
    return ToWxString( static_cast<QLineEdit*>( m_widget )->text() );
}


void wxFileDialogTextCtrl::SetValue( const wxString& aValue )
{
    static_cast<QLineEdit*>( m_widget )->setText( ToQString( aValue ) );
}


void wxFileDialogStaticText::SetLabelText( const wxString& aText )
{
    static_cast<QLabel*>( m_widget )->setText( ToQString( aText ) );
}


wxFileDialogCustomize::wxFileDialogCustomize( QWidget* aContainer ) :
        m_container( aContainer ),
        m_layout( new QVBoxLayout( aContainer ) )
{
    m_layout->setContentsMargins( 0, 0, 0, 0 );
}


wxFileDialogCustomize::~wxFileDialogCustomize() = default;


template <typename CONTROL>
CONTROL* wxFileDialogCustomize::add( QWidget* aWidget )
{
    m_layout->addWidget( aWidget );

    CONTROL* control = new CONTROL( aWidget );
    m_controls.emplace_back( control );
    return control;
}


wxFileDialogButton* wxFileDialogCustomize::AddButton( const wxString& aLabel )
{
    QPushButton*        button = new QPushButton( ToQString( aLabel ), m_container );
    wxFileDialogButton* control = add<wxFileDialogButton>( button );

    QObject::connect( button, &QPushButton::clicked, button,
                      [control]()
                      {
                          wxCommandEvent event( wxEVT_BUTTON );
                          control->ProcessEvent( event );
                      } );

    return control;
}


wxFileDialogCheckBox* wxFileDialogCustomize::AddCheckBox( const wxString& aLabel )
{
    return add<wxFileDialogCheckBox>( new QCheckBox( ToQString( aLabel ), m_container ) );
}


wxFileDialogRadioButton* wxFileDialogCustomize::AddRadioButton( const wxString& aLabel )
{
    return add<wxFileDialogRadioButton>( new QRadioButton( ToQString( aLabel ), m_container ) );
}


wxFileDialogChoice* wxFileDialogCustomize::AddChoice( size_t aCount, const wxString* aStrings )
{
    QComboBox* combo = new QComboBox( m_container );

    for( size_t i = 0; i < aCount; ++i )
        combo->addItem( ToQString( aStrings[i] ) );

    return add<wxFileDialogChoice>( combo );
}


wxFileDialogTextCtrl* wxFileDialogCustomize::AddTextCtrl( const wxString& aLabel )
{
    QLineEdit* edit = new QLineEdit( m_container );
    edit->setPlaceholderText( ToQString( aLabel ) );
    return add<wxFileDialogTextCtrl>( edit );
}


wxFileDialogStaticText* wxFileDialogCustomize::AddStaticText( const wxString& aLabel )
{
    // The blank lines that make room in the native dialogs of wxWidgets are left out
    QString label = ToQString( aLabel ).trimmed();
    QLabel* text = new QLabel( label, m_container );
    text->setVisible( !label.isEmpty() );
    return add<wxFileDialogStaticText>( text );
}


// wxFileDialog -----------------------------------------------------------------------------

wxFileDialog::wxFileDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                            const wxString& aDefaultDir, const wxString& aDefaultFile,
                            const wxString& aWildcard, long aStyle, const wxPoint& aPos,
                            const wxSize& aSize, const wxString& aName ) :
        m_parent( aParent.get() ),
        m_message( aMessage ),
        m_directory( aDefaultDir ),
        m_filename( aDefaultFile ),
        m_wildcard( aWildcard ),
        m_style( aStyle ),
        m_filterIndex( 0 )
{
}


int wxFileDialog::ShowModal()
{
    QFileDialog dialog( m_parent, ToQString( m_message ), ToQString( m_directory ) );
    QStringList filters = ToQString( wxWildcardToQtFiltersCompat( m_wildcard ) )
                                  .split( QStringLiteral( ";;" ) );

    dialog.setNameFilters( filters );

    if( m_filterIndex >= 0 && m_filterIndex < filters.size() )
        dialog.selectNameFilter( filters[m_filterIndex] );

    if( m_style & wxFD_SAVE )
    {
        dialog.setAcceptMode( QFileDialog::AcceptSave );
        dialog.setFileMode( QFileDialog::AnyFile );

        if( !( m_style & wxFD_OVERWRITE_PROMPT ) )
            dialog.setOption( QFileDialog::DontConfirmOverwrite );
    }
    else
    {
        dialog.setAcceptMode( QFileDialog::AcceptOpen );
        dialog.setFileMode( ( m_style & wxFD_MULTIPLE ) ? QFileDialog::ExistingFiles
                                                       : QFileDialog::ExistingFile );
    }

    if( !m_filename.IsEmpty() )
        dialog.selectFile( ToQString( m_filename ) );

    // The controls of the hook, below the ones of the dialog (of Qt: the native dialogs have
    // no room for them)
    std::unique_ptr<wxFileDialogCustomize> customizer;

    if( m_hook )
    {
        dialog.setOption( QFileDialog::DontUseNativeDialog );

        QWidget* container = new QWidget( &dialog );
        customizer = std::make_unique<wxFileDialogCustomize>( container );
        m_hook->AddCustomControls( *customizer );

        if( QGridLayout* grid = qobject_cast<QGridLayout*>( dialog.layout() ) )
            grid->addWidget( container, grid->rowCount(), 0, 1, grid->columnCount() );
        else if( dialog.layout() )
            dialog.layout()->addWidget( container );
    }

    if( dialog.exec() != QDialog::Accepted )
        return wxID_CANCEL;

    if( m_hook )
        m_hook->TransferDataFromCustomControls();

    m_paths.clear();

    for( const QString& file : dialog.selectedFiles() )
        m_paths.push_back( ToWxString( file ) );

    m_filterIndex = std::max<int>( 0, filters.indexOf( dialog.selectedNameFilter() ) );

    if( !m_paths.empty() )
    {
        QFileInfo info( ToQString( m_paths.front() ) );
        m_directory = ToWxString( info.absolutePath() );
        m_filename = ToWxString( info.fileName() );
    }

    return wxID_OK;
}


wxString wxFileDialog::GetFilename() const
{
    return m_paths.empty() ? m_filename
                           : ToWxString( QFileInfo( ToQString( m_paths.front() ) ).fileName() );
}


void wxFileDialog::GetFilenames( wxArrayString& aFiles ) const
{
    aFiles.clear();

    for( const wxString& path : m_paths )
        aFiles.push_back( ToWxString( QFileInfo( ToQString( path ) ).fileName() ) );
}


void wxFileDialog::SetPath( const wxString& aPath )
{
    QFileInfo info( ToQString( aPath ) );
    m_directory = ToWxString( info.absolutePath() );
    m_filename = ToWxString( info.fileName() );
}


// wxDirDialog ------------------------------------------------------------------------------

wxDirDialog::wxDirDialog( wxWidgetArgCompat aParent, const wxString& aMessage,
                          const wxString& aDefaultPath, long aStyle, const wxPoint& aPos,
                          const wxSize& aSize, const wxString& aName ) :
        m_parent( aParent.get() ),
        m_message( aMessage ),
        m_path( aDefaultPath ),
        m_style( aStyle )
{
}


int wxDirDialog::ShowModal()
{
    QString dir = QFileDialog::getExistingDirectory( m_parent, ToQString( m_message ),
                                                     ToQString( m_path ) );

    if( dir.isEmpty() )
        return wxID_CANCEL;

    m_path = ToWxString( dir );
    return wxID_OK;
}


// The selectors ----------------------------------------------------------------------------

wxString wxFileSelector( const wxString& aMessage, const wxString& aDefaultPath,
                         const wxString& aDefaultFilename, const wxString& aDefaultExtension,
                         const wxString& aWildcard, int aFlags, wxWidgetArgCompat aParent, int aX,
                         int aY )
{
    wxFileDialog dialog( aParent, aMessage, aDefaultPath, aDefaultFilename, aWildcard,
                         aFlags ? aFlags : wxFD_DEFAULT_STYLE );

    return dialog.ShowModal() == wxID_OK ? dialog.GetPath() : wxString();
}


wxString wxDirSelector( const wxString& aMessage, const wxString& aDefaultPath, long aStyle,
                        const wxPoint& aPos, wxWidgetArgCompat aParent )
{
    wxDirDialog dialog( aParent, aMessage, aDefaultPath, aStyle );
    return dialog.ShowModal() == wxID_OK ? dialog.GetPath() : wxString();
}
