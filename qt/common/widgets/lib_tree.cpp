/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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
 * @file lib_tree.cpp
 * LIB_TREE of the Qt build (common/widgets/lib_tree.cpp with wxWidgets): a QLineEdit to search,
 * a menu to sort, the QTreeView of the LIB_TREE_MODEL_ADAPTER and an HTML_WINDOW for the details.
 * The previews of the items when hovering them are not shown yet.
 */

#include <widgets/lib_tree.h>

#include <QAction>
#include <QCursor>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QTimer>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include <bitmaps.h>
#include <common.h>
#include <core/kicad_algo.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <widgets/bitmap_button.h>

#include <qt_strings.h>
#include <ui_hooks.h>

#include <algorithm>


constexpr int RECENT_SEARCHES_MAX = 10;

static std::map<wxString, std::vector<wxString>> g_recentSearches;


namespace
{

/// The hotkey of KiCad of a key event of Qt (the key code and the modifiers MD_...).
int hotkeyOf( const QKeyEvent* aEvent )
{
    int hotkey = aEvent->key();

    // The letters are the upper case ASCII codes in KiCad, as in Qt
    if( aEvent->modifiers() & Qt::ControlModifier )
        hotkey |= MD_CTRL;

    if( aEvent->modifiers() & Qt::AltModifier )
        hotkey |= MD_ALT;

    if( aEvent->modifiers() & Qt::ShiftModifier )
        hotkey |= MD_SHIFT;

    if( aEvent->modifiers() & Qt::MetaModifier )
        hotkey |= MD_META;

    return hotkey;
}

} // namespace


LIB_TREE::LIB_TREE( QWidget* aParent, const wxString& aRecentSearchesKey,
                    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter, int aFlags,
                    HTML_WINDOW* aDetails ) :
        QWidget( aParent ),
        m_adapter( aAdapter ),
        m_query_ctrl( nullptr ),
        m_sort_ctrl( nullptr ),
        m_details_ctrl( nullptr ),
        m_debounceTimer( nullptr ),
        m_inTimerEvent( false ),
        m_recentSearchesKey( aRecentSearchesKey ),
        m_filtersSizer( nullptr ),
        m_previewDisabled( false )
{
    QVBoxLayout* sizer = new QVBoxLayout( this );
    sizer->setContentsMargins( 0, 0, 0, 0 );
    sizer->setSpacing( 0 );

    // Search text control
    if( aFlags & SEARCH )
    {
        QHBoxLayout* search_sizer = new QHBoxLayout();
        search_sizer->setContentsMargins( 0, 0, 0, 0 );
        search_sizer->setSpacing( 4 );

        m_query_ctrl = new QLineEdit( this );
        m_query_ctrl->setClearButtonEnabled( true );
        search_sizer->addWidget( m_query_ctrl, 1 );

        QFrame* separator = new QFrame( this );
        separator->setFrameShape( QFrame::VLine );
        separator->setFrameShadow( QFrame::Sunken );
        search_sizer->addWidget( separator );

        m_sort_ctrl = new BITMAP_BUTTON( this );
        m_sort_ctrl->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
        search_sizer->addWidget( m_sort_ctrl );

        QObject::connect( m_sort_ctrl, &QAbstractButton::pressed, this,
                [this]()
                {
                    QMenu menu( this );

                    QAction* bestMatch = menu.addAction( ToQString( _( "Sort by Best Match" ) ) );
                    QAction* alphabetic = menu.addAction( ToQString( _( "Sort Alphabetically" ) ) );
                    menu.addSeparator();
                    QAction* expandAll = menu.addAction( ToQString( ACTIONS::expandAll.GetMenuItem() ) );
                    QAction* collapseAll = menu.addAction( ToQString( ACTIONS::collapseAll.GetMenuItem() ) );

                    bestMatch->setCheckable( true );
                    alphabetic->setCheckable( true );

                    if( m_adapter->GetSortMode() == LIB_TREE_MODEL_ADAPTER::BEST_MATCH )
                        bestMatch->setChecked( true );
                    else
                        alphabetic->setChecked( true );

                    QAction* chosen = menu.exec( m_sort_ctrl->mapToGlobal(
                            QPoint( 0, m_sort_ctrl->height() ) ) );

                    if( chosen == bestMatch )
                    {
                        m_adapter->SetSortMode( LIB_TREE_MODEL_ADAPTER::BEST_MATCH );
                        Regenerate( true );
                    }
                    else if( chosen == alphabetic )
                    {
                        m_adapter->SetSortMode( LIB_TREE_MODEL_ADAPTER::ALPHABETIC );
                        Regenerate( true );
                    }
                    else if( chosen == expandAll )
                    {
                        ExpandAll();
                    }
                    else if( chosen == collapseAll )
                    {
                        CollapseAll();
                    }
                } );

        sizer->addLayout( search_sizer );

        m_debounceTimer = new QTimer( this );
        m_debounceTimer->setSingleShot( true );

        QObject::connect( m_debounceTimer, &QTimer::timeout, this, [this]() { onDebounceTimer(); } );
        QObject::connect( m_query_ctrl, &QLineEdit::textChanged, this, [this]() { onQueryText(); } );

        KIQT::FilterEvents( m_query_ctrl,
                [this]( QEvent* aEvent )
                {
                    if( aEvent->type() == QEvent::KeyPress )
                        return onQueryKey( static_cast<QKeyEvent*>( aEvent ) );

                    return false;
                } );
    }

    if( aFlags & FILTERS )
    {
        m_filtersSizer = new QVBoxLayout();
        m_filtersSizer->setContentsMargins( 4, 0, 0, 0 );
        sizer->addLayout( m_filtersSizer );
    }

    // Tree control
    m_tree_ctrl = new QTreeView( this );
    m_tree_ctrl->setSelectionMode( ( aFlags & MULTISELECT ) ? QAbstractItemView::ExtendedSelection
                                                            : QAbstractItemView::SingleSelection );
    m_tree_ctrl->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_tree_ctrl->setUniformRowHeights( true );
    m_tree_ctrl->setContextMenuPolicy( Qt::CustomContextMenu );
    m_tree_ctrl->header()->setContextMenuPolicy( Qt::CustomContextMenu );
    m_tree_ctrl->header()->setStretchLastSection( true );
    m_tree_ctrl->setExpandsOnDoubleClick( false );

    m_adapter->AttachTo( m_tree_ctrl );

    sizer->addWidget( m_tree_ctrl, 5 );

    // Description panel
    if( aFlags & DETAILS )
    {
        if( !aDetails )
        {
            m_details_ctrl = new HTML_WINDOW( this );
            m_details_ctrl->setMinimumHeight( 80 );
            sizer->addSpacing( 5 );
            sizer->addWidget( m_details_ctrl, 2 );
        }
        else
        {
            m_details_ctrl = aDetails;
        }

        m_details_ctrl->setOpenLinks( false );

        QObject::connect( m_details_ctrl, &QTextBrowser::anchorClicked, this,
                          [this]( const QUrl& aUrl ) { onDetailsLink( ToWxString( aUrl.toString() ) ); } );
    }

    QObject::connect( m_tree_ctrl, &QAbstractItemView::activated, this, [this]() { onTreeActivate(); } );
    QObject::connect( m_tree_ctrl, &QAbstractItemView::doubleClicked, this, [this]() { onTreeActivate(); } );

    QObject::connect( m_tree_ctrl->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                      [this]() { onTreeSelect(); } );

    QObject::connect( m_tree_ctrl, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos ) { onItemContextMenu( aPos ); } );

    QObject::connect( m_tree_ctrl->header(), &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos ) { onHeaderContextMenu( aPos ); } );

    // Process hotkeys when the tree control has focus:
    KIQT::FilterEvents( m_tree_ctrl,
            [this]( QEvent* aEvent )
            {
                if( aEvent->type() == QEvent::KeyPress )
                    return onTreeKey( static_cast<QKeyEvent*>( aEvent ) );

                return false;
            } );

    if( m_query_ctrl )
    {
        m_query_ctrl->setPlaceholderText( ToQString( _( "Search" ) ) );
        m_query_ctrl->setFocus();
        updateRecentSearchMenu();

        // Force an update of the adapter with the empty text to ensure preselect is done
        Regenerate( false );
    }
    else
    {
        // There may be a part preselected in the model. Make sure it is displayed.
        // Regenerate does this in the other branch
        postPreselectEvent();
    }
}


LIB_TREE::~LIB_TREE()
{
    // Stop the timer during destruction early to avoid potential race conditions
    if( m_debounceTimer )
        m_debounceTimer->stop();

    // The model lives as long as its references: the view must not use it any more
    m_tree_ctrl->setModel( nullptr );
}


void LIB_TREE::ShutdownPreviews()
{
    m_previewDisabled = true;
}


void LIB_TREE::ShowChangedLanguage()
{
    if( m_query_ctrl )
        m_query_ctrl->setPlaceholderText( ToQString( _( "Search" ) ) );

    if( m_adapter )
        m_adapter->ShowChangedLanguage();
}


QModelIndex LIB_TREE::currentSelection() const
{
    QItemSelectionModel* selection = m_tree_ctrl->selectionModel();

    if( !selection )
        return QModelIndex();

    QModelIndexList rows = selection->selectedRows();

    if( rows.size() != 1 )
        return rows.isEmpty() ? QModelIndex() : selection->currentIndex();

    return rows.first();
}


int LIB_TREE::GetSelectionCount() const
{
    QItemSelectionModel* selection = m_tree_ctrl->selectionModel();
    return selection ? static_cast<int>( selection->selectedRows().size() ) : 0;
}


LIB_ID LIB_TREE::GetSelectedLibId( int* aUnit ) const
{
    QModelIndex sel = currentSelection();

    if( !sel.isValid() )
        return LIB_ID();

    LIB_TREE_NODE* node = m_adapter->GetTreeNodeFor( sel );

    if( !node || node->m_IsAlreadyPlacedGroup || node->m_IsRecentlyUsedGroup )
        return LIB_ID();

    if( aUnit )
        *aUnit = m_adapter->GetUnitFor( sel );

    return m_adapter->GetAliasFor( sel );
}


int LIB_TREE::GetSelectedLibIds( std::vector<LIB_ID>& aSelection, std::vector<int>* aUnit ) const
{
    QModelIndexList selection = m_tree_ctrl->selectionModel()->selectedRows();

    for( const QModelIndex& item : selection )
    {
        aSelection.emplace_back( m_adapter->GetAliasFor( item ) );

        if( aUnit )
            aUnit->emplace_back( m_adapter->GetUnitFor( item ) );
    }

    return static_cast<int>( selection.size() );
}


LIB_TREE_NODE* LIB_TREE::GetCurrentTreeNode() const
{
    QModelIndex sel = currentSelection();

    if( !sel.isValid() )
        return nullptr;

    return m_adapter->GetTreeNodeFor( sel );
}


int LIB_TREE::GetSelectedTreeNodes( std::vector<LIB_TREE_NODE*>& aSelection ) const
{
    QModelIndexList selection = m_tree_ctrl->selectionModel()->selectedRows();

    for( const QModelIndex& item : selection )
        aSelection.push_back( m_adapter->GetTreeNodeFor( item ) );

    return static_cast<int>( selection.size() );
}


void LIB_TREE::SelectLibId( const LIB_ID& aLibId )
{
    QModelIndex item = m_adapter->FindItem( aLibId );

    // Expand the parent library so the row is visible and can be selected/scrolled to.
    for( QModelIndex parent = item.parent(); parent.isValid(); parent = parent.parent() )
        m_tree_ctrl->expand( parent );

    selectIfValid( item );
}


void LIB_TREE::CenterLibId( const LIB_ID& aLibId )
{
    centerIfValid( m_adapter->FindItem( aLibId ) );
}


void LIB_TREE::Unselect()
{
    m_tree_ctrl->clearSelection();
}


void LIB_TREE::ExpandLibId( const LIB_ID& aLibId )
{
    expandIfValid( m_adapter->FindItem( aLibId ) );
}


std::vector<LIB_ID> LIB_TREE::GetExpandedLibraries() const
{
    std::vector<LIB_ID> expanded;

    for( int row = 0; row < m_adapter->rowCount(); ++row )
    {
        QModelIndex item = m_adapter->index( row, 0 );

        if( m_tree_ctrl->isExpanded( item ) )
        {
            if( LIB_TREE_NODE* node = m_adapter->GetTreeNodeFor( item ) )
                expanded.push_back( node->m_LibId );
        }
    }

    return expanded;
}


void LIB_TREE::ExpandAll()
{
    m_tree_ctrl->expandAll();
}


void LIB_TREE::CollapseAll()
{
    m_tree_ctrl->collapseAll();
}


void LIB_TREE::SetSearchString( const wxString& aSearchString )
{
    if( !m_query_ctrl )
        return;

    // Without the event of a change of the text (wxTextEntry::ChangeValue())
    QSignalBlocker blocker( m_query_ctrl );
    m_query_ctrl->setText( ToQString( aSearchString ) );
}


wxString LIB_TREE::GetSearchString() const
{
    return m_query_ctrl ? ToWxString( m_query_ctrl->text() ) : wxString();
}


void LIB_TREE::updateRecentSearchMenu()
{
    if( !m_query_ctrl )
        return;

    wxString newEntry = GetSearchString();

    std::vector<wxString>& recents = g_recentSearches[ m_recentSearchesKey ];

    if( !newEntry.IsEmpty() )
    {
        if( alg::contains( recents, newEntry ) )
            std::erase( recents, newEntry );

        if( recents.size() >= RECENT_SEARCHES_MAX )
            recents.pop_back();

        recents.insert( recents.begin(), newEntry );
    }

    // The recent searches are in the menu of the search icon of the field
    for( QAction* action : m_query_ctrl->actions() )
    {
        m_query_ctrl->removeAction( action );
        action->deleteLater();
    }

    QAction* recentAction = m_query_ctrl->addAction( KiBitmapBundle( BITMAPS::find ), QLineEdit::LeadingPosition );
    recentAction->setToolTip( ToQString( recents.empty() ? _( "recent searches" ) : wxString() ) );

    QObject::connect( recentAction, &QAction::triggered, this,
            [this]()
            {
                QMenu                        menu( this );
                const std::vector<wxString>& searches = g_recentSearches[ m_recentSearchesKey ];

                for( const wxString& recent : searches )
                {
                    QAction* item = menu.addAction( ToQString( recent ) );
                    QObject::connect( item, &QAction::triggered, this,
                                      [this, recent]() { m_query_ctrl->setText( ToQString( recent ) ); } );
                }

                if( searches.empty() )
                    menu.addAction( ToQString( _( "recent searches" ) ) )->setEnabled( false );

                menu.exec( m_query_ctrl->mapToGlobal( QPoint( 0, m_query_ctrl->height() ) ) );
            } );
}


void LIB_TREE::Regenerate( bool aKeepState )
{
    STATE current;

    // Store the state
    if( aKeepState )
        current = getState();

    m_adapter->UpdateSearchString( GetSearchString(), aKeepState );
    postPreselectEvent();

    // Restore the state
    if( aKeepState )
        setState( current );
}


void LIB_TREE::RefreshLibTree()
{
    m_adapter->RefreshTree();
}


QWidget* LIB_TREE::GetFocusTarget()
{
    if( m_query_ctrl )
        return m_query_ctrl;
    else
        return m_tree_ctrl;
}


void LIB_TREE::FocusSearchFieldIfExists()
{
    if( m_query_ctrl )
        m_query_ctrl->setFocus();
}


void LIB_TREE::toggleExpand( const QModelIndex& aTreeId )
{
    if( !aTreeId.isValid() )
        return;

    m_tree_ctrl->setExpanded( aTreeId, !m_tree_ctrl->isExpanded( aTreeId ) );
}


void LIB_TREE::selectIfValid( const QModelIndex& aTreeId )
{
    if( aTreeId.isValid() )
    {
        m_tree_ctrl->scrollTo( aTreeId );
        m_tree_ctrl->setCurrentIndex( aTreeId );
        m_tree_ctrl->selectionModel()->select( aTreeId, QItemSelectionModel::ClearAndSelect
                                                                | QItemSelectionModel::Rows );
        postPreselectEvent();
    }
}


void LIB_TREE::centerIfValid( const QModelIndex& aTreeId )
{
    if( aTreeId.isValid() )
        m_tree_ctrl->scrollTo( aTreeId, QAbstractItemView::PositionAtCenter );
}


void LIB_TREE::expandIfValid( const QModelIndex& aTreeId )
{
    if( aTreeId.isValid() && !m_tree_ctrl->isExpanded( aTreeId ) )
        m_tree_ctrl->expand( aTreeId );
}


void LIB_TREE::postPreselectEvent()
{
    // From the event loop, as the event of the wxWidgets build
    QMetaObject::invokeMethod( this, [this]() { onPreselect(); }, Qt::QueuedConnection );
}


void LIB_TREE::postSelectEvent()
{
    QMetaObject::invokeMethod( this,
            [this]()
            {
                if( m_chosenHandler )
                    m_chosenHandler();
            },
            Qt::QueuedConnection );
}


LIB_TREE::STATE LIB_TREE::getState() const
{
    STATE state;

    for( int row = 0; row < m_adapter->rowCount(); ++row )
    {
        QModelIndex item = m_adapter->index( row, 0 );

        if( m_tree_ctrl->isExpanded( item ) )
            state.expanded.push_back( m_adapter->GetTreeNodeFor( item ) );
    }

    state.selection = GetSelectedLibId();

    return state;
}


void LIB_TREE::setState( const STATE& aState )
{
    for( const LIB_TREE_NODE* node : aState.expanded )
    {
        QModelIndex item = m_adapter->ToIndex( node );

        if( item.isValid() )
            m_tree_ctrl->expand( item );
    }

    if( !aState.selection.GetLibItemName().empty() || !aState.selection.GetLibNickname().empty() )
        SelectLibId( aState.selection );
}


void LIB_TREE::onQueryText()
{
    m_debounceTimer->start( 200 );
}


void LIB_TREE::onDebounceTimer()
{
    m_inTimerEvent = true;
    Regenerate( false );
    m_inTimerEvent = false;
}


bool LIB_TREE::onQueryKey( QKeyEvent* aEvent )
{
    int hotkey = hotkeyOf( aEvent );

    if( hotkey == ACTIONS::expandAll.GetHotKey() || hotkey == ACTIONS::expandAll.GetHotKeyAlt() )
    {
        m_tree_ctrl->expandAll();
        return true;
    }
    else if( hotkey == ACTIONS::collapseAll.GetHotKey() || hotkey == ACTIONS::collapseAll.GetHotKeyAlt() )
    {
        m_tree_ctrl->collapseAll();
        return true;
    }

    QModelIndex sel = currentSelection();

    if( !sel.isValid() )
        sel = m_adapter->GetCurrentDataViewItem();

    LIB_TREE_NODE::TYPE type = sel.isValid() ? m_adapter->GetTypeFor( sel )
                                             : LIB_TREE_NODE::TYPE::INVALID;

    switch( aEvent->key() )
    {
    case Qt::Key_Up:
        updateRecentSearchMenu();
        selectIfValid( sel.isValid() ? m_tree_ctrl->indexAbove( sel ) : sel );
        return true;

    case Qt::Key_Down:
        updateRecentSearchMenu();
        selectIfValid( sel.isValid() ? m_tree_ctrl->indexBelow( sel ) : m_adapter->index( 0, 0 ) );
        return true;

    case Qt::Key_Plus:
        if( !( aEvent->modifiers() & Qt::KeypadModifier ) )
            return false;

        updateRecentSearchMenu();

        if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            m_tree_ctrl->expand( sel );

        return true;

    case Qt::Key_Minus:
        if( !( aEvent->modifiers() & Qt::KeypadModifier ) )
            return false;

        updateRecentSearchMenu();

        if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            m_tree_ctrl->collapse( sel );

        return true;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        updateRecentSearchMenu();

        if( GetSelectedLibId().IsValid() )
            postSelectEvent();
        else if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            toggleExpand( sel );

        return true;

    default:
        return false;   // Any other key: pass on to search box directly.
    }
}


bool LIB_TREE::onTreeKey( QKeyEvent* aEvent )
{
    // Bare Left/Right collapse/expand the focused node. Consume them here so they
    // do not fall through to the global cursorLeft/cursorRight canvas actions.
    if( aEvent->modifiers() == Qt::NoModifier
        && ( aEvent->key() == Qt::Key_Left || aEvent->key() == Qt::Key_Right ) )
    {
        QModelIndex sel = currentSelection();

        if( !sel.isValid() )
            sel = m_adapter->GetCurrentDataViewItem();

        if( sel.isValid() )
            m_tree_ctrl->setExpanded( sel, aEvent->key() == Qt::Key_Right );

        return true;
    }

    // The arrows move in the tree itself
    if( aEvent->key() == Qt::Key_Up || aEvent->key() == Qt::Key_Down )
        return false;

    if( onQueryKey( aEvent ) )
        return true;

    if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
    {
        if( tool->GetManager()->GetActionManager()->RunHotKey( hotkeyOf( aEvent ) ) )
            return true;
    }

    return false;
}


void LIB_TREE::onTreeSelect()
{
    if( !m_inTimerEvent )
        updateRecentSearchMenu();

    postPreselectEvent();
}


void LIB_TREE::onTreeActivate()
{
    if( !m_inTimerEvent )
        updateRecentSearchMenu();

    if( !GetSelectedLibId().IsValid() )
        toggleExpand( currentSelection() );     // Expand library/part units subtree
    else
        postSelectEvent();                      // Open symbol/footprint
}


void LIB_TREE::onDetailsLink( const wxString& aHref )
{
    PROJECT& prj = Pgm().GetSettingsManager().Prj();
    wxString docname = ExpandEnvVarSubstitutions( aHref, &prj );

    QDesktopServices::openUrl( QUrl::fromUserInput( ToQString( docname ) ) );
}


void LIB_TREE::onPreselect()
{
    if( m_details_ctrl )
    {
        int    unit = 0;
        LIB_ID id = GetSelectedLibId( &unit );

        if( id.IsValid() )
            m_details_ctrl->SetPage( m_adapter->GenerateInfo( id, unit ) );
        else
            m_details_ctrl->SetPage( wxEmptyString );
    }

    if( m_selectedHandler )
        m_selectedHandler();
}


void LIB_TREE::onItemContextMenu( const QPoint& aPos )
{
    m_previewDisabled = true;

    // Select the item under the cursor before showing the context menu
    QModelIndex item = m_tree_ctrl->indexAt( aPos );

    if( item.isValid() )
    {
        m_tree_ctrl->setFocus();

        if( !m_tree_ctrl->selectionModel()->isRowSelected( item.row(), item.parent() ) )
        {
            m_tree_ctrl->setCurrentIndex( item );
            m_tree_ctrl->selectionModel()->select( item, QItemSelectionModel::ClearAndSelect
                                                                 | QItemSelectionModel::Rows );
        }
    }

    if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
    {
        tool->Activate();
        tool->GetManager()->VetoContextMenuMouseWarp();
        tool->GetToolMenu().ShowContextMenu();

        TOOL_EVENT evt( TC_MOUSE, TA_MOUSE_CLICK, BUT_RIGHT );
        tool->GetManager()->DispatchContextMenu( evt );
    }
    else
    {
        LIB_TREE_NODE* current = GetCurrentTreeNode();

        if( current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
        {
            ACTION_MENU menu( true, nullptr );

            if( current->m_Pinned )
            {
                menu.Add( ACTIONS::unpinLibrary );

                if( menu.exec( QCursor::pos() ) )
                    m_adapter->UnpinLibrary( current );
            }
            else
            {
                menu.Add( ACTIONS::pinLibrary );

                if( menu.exec( QCursor::pos() ) )
                    m_adapter->PinLibrary( current );
            }
        }
    }

    m_previewDisabled = false;
}


void LIB_TREE::onHeaderContextMenu( const QPoint& aPos )
{
    m_previewDisabled = true;

    // The columns shown, in a menu (EDA_REORDERABLE_LIST_DIALOG of the wxWidgets build)
    QMenu                 menu( this );
    std::vector<wxString> shown = m_adapter->GetShownColumns();

    for( const wxString& column : m_adapter->GetAvailableColumns() )
    {
        QAction* action = menu.addAction( ToQString( wxGetTranslation( column ) ) );
        action->setCheckable( true );
        action->setChecked( alg::contains( shown, column ) );

        // The name column is always shown
        if( column == wxT( "Item" ) )
            action->setEnabled( false );

        QObject::connect( action, &QAction::toggled, this,
                [this, column]( bool aChecked )
                {
                    std::vector<wxString> columns = m_adapter->GetShownColumns();

                    if( aChecked && !alg::contains( columns, column ) )
                        columns.push_back( column );
                    else if( !aChecked )
                        std::erase( columns, column );

                    m_adapter->SetShownColumns( columns );
                    Regenerate( true );
                } );
    }

    menu.exec( m_tree_ctrl->header()->mapToGlobal( aPos ) );

    m_previewDisabled = false;
}
