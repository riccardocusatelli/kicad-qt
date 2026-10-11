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
 * @file panel_design_block_chooser_qt.cpp
 * The widgets of PANEL_DESIGN_BLOCK_CHOOSER with Qt (common/widgets/panel_design_block_chooser_wx.cpp
 * with wxWidgets): the QSplitter of the LIB_TREE and of the details panel, and the timers.  What
 * the panel does is in common/widgets/panel_design_block_chooser.cpp.
 */

#include <widgets/panel_design_block_chooser.h>

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

#include <design_block_library_adapter.h>
#include <eda_draw_frame.h>
#include <settings/app_settings.h>
#include <widgets/design_block_pane.h>
#include <widgets/design_block_preview_widget.h>

#include <qt_ui.h>
#include <ui_hooks.h>


PANEL_DESIGN_BLOCK_CHOOSER::PANEL_DESIGN_BLOCK_CHOOSER( EDA_DRAW_FRAME* aFrame, DESIGN_BLOCK_PANE* aParent,
                                                        std::vector<LIB_ID>&  aHistoryList,
                                                        std::function<void()> aSelectHandler,
                                                        TOOL_INTERACTIVE*     aContextMenuTool ) :
        QWidget( aParent ),
        m_dbl_click_timer( nullptr ),
        m_open_libs_timer( nullptr ),
        m_vsplitter( nullptr ),
        m_tree( nullptr ),
        m_preview( nullptr ),
        m_parent( aParent ),
        m_frame( aFrame ),
        m_selectHandler( std::move( aSelectHandler ) ),
        m_historyList( aHistoryList ),
        m_showSelection( false )
{
    DESIGN_BLOCK_LIBRARY_ADAPTER* libs = m_frame->Prj().DesignBlockLibs();

    m_adapter = DESIGN_BLOCK_TREE_MODEL_ADAPTER::Create(
            m_frame, libs, m_frame->config()->m_DesignBlockChooserPanel.tree, aContextMenuTool );

    // -------------------------------------------------------------------------------------
    // Construct the actual panel
    //

    QVBoxLayout* sizer = new QVBoxLayout( this );
    sizer->setContentsMargins( 0, 0, 0, 0 );

    m_vsplitter = new QSplitter( Qt::Vertical, this );
    m_vsplitter->setChildrenCollapsible( false );

    QWidget*     treePanel = new QWidget( m_vsplitter );
    QVBoxLayout* treeSizer = new QVBoxLayout( treePanel );
    treeSizer->setContentsMargins( 0, 0, 0, 0 );

    m_detailsPanel = new QWidget( m_vsplitter );
    m_detailsSizer = new QVBoxLayout( m_detailsPanel );
    m_detailsSizer->setContentsMargins( 0, 0, 0, 0 );

    m_vsplitter->addWidget( treePanel );
    m_vsplitter->addWidget( m_detailsPanel );
    m_vsplitter->setStretchFactor( 0, 1 );
    m_vsplitter->setStretchFactor( 1, 1 );

    sizer->addWidget( m_vsplitter, 1 );

    m_tree = new LIB_TREE( treePanel, wxT( "design_blocks" ), m_adapter, LIB_TREE::FLAGS::ALL_WIDGETS, nullptr );

    treeSizer->addWidget( m_tree, 1 );

    RefreshLibs();
    m_adapter->FinishTreeInitialization();

    m_tree->SetSearchString( g_designBlockSearchString );

    m_dbl_click_timer = new QTimer( this );
    m_dbl_click_timer->setSingleShot( true );
    m_open_libs_timer = new QTimer( this );
    m_open_libs_timer->setSingleShot( true );

    QObject::connect( m_dbl_click_timer, &QTimer::timeout, this, [this]() { onCloseTimer(); } );
    QObject::connect( m_open_libs_timer, &QTimer::timeout, this, [this]() { onOpenLibsTimer(); } );

    m_tree->SetChosenHandler( [this]() { onDesignBlockChosen(); } );
    m_tree->SetSelectedHandler( [this]() { onDesignBlockSelected(); } );

    KIQT::FilterEvents( this,
            [this]( QEvent* aEvent )
            {
                if( aEvent->type() == QEvent::ShortcutOverride || aEvent->type() == QEvent::KeyPress )
                    return OnChar( static_cast<QKeyEvent*>( aEvent ) );

                return false;
            } );

    // Open the user's previously opened libraries on timer expiration (after the search debounce
    // timer of the tree).
    m_open_libs_timer->start( 300 );
}


PANEL_DESIGN_BLOCK_CHOOSER::~PANEL_DESIGN_BLOCK_CHOOSER()
{
    // Stop the timers during destruction early to avoid potential race conditions
    m_dbl_click_timer->stop();
    m_open_libs_timer->stop();

    m_tree->SetChosenHandler( nullptr );
    m_tree->SetSelectedHandler( nullptr );
}


void PANEL_DESIGN_BLOCK_CHOOSER::SaveSettings()
{
    g_designBlockSearchString = m_tree->GetSearchString();

    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        // Save any changes to column widths, etc.
        m_adapter->SaveSettings();

        QWidget* parent = parentWidget() ? parentWidget() : this;

        cfg->m_DesignBlockChooserPanel.width = parent->width();
        cfg->m_DesignBlockChooserPanel.height = parent->height();
        cfg->m_DesignBlockChooserPanel.sash_pos_v = m_vsplitter->sizes().value( 0 );
        cfg->m_DesignBlockChooserPanel.sort_mode = m_tree->GetSortMode();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::SetPreviewWidget( DESIGN_BLOCK_PREVIEW_WIDGET* aPreview )
{
    m_preview = aPreview;
    m_detailsSizer->addWidget( m_preview, 1 );
}


bool PANEL_DESIGN_BLOCK_CHOOSER::OnChar( QKeyEvent* aEvent )
{
    if( aEvent->key() != Qt::Key_Escape )
        return false;

    // First escape cancels search string value
    if( dynamic_cast<QLineEdit*>( QApplication::focusWidget() ) && !m_tree->GetSearchString().IsEmpty() )
    {
        if( aEvent->type() == QEvent::KeyPress )
            m_tree->SetSearchString( wxEmptyString );

        return true;
    }

    return false;
}


void PANEL_DESIGN_BLOCK_CHOOSER::FinishSetup()
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        QWidget* parent = parentWidget() ? parentWidget() : this;

        auto horizPixelsFromDU =
                [&]( int x ) -> int
                {
                    return KIQT::DialogUnitsToPixels( parent, wxSize( x, 0 ) ).x;
                };

        APP_SETTINGS_BASE::PANEL_DESIGN_BLOCK_CHOOSER& panelCfg = cfg->m_DesignBlockChooserPanel;

        int w = panelCfg.width > 40 ? panelCfg.width : horizPixelsFromDU( 440 );
        int h = panelCfg.height > 40 ? panelCfg.height : horizPixelsFromDU( 340 );

        parent->resize( w, h );

        if( panelCfg.sash_pos_h < 0 )
            panelCfg.sash_pos_h = horizPixelsFromDU( 220 );

        if( panelCfg.sash_pos_v < 0 )
            panelCfg.sash_pos_v = horizPixelsFromDU( 230 );

        if( m_vsplitter )
            m_vsplitter->setSizes( { panelCfg.sash_pos_v, std::max( 20, h - panelCfg.sash_pos_v ) } );

        m_adapter->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) panelCfg.sort_mode );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer()
{
    // The design block is placed once the mouse button is released (the double click of the
    // tree)
    if( QApplication::mouseButtons() & Qt::LeftButton )
    {
        m_dbl_click_timer->start( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
    }
    else
    {
        if( m_frame->GetCanvas() )
            m_frame->GetCanvas()->setFocus();

        m_selectHandler();
        addDesignBlockToHistory( m_tree->GetSelectedLibId() );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::onOpenLibsTimer()
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
        m_adapter->OpenLibs( cfg->m_LibTree.open_libs );

    // The selections show their design block from now on (not during the initial load)
    m_showSelection = true;
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected()
{
    if( m_showSelection )
        showSelectedDesignBlock();
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen()
{
    // The design block is placed after the double click (see onCloseTimer())
    if( m_tree->GetSelectedLibId().IsValid() )
        m_dbl_click_timer->start( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
}


void PANEL_DESIGN_BLOCK_CHOOSER::refreshView()
{
    update();
}
