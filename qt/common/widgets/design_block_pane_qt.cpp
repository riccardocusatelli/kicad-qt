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
 * @file design_block_pane_qt.cpp
 * DESIGN_BLOCK_PANE with Qt (common/widgets/design_block_pane_wx.cpp with wxWidgets): the
 * language changes and the dialogs of the pane.  What the pane does is in
 * common/widgets/design_block_pane.cpp.
 */

#include <widgets/design_block_pane.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include <design_block.h>
#include <dialogs/dialog_design_block_properties.h>
#include <widgets/panel_design_block_chooser.h>
#include <wildcards_and_files_ext.h>

#include <qt_strings.h>
#include <qt_ui.h>


DESIGN_BLOCK_PANE::DESIGN_BLOCK_PANE( EDA_DRAW_FRAME* aParent, const LIB_ID* aPreselect,
                                      std::vector<LIB_ID>& aHistoryList ) :
        WX_PANEL( aParent ),
        m_frame( aParent )
{
    // The settings of the pane are saved with the ones of the frame (its dock is part of the
    // layout of the frame)
    m_frame->AddLanguageChangedListener( this, [this]() { OnLanguageChanged(); } );
}


DESIGN_BLOCK_PANE::~DESIGN_BLOCK_PANE()
{
}


void DESIGN_BLOCK_PANE::OnLanguageChanged()
{
    if( m_chooserPanel )
        m_chooserPanel->ShowChangedLanguage();

    setLabelsAndTooltips();
}


bool DESIGN_BLOCK_PANE::chooseNewLibraryPath( const wxString& aDialogTitle, wxFileName& aFileName,
                                              bool& aIsGlobal )
{
    QString path = QFileDialog::getSaveFileName( m_frame, ToQString( aDialogTitle ),
                                                 ToQString( m_frame->Prj().GetProjectPath() ),
                                                 KIQT::FileDialogFilter( FILEEXT::KiCadDesignBlockLibPathWildcard() ) );

    if( path.isEmpty() )
        return false;

    aFileName = wxFileName( ToWxString( path ) );

    if( aFileName.GetExt() != FILEEXT::KiCadDesignBlockLibPathExtension )
        aFileName.SetExt( FILEEXT::KiCadDesignBlockLibPathExtension );

    // The table of the library (FILEDLG_HOOK_NEW_LIBRARY of the wxWidgets build)
    QMessageBox box( QMessageBox::Question, ToQString( aDialogTitle ),
                     ToQString( _( "Add new library to:" ) ), QMessageBox::Cancel, m_frame );

    QPushButton* global = box.addButton( ToQString( _( "Global Library Table" ) ), QMessageBox::AcceptRole );
    QPushButton* project = box.addButton( ToQString( _( "Project Library Table" ) ), QMessageBox::AcceptRole );

    box.setDefaultButton( aIsGlobal ? global : project );
    box.exec();

    if( box.clickedButton() == global )
        aIsGlobal = true;
    else if( box.clickedButton() == project )
        aIsGlobal = false;
    else
        return false;

    return true;
}


bool DESIGN_BLOCK_PANE::confirmOverwriteLibrary( const wxString& aMessage )
{
    QMessageBox box( QMessageBox::Warning, ToQString( _( "Confirmation" ) ), ToQString( aMessage ),
                     QMessageBox::Cancel, m_frame );

    QPushButton* overwrite = box.addButton( ToQString( _( "Overwrite" ) ), QMessageBox::AcceptRole );
    box.exec();

    return box.clickedButton() == overwrite;
}


wxString DESIGN_BLOCK_PANE::askLibraryDescription( const wxString& aDialogTitle )
{
    return ToWxString( QInputDialog::getText( m_frame, ToQString( aDialogTitle ),
                                              ToQString( _( "Enter a description for the library:" ) ) ) );
}


bool DESIGN_BLOCK_PANE::editDesignBlockProperties( DESIGN_BLOCK* aDesignBlock )
{
    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( m_frame, aDesignBlock );

    return dlg.ShowModal() == wxID_OK;
}
