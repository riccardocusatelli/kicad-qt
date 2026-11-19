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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file editor_tabs_panel_qt.cpp
 * EDITOR_TABS_PANEL with a QTabBar (see common/widgets/editor_tabs_panel.cpp for the wxWidgets
 * panel and for EDITOR_TABS_MODEL, shared by both builds).
 *
 * The tab bar only holds the strip: the shared canvas stays where the frame put it. A tab change of
 * the user activates the document; the programmatic selections block the signals of the bar and
 * activate the document themselves, as ChangeSelection does in the wxWidgets panel.
 */

#include <widgets/editor_tabs_panel.h>

#include <algorithm>

#include <QFont>
#include <QMenu>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTabBar>
#include <QVBoxLayout>

#include <wx/intl.h>

#include <qt_strings.h>


EDITOR_TABS_PANEL::EDITOR_TABS_PANEL( QWidget* aParent, EDA_DRAW_PANEL_GAL* aSharedCanvas ) :
        QWidget( aParent ),
        m_sharedCanvas( aSharedCanvas )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    m_tabs = new QTabBar( this );
    m_tabs->setTabsClosable( true );
    m_tabs->setMovable( false );
    m_tabs->setExpanding( false );
    m_tabs->setDocumentMode( true );
    m_tabs->setUsesScrollButtons( true );
    m_tabs->setElideMode( Qt::ElideNone );
    m_tabs->setContextMenuPolicy( Qt::CustomContextMenu );
    layout->addWidget( m_tabs );

    QObject::connect( m_tabs, &QTabBar::currentChanged, this,
                      [this]( int aIdx )
                      {
                          if( aIdx >= 0 && aIdx < (int) m_model.Entries().size() )
                              activateTab( aIdx );
                      } );

    // The close button of a tab goes through CloseTab, which owns the single close prompt
    QObject::connect( m_tabs, &QTabBar::tabCloseRequested, this,
                      [this]( int aIdx ) { CloseTab( aIdx ); } );

    // Double-click is the gesture that promotes a preview tab
    QObject::connect( m_tabs, &QTabBar::tabBarDoubleClicked, this,
                      [this]( int aIdx ) { PromoteTab( aIdx ); } );

    QObject::connect( m_tabs, &QTabBar::customContextMenuRequested, this,
                      [this]( const QPoint& aPos ) { showContextMenu( aPos ); } );

    // Ctrl+Tab and Ctrl+Shift+Tab walk the tabs in most-recently-used order (the char hook of the
    // wxWidgets frame)
    if( aParent )
    {
        QShortcut* next = new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_Tab ), aParent );
        QShortcut* prev = new QShortcut( QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab ),
                                         aParent );

        QObject::connect( next, &QShortcut::activated, this, [this]() { AdvanceTab( true ); } );
        QObject::connect( prev, &QShortcut::activated, this, [this]() { AdvanceTab( false ); } );
    }

    // The strip has nothing to show until a document is open
    setVisible( false );
}


EDITOR_TABS_PANEL::~EDITOR_TABS_PANEL() = default;


TAB_VISUAL_STATE EDITOR_TABS_PANEL::visualStateForIndex( int aIdx ) const
{
    if( aIdx < 0 || aIdx >= (int) m_model.Entries().size() )
        return TAB_VISUAL_STATE{};

    if( onQueryVisualState )
        return onQueryVisualState( aIdx );

    const EDITOR_TABS_MODEL::ENTRY& e = m_model.Entries()[aIdx];

    return ResolveTabVisualState( e.preview, e.modified );
}


void EDITOR_TABS_PANEL::applyLabel( int aIdx )
{
    if( aIdx < 0 || aIdx >= m_tabs->count() || aIdx >= (int) m_labels.size() )
        return;

    // QTabBar has no font per tab: the styles of the state go to the text (an asterisk for a
    // modified document, as the wxWidgets art) and to its colour (a dimmed preview tab).
    TAB_VISUAL_STATE state = visualStateForIndex( aIdx );
    QString          text = ToQString( m_labels[aIdx] );

    if( state.showAsterisk )
        text.prepend( QLatin1Char( '*' ) );

    m_tabs->setTabText( aIdx, text );
    m_tabs->setTabToolTip( aIdx, state.italic ? ToQString( _( "Preview (double-click to keep "
                                                              "open)" ) )
                                              : QString() );
    m_tabs->setTabTextColor( aIdx, state.italic ? palette().color( QPalette::PlaceholderText )
                                                : QColor() );
}


int EDITOR_TABS_PANEL::ActivateOrCreateTab( const wxString& aKey, const wxString& aLabel,
                                            bool aAsPreview )
{
    const int existing = m_model.FindIndex( aKey );

    if( existing >= 0 )
    {
        m_model.OpenDocument( aKey, aAsPreview );
        SelectTab( existing );
        return existing;
    }

    // Capture the preview slot's key before OpenDocument overwrites it, else a reused slot leaves
    // the old key as a dead step in the MRU order.
    wxString reusedOldKey;

    if( aAsPreview )
    {
        const int previewSlot = m_model.PreviewIndex();

        if( previewSlot >= 0 )
            reusedOldKey = m_model.Entries()[previewSlot].key;
    }

    const int reuse = m_model.OpenDocument( aKey, aAsPreview );

    if( reuse < m_tabs->count() )
    {
        m_labels[reuse] = aLabel;

        if( !reusedOldKey.empty() && reusedOldKey != aKey )
            forgetMru( reusedOldKey );
    }
    else
    {
        QSignalBlocker blocker( m_tabs );

        m_tabs->addTab( ToQString( aLabel ) );
        m_labels.push_back( aLabel );
    }

    setVisible( true );

    touchMru( aKey );
    RefreshTabLabels();
    SelectTab( reuse );

    return reuse;
}


bool EDITOR_TABS_PANEL::CloseTab( int aIdx )
{
    return closeTabInternal( aIdx );
}


bool EDITOR_TABS_PANEL::closeTabInternal( int aIdx )
{
    if( aIdx < 0 || aIdx >= m_tabs->count() )
        return true;

    // Remember the active tab as a key, not an index, since indices shift on removal.
    const int      activeBefore = GetActiveTab();
    const wxString activeBeforeKey = ( activeBefore >= 0
                                       && activeBefore < (int) m_model.Entries().size() )
                                             ? m_model.Entries()[activeBefore].key
                                             : wxString();

    // Every close entry point funnels through here so the user is prompted exactly once.
    if( onCloseTabRequested && !onCloseTabRequested( aIdx ) )
        return false;

    // The host may have closed tabs itself while prompting
    if( aIdx >= m_tabs->count() || aIdx >= (int) m_model.Entries().size() )
        return true;

    const wxString key = m_model.Entries()[aIdx].key;

    {
        // The successor selection is chosen explicitly below
        QSignalBlocker blocker( m_tabs );
        m_tabs->removeTab( aIdx );
    }

    m_labels.erase( m_labels.begin() + aIdx );
    m_model.CloseDocument( key );
    forgetMru( key );

    const int newCount = m_tabs->count();

    if( newCount <= 0 )
    {
        setVisible( false );
        return true;
    }

    if( m_suppressActivateOnClose )
    {
        // The host already installed the successor document, so select the page visually only
        // and keep the MRU consistent.
        int visualIdx;

        if( activeBeforeKey.empty() || activeBeforeKey == key )
            visualIdx = std::min( aIdx, newCount - 1 );
        else
            visualIdx = m_model.FindIndex( activeBeforeKey );

        if( visualIdx >= 0 && visualIdx < newCount )
        {
            QSignalBlocker blocker( m_tabs );
            m_tabs->setCurrentIndex( visualIdx );
            touchMru( m_model.Entries()[visualIdx].key );
        }
    }
    else if( !activeBeforeKey.empty() && activeBeforeKey != key )
    {
        // Closing a background tab keeps the active document
        int idx = m_model.FindIndex( activeBeforeKey );

        QSignalBlocker blocker( m_tabs );
        m_tabs->setCurrentIndex( idx );
    }
    else
    {
        SelectTab( std::min( aIdx, newCount - 1 ) );
    }

    return true;
}


void EDITOR_TABS_PANEL::CloseOthers( int aKeepIdx )
{
    if( aKeepIdx < 0 || aKeepIdx >= m_tabs->count() )
        return;

    const wxString keepKey = m_model.Entries()[aKeepIdx].key;

    // Close from the back so earlier indices stay valid.
    for( int i = m_tabs->count() - 1; i >= 0; --i )
    {
        if( i < (int) m_model.Entries().size() && m_model.Entries()[i].key != keepKey )
            CloseTab( i );
    }
}


void EDITOR_TABS_PANEL::CloseToRight( int aIdx )
{
    if( aIdx < 0 || aIdx >= m_tabs->count() )
        return;

    for( int i = m_tabs->count() - 1; i > aIdx; --i )
        CloseTab( i );
}


void EDITOR_TABS_PANEL::CloseAll()
{
    for( int i = m_tabs->count() - 1; i >= 0; --i )
        CloseTab( i );
}


void EDITOR_TABS_PANEL::MarkModified( int aIdx, bool aModified )
{
    if( aIdx < 0 || aIdx >= (int) m_model.Entries().size() )
        return;

    // Clearing the preview flag stops the next library-open from replacing this edited document.
    m_model.MarkModified( m_model.Entries()[aIdx].key, aModified );
    RefreshTabLabels();
}


void EDITOR_TABS_PANEL::PromoteTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= (int) m_model.Entries().size() )
        return;

    if( !m_model.Entries()[aIdx].preview )
        return;

    m_model.Promote( m_model.Entries()[aIdx].key );
    RefreshTabLabels();
}


void EDITOR_TABS_PANEL::SelectTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= m_tabs->count() )
        return;

    // The bar moves without its signal, so activation is driven once, here.
    {
        QSignalBlocker blocker( m_tabs );
        m_tabs->setCurrentIndex( aIdx );
    }

    activateTab( aIdx );
}


void EDITOR_TABS_PANEL::activateTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= (int) m_model.Entries().size() )
        return;

    if( m_activating )
        return;

    m_activating = true;
    touchMru( m_model.Entries()[aIdx].key );

    if( onActivateTab )
        onActivateTab( aIdx );

    m_activating = false;
}


void EDITOR_TABS_PANEL::AdvanceTab( bool aForward )
{
    if( m_tabs->count() < 2 )
        return;

    // Cycle the MRU order so repeated Ctrl+Tab walks recently visited tabs first.
    const int active = GetActiveTab();

    if( active < 0 || active >= (int) m_model.Entries().size() )
        return;

    const wxString activeKey = m_model.Entries()[active].key;
    auto           it = std::find( m_mru.begin(), m_mru.end(), activeKey );

    if( it == m_mru.end() )
        return;

    const int pos = (int) ( it - m_mru.begin() );
    const int count = (int) m_mru.size();
    const int next = aForward ? ( pos + 1 ) % count : ( pos - 1 + count ) % count;
    const int idx = m_model.FindIndex( m_mru[next] );

    if( idx >= 0 )
        SelectTab( idx );
}


int EDITOR_TABS_PANEL::GetActiveTab() const
{
    return m_tabs->currentIndex();
}


int EDITOR_TABS_PANEL::FindTab( const wxString& aKey ) const
{
    return m_model.FindIndex( aKey );
}


void EDITOR_TABS_PANEL::RenameTab( const wxString& aOldKey, const wxString& aNewKey,
                                   const wxString& aNewLabel )
{
    const int idx = m_model.FindIndex( aOldKey );

    if( idx < 0 )
        return;

    m_model.Rename( aOldKey, aNewKey );

    if( idx < (int) m_labels.size() )
    {
        m_labels[idx] = aNewLabel;
        applyLabel( idx );
    }

    // Keep the Ctrl+Tab order pointing at the new key.
    for( wxString& key : m_mru )
    {
        if( key == aOldKey )
            key = aNewKey;
    }
}


void EDITOR_TABS_PANEL::RefreshTabLabels()
{
    for( int ii = 0; ii < m_tabs->count(); ++ii )
        applyLabel( ii );
}


void EDITOR_TABS_PANEL::touchMru( const wxString& aKey )
{
    forgetMru( aKey );
    m_mru.insert( m_mru.begin(), aKey );
}


void EDITOR_TABS_PANEL::forgetMru( const wxString& aKey )
{
    m_mru.erase( std::remove( m_mru.begin(), m_mru.end(), aKey ), m_mru.end() );
}


void EDITOR_TABS_PANEL::showContextMenu( const QPoint& aPos )
{
    const int idx = m_tabs->tabAt( aPos );

    if( idx < 0 )
        return;

    QMenu menu( this );

    QObject::connect( menu.addAction( ToQString( _( "Close Tab" ) ) ), &QAction::triggered, this,
                      [this, idx]() { CloseTab( idx ); } );
    QObject::connect( menu.addAction( ToQString( _( "Close Other Tabs" ) ) ), &QAction::triggered,
                      this, [this, idx]() { CloseOthers( idx ); } );
    QObject::connect( menu.addAction( ToQString( _( "Close Tabs to the Right" ) ) ),
                      &QAction::triggered, this, [this, idx]() { CloseToRight( idx ); } );
    QObject::connect( menu.addAction( ToQString( _( "Close All Tabs" ) ) ), &QAction::triggered,
                      this, [this]() { CloseAll(); } );

    menu.exec( m_tabs->mapToGlobal( aPos ) );
}
