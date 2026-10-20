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
 * @file kiwx_pickers.h
 * The link and picker controls of the wxWidgets compatibility layer of the Qt build (wxcompat):
 * wxHyperlinkCtrl on a QLabel, wxFilePickerCtrl and wxDirPickerCtrl as a text control with a
 * browse button, wxCollapsiblePane as a button and a pane.
 */

#ifndef KIWX_PICKERS_H
#define KIWX_PICKERS_H

#include <kiqt.h>

#include <wx/filename.h>
#include <wx/kiwx_gui.h>
#include <wx/kiwx_filedlg.h>

class QToolButton;


#define wxHL_CONTEXTMENU 0x0001
#define wxHL_ALIGN_LEFT 0x0002
#define wxHL_ALIGN_RIGHT 0x0004
#define wxHL_ALIGN_CENTRE 0x0008
#define wxHL_DEFAULT_STYLE ( wxHL_CONTEXTMENU | wxNO_BORDER | wxHL_ALIGN_CENTRE )

#define wxCP_NO_TLW_RESIZE 0x0002
#define wxCP_DEFAULT_STYLE ( wxTAB_TRAVERSAL | wxNO_BORDER )

#define wxPB_USE_TEXTCTRL 0x0002
#define wxPB_SMALL 0x8000
#define wxFLP_OPEN 0x0400
#define wxFLP_SAVE 0x0800
#define wxFLP_OVERWRITE_PROMPT 0x1000
#define wxFLP_FILE_MUST_EXIST 0x2000
#define wxFLP_CHANGE_DIR 0x4000
#define wxFLP_SMALL wxPB_SMALL
#define wxFLP_USE_TEXTCTRL wxPB_USE_TEXTCTRL
#define wxFLP_DEFAULT_STYLE ( wxFLP_OPEN | wxFLP_FILE_MUST_EXIST )
#define wxDIRP_DIR_MUST_EXIST 0x0008
#define wxDIRP_CHANGE_DIR 0x0010
#define wxDIRP_SMALL wxPB_SMALL
#define wxDIRP_USE_TEXTCTRL wxPB_USE_TEXTCTRL
#define wxDIRP_DEFAULT_STYLE wxDIRP_DIR_MUST_EXIST


/// A link (wxHyperlinkCtrl): its event, if no handler processes it, opens the URL.
class KIQT_API wxHyperlinkCtrl : public wxWindowCompat<QLabel>
{
public:
    explicit wxHyperlinkCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                              const wxString& aLabel = wxEmptyString,
                              const wxString& aUrl = wxEmptyString,
                              const wxPoint& aPos = wxDefaultPosition,
                              const wxSize& aSize = wxDefaultSize, long aStyle = wxHL_DEFAULT_STYLE );

    wxString GetURL() const { return m_url; }
    void SetURL( const wxString& aUrl );
    void SetLabel( const wxString& aLabel ) override;
    wxString GetLabel() const override { return m_label; }

    bool GetVisited() const { return m_visited; }
    void SetVisited( bool aVisited = true ) { m_visited = aVisited; }
    void SetNormalColour( const wxColour& aColour ) {}
    void SetHoverColour( const wxColour& aColour ) {}
    void SetVisitedColour( const wxColour& aColour ) {}

private:
    void update();

    wxString m_label;
    wxString m_url;
    bool     m_visited;
};


/// The event of a change of the path of a picker (wxFileDirPickerEvent).
class wxFileDirPickerEvent : public wxCommandEvent
{
public:
    explicit wxFileDirPickerEvent( wxEventType aType = wxEVT_NULL, int aId = 0,
                                   const wxString& aPath = wxEmptyString ) :
            wxCommandEvent( aType, aId ),
            m_path( aPath )
    {
    }

    wxString GetPath() const { return m_path; }
    void SetPath( const wxString& aPath ) { m_path = aPath; }

private:
    wxString m_path;
};

#define wxFileDirPickerEventHandler( func ) ( &func )


/// The base of the pickers: a text control with a browse button.
class KIQT_API wxFileDirPickerCtrlBase : public wxWindowCompat<QWidget>
{
public:
    wxString GetPath() const;
    void SetPath( const wxString& aPath );

    wxTextCtrl* GetTextCtrl() const { return m_text; }
    wxString GetTextCtrlValue() const { return GetPath(); }
    bool HasTextCtrl() const { return true; }
    void SetInitialDirectory( const wxString& aDir ) { m_initialDir = aDir; }

    /// The button of the picker (wxPickerBase::GetPickerCtrl()).
    QToolButton* GetPickerCtrl() const { return m_button; }

protected:
    wxFileDirPickerCtrlBase( wxWidgetArgCompat aParent, int aId, const wxString& aPath,
                             const wxString& aMessage, long aStyle, wxEventType aEventType );

    /// Choose a path with the dialog of the picker; an empty string if canceled.
    virtual wxString browse() = 0;

    void sendChanged();

    wxTextCtrl*  m_text;
    QToolButton* m_button;
    wxString     m_message;
    wxString     m_initialDir;
    long         m_pickerStyle;
    wxEventType  m_eventType;
};


class KIQT_API wxFilePickerCtrl : public wxFileDirPickerCtrlBase
{
public:
    explicit wxFilePickerCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                               const wxString& aPath = wxEmptyString,
                               const wxString& aMessage = wxFileSelectorPromptStr,
                               const wxString& aWildcard = wxFileSelectorDefaultWildcardStr,
                               const wxPoint& aPos = wxDefaultPosition,
                               const wxSize& aSize = wxDefaultSize,
                               long aStyle = wxFLP_DEFAULT_STYLE );

    wxFileName GetFileName() const { return wxFileName( GetPath() ); }
    void SetFileName( const wxFileName& aFileName ) { SetPath( aFileName.GetFullPath() ); }
    void SetWildcard( const wxString& aWildcard ) { m_wildcard = aWildcard; }

protected:
    wxString browse() override;

private:
    wxString m_wildcard;
};


class KIQT_API wxDirPickerCtrl : public wxFileDirPickerCtrlBase
{
public:
    explicit wxDirPickerCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                              const wxString& aPath = wxEmptyString,
                              const wxString& aMessage = wxDirSelectorPromptStr,
                              const wxPoint& aPos = wxDefaultPosition,
                              const wxSize& aSize = wxDefaultSize,
                              long aStyle = wxDIRP_DEFAULT_STYLE );

    wxFileName GetDirName() const { return wxFileName::DirName( GetPath() ); }
    void SetDirName( const wxFileName& aDirName ) { SetPath( aDirName.GetPath() ); }

protected:
    wxString browse() override;
};

/// The event of a wxCollapsiblePane that the user collapses or expands.
class wxCollapsiblePaneEvent : public wxCommandEvent
{
public:
    wxCollapsiblePaneEvent( wxObject* aGenerator = nullptr, int aId = 0, bool aCollapsed = true ) :
            wxCommandEvent( wxEVT_COLLAPSIBLEPANE_CHANGED, aId ),
            m_collapsed( aCollapsed )
    {
        SetEventObject( aGenerator );
    }

    bool GetCollapsed() const { return m_collapsed; }
    void SetCollapsed( bool aCollapsed ) { m_collapsed = aCollapsed; }

private:
    bool m_collapsed;
};


/**
 * A pane that the user shows and hides (wxCollapsiblePane): a button with an arrow and the
 * label, and the pane (a wxPanel) under it, collapsed at first.
 */
class KIQT_API wxCollapsiblePane : public wxWindowCompat<QWidget>
{
public:
    explicit wxCollapsiblePane( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                                const wxString& aLabel = wxEmptyString,
                                const wxPoint& aPos = wxDefaultPosition,
                                const wxSize& aSize = wxDefaultSize,
                                long aStyle = wxCP_DEFAULT_STYLE );

    /// The window of the content of the pane.
    wxWindow* GetPane() const;

    void Collapse( bool aCollapse = true );
    void Expand() { Collapse( false ); }
    bool IsCollapsed() const;
    bool IsExpanded() const { return !IsCollapsed(); }

    void SetLabel( const wxString& aLabel ) override;
    wxString GetLabel() const override;

private:
    QToolButton* m_button;
    wxPanel*     m_pane;
};

#endif // KIWX_PICKERS_H
