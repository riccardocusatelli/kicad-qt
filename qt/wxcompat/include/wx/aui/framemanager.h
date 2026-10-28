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
 * @file framemanager.h
 * The panes of a frame of the wxWidgets compatibility layer (wxAuiManager, wxAuiPaneInfo) on a
 * QMainWindow: the toolbars go to its toolbar areas, the center panes (with the panes above and
 * below them without a caption: the info bar, the message panel) to its central widget, the
 * other panes to its dock widgets.
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_window.h>

class QDockWidget;
class QMainWindow;
class QWidget;


/// The property of the widgets wxAuiManager puts the panes in (their docks, the central area):
/// for the code of wxWidgets, the parent of a pane is still the window it was made in.
inline constexpr char wxAUI_CONTAINER_PROPERTY_COMPAT[] = "kiwxAuiContainer";


enum wxAuiManagerDock
{
    wxAUI_DOCK_NONE = 0,
    wxAUI_DOCK_TOP = 1,
    wxAUI_DOCK_RIGHT = 2,
    wxAUI_DOCK_BOTTOM = 3,
    wxAUI_DOCK_LEFT = 4,
    wxAUI_DOCK_CENTER = 5,
    wxAUI_DOCK_CENTRE = wxAUI_DOCK_CENTER
};


enum wxAuiManagerOption
{
    wxAUI_MGR_ALLOW_FLOATING = 1 << 0,
    wxAUI_MGR_ALLOW_ACTIVE_PANE = 1 << 1,
    wxAUI_MGR_TRANSPARENT_DRAG = 1 << 2,
    wxAUI_MGR_TRANSPARENT_HINT = 1 << 3,
    wxAUI_MGR_VENETIAN_BLINDS_HINT = 1 << 4,
    wxAUI_MGR_RECTANGLE_HINT = 1 << 5,
    wxAUI_MGR_HINT_FADE = 1 << 6,
    wxAUI_MGR_NO_VENETIAN_BLINDS_FADE = 1 << 7,
    wxAUI_MGR_LIVE_RESIZE = 1 << 8,
    wxAUI_MGR_DEFAULT = wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_TRANSPARENT_HINT
                        | wxAUI_MGR_HINT_FADE | wxAUI_MGR_NO_VENETIAN_BLINDS_FADE
};


/// A pane of a frame (its window, where it is docked, its decorations), as wxWidgets.
class KIQT_API wxAuiPaneInfo
{
public:
    enum wxAuiPaneState
    {
        optionFloating = 1 << 0,
        optionHidden = 1 << 1,
        optionLeftDockable = 1 << 2,
        optionRightDockable = 1 << 3,
        optionTopDockable = 1 << 4,
        optionBottomDockable = 1 << 5,
        optionFloatable = 1 << 6,
        optionMovable = 1 << 7,
        optionResizable = 1 << 8,
        optionPaneBorder = 1 << 9,
        optionCaption = 1 << 10,
        optionGripper = 1 << 11,
        optionDestroyOnClose = 1 << 12,
        optionToolbar = 1 << 13,
        optionActive = 1 << 14,
        optionGripperTop = 1 << 15,
        optionMaximized = 1 << 16,
        optionDockFixed = 1 << 17,

        buttonClose = 1 << 21,
        buttonMaximize = 1 << 22,
        buttonMinimize = 1 << 23,
        buttonPin = 1 << 24
    };

    wxAuiPaneInfo();
    virtual ~wxAuiPaneInfo() = default;

    bool IsOk() const { return window != nullptr; }
    bool IsShown() const { return !HasFlag( optionHidden ); }
    bool IsFloating() const { return HasFlag( optionFloating ); }
    bool IsDocked() const { return !IsFloating(); }
    bool IsToolbar() const { return HasFlag( optionToolbar ); }
    bool IsResizable() const { return HasFlag( optionResizable ); }
    bool IsFixed() const { return !IsResizable(); }
    bool IsMovable() const { return HasFlag( optionMovable ); }
    bool IsFloatable() const { return HasFlag( optionFloatable ); }
    bool HasCaption() const { return HasFlag( optionCaption ); }
    bool HasCloseButton() const { return HasFlag( buttonClose ); }
    bool HasBorder() const { return HasFlag( optionPaneBorder ); }
    bool HasGripper() const { return HasFlag( optionGripper ); }

    bool HasFlag( int aFlag ) const { return ( state & aFlag ) != 0; }

    wxAuiPaneInfo& SetFlag( int aFlag, bool aOn )
    {
        if( aOn )
            state |= aFlag;
        else
            state &= ~aFlag;

        return *this;
    }

    wxAuiPaneInfo& Window( wxWidgetArgCompat aWindow );
    wxAuiPaneInfo& Name( const wxString& aName ) { name = aName; return *this; }
    wxAuiPaneInfo& Caption( const wxString& aCaption ) { caption = aCaption; return *this; }

    wxAuiPaneInfo& Left() { dock_direction = wxAUI_DOCK_LEFT; return *this; }
    wxAuiPaneInfo& Right() { dock_direction = wxAUI_DOCK_RIGHT; return *this; }
    wxAuiPaneInfo& Top() { dock_direction = wxAUI_DOCK_TOP; return *this; }
    wxAuiPaneInfo& Bottom() { dock_direction = wxAUI_DOCK_BOTTOM; return *this; }
    wxAuiPaneInfo& Center() { dock_direction = wxAUI_DOCK_CENTER; return *this; }
    wxAuiPaneInfo& Centre() { return Center(); }
    wxAuiPaneInfo& Direction( int aDirection ) { dock_direction = aDirection; return *this; }
    wxAuiPaneInfo& Layer( int aLayer ) { dock_layer = aLayer; return *this; }
    wxAuiPaneInfo& Row( int aRow ) { dock_row = aRow; return *this; }
    wxAuiPaneInfo& Position( int aPosition ) { dock_pos = aPosition; return *this; }

    wxAuiPaneInfo& CenterPane();
    wxAuiPaneInfo& CentrePane() { return CenterPane(); }
    wxAuiPaneInfo& ToolbarPane();

    wxAuiPaneInfo& BestSize( const wxSize& aSize ) { best_size = aSize; return *this; }
    wxAuiPaneInfo& BestSize( int aWidth, int aHeight ) { return BestSize( wxSize( aWidth, aHeight ) ); }
    wxAuiPaneInfo& MinSize( const wxSize& aSize ) { min_size = aSize; return *this; }
    wxAuiPaneInfo& MinSize( int aWidth, int aHeight ) { return MinSize( wxSize( aWidth, aHeight ) ); }
    wxAuiPaneInfo& MaxSize( const wxSize& aSize ) { max_size = aSize; return *this; }
    wxAuiPaneInfo& MaxSize( int aWidth, int aHeight ) { return MaxSize( wxSize( aWidth, aHeight ) ); }
    wxAuiPaneInfo& FloatingSize( const wxSize& aSize ) { floating_size = aSize; return *this; }
    wxAuiPaneInfo& FloatingSize( int aWidth, int aHeight ) { return FloatingSize( wxSize( aWidth, aHeight ) ); }
    wxAuiPaneInfo& FloatingPosition( const wxPoint& aPos ) { floating_pos = aPos; return *this; }
    wxAuiPaneInfo& FloatingPosition( int aX, int aY ) { return FloatingPosition( wxPoint( aX, aY ) ); }

    wxAuiPaneInfo& Show( bool aShow = true ) { return SetFlag( optionHidden, !aShow ); }
    wxAuiPaneInfo& Hide() { return Show( false ); }
    wxAuiPaneInfo& Float() { return SetFlag( optionFloating, true ); }
    wxAuiPaneInfo& Dock() { return SetFlag( optionFloating, false ); }
    wxAuiPaneInfo& Fixed() { return SetFlag( optionResizable, false ); }
    wxAuiPaneInfo& Resizable( bool aOn = true ) { return SetFlag( optionResizable, aOn ); }
    wxAuiPaneInfo& CaptionVisible( bool aOn = true ) { return SetFlag( optionCaption, aOn ); }
    wxAuiPaneInfo& CloseButton( bool aOn = true ) { return SetFlag( buttonClose, aOn ); }
    wxAuiPaneInfo& MaximizeButton( bool aOn = true ) { return SetFlag( buttonMaximize, aOn ); }
    wxAuiPaneInfo& MinimizeButton( bool aOn = true ) { return SetFlag( buttonMinimize, aOn ); }
    wxAuiPaneInfo& PinButton( bool aOn = true ) { return SetFlag( buttonPin, aOn ); }
    wxAuiPaneInfo& PaneBorder( bool aOn = true ) { return SetFlag( optionPaneBorder, aOn ); }
    wxAuiPaneInfo& Gripper( bool aOn = true ) { return SetFlag( optionGripper, aOn ); }
    wxAuiPaneInfo& GripperTop( bool aOn = true ) { return SetFlag( optionGripperTop, aOn ); }
    wxAuiPaneInfo& Floatable( bool aOn = true ) { return SetFlag( optionFloatable, aOn ); }
    wxAuiPaneInfo& Movable( bool aOn = true ) { return SetFlag( optionMovable, aOn ); }
    wxAuiPaneInfo& DockFixed( bool aOn = true ) { return SetFlag( optionDockFixed, aOn ); }
    wxAuiPaneInfo& DestroyOnClose( bool aOn = true ) { return SetFlag( optionDestroyOnClose, aOn ); }
    wxAuiPaneInfo& LeftDockable( bool aOn = true ) { return SetFlag( optionLeftDockable, aOn ); }
    wxAuiPaneInfo& RightDockable( bool aOn = true ) { return SetFlag( optionRightDockable, aOn ); }
    wxAuiPaneInfo& TopDockable( bool aOn = true ) { return SetFlag( optionTopDockable, aOn ); }
    wxAuiPaneInfo& BottomDockable( bool aOn = true ) { return SetFlag( optionBottomDockable, aOn ); }

    wxAuiPaneInfo& Dockable( bool aOn = true )
    {
        return LeftDockable( aOn ).RightDockable( aOn ).TopDockable( aOn ).BottomDockable( aOn );
    }

    wxString  name;
    wxString  caption;
    wxWindow* window = nullptr;
    QWidget*  widget = nullptr;     ///< The widget of the pane (the window may not be a wxWindow)
    int       dock_direction = wxAUI_DOCK_LEFT;
    int       dock_layer = 0;
    int       dock_row = 0;
    int       dock_pos = 0;
    wxSize    best_size = wxDefaultSize;
    wxSize    min_size = wxDefaultSize;
    wxSize    max_size = wxDefaultSize;
    wxPoint   floating_pos = wxDefaultPosition;
    wxSize    floating_size = wxDefaultSize;
    unsigned  state = 0;
};


/**
 * The panes of a frame (a QMainWindow), as wxWidgets: the panes are added, changed through
 * GetPane(), and take effect with Update().
 */
class KIQT_API wxAuiManager
{
public:
    explicit wxAuiManager( wxWidgetArgCompat aManagedWindow = nullptr,
                           unsigned int aFlags = wxAUI_MGR_DEFAULT );
    virtual ~wxAuiManager();

    wxAuiManager( const wxAuiManager& ) = delete;
    wxAuiManager& operator=( const wxAuiManager& ) = delete;

    void SetManagedWindow( wxWidgetArgCompat aManagedWindow );
    QWidget* GetManagedWindow() const;
    void UnInit();

    void SetFlags( unsigned int aFlags ) { m_flags = aFlags; }
    unsigned int GetFlags() const { return m_flags; }

    bool AddPane( wxWidgetArgCompat aWindow, const wxAuiPaneInfo& aInfo );
    bool AddPane( wxWidgetArgCompat aWindow, int aDirection = wxLEFT,
                  const wxString& aCaption = wxEmptyString );
    bool InsertPane( wxWidgetArgCompat aWindow, const wxAuiPaneInfo& aInfo, int aLevel = 0 );
    bool DetachPane( wxWidgetArgCompat aWindow );

    /// @return the pane @a aName (or of @a aWindow), or a pane that is not IsOk().
    wxAuiPaneInfo& GetPane( const wxString& aName );
    wxAuiPaneInfo& GetPane( wxWidgetArgCompat aWindow );

    std::vector<wxAuiPaneInfo*> GetAllPanes();

    /// Apply the changes of the panes to the frame.
    void Update();

    /// The layouts of the panes are kept by Qt (QMainWindow::saveState()).
    wxString SavePerspective() { return wxEmptyString; }
    bool LoadPerspective( const wxString& aPerspective, bool aUpdate = true ) { return false; }

    void ShowHint( const wxRect& aRect ) {}
    void HideHint() {}

private:
    struct PANE
    {
        wxAuiPaneInfo info;
        QDockWidget*  dock = nullptr;     ///< The dock widget of the pane (once placed)
        bool          placed = false;
        bool          shownState = true;  ///< Shown by the last Update()
    };

    void placePane( PANE& aPane );
    void layoutCentralArea();

    QMainWindow*                       m_frame = nullptr;
    QWidget*                           m_central = nullptr;
    unsigned int                       m_flags;
    std::vector<std::unique_ptr<PANE>> m_panes;
    wxAuiPaneInfo                      m_invalidPane;
};
