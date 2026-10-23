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
 * @file kiwx_filedlg.h
 * The file and directory dialogs of the wxWidgets compatibility layer of the Qt build (wxcompat):
 * wxFileDialog and wxDirDialog on QFileDialog.  The wildcards of wxWidgets
 * ("Gerber files (*.gbr)|*.gbr|All files|*") are the filters of Qt.
 */

#ifndef KIWX_FILEDLG_H
#define KIWX_FILEDLG_H

#include <kiqt.h>

#include <memory>
#include <vector>

#include <wx/arrstr.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_window.h>

enum
{
    wxFD_OPEN = 0x0001,
    wxFD_SAVE = 0x0002,
    wxFD_OVERWRITE_PROMPT = 0x0004,
    wxFD_NO_FOLLOW = 0x0008,
    wxFD_FILE_MUST_EXIST = 0x0010,
    wxFD_CHANGE_DIR = 0x0080,
    wxFD_PREVIEW = 0x0100,
    wxFD_MULTIPLE = 0x0200,
    wxFD_SHOW_HIDDEN = 0x0400
};

#define wxFD_DEFAULT_STYLE wxFD_OPEN

enum
{
    wxDD_CHANGE_DIR = 0x0100,
    wxDD_DIR_MUST_EXIST = 0x0200,
    wxDD_MULTIPLE = 0x0400,
    wxDD_SHOW_HIDDEN = 0x0001,
    wxDD_NEW_DIR_BUTTON = 0
};

#define wxDD_DEFAULT_STYLE 0

#define wxFileSelectorPromptStr wxString( "Select a file" )
#define wxFileSelectorDefaultWildcardStr wxString( "*" )
#define wxDirSelectorPromptStr wxString( "Select a directory" )


class QWidget;
class QVBoxLayout;


/// A control added to a file dialog (wxFileDialogCustomControl): a widget of Qt.
class KIQT_API wxFileDialogCustomControl : public wxEvtHandler
{
public:
    explicit wxFileDialogCustomControl( QWidget* aWidget ) : m_widget( aWidget ) {}

    void Show( bool aShow = true );
    void Hide() { Show( false ); }
    void Enable( bool aEnable = true );
    void Disable() { Enable( false ); }

protected:
    QWidget* m_widget;
};


class KIQT_API wxFileDialogButton : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;
};


class KIQT_API wxFileDialogCheckBox : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;

    bool GetValue() const;
    void SetValue( bool aValue );
};


class KIQT_API wxFileDialogRadioButton : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;

    bool GetValue() const;
    void SetValue( bool aValue );
};


class KIQT_API wxFileDialogChoice : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;

    int GetSelection() const;
    void SetSelection( int aIndex );
};


class KIQT_API wxFileDialogTextCtrl : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;

    wxString GetValue() const;
    void SetValue( const wxString& aValue );
};


class KIQT_API wxFileDialogStaticText : public wxFileDialogCustomControl
{
public:
    using wxFileDialogCustomControl::wxFileDialogCustomControl;

    void SetLabelText( const wxString& aText );
};


/// The controls added to a file dialog by its hook (wxFileDialogCustomize).
class KIQT_API wxFileDialogCustomize
{
public:
    explicit wxFileDialogCustomize( QWidget* aContainer );
    ~wxFileDialogCustomize();

    wxFileDialogCustomize( const wxFileDialogCustomize& ) = delete;
    wxFileDialogCustomize& operator=( const wxFileDialogCustomize& ) = delete;

    wxFileDialogButton* AddButton( const wxString& aLabel );
    wxFileDialogCheckBox* AddCheckBox( const wxString& aLabel );
    wxFileDialogRadioButton* AddRadioButton( const wxString& aLabel );
    wxFileDialogChoice* AddChoice( size_t aCount, const wxString* aStrings );
    wxFileDialogTextCtrl* AddTextCtrl( const wxString& aLabel = wxEmptyString );
    wxFileDialogStaticText* AddStaticText( const wxString& aLabel );

private:
    template <typename CONTROL>
    CONTROL* add( QWidget* aWidget );

    QWidget*                                                m_container;
    QVBoxLayout*                                            m_layout;
    std::vector<std::unique_ptr<wxFileDialogCustomControl>> m_controls;
};


/// The controls of KiCad in a file dialog (wxFileDialogCustomizeHook).
class KIQT_API wxFileDialogCustomizeHook
{
public:
    virtual ~wxFileDialogCustomizeHook() = default;

    virtual void AddCustomControls( wxFileDialogCustomize& aCustomizer ) = 0;
    virtual void UpdateCustomControls() {}
    virtual void TransferDataFromCustomControls() {}
};


/// The filters of a file dialog of Qt from a wildcard of wxWidgets.
KIQT_API wxString wxWildcardToQtFiltersCompat( const wxString& aWildcard );


/**
 * Split a wildcard of wxWidgets ("description|pattern|description|pattern...") in its
 * descriptions and its patterns.
 *
 * @return the number of the filters.
 */
KIQT_API int wxParseCommonDialogsFilter( const wxString& aFilterStr, wxArrayString& aDescriptions,
                                         wxArrayString& aFilters );


/// A dialog choosing files (wxFileDialog).
class KIQT_API wxFileDialog
{
public:
    wxFileDialog( wxWidgetArgCompat aParent, const wxString& aMessage = wxFileSelectorPromptStr,
                  const wxString& aDefaultDir = wxEmptyString,
                  const wxString& aDefaultFile = wxEmptyString,
                  const wxString& aWildcard = wxFileSelectorDefaultWildcardStr,
                  long aStyle = wxFD_DEFAULT_STYLE, const wxPoint& aPos = wxDefaultPosition,
                  const wxSize& aSize = wxDefaultSize, const wxString& aName = wxEmptyString );

    ~wxFileDialog();

    wxFileDialog( const wxFileDialog& ) = delete;
    wxFileDialog& operator=( const wxFileDialog& ) = delete;

    /// Show the dialog: wxID_OK or wxID_CANCEL.
    int ShowModal();

    wxString GetPath() const { return m_paths.empty() ? wxString() : m_paths.front(); }
    void GetPaths( wxArrayString& aPaths ) const { aPaths = m_paths; }
    wxString GetFilename() const;
    void GetFilenames( wxArrayString& aFiles ) const;
    wxString GetDirectory() const { return m_directory; }
    wxString GetWildcard() const { return m_wildcard; }
    wxString GetMessage() const { return m_message; }
    int GetFilterIndex() const { return m_filterIndex; }
    long GetWindowStyle() const { return m_style; }

    void SetPath( const wxString& aPath );
    void SetDirectory( const wxString& aDir ) { m_directory = aDir; }
    void SetFilename( const wxString& aName ) { m_filename = aName; }
    void SetWildcard( const wxString& aWildcard ) { m_wildcard = aWildcard; }
    void SetMessage( const wxString& aMessage ) { m_message = aMessage; }
    void SetFilterIndex( int aIndex ) { m_filterIndex = aIndex; }

    /// The controls of KiCad shown in the dialog (the dialog of Qt is then not the native one).
    bool SetCustomizeHook( wxFileDialogCustomizeHook& aHook )
    {
        m_hook = &aHook;
        return true;
    }

    /// A window of KiCad shown in the dialog (made by @a aCreator when the dialog is shown); it
    /// lives until the wxFileDialog is deleted, to be read after the dialog.
    typedef wxWindow* ( *ExtraControlCreatorFunction )( wxWindow* );

    bool SetExtraControlCreator( ExtraControlCreatorFunction aCreator )
    {
        m_extraCreator = aCreator;
        return true;
    }

    wxWindow* GetExtraControl() const { return m_extraControl; }

private:
    void deleteExtraControl();

    wxFileDialogCustomizeHook*  m_hook = nullptr;
    ExtraControlCreatorFunction m_extraCreator = nullptr;
    wxWindow*                   m_extraControl = nullptr;
    QWidget*      m_parent;
    wxString      m_message;
    wxString      m_directory;
    wxString      m_filename;
    wxString      m_wildcard;
    long          m_style;
    int           m_filterIndex;
    wxArrayString m_paths;
};


/// A dialog choosing a directory (wxDirDialog).
class KIQT_API wxDirDialog
{
public:
    wxDirDialog( wxWidgetArgCompat aParent, const wxString& aMessage = wxDirSelectorPromptStr,
                 const wxString& aDefaultPath = wxEmptyString, long aStyle = wxDD_DEFAULT_STYLE,
                 const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                 const wxString& aName = wxEmptyString );

    /// Show the dialog: wxID_OK or wxID_CANCEL.
    int ShowModal();

    wxString GetPath() const { return m_path; }
    void GetPaths( wxArrayString& aPaths ) const { aPaths = wxArrayString( 1, &m_path ); }
    wxString GetMessage() const { return m_message; }

    void SetPath( const wxString& aPath ) { m_path = aPath; }
    void SetMessage( const wxString& aMessage ) { m_message = aMessage; }

private:
    QWidget* m_parent;
    wxString m_message;
    wxString m_path;
    long     m_style;
};


/// Choose a file (wxFileSelector()): its path, or an empty string.
KIQT_API wxString wxFileSelector( const wxString& aMessage = wxFileSelectorPromptStr,
                                  const wxString& aDefaultPath = wxEmptyString,
                                  const wxString& aDefaultFilename = wxEmptyString,
                                  const wxString& aDefaultExtension = wxEmptyString,
                                  const wxString& aWildcard = wxFileSelectorDefaultWildcardStr,
                                  int aFlags = 0, wxWidgetArgCompat aParent = nullptr,
                                  int aX = -1, int aY = -1 );

/// Choose a directory (wxDirSelector()): its path, or an empty string.
KIQT_API wxString wxDirSelector( const wxString& aMessage = wxDirSelectorPromptStr,
                                 const wxString& aDefaultPath = wxEmptyString,
                                 long aStyle = wxDD_DEFAULT_STYLE,
                                 const wxPoint& aPos = wxDefaultPosition,
                                 wxWidgetArgCompat aParent = nullptr );

#endif // KIWX_FILEDLG_H
