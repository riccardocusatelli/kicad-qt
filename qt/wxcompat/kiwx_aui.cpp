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
 * @file kiwx_aui.cpp
 * The panes of a frame of the wxWidgets compatibility layer (wxAuiManager) on a QMainWindow.
 */

#include <algorithm>

#include <QDockWidget>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <wx/aui/framemanager.h>

#include <qt_strings.h>


namespace
{

Qt::DockWidgetArea dockArea( int aDirection )
{
    switch( aDirection )
    {
    case wxAUI_DOCK_TOP:    return Qt::TopDockWidgetArea;
    case wxAUI_DOCK_RIGHT:  return Qt::RightDockWidgetArea;
    case wxAUI_DOCK_BOTTOM: return Qt::BottomDockWidgetArea;
    default:                return Qt::LeftDockWidgetArea;
    }
}


Qt::ToolBarArea toolBarArea( int aDirection )
{
    switch( aDirection )
    {
    case wxAUI_DOCK_RIGHT:  return Qt::RightToolBarArea;
    case wxAUI_DOCK_BOTTOM: return Qt::BottomToolBarArea;
    case wxAUI_DOCK_LEFT:   return Qt::LeftToolBarArea;
    default:                return Qt::TopToolBarArea;
    }
}


/// A pane of the central area of the frame: the center panes, and the panes without a caption
/// above or below them (the info bar, the message panel, a bar of buttons).
bool isCentral( const wxAuiPaneInfo& aInfo )
{
    if( aInfo.dock_direction == wxAUI_DOCK_CENTER )
        return true;

    return ( aInfo.dock_direction == wxAUI_DOCK_TOP || aInfo.dock_direction == wxAUI_DOCK_BOTTOM )
           && !aInfo.HasCaption();
}

} // namespace


// wxAuiPaneInfo ------------------------------------------------------------------------------

wxAuiPaneInfo::wxAuiPaneInfo()
{
    // The defaults of wxWidgets (DefaultPane())
    state = optionTopDockable | optionBottomDockable | optionLeftDockable | optionRightDockable
            | optionFloatable | optionMovable | optionResizable | optionCaption | optionPaneBorder
            | buttonClose;
}


wxAuiPaneInfo& wxAuiPaneInfo::Window( wxWidgetArgCompat aWindow )
{
    widget = aWindow.get();
    window = aWindow.window();
    return *this;
}


wxAuiPaneInfo& wxAuiPaneInfo::CenterPane()
{
    state = 0;
    return Center().PaneBorder().Resizable();
}


wxAuiPaneInfo& wxAuiPaneInfo::ToolbarPane()
{
    SetFlag( optionToolbar, true );
    SetFlag( optionResizable, false );
    SetFlag( optionCaption, false );
    SetFlag( buttonClose, false );
    return *this;
}


// wxAuiManager -------------------------------------------------------------------------------

wxAuiManager::wxAuiManager( wxWidgetArgCompat aManagedWindow, unsigned int aFlags ) :
        m_flags( aFlags )
{
    SetManagedWindow( aManagedWindow );
}


wxAuiManager::~wxAuiManager() = default;


void wxAuiManager::SetManagedWindow( wxWidgetArgCompat aManagedWindow )
{
    m_frame = qobject_cast<QMainWindow*>( aManagedWindow.get() );
}


QWidget* wxAuiManager::GetManagedWindow() const
{
    return m_frame;
}


void wxAuiManager::UnInit()
{
    // The panes are children of the frame: they go with it
    m_frame = nullptr;
    m_central = nullptr;
}


bool wxAuiManager::AddPane( wxWidgetArgCompat aWindow, const wxAuiPaneInfo& aInfo )
{
    if( !aWindow || GetPane( aWindow ).IsOk() )
        return false;

    auto pane = std::make_unique<PANE>();
    pane->info = aInfo;
    pane->info.Window( aWindow );

    if( pane->info.name.IsEmpty() )
        pane->info.name = wxString::Format( wxS( "pane%zu" ), m_panes.size() );

    m_panes.push_back( std::move( pane ) );
    return true;
}


bool wxAuiManager::AddPane( wxWidgetArgCompat aWindow, int aDirection, const wxString& aCaption )
{
    wxAuiPaneInfo info;
    info.Caption( aCaption );

    switch( aDirection )
    {
    case wxTOP:    info.Top();    break;
    case wxBOTTOM: info.Bottom(); break;
    case wxRIGHT:  info.Right();  break;
    case wxCENTER: info.CenterPane(); break;
    default:       info.Left();   break;
    }

    return AddPane( aWindow, info );
}


bool wxAuiManager::InsertPane( wxWidgetArgCompat aWindow, const wxAuiPaneInfo& aInfo, int aLevel )
{
    if( wxAuiPaneInfo& existing = GetPane( aWindow ); existing.IsOk() )
    {
        existing.Direction( aInfo.dock_direction ).Layer( aInfo.dock_layer )
                .Row( aInfo.dock_row ).Position( aInfo.dock_pos );
        return true;
    }

    return AddPane( aWindow, aInfo );
}


bool wxAuiManager::DetachPane( wxWidgetArgCompat aWindow )
{
    auto it = std::find_if( m_panes.begin(), m_panes.end(),
                            [&]( const std::unique_ptr<PANE>& aPane )
                            {
                                return aPane->info.widget == aWindow.get();
                            } );

    if( it == m_panes.end() )
        return false;

    PANE& pane = **it;

    if( pane.dock )
    {
        // The window stays (as wxWidgets), out of its dock
        pane.dock->setWidget( nullptr );

        if( pane.info.widget && m_frame )
            pane.info.widget->setParent( m_frame );

        delete pane.dock;
    }

    bool central = pane.placed && isCentral( pane.info );
    m_panes.erase( it );

    if( central )
        layoutCentralArea();

    return true;
}


wxAuiPaneInfo& wxAuiManager::GetPane( const wxString& aName )
{
    for( std::unique_ptr<PANE>& pane : m_panes )
    {
        if( pane->info.name == aName )
            return pane->info;
    }

    m_invalidPane = wxAuiPaneInfo();
    return m_invalidPane;
}


wxAuiPaneInfo& wxAuiManager::GetPane( wxWidgetArgCompat aWindow )
{
    for( std::unique_ptr<PANE>& pane : m_panes )
    {
        if( aWindow && pane->info.widget == aWindow.get() )
            return pane->info;
    }

    m_invalidPane = wxAuiPaneInfo();
    return m_invalidPane;
}


std::vector<wxAuiPaneInfo*> wxAuiManager::GetAllPanes()
{
    std::vector<wxAuiPaneInfo*> panes;

    for( std::unique_ptr<PANE>& pane : m_panes )
        panes.push_back( &pane->info );

    return panes;
}


void wxAuiManager::Update()
{
    if( !m_frame )
        return;

    bool centralChanged = false;

    for( std::unique_ptr<PANE>& pane : m_panes )
    {
        if( !pane->placed )
        {
            placePane( *pane );
            centralChanged |= isCentral( pane->info );
        }

        const wxAuiPaneInfo& info = pane->info;
        QWidget*             target = pane->dock ? pane->dock : info.widget;

        // Only the changes of the panes are applied: the panes of the central area show and
        // hide themselves too (the info bar)
        if( target && pane->shownState != info.IsShown() )
        {
            target->setVisible( info.IsShown() );
            pane->shownState = info.IsShown();
        }

        if( pane->dock )
        {
            pane->dock->setWindowTitle( ToQString( info.caption ) );
            pane->dock->setFloating( info.IsFloating() );
        }
    }

    if( centralChanged )
        layoutCentralArea();
}


void wxAuiManager::placePane( PANE& aPane )
{
    wxAuiPaneInfo& info = aPane.info;
    QWidget*       widget = info.widget;

    aPane.placed = true;
    aPane.shownState = info.IsShown();

    if( !widget )
        return;

    // A toolbar goes to the toolbars of the frame (where it is not already)
    if( QToolBar* toolbar = qobject_cast<QToolBar*>( widget ) )
    {
        if( m_frame->toolBarArea( toolbar ) == Qt::NoToolBarArea )
            m_frame->addToolBar( toolBarArea( info.dock_direction ), toolbar );

        if( !info.IsShown() )
            toolbar->hide();

        return;
    }

    if( isCentral( info ) )
    {
        if( !info.IsShown() )
            widget->hide();

        return;
    }

    QDockWidget* dock = new QDockWidget( ToQString( info.caption ), m_frame );

    dock->setObjectName( ToQString( info.name ) );
    dock->setProperty( wxAUI_CONTAINER_PROPERTY_COMPAT, true );
    dock->setWidget( widget );

    if( !info.HasCaption() )
        dock->setTitleBarWidget( new QWidget( dock ) );

    QDockWidget::DockWidgetFeatures features;

    if( info.HasCloseButton() )
        features |= QDockWidget::DockWidgetClosable;

    if( info.IsMovable() )
        features |= QDockWidget::DockWidgetMovable;

    if( info.IsFloatable() && ( m_flags & wxAUI_MGR_ALLOW_FLOATING ) )
        features |= QDockWidget::DockWidgetFloatable;

    dock->setFeatures( features );
    m_frame->addDockWidget( dockArea( info.dock_direction ), dock );

    if( info.min_size.x > 0 )
        widget->setMinimumWidth( info.min_size.x );

    if( info.min_size.y > 0 )
        widget->setMinimumHeight( info.min_size.y );

    // The best size of the pane, once the frame is laid out
    bool horizontal = info.dock_direction == wxAUI_DOCK_LEFT
                      || info.dock_direction == wxAUI_DOCK_RIGHT;
    int  size = horizontal ? info.best_size.x : info.best_size.y;

    if( size > 0 )
    {
        QPointer<QMainWindow> frame( m_frame );
        Qt::Orientation       orientation = horizontal ? Qt::Horizontal : Qt::Vertical;

        m_frame->resizeDocks( { dock }, { size }, orientation );

        QTimer::singleShot( 0, dock,
                            [frame, dock, size, orientation]()
                            {
                                if( frame )
                                    frame->resizeDocks( { dock }, { size }, orientation );
                            } );
    }

    if( !info.IsShown() )
        dock->hide();

    aPane.dock = dock;
}


void wxAuiManager::layoutCentralArea()
{
    if( !m_frame )
        return;

    std::vector<PANE*> top, center, bottom;

    for( std::unique_ptr<PANE>& pane : m_panes )
    {
        if( !pane->placed || pane->dock || !pane->info.widget || !isCentral( pane->info )
                || qobject_cast<QToolBar*>( pane->info.widget ) )
        {
            continue;
        }

        switch( pane->info.dock_direction )
        {
        case wxAUI_DOCK_TOP:    top.push_back( pane.get() );    break;
        case wxAUI_DOCK_BOTTOM: bottom.push_back( pane.get() ); break;
        default:                center.push_back( pane.get() ); break;
        }
    }

    // The outer layers are the farthest from the center
    std::stable_sort( top.begin(), top.end(),
                      []( PANE* a, PANE* b ) { return a->info.dock_layer > b->info.dock_layer; } );
    std::stable_sort( bottom.begin(), bottom.end(),
                      []( PANE* a, PANE* b ) { return a->info.dock_layer < b->info.dock_layer; } );

    if( !m_central || m_frame->centralWidget() != m_central )
    {
        m_central = new QWidget( m_frame );
        m_central->setProperty( wxAUI_CONTAINER_PROPERTY_COMPAT, true );
        m_frame->setCentralWidget( m_central );
    }

    delete m_central->layout();

    QVBoxLayout* layout = new QVBoxLayout( m_central );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    auto add = [&]( PANE* aPane, int aStretch )
               {
                   bool hidden = aPane->info.widget->isHidden();

                   aPane->info.widget->setParent( m_central );
                   layout->addWidget( aPane->info.widget, aStretch );
                   aPane->info.widget->setVisible( !hidden );
               };

    for( PANE* pane : top )
        add( pane, 0 );

    for( PANE* pane : center )
        add( pane, 1 );

    for( PANE* pane : bottom )
        add( pane, 0 );
}
