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
 * @file design_block_pane_wx.cpp
 * DESIGN_BLOCK_PANE with wxWidgets: the events of the pane and its dialogs (the Qt build has its
 * own version).  What the pane does is in design_block_pane.cpp.
 */

#include <design_block.h>
#include <kidialog.h>
#include <widgets/design_block_pane.h>
#include <dialog_design_block_properties.h>
#include <widgets/panel_design_block_chooser.h>
#include <widgets/filedlg_hook_new_library.h>
#include <wildcards_and_files_ext.h>
#include <wx/textdlg.h>


DESIGN_BLOCK_PANE::DESIGN_BLOCK_PANE( EDA_DRAW_FRAME* aParent, const LIB_ID* aPreselect,
                                      std::vector<LIB_ID>& aHistoryList ) :
        WX_PANEL( aParent ),
        m_frame( aParent )
{
    m_frame->Bind( wxEVT_AUI_PANE_CLOSE, &DESIGN_BLOCK_PANE::OnClosed, this );
    m_frame->Bind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
}


DESIGN_BLOCK_PANE::~DESIGN_BLOCK_PANE()
{
    m_frame->Unbind( wxEVT_AUI_PANE_CLOSE, &DESIGN_BLOCK_PANE::OnClosed, this );
    m_frame->Unbind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
}


void DESIGN_BLOCK_PANE::OnLanguageChanged( wxCommandEvent& aEvent )
{
    if( m_chooserPanel )
        m_chooserPanel->ShowChangedLanguage();

    setLabelsAndTooltips();

    aEvent.Skip();
}


void DESIGN_BLOCK_PANE::OnClosed( wxAuiManagerEvent& aEvent )
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        if( IsShownOnScreen() ) // Ensure the panel is shown when trying to save its size
            m_frame->SaveSettings( cfg );
    }

    aEvent.Skip();
}


bool DESIGN_BLOCK_PANE::chooseNewLibraryPath( const wxString& aDialogTitle, wxFileName& aFileName,
                                              bool& aIsGlobal )
{
    FILEDLG_HOOK_NEW_LIBRARY tableChooser( aIsGlobal );

    if( !m_frame->LibraryFileBrowser( aDialogTitle, false, aFileName, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                      FILEEXT::KiCadDesignBlockLibPathExtension, false, &tableChooser ) )
    {
        return false;
    }

    aIsGlobal = tableChooser.GetUseGlobalTable();
    return true;
}


bool DESIGN_BLOCK_PANE::confirmOverwriteLibrary( const wxString& aMessage )
{
    KIDIALOG dlg( m_frame, aMessage, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetOKLabel( _( "Overwrite" ) );
    dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

    return dlg.ShowModal() != wxID_CANCEL;
}


wxString DESIGN_BLOCK_PANE::askLibraryDescription( const wxString& aDialogTitle )
{
    return wxGetTextFromUser( _( "Enter a description for the library:" ), aDialogTitle, wxEmptyString,
                              m_frame );
}


bool DESIGN_BLOCK_PANE::editDesignBlockProperties( DESIGN_BLOCK* aDesignBlock )
{
    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( m_frame, aDesignBlock );

    return dlg.ShowModal() == wxID_OK;
}
