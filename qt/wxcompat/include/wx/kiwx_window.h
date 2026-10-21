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
 * @file kiwx_window.h
 * The windows of the wxWidgets compatibility layer of the Qt build (wxcompat): wxWindow, the API
 * of wxWidgets on a widget of Qt.
 *
 * The controls of the layer are widgets of Qt and windows (wxWindowCompat<QT>); a plain widget
 * of Qt gets a window of its own when the code asks for one (wxWindowFromQt()).  The functions
 * of the layer take their windows as a QWidget* or a wxWindow* (wxWidgetArgCompat) and give
 * wxWindow*, as wxWidgets; GetHandle() is the widget of a window.
 */

#ifndef KIWX_WINDOW_H
#define KIWX_WINDOW_H

#include <kiqt.h>

#include <memory>
#include <type_traits>
#include <vector>

#include <QLayout>
#include <QWidget>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_events.h>
#include <wx/kiwx_gdi.h>

class wxMenu;
class wxValidator;
class wxWindow;


/// The ids of the windows (wxID_ANY gets a new negative id, as with wxWidgets).
KIQT_API int wxWindowIDNew();

/// The id of a window (a new one if it has none).
KIQT_API int wxGetWindowIdCompat( const QObject* aWindow );

/// Give an id to a window (a new one for wxID_ANY).
KIQT_API void wxSetWindowIdCompat( QObject* aWindow, int aId );

/// The window of a widget: the window of a control of the layer, or one for a plain widget.
KIQT_API wxWindow* wxWindowFromQt( QWidget* aWidget );


/**
 * A window given as a QWidget* or as a wxWindow* (the parents of the controls, the windows of
 * the sizers...): it converts to the widget.
 */
class wxWidgetArgCompat
{
public:
    wxWidgetArgCompat( std::nullptr_t = nullptr ) {}

    template <typename T, typename = std::enable_if_t<std::is_base_of_v<QWidget, T>
                                                      || std::is_base_of_v<wxWindow, T>>>
    wxWidgetArgCompat( T* aWindow ) : m_widget( widgetOf( aWindow ) )
    {
    }

    operator QWidget*() const { return m_widget; }
    operator wxWindow*() const { return window(); }
    explicit operator bool() const { return m_widget != nullptr; }
    QWidget* operator->() const { return m_widget; }
    QWidget* get() const { return m_widget; }

    /// The window of the widget.
    wxWindow* window() const { return m_widget ? wxWindowFromQt( m_widget ) : nullptr; }

private:
    template <typename T>
    static QWidget* widgetOf( T* aWindow );

    QWidget* m_widget = nullptr;
};


/**
 * The children of a window (wxWindow::GetChildren()): a list of windows, with the nodes of
 * wxList (GetFirst(), GetNext(), GetData()) and the iterators of the standard library.
 */
class KIQT_API wxWindowList
{
public:
    class Node
    {
    public:
        wxWindow* GetData() const { return m_data; }
        Node* GetNext() const { return m_next; }

    private:
        friend class wxWindowList;

        wxWindow* m_data = nullptr;
        Node*     m_next = nullptr;
    };

    using compatibility_iterator = Node*;

    explicit wxWindowList( const QWidget* aParent );

    wxWindowList( const wxWindowList& aOther ) : wxWindowList( aOther.m_windows ) {}
    wxWindowList& operator=( const wxWindowList& ) = delete;

    Node* GetFirst() const { return m_nodes.empty() ? nullptr : const_cast<Node*>( &m_nodes.front() ); }
    Node* GetLast() const { return m_nodes.empty() ? nullptr : const_cast<Node*>( &m_nodes.back() ); }
    size_t GetCount() const { return m_windows.size(); }
    size_t size() const { return m_windows.size(); }
    bool IsEmpty() const { return m_windows.empty(); }
    bool empty() const { return m_windows.empty(); }
    wxWindow* front() const { return m_windows.front(); }
    wxWindow* back() const { return m_windows.back(); }
    wxWindow* operator[]( size_t aIndex ) const { return m_windows[aIndex]; }

    std::vector<wxWindow*>::const_iterator begin() const { return m_windows.begin(); }
    std::vector<wxWindow*>::const_iterator end() const { return m_windows.end(); }

private:
    explicit wxWindowList( const std::vector<wxWindow*>& aWindows );
    void makeNodes();

    std::vector<wxWindow*> m_windows;
    std::vector<Node>      m_nodes;
};


/// The alignment of Qt of the flags of an item of a sizer (wxALIGN_..., wxEXPAND).
KIQT_API Qt::Alignment wxSizerFlagsAlignment( int aFlags, bool aVertical );


class wxSizer;


/// An item of a sizer: a window or a sizer of its layout (wxSizerItem).
class KIQT_API wxSizerItem
{
public:
    wxSizerItem( QWidget* aWindow, QLayout* aLayout ) :
            m_window( aWindow ),
            m_layout( aLayout )
    {
    }

    bool IsWindow() const { return m_window != nullptr; }
    bool IsSizer() const { return m_layout != nullptr; }
    bool IsSpacer() const { return !m_window && !m_layout; }

    wxWindow* GetWindow() const;
    wxSizer* GetSizer() const;

    void Show( bool aShow );
    bool IsShown() const;

    // The layout of Qt has no per-item flags, border or proportion to change afterwards
    void SetFlag( int ) {}
    void SetBorder( int ) {}
    void SetProportion( int ) {}
    void SetMinSize( const wxSize& ) {}
    void SetMinSize( int, int ) {}

    QWidget* GetWidgetCompat() const { return m_window; }
    QLayout* GetLayoutCompat() const { return m_layout; }

private:
    QWidget* m_window;
    QLayout* m_layout;
};


/**
 * A sizer (wxSizer): the API of wxSizer on a layout of Qt (GetLayoutCompat()).  The sizers of
 * the compatibility layer (wxBoxSizer...) are layouts; the layout of a window that isn't one
 * is a sizer through wxSizerProxy.  The items are the widgets and the layouts of the layout.
 */
class KIQT_API wxSizer
{
public:
    wxSizer() = default;
    virtual ~wxSizer() = default;

    wxSizer( const wxSizer& ) = delete;
    wxSizer& operator=( const wxSizer& ) = delete;

    /// The layout of Qt of the sizer (nullptr if none).
    virtual QLayout* GetLayoutCompat() const = 0;

    // The items

    virtual void Add( wxWidgetArgCompat aWindow, int aProportion = 0, int aFlag = 0,
                      int aBorder = 0, void* aUserData = nullptr );
    virtual void Add( wxSizer* aSizer, int aProportion = 0, int aFlag = 0, int aBorder = 0,
                      void* aUserData = nullptr );

    /// Add a spacer (a stretch if @a aProportion > 0).
    virtual void Add( int aWidth, int aHeight, int aProportion = 0, int aFlag = 0,
                      int aBorder = 0, void* aUserData = nullptr );

    virtual void Insert( size_t aIndex, wxWidgetArgCompat aWindow, int aProportion = 0,
                         int aFlag = 0, int aBorder = 0, void* aUserData = nullptr );
    virtual void Insert( size_t aIndex, wxSizer* aSizer, int aProportion = 0, int aFlag = 0,
                         int aBorder = 0, void* aUserData = nullptr );

    void Prepend( wxWidgetArgCompat aWindow, int aProportion = 0, int aFlag = 0, int aBorder = 0 )
    {
        Insert( 0, aWindow, aProportion, aFlag, aBorder );
    }

    void Prepend( wxSizer* aSizer, int aProportion = 0, int aFlag = 0, int aBorder = 0 )
    {
        Insert( 0, aSizer, aProportion, aFlag, aBorder );
    }

    virtual void AddSpacer( int aSize );
    virtual void AddStretchSpacer( int aProportion = 1 );

    /// Remove a widget or a sizer from the layout (a widget stays a child of its parent).
    bool Detach( wxWidgetArgCompat aWindow );
    bool Detach( wxSizer* aSizer );

    size_t GetItemCount() const;

    /// Remove an item (a sizer is deleted, a window stays a child of its parent).
    bool Remove( int aIndex );
    bool Remove( wxSizer* aSizer );

    /// The item of a window or of a sizer of the layout (nullptr if none).
    wxSizerItem* FindItem( wxWidgetArgCompat aWindow, bool aRecursive = false );
    wxSizerItem* FindItem( wxSizer* aSizer, bool aRecursive = false );
    wxSizerItem* GetItem( wxWidgetArgCompat aWindow, bool aRecursive = false )
    {
        return FindItem( aWindow, aRecursive );
    }

    wxSizerItem* GetItem( size_t aIndex );

    /// Remove the items; delete their widgets if @a aDeleteWindows.
    void Clear( bool aDeleteWindows = false );

    // The layout

    /// Lay the items out again.
    void Layout();

    /// Resize @a aWindow to the size of the layout; return the size.
    wxSize Fit( wxWidgetArgCompat aWindow );
    void FitInside( wxWidgetArgCompat aWindow ) { Fit( aWindow ); }

    /// The minimum size of @a aWindow is the minimum size of the layout.
    void SetSizeHints( wxWidgetArgCompat aWindow );

    wxSize GetMinSize() const;

    /// The size of the area of the layout.
    wxSize GetSize() const;

    void SetMinSize( const wxSize& ) {}
    void SetMinSize( int, int ) {}
    void SetDimension( const wxPoint&, const wxSize& ) {}
    void SetDimension( int, int, int, int ) {}
    void RecalcSizes() { Layout(); }

    /// The window the sizer lays out (nullptr if none).
    wxWindow* GetContainingWindow() const;

    // Showing the items

    bool Show( wxWidgetArgCompat aWindow, bool aShow = true, bool aRecursive = false );
    bool Show( wxSizer* aSizer, bool aShow = true, bool aRecursive = false );

    /// Show or hide all the items.
    void Show( bool aShow ) { ShowItems( aShow ); }
    void ShowItems( bool aShow );

    bool Hide( wxWidgetArgCompat aWindow, bool aRecursive = false ) { return Show( aWindow, false ); }
    bool Hide( wxSizer* aSizer, bool aRecursive = false ) { return Show( aSizer, false ); }

    bool IsShown( wxWidgetArgCompat aWindow ) const;

protected:
    /// The item of a widget or of a layout (kept by the sizer, for the code holding it).
    wxSizerItem* itemCompat( QWidget* aWindow, QLayout* aLayout );

private:
    std::vector<std::unique_ptr<wxSizerItem>> m_itemsCompat;
};


/**
 * The layout of a window that isn't a sizer of the compatibility layer (a layout of Qt), as a
 * sizer (wxWindow::GetSizer()).
 */
class KIQT_API wxSizerProxy : public wxSizer
{
public:
    explicit wxSizerProxy( QWidget* aWindow ) :
            m_window( aWindow )
    {
    }

    QLayout* GetLayoutCompat() const override { return m_window ? m_window->layout() : nullptr; }

private:
    QWidget* m_window;
};


/**
 * A window (wxWindow): the API of wxWindow on its widget (GetHandle()).  A window is a handler
 * of events: the handlers bound to it get the events made from the signals and the events of
 * its widget.
 */
class KIQT_API wxWindow : public wxEvtHandler
{
public:
    wxWindow() = default;
    ~wxWindow() override = default;

    /// The widget of the window.
    virtual QWidget* GetHandle() const = 0;

    /// The widget of the children and of the sizer of the window (the content of a scrolled
    /// window): the widget of the window by default.
    virtual QWidget* GetClientWidgetCompat() const { return GetHandle(); }

    // The identity of the window

    int GetId() const;
    void SetId( int aId );
    wxString GetName() const;
    void SetName( const wxString& aName );
    virtual wxString GetLabel() const;
    virtual void SetLabel( const wxString& aLabel );

    // The state of the window

    virtual bool Enable( bool aEnable = true );
    bool Disable() { return Enable( false ); }
    bool IsEnabled() const;
    bool IsThisEnabled() const;

    virtual bool Show( bool aShow = true );
    bool Hide() { return Show( false ); }
    bool IsShown() const;
    bool IsShownOnScreen() const;

    virtual void SetFocus();
    bool HasFocus() const;

    /// Move the focus to the next (or previous) window of the keyboard navigation.
    bool Navigate( int aFlags = 1 );
    bool NavigateIn( int aFlags = 1 );
    virtual bool AcceptsFocus() const;

    /// The window with the keyboard focus (nullptr if none).
    static wxWindow* FindFocus();

    // The appearance

    virtual bool SetFont( const wxFont& aFont );
    wxFont GetFont() const;
    virtual bool SetForegroundColour( const wxColour& aColour );
    virtual bool SetBackgroundColour( const wxColour& aColour );
    wxColour GetForegroundColour() const;
    wxColour GetBackgroundColour() const;
    void SetOwnFont( const wxFont& aFont ) { SetFont( aFont ); }
    void SetOwnForegroundColour( const wxColour& aColour ) { SetForegroundColour( aColour ); }
    void SetOwnBackgroundColour( const wxColour& aColour ) { SetBackgroundColour( aColour ); }

    void SetToolTip( const wxString& aTip );
    void UnsetToolTip() { SetToolTip( wxEmptyString ); }
    wxString GetToolTipText() const;

    long GetWindowStyle() const { return m_windowStyle; }
    long GetWindowStyleFlag() const { return m_windowStyle; }
    virtual void SetWindowStyleFlag( long aStyle ) { m_windowStyle = aStyle; }
    void SetWindowStyle( long aStyle ) { SetWindowStyleFlag( aStyle ); }
    bool HasFlag( int aFlag ) const { return ( m_windowStyle & aFlag ) != 0; }
    long GetExtraStyle() const { return m_extraStyle; }
    void SetExtraStyle( long aStyle ) { m_extraStyle = aStyle; }

    // The windows around

    wxWindow* GetParent() const;
    wxWindow* GetGrandParent() const;
    wxWindowList GetChildren() const;
    wxWindow* GetPrevSibling() const;
    wxWindow* GetNextSibling() const;
    bool IsTopLevel() const;
    bool IsDescendant( wxWindow* aWindow ) const;
    virtual bool Reparent( wxWindow* aParent );

    /// A descendant with an id or a name (or this window), nullptr if none.
    wxWindow* FindWindow( long aId ) const;
    wxWindow* FindWindow( const wxString& aName ) const;

    /// A window of a parent (or of any top level window) by its name, id or label.
    static wxWindow* FindWindowByName( const wxString& aName, const wxWindow* aParent = nullptr );
    static wxWindow* FindWindowById( long aId, const wxWindow* aParent = nullptr );
    static wxWindow* FindWindowByLabel( const wxString& aLabel, const wxWindow* aParent = nullptr );

    // The geometry (in the logical pixels of Qt)

    virtual void SetMinSize( const wxSize& aSize );
    void SetMaxSize( const wxSize& aSize );
    void SetMinClientSize( const wxSize& aSize ) { SetMinSize( aSize ); }
    void SetMaxClientSize( const wxSize& aSize ) { SetMaxSize( aSize ); }
    void SetInitialSize( const wxSize& aSize = wxDefaultSize );
    void SetSizeHints( const wxSize& aMinSize, const wxSize& aMaxSize = wxDefaultSize,
                       const wxSize& aIncSize = wxDefaultSize );
    void SetSizeHints( int aMinW, int aMinH, int aMaxW = -1, int aMaxH = -1, int aIncW = -1,
                       int aIncH = -1 );

    wxSize GetMinSize() const;
    wxSize GetMaxSize() const;
    wxSize GetMinClientSize() const { return GetMinSize(); }
    wxSize GetEffectiveMinSize() const;
    wxSize GetSize() const;
    void GetSize( int* aWidth, int* aHeight ) const;
    wxSize GetClientSize() const;
    void GetClientSize( int* aWidth, int* aHeight ) const;
    wxRect GetClientRect() const;
    wxSize GetBestSize() const;
    wxSize GetVirtualSize() const { return GetClientSize(); }
    wxPoint GetPosition() const;
    void GetPosition( int* aX, int* aY ) const;
    wxPoint GetScreenPosition() const;
    wxRect GetRect() const;
    wxRect GetScreenRect() const;

    void SetSize( const wxSize& aSize );
    void SetSize( int aWidth, int aHeight ) { SetSize( wxSize( aWidth, aHeight ) ); }
    void SetSize( const wxRect& aRect, int aFlags = 0 );
    void SetSize( int aX, int aY, int aWidth, int aHeight, int aFlags = 0 );
    void SetClientSize( const wxSize& aSize ) { SetSize( aSize ); }
    void SetClientSize( int aWidth, int aHeight ) { SetSize( wxSize( aWidth, aHeight ) ); }
    void Move( const wxPoint& aPos, int aFlags = 0 );
    void Move( int aX, int aY, int aFlags = 0 ) { Move( wxPoint( aX, aY ) ); }
    void SetPosition( const wxPoint& aPos ) { Move( aPos ); }

    wxPoint ClientToScreen( const wxPoint& aPos ) const;
    void ClientToScreen( int* aX, int* aY ) const;
    wxPoint ScreenToClient( const wxPoint& aPos ) const;
    void ScreenToClient( int* aX, int* aY ) const;

    /// The size of a text in the font of the window (or in @a aFont).
    wxSize GetTextExtent( const wxString& aText ) const;
    void GetTextExtent( const wxString& aText, int* aWidth, int* aHeight, int* aDescent = nullptr,
                        int* aExternalLeading = nullptr, const wxFont* aFont = nullptr ) const;
    int GetCharWidth() const;
    int GetCharHeight() const;

    /// Sizes in the logical pixels of Qt: already independent of the DPI.
    wxSize FromDIP( const wxSize& aSize ) const { return aSize; }
    wxPoint FromDIP( const wxPoint& aPoint ) const { return aPoint; }
    int FromDIP( int aValue ) const { return aValue; }
    wxSize ToDIP( const wxSize& aSize ) const { return aSize; }
    wxPoint ToDIP( const wxPoint& aPoint ) const { return aPoint; }
    int ToDIP( int aValue ) const { return aValue; }
    wxSize FromPhys( const wxSize& aSize ) const;
    int FromPhys( int aValue ) const;
    wxSize ToPhys( const wxSize& aSize ) const;
    int ToPhys( int aValue ) const;
    double GetContentScaleFactor() const;
    double GetDPIScaleFactor() const { return GetContentScaleFactor(); }

    /// The dialog units of wxWidgets (a fourth of the average width of a character, an eighth
    /// of its height).
    wxPoint ConvertDialogToPixels( const wxPoint& aPoint ) const;
    wxSize ConvertDialogToPixels( const wxSize& aSize ) const;
    wxPoint ConvertPixelsToDialog( const wxPoint& aPoint ) const;
    wxSize ConvertPixelsToDialog( const wxSize& aSize ) const;

    // The painting and the layout

    virtual void Refresh( bool aEraseBackground = true, const wxRect* aRect = nullptr );
    void RefreshRect( const wxRect& aRect, bool aEraseBackground = true ) { Refresh( aEraseBackground, &aRect ); }
    void Update();
    void Freeze();
    void Thaw();
    bool IsFrozen() const { return m_freezeCount > 0; }
    virtual bool Layout();
    virtual void Fit();
    virtual void FitInside() { Fit(); }
    void InvalidateBestSize() { GetHandle()->updateGeometry(); }
    void PostSizeEvent();
    void SendSizeEvent( int aFlags = 0 ) { PostSizeEvent(); }

    /// The layout of the window, as a sizer (nullptr if it has none).
    wxSizer* GetSizer();
    void SetSizer( wxSizer* aSizer, bool aDeleteOld = true );
    void SetSizerAndFit( wxSizer* aSizer, bool aDeleteOld = true );

    /// Center a window on its parent (or the screen).
    void Centre( int aDirection = wxBOTH );
    void Center( int aDirection = wxBOTH ) { Centre( aDirection ); }
    void CentreOnParent( int aDirection = wxBOTH ) { Centre( aDirection ); }
    void CenterOnParent( int aDirection = wxBOTH ) { Centre( aDirection ); }
    void CentreOnScreen( int aDirection = wxBOTH ) { Centre( aDirection ); }
    void CenterOnScreen( int aDirection = wxBOTH ) { Centre( aDirection ); }

    void Raise();
    void Lower();

    void CaptureMouse();
    void ReleaseMouse();
    bool HasCapture() const;

    /// Close the window (a dialog, a frame): its close event first.
    bool Close( bool aForce = false );

    /// Delete the window later (on the event loop).
    virtual bool Destroy();

    // The data of the window

    void* GetClientData() const { return m_clientData; }
    void SetClientData( void* aData ) { m_clientData = aData; }

    /// The validator of the window (a copy of @a aValidator).
    void SetValidator( const wxValidator& aValidator );
    wxValidator* GetValidator() const;

    /**
     * The data transfers of the window: by default, the ones of the validators of the children
     * and of the descendant windows (as with wxWS_EX_VALIDATE_RECURSIVELY).
     */
    virtual bool TransferDataToWindow();
    virtual bool TransferDataFromWindow();
    virtual bool Validate();

    void SetHelpText( const wxString& aText ) {}

    /// The icon of a top level window.
    void SetIcon( const wxBitmap& aIcon ) { GetHandle()->setWindowIcon( QIcon( aIcon ) ); }
    void SetIcons( const wxBitmapBundle& aIcons ) { GetHandle()->setWindowIcon( aIcons ); }
    void SetLayoutDirection( int aDirection ) {}

    // The handlers of the events

    /// The first handler of the chain of the window (the last pushed, or the window).
    wxEvtHandler* GetEventHandler() { return GetEventHandlerCompat(); }

    void PushEventHandler( wxEvtHandler* aHandler ) { PushEventHandlerCompat( aHandler ); }
    wxEvtHandler* PopEventHandler( bool aDeleteHandler = false )
    {
        return PopEventHandlerCompat( aDeleteHandler );
    }

    bool RemoveEventHandler( wxEvtHandler* aHandler ) { return RemoveEventHandlerCompat( aHandler ); }

    /// Give an event to the handlers pushed on the window, then to the ones of the window.
    bool ProcessEvent( wxEvent& aEvent ) override;
    bool ProcessWindowEvent( wxEvent& aEvent ) { return ProcessEvent( aEvent ); }
    bool HandleWindowEvent( wxEvent& aEvent ) { return ProcessEvent( aEvent ); }

    /// No events from the widget and its handlers.
    void SetEvtHandlerEnabled( bool aEnable );

    /// The handlers of the window.
    wxCompatEventSink& GetEventSink() { return m_eventsCompat; }

    /// Show a popup menu and give the event of the chosen item to the window.
    bool PopupMenu( wxMenu* aMenu, const wxPoint& aPos = wxDefaultPosition );
    bool PopupMenu( wxMenu* aMenu, int aX, int aY ) { return PopupMenu( aMenu, wxPoint( aX, aY ) ); }

    /// Show a menu and return the id of the chosen item (wxID_NONE if none), without its event.
    int GetPopupMenuSelectionFromUser( wxMenu& aMenu, const wxPoint& aPos = wxDefaultPosition );
    int GetPopupMenuSelectionFromUser( wxMenu& aMenu, int aX, int aY )
    {
        return GetPopupMenuSelectionFromUser( aMenu, wxPoint( aX, aY ) );
    }

protected:
    /// The handlers also get the events made from the signals and the events of the widget.
    void bindHandlerCompat( wxEventType aType, wxCompatEventSink::HANDLER aHandler,
                            const void* aKey, int aId, int aLastId ) override;

    /// Record this window as the window of its widget (the constructors of the controls).
    void registerWindowCompat();

    /// Forget it (the destructors of the controls, while their widget is still whole).
    void unregisterWindowCompat();

    /// The parent of the window when it was made (as the member of wxWindow).
    wxWindow* m_parent = nullptr;

private:
    void*                         m_clientData = nullptr;
    long                          m_windowStyle = 0;
    long                          m_extraStyle = 0;
    int                           m_freezeCount = 0;
    std::unique_ptr<wxSizerProxy> m_sizerProxy;
};


template <typename T>
QWidget* wxWidgetArgCompat::widgetOf( T* aWindow )
{
    using WINDOW = std::remove_const_t<T>;

    if constexpr( std::is_base_of_v<QWidget, WINDOW> )
        return const_cast<WINDOW*>( aWindow );
    else
        return aWindow ? aWindow->GetHandle() : nullptr;
}


/**
 * A control of the layer: a widget of Qt (@a QT) that is a window.  The constructors forward
 * their arguments to the widget.
 */
template <class QT>
class wxWindowCompat : public QT, public wxWindow
{
public:
    template <typename... ARGS>
    explicit wxWindowCompat( ARGS&&... aArgs ) :
            QT( std::forward<ARGS>( aArgs )... )
    {
        registerWindowCompat();
    }

    ~wxWindowCompat() override { unregisterWindowCompat(); }

    // The Qt widgets are not copyable; keep the forwarding constructor from taking over copies
    wxWindowCompat( const wxWindowCompat& ) = delete;
    wxWindowCompat( wxWindowCompat&& ) = delete;
    wxWindowCompat& operator=( const wxWindowCompat& ) = delete;

    QWidget* GetHandle() const override
    {
        return const_cast<QWidget*>( static_cast<const QWidget*>( this ) );
    }

    /// Call a function later, on the event loop (not after the window is deleted).
    template <typename FUNCTOR>
    void CallAfter( FUNCTOR aFunctor )
    {
        QMetaObject::invokeMethod( static_cast<QObject*>( this ), std::move( aFunctor ),
                                   Qt::QueuedConnection );
    }

    template <typename CLASS>
    void CallAfter( void ( CLASS::*aMethod )() )
    {
        CLASS* self = static_cast<CLASS*>( this );
        QMetaObject::invokeMethod( static_cast<QObject*>( this ),
                                   [self, aMethod]() { ( self->*aMethod )(); },
                                   Qt::QueuedConnection );
    }
};


/// The data transfers of @a aWindow (wxWindow::TransferDataToWindow()...), true for no window.
KIQT_API bool wxTransferDataToWindowCompat( wxWidgetArgCompat aWindow );
KIQT_API bool wxTransferDataFromWindowCompat( wxWidgetArgCompat aWindow );
KIQT_API bool wxValidateCompat( wxWidgetArgCompat aWindow );

/// The default transfers of a window: the ones of the validators of its children and of its
/// descendant windows.
KIQT_API bool wxTransferDataToChildrenCompat( wxWidgetArgCompat aWindow );
KIQT_API bool wxTransferDataFromChildrenCompat( wxWidgetArgCompat aWindow );
KIQT_API bool wxValidateChildrenCompat( wxWidgetArgCompat aWindow );

/// The top level window of a window (its dialog or its frame).
KIQT_API wxWindow* wxGetTopLevelParent( wxWidgetArgCompat aWindow );

/// Center a window on its parent, or on the screen.
KIQT_API void wxCentreWindow( wxWidgetArgCompat aWindow, int aDirection );

/// A control (wxControl): the controls of the layer are windows.
using wxControl = wxWindow;

#endif // KIWX_WINDOW_H
