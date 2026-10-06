/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "schematic_document.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <unordered_set>
#include <set>
#include <utility>

#include <QHeaderView>
#include <QKeySequence>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <base_units.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <eeschema_helpers.h>
#include <eeschema_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <io/io_mgr.h>
#include <richio.h>
#include <lib_symbol.h>
#include <math/util.h>
#include <project.h>
#include <sch_commit.h>
#include <sch_group.h>
#include <sch_io/sch_io.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_painter.h>
#include <sch_paste.h>
#include <sch_pin.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_undo_redo.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <tools/sch_selection.h>
#include <zoom_defines.h>

#include <gal_canvas.h>
#include <inspectable_rows.h>
#include <property_panel.h>
#include <qt_strings.h>
#include <undo_stack.h>

#include "schematic_tools.h"


/// Role of the sheet list items holding the page index
static constexpr int PAGE_ROLE = Qt::UserRole;

/// Distance of a click to an item that still hits it, in pixels
static constexpr double HIT_DISTANCE_PX = 4.0;


/// The settings of the schematic editor, if any.
static EESCHEMA_SETTINGS* eeschemaSettings()
{
    return GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
}


/// The color theme of the schematic editor.
static COLOR_SETTINGS* schematicColors()
{
    if( EESCHEMA_SETTINGS* cfg = eeschemaSettings() )
        return ::GetColorSettings( cfg->m_ColorTheme );

    return ::GetColorSettings( DEFAULT_THEME );
}


/// The item drawn with @a aItem that a selection takes: its symbol, sheet or label.
static SCH_ITEM* topLevelItem( SCH_ITEM* aItem )
{
    EDA_ITEM* parent = aItem->GetParent();

    if( parent && parent->IsSCH_ITEM() && parent->Type() != SCH_SCREEN_T )
        return static_cast<SCH_ITEM*>( parent );

    return aItem;
}


/**
 * Keeps the view and the selection of a SCHEMATIC_DOCUMENT in step with the schematic when a
 * commit is pushed, and records its changes for undo.
 */
class SCHEMATIC_DOCUMENT::COMMIT_LISTENER : public SCH_COMMIT_LISTENER
{
public:
    explicit COMMIT_LISTENER( SCHEMATIC_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    SCH_GROUP* EnteredGroup() const override { return nullptr; }

    SCH_SCREEN* CurrentScreen() const override { return m_doc.currentScreen(); }

    bool GetCurrentSheet( SCH_SHEET_PATH& aSheet ) const override
    {
        aSheet = m_doc.m_schematic->CurrentSheet();
        return true;
    }

    void UpdateHopOveredWires( SCH_ITEM* aItem ) override
    {
        m_doc.view()->UpdateHopOveredWires( aItem );
    }

    void ItemAdded( SCH_ITEM* aItem ) override { m_doc.view()->Add( aItem ); }

    void ItemRemoved( SCH_ITEM* aItem ) override { m_doc.view()->Remove( aItem ); }

    void UpdateItem( SCH_ITEM* aItem, SCH_SCREEN* aScreen, bool aAddOrDelete ) override
    {
        if( aScreen == m_doc.currentScreen() )
            m_doc.redrawItem( aItem, aAddOrDelete, true );
        else
            aScreen->Update( aItem );
    }

    void Deselect( SCH_ITEM* aItem ) override { m_doc.forgetSelected( aItem ); }

    void HierarchyChanged() override { m_doc.fillSheets(); }

    void Pushed( SCH_COMMIT& aCommit, PICKED_ITEMS_LIST& aUndoList, int aCommitFlags,
                 const PUSH_RESULT& aResult ) override
    {
        if( !( aCommitFlags & SKIP_UNDO ) && aUndoList.GetCount() > 0 )
            m_doc.recordChange( aUndoList );

        if( aResult.dirtyConnectivity )
        {
            m_doc.recalculateConnections( &aCommit,
                                          static_cast<SCH_CLEANUP_FLAGS>( aResult.connectivityCleanUp ) );
        }
    }

    void Reverted() override { m_doc.recalculateConnections( nullptr, NO_CLEANUP ); }

    // The shell does not edit library symbols: its commits are not symbol editor ones

    LIB_SYMBOL* CurrentSymbol() const override { return nullptr; }

    void SymbolPushed( LIB_SYMBOL* aSymbol, LIB_SYMBOL* aCopy, const wxString& aMessage,
                       int aCommitFlags ) override
    {
        delete aCopy;
    }

    void SymbolReverted( LIB_SYMBOL* aCopy ) override { delete aCopy; }

    void Committed( int aCommitFlags ) override {}

private:
    SCHEMATIC_DOCUMENT& m_doc;
};


/// Keeps the view of a SCHEMATIC_DOCUMENT in step with the schematic during an undo or a redo.
class SCHEMATIC_DOCUMENT::UNDO_LISTENER : public SCH_UNDO_REDO::LISTENER
{
public:
    explicit UNDO_LISTENER( SCHEMATIC_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    const SCH_SHEET_PATH& CurrentSheet() const override { return m_doc.m_schematic->CurrentSheet(); }

    void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        SCH_SCREEN* screen = aScreen ? aScreen : m_doc.currentScreen();

        if( aItem->Type() != SCH_TABLECELL_T )
            screen->Append( static_cast<SCH_ITEM*>( aItem ) );

        if( screen == m_doc.currentScreen() )
        {
            m_doc.view()->Add( aItem );
            m_doc.redrawItem( aItem, true, false );
        }
    }

    void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        SCH_SCREEN* screen = aScreen ? aScreen : m_doc.currentScreen();
        const bool  shown = screen == m_doc.currentScreen();

        if( shown )
            m_doc.view()->Remove( aItem );

        if( aItem->Type() != SCH_TABLECELL_T )
            screen->Remove( static_cast<SCH_ITEM*>( aItem ) );

        if( shown )
            m_doc.redrawItem( aItem, true, false );
    }

    void SheetRemoving( SCH_SHEET* aSheet ) override
    {
        // The sheet shown leaves the hierarchy: the document shows the first page after
        // the undo or the redo
        if( aSheet->GetScreen() == m_doc.currentScreen() )
            m_doc.m_leaveSheet = true;
    }

    void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem, const SCH_SHEET_PATH& aSheet ) override
    {
        // The shell does not change the page settings: its changes have no such entries
    }

    // The shell does not repeat items (the insert key of the schematic editor)
    void ClearRepeatItems() override {}
    void AddRepeatItem( SCH_ITEM* aItem ) override {}

private:
    SCHEMATIC_DOCUMENT& m_doc;
};


QT_DOCUMENT* CreateQtSchematicDocument( QWidget* aParent )
{
    return new SCHEMATIC_DOCUMENT( aParent );
}


SCHEMATIC_DOCUMENT::SCHEMATIC_DOCUMENT( QWidget* aParent ) :
        QT_DOCUMENT( aParent ),
        m_undo( std::make_unique<UNDO_STACK>( &SCH_UNDO_REDO::DeleteItems ) )
{
    m_canvas = new GAL_CANVAS;
    m_sheets = new QTreeWidget;
    m_properties = new PROPERTY_PANEL;

    m_sheets->setColumnCount( 2 );
    m_sheets->setHeaderLabels( { tr( "Page" ), tr( "Sheet" ) } );
    m_sheets->header()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    m_sheets->setRootIsDecorated( false );
    m_sheets->setUniformRowHeights( true );

    auto* side = new QTabWidget;
    side->addTab( m_sheets, tr( "Sheets" ) );
    side->addTab( m_properties, tr( "Properties" ) );

    auto* splitter = new QSplitter( Qt::Horizontal );
    splitter->addWidget( m_canvas );
    splitter->addWidget( side );
    splitter->setStretchFactor( 0, 1 );
    splitter->setStretchFactor( 1, 0 );
    splitter->setSizes( { 900, 280 } );

    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( splitter );

    setupEditing( m_canvas );

    addTool( tr( "Draw Wire" ), QKeySequence( Qt::Key_W ), std::make_unique<WIRE_TOOL>( *this ) );
    addTool( tr( "Place Label" ), QKeySequence( Qt::Key_L ), std::make_unique<LABEL_TOOL>( *this ) );

    connect( m_canvas, &GAL_CANVAS::cursorMoved, this, &SCHEMATIC_DOCUMENT::onCursorMoved );
    connect( m_properties, &PROPERTY_PANEL::valueEdited, this, &SCHEMATIC_DOCUMENT::onPropertyEdited );
    connect( m_canvas, &GAL_CANVAS::zoomToFitRequested, this, &SCHEMATIC_DOCUMENT::ZoomToFit );
    connect( m_sheets, &QTreeWidget::itemActivated, this,
             &SCHEMATIC_DOCUMENT::onSheetActivated );
}


SCHEMATIC_DOCUMENT::~SCHEMATIC_DOCUMENT()
{
    // The tool and the move in progress drop their work while the view is there
    setTool( nullptr );
    endMove( false );

    // The undo entries own items that refer to the schematic; the view refers to the items
    // of the schematic
    m_undo->Clear();

    delete m_canvas;
    m_canvas = nullptr;
}


KIGFX::SCH_VIEW* SCHEMATIC_DOCUMENT::view() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_canvas->GetView() );
}


SCH_SCREEN* SCHEMATIC_DOCUMENT::currentScreen() const
{
    return m_schematic->GetCurrentScreen();
}


bool SCHEMATIC_DOCUMENT::Open( const QString& aFileName, QString& aError )
{
    SCHEMATIC* schematic = EESCHEMA_HELPERS::LoadSchematic( ToWxString( aFileName ), true, false );

    if( !schematic )
    {
        aError = tr( "The schematic could not be loaded." );
        return false;
    }

    dropSelection();
    m_undo->Clear();
    m_schematic.reset( schematic );
    m_fileName = aFileName;

    KIGFX::GAL* gal = m_canvas->GetGAL();
    gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    auto view = std::make_unique<KIGFX::SCH_VIEW>( this );
    auto painter = std::make_unique<KIGFX::SCH_PAINTER>( gal );

    painter->SetSchematic( m_schematic.get() );

    // As the schematic editor draws the schematic
    SCH_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->LoadColors( schematicColors() );
    settings->m_ShowPinsElectricalType = false;
    settings->m_ShowPinNumbers = false;

    if( EESCHEMA_SETTINGS* cfg = eeschemaSettings() )
    {
        settings->m_ShowPinAltIcons = cfg->m_Appearance.show_pin_alt_icons;
        settings->SetDefaultFont( cfg->m_Appearance.default_font );
    }

    view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );

    KIGFX::SCH_VIEW* schView = view.get();
    m_canvas->SetView( std::move( view ), std::move( painter ) );

    schView->SetMirror( false, false );

    schView->SetDefaultLayerOrder();
    schView->SetDefaultLayerDeps( false );      // the Cairo GAL does not cache
    schView->UpdateAllLayersOrder();

    setupGrid();
    fillSheets();
    showSheet( 0 );

    emit statusMessage( tr( "%1: %2 sheets" ).arg( aFileName ).arg( m_sheets->topLevelItemCount() ) );
    emit stateChanged();
    return true;
}


bool SCHEMATIC_DOCUMENT::Save( QString& aError )
{
    if( !m_schematic )
        return false;

    // The files of the sheets changed, as the schematic editor saves them
    SCH_SCREENS screens( m_schematic->Root() );

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        SCH_SCREEN* screen = screens.GetScreen( i );
        SCH_SHEET*  sheet = screens.GetSheet( i );

        if( !screen || !sheet || !screen->IsContentModified() )
            continue;

        const wxString fileName = m_schematic->Project().AbsolutePath( screen->GetFileName() );

        SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::GuessPluginTypeFromSchPath( fileName );

        if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
            type = SCH_IO_MGR::SCH_KICAD;

        try
        {
            IO_RELEASER<SCH_IO> io( SCH_IO_MGR::FindPlugin( type ) );
            io->SaveSchematicFile( fileName, sheet, m_schematic.get() );
        }
        catch( const IO_ERROR& error )
        {
            aError = tr( "The schematic could not be saved to %1.\n%2" )
                             .arg( ToQString( fileName ), ToQString( error.What() ) );
            return false;
        }

        screen->SetContentModified( false );
    }

    m_undo->SetSaved();
    emit statusMessage( tr( "%1 saved." ).arg( m_fileName ) );
    emit stateChanged();
    return true;
}


bool SCHEMATIC_DOCUMENT::IsModified() const
{
    return m_undo->IsModified();
}


bool SCHEMATIC_DOCUMENT::CanUndo() const
{
    return m_undo->CanUndo();
}


bool SCHEMATIC_DOCUMENT::CanRedo() const
{
    return m_undo->CanRedo();
}


bool SCHEMATIC_DOCUMENT::GetCurrentSheetInfo( SCH_CURRENT_SHEET_INFO& aInfo ) const
{
    if( !m_schematic )
        return false;

    const SCH_SHEET_PATH& sheet = m_schematic->CurrentSheet();

    aInfo.m_Screen = sheet.LastScreen();
    aInfo.m_SheetScreen = sheet.LastScreen();
    aInfo.m_VirtualPageNumber = sheet.GetVirtualPageNumber();
    aInfo.m_Name = sheet.Last()->GetName();
    aInfo.m_Path = sheet.PathHumanReadable();
    return true;
}


void SCHEMATIC_DOCUMENT::setupGrid()
{
    m_gridSize = VECTOR2D( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ) );

    const GRID_SETTINGS* settings = nullptr;

    if( auto* cfg = eeschemaSettings() )
    {
        settings = &cfg->m_Window.grid;

        if( settings->last_size_idx >= 0
                && settings->last_size_idx < static_cast<int>( settings->grids.size() ) )
        {
            m_gridSize = settings->grids[settings->last_size_idx].ToDouble( schIUScale );
        }
    }

    m_canvas->SetGrid( m_gridSize, VECTOR2D( 0, 0 ), settings );
}


void SCHEMATIC_DOCUMENT::fillSheets()
{
    const QSignalBlocker blocker( m_sheets );
    m_sheets->clear();

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    for( size_t i = 0; i < sheets.size(); ++i )
    {
        auto* item = new QTreeWidgetItem( m_sheets );
        item->setText( 0, ToQString( sheets[i].GetPageNumber() ) );
        item->setText( 1, ToQString( sheets[i].PathHumanReadable( false, true ) ) );
        item->setData( 0, PAGE_ROLE, static_cast<int>( i ) );

        if( sheets[i] == m_schematic->CurrentSheet() )
            m_sheets->setCurrentItem( item );
    }
}


void SCHEMATIC_DOCUMENT::showSheet( int aPage )
{
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    if( aPage < 0 || aPage >= static_cast<int>( sheets.size() ) )
        return;

    ClearSelection();

    // As the editor shows a sheet: its page number and count, the references of its symbols
    // (units of shared screens), the dangling ends
    m_schematic->SetCurrentSheet( sheets[aPage] );
    m_schematic->SetSheetNumberAndCount();
    m_schematic->CurrentSheet().UpdateAllScreenReferences();
    m_schematic->CurrentSheet().LastScreen()->TestDanglingEnds();

    KIGFX::SCH_VIEW* schView = view();
    schView->Clear();
    schView->DisplaySheet( sheets[aPage].LastScreen() );

    if( QTreeWidgetItem* item = m_sheets->topLevelItem( aPage ) )
    {
        const QSignalBlocker blocker( m_sheets );
        m_sheets->setCurrentItem( item );
    }

    ZoomToFit();
    m_canvas->Refresh();
}


void SCHEMATIC_DOCUMENT::onSheetActivated( QTreeWidgetItem* aItem )
{
    if( aItem && m_schematic )
        showSheet( aItem->data( 0, PAGE_ROLE ).toInt() );
}


void SCHEMATIC_DOCUMENT::ZoomToFit()
{
    if( !m_schematic || !view() )
        return;

    // The page of the drawing sheet, or the items without it
    if( DS_PROXY_VIEW_ITEM* drawingSheet = view()->GetDrawingSheet() )
    {
        m_canvas->ZoomToFit( drawingSheet->ViewBBox() );
        return;
    }

    BOX2I box;

    for( SCH_ITEM* item : currentScreen()->Items() )
        box.Merge( item->GetBoundingBox() );

    m_canvas->ZoomToFit( box );
}


void SCHEMATIC_DOCUMENT::onCursorMoved( double aX, double aY )
{
    emit cursorPositionChanged( QStringLiteral( "X %1  Y %2 mm" )
                                        .arg( schIUScale.IUTomm( aX ), 0, 'f', 2 )
                                        .arg( schIUScale.IUTomm( aY ), 0, 'f', 2 ) );
}


EDA_ITEM* SCHEMATIC_DOCUMENT::itemAt( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    if( !m_schematic )
        return nullptr;

    KIGFX::SCH_VIEW* schView = view();

    const int accuracy = static_cast<int>( schView->ToWorld( HIT_DISTANCE_PX ) );
    BOX2I     area( aPoint - VECTOR2I( accuracy, accuracy ), VECTOR2I( 2 * accuracy,
                                                                       2 * accuracy ) );

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> found;
    schView->Query( area, found );

    SCH_ITEM* best = nullptr;
    int       bestOrder = std::numeric_limits<int>::max();

    for( const auto& [viewItem, layer] : found )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( viewItem );

        if( !item || !schView->IsLayerVisible( layer ) || !schView->IsVisible( item ) )
            continue;

        if( !item->HitTest( aPoint, accuracy ) )
            continue;

        // The view draws the layers of lower order on top
        int order = schView->GetLayerOrder( layer );

        if( order < bestOrder )
        {
            best = item;
            bestOrder = order;
        }
    }

    // A click on a field or a pin of a symbol selects the symbol, as in the editor; Ctrl
    // selects the field or the pin
    if( best && !( aModifiers & Qt::ControlModifier ) )
    {
        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( best->GetParentSymbol() ) )
            best = symbol;
    }

    return best;
}


std::vector<EDA_ITEM*> SCHEMATIC_DOCUMENT::itemsInArea( const BOX2I& aArea, bool aCrossing )
{
    std::vector<EDA_ITEM*> items;

    if( !m_schematic )
        return items;

    KIGFX::SCH_VIEW* schView = view();

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> found;
    schView->Query( aArea, found );

    std::unordered_set<SCH_ITEM*> seen;

    for( const auto& [viewItem, layer] : found )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( viewItem );

        if( !item || !schView->IsLayerVisible( layer ) || !schView->IsVisible( item ) )
            continue;

        // An area selects whole symbols, sheets and labels
        item = topLevelItem( item );

        if( !seen.insert( item ).second )
            continue;

        if( item->HitTest( aArea, !aCrossing ) )
            items.push_back( item );
    }

    return items;
}


std::vector<EDA_ITEM*> SCHEMATIC_DOCUMENT::allItems()
{
    std::vector<EDA_ITEM*> items;

    if( m_schematic )
    {
        for( SCH_ITEM* item : currentScreen()->Items() )
            items.push_back( item );
    }

    return items;
}


void SCHEMATIC_DOCUMENT::setItemSelected( EDA_ITEM* aItem, bool aSelected )
{
    if( aSelected )
        aItem->SetSelected();
    else
        aItem->ClearSelected();

    view()->UpdateWithParent( aItem, false );
}


void SCHEMATIC_DOCUMENT::selectionChanged()
{
    const std::vector<EDA_ITEM*>& selection = Selection();
    QString                       title;

    if( selection.size() == 1 )
        title = ToQString( selection.front()->GetFriendlyName() );
    else if( selection.size() > 1 )
        title = tr( "%1 items" ).arg( selection.size() );

    m_properties->ShowProperties( title, PropertyRows( selection, schIUScale, EDA_UNITS::MM ) );
}


void SCHEMATIC_DOCUMENT::onPropertyEdited( const void* aKey, const QString& aValue )
{
    if( Selection().empty() )
        return;

    auto    commit = makeCommit();
    QString error;

    for( EDA_ITEM* item : Selection() )
    {
        commit->Modify( item, currentScreen() );

        if( !SetPropertyValue( item, aKey, aValue, schIUScale, EDA_UNITS::MM, error ) )
        {
            commit->Revert();
            emit statusMessage( error );
            selectionChanged();     // the values as they are
            return;
        }
    }

    commit->Push( ToWxString( tr( "Edit Property" ) ) );

    selectionChanged();
    m_canvas->update();
    emit stateChanged();
}


VECTOR2I SCHEMATIC_DOCUMENT::snapToGrid( const VECTOR2D& aDelta ) const
{
    return VECTOR2I( KiROUND( std::round( aDelta.x / m_gridSize.x ) * m_gridSize.x ),
                     KiROUND( std::round( aDelta.y / m_gridSize.y ) * m_gridSize.y ) );
}


VECTOR2I SCHEMATIC_DOCUMENT::snapPoint( const VECTOR2D& aPoint ) const
{
    // The grid of the schematics starts at (0, 0)
    return snapToGrid( aPoint );
}


std::unique_ptr<SCH_COMMIT> SCHEMATIC_DOCUMENT::MakeCommit()
{
    return makeCommit();
}


std::unique_ptr<SCH_COMMIT> SCHEMATIC_DOCUMENT::makeCommit()
{
    return std::make_unique<SCH_COMMIT>( std::make_unique<COMMIT_LISTENER>( *this ) );
}


std::vector<SCH_ITEM*> SCHEMATIC_DOCUMENT::changeableItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::unordered_set<EDA_ITEM*> all( aItems.begin(), aItems.end() );
    std::vector<SCH_ITEM*>        items;

    for( EDA_ITEM* item : aItems )
    {
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        if( schItem->IsLocked() )
            continue;

        // A child changes with its symbol, sheet or label
        if( topLevelItem( schItem ) != schItem && all.count( topLevelItem( schItem ) ) )
            continue;

        items.push_back( schItem );
    }

    if( items.size() < aItems.size() )
        emit statusMessage( tr( "Locked items are not changed." ) );

    return items;
}


void SCHEMATIC_DOCUMENT::redrawItem( EDA_ITEM* aItem, bool aIsAddOrDelete, bool aUpdateRtree )
{
    view()->UpdateWithParent( aItem, aIsAddOrDelete );

    if( !aItem->IsSCH_ITEM() )
        return;

    SCH_ITEM* item = static_cast<SCH_ITEM*>( aItem );

    // Update will invalidate the RTree iterators of the screen
    if( aUpdateRtree )
    {
        currentScreen()->Update( item );

        // The children of a group are in the RTree too
        if( SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( item ) )
        {
            group->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        currentScreen()->Update( aChild );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    item->ClearCaches();
}


bool SCHEMATIC_DOCUMENT::beginMove( bool aWithConnections )
{
    m_moving = changeableItems( Selection() );

    if( m_moving.empty() )
        return false;

    m_moveCommit = makeCommit();

    for( SCH_ITEM* item : m_moving )
        m_moveCommit->Modify( item, currentScreen() );

    if( aWithConnections )
        findConnectedWires();

    return true;
}


void SCHEMATIC_DOCUMENT::findConnectedWires()
{
    m_followingEnds.clear();
    m_dragged = VECTOR2I( 0, 0 );

    std::unordered_set<SCH_ITEM*> moving( m_moving.begin(), m_moving.end() );
    std::vector<VECTOR2I>         points;

    // The points the moving items connect at: pins, wire ends, label anchors...
    for( SCH_ITEM* item : m_moving )
    {
        for( const VECTOR2I& point : item->GetConnectionPoints() )
            points.push_back( point );
    }

    auto connected =
            [&]( const VECTOR2I& aPoint )
            {
                return std::find( points.begin(), points.end(), aPoint ) != points.end();
            };

    std::vector<SCH_ITEM*> joined;

    for( SCH_ITEM* item : currentScreen()->Items() )
    {
        if( moving.count( item ) || item->IsLocked() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
        {
            // A junction on a moving pin or wire end goes with it
            if( connected( item->GetPosition() ) )
                joined.push_back( item );

            continue;
        }

        SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );

        if( !line || !( line->IsWire() || line->IsBus() ) )
            continue;

        const bool start = connected( line->GetStartPoint() );
        const bool end = connected( line->GetEndPoint() );

        if( start && end )
        {
            joined.push_back( line );
        }
        else if( start || end )
        {
            m_followingEnds.push_back( { line, start,
                                         start ? line->GetStartPoint() : line->GetEndPoint(),
                                         start ? line->GetEndPoint() : line->GetStartPoint() } );
        }
    }

    for( SCH_ITEM* item : joined )
    {
        m_moveCommit->Modify( item, currentScreen() );
        m_moving.push_back( item );
    }

    for( const WIRE_END& end : m_followingEnds )
        m_moveCommit->Modify( end.m_Wire, currentScreen() );
}


void SCHEMATIC_DOCUMENT::moveBy( const VECTOR2I& aDelta )
{
    for( SCH_ITEM* item : m_moving )
    {
        item->Move( aDelta );

        // The pasted items are drawn by the preview of the view: they are not on the screen
        if( !m_pasting )
            redrawItem( item, false, true );
    }

    m_dragged += aDelta;

    for( WIRE_END& end : m_followingEnds )
    {
        const VECTOR2I target = end.m_Origin + m_dragged;
        VECTOR2I       bend = target;

        // A horizontal or vertical wire keeps its direction; the jog goes to the moved point
        if( end.m_Fixed.y == end.m_Origin.y )
            bend = VECTOR2I( target.x, end.m_Fixed.y );
        else if( end.m_Fixed.x == end.m_Origin.x )
            bend = VECTOR2I( end.m_Fixed.x, target.y );

        if( end.m_Start )
            end.m_Wire->SetStartPoint( bend );
        else
            end.m_Wire->SetEndPoint( bend );

        if( bend != target && !end.m_Jog )
        {
            end.m_Jog = new SCH_LINE( bend, end.m_Wire->GetLayer() );
            end.m_Jog->SetParent( currentScreen() );
            view()->AddToPreview( end.m_Jog, false );
        }

        if( end.m_Jog )
        {
            end.m_Jog->SetStartPoint( bend );
            end.m_Jog->SetEndPoint( target );
        }

        redrawItem( end.m_Wire, false, true );
    }

    if( m_pasting || !m_followingEnds.empty() )
        view()->MarkDirty();
}


void SCHEMATIC_DOCUMENT::endMove( bool aCommit )
{
    if( m_pasting )
    {
        // The pasted items go to the screen shown, or are dropped
        m_pasting = false;
        view()->ClearPreview();

        if( aCommit )
        {
            std::unique_ptr<SCH_COMMIT> commit = makeCommit();

            for( SCH_ITEM* item : m_moving )
                commit->Add( item, currentScreen() );

            commit->Push( ToWxString( tr( "Paste" ) ) );

            std::vector<EDA_ITEM*> pasted( m_moving.begin(), m_moving.end() );
            m_moving.clear();
            setSelection( pasted );
        }
        else
        {
            for( SCH_ITEM* item : m_moving )
                delete item;

            m_moving.clear();
        }

        m_canvas->update();
        return;
    }

    if( !m_moveCommit )
        return;

    // The jogs of the wires join the drag, or are dropped
    view()->ClearPreview();

    for( const WIRE_END& end : m_followingEnds )
    {
        if( !end.m_Jog )
            continue;

        if( aCommit && end.m_Jog->GetStartPoint() != end.m_Jog->GetEndPoint() )
            m_moveCommit->Add( end.m_Jog, currentScreen() );
        else
            delete end.m_Jog;
    }

    if( aCommit )
        m_moveCommit->Push( ToWxString( m_followingEnds.empty() ? tr( "Move" ) : tr( "Drag" ) ) );
    else
        m_moveCommit->Revert();

    m_moveCommit.reset();
    m_moving.clear();
    m_followingEnds.clear();
    m_canvas->update();
}


QString SCHEMATIC_DOCUMENT::copyItems( const std::vector<EDA_ITEM*>& aItems )
{
    if( !m_schematic || aItems.empty() )
        return QString();

    SCH_SELECTION selection( currentScreen() );

    for( EDA_ITEM* item : aItems )
        selection.Add( item );

    STRING_FORMATTER   formatter;
    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET_PATH     sheet = m_schematic->CurrentSheet();

    io.Format( &selection, &sheet, *m_schematic, &formatter, true );

    return QString::fromStdString( formatter.GetString() );
}


bool SCHEMATIC_DOCUMENT::beginPaste( const QString& aText, const VECTOR2I& aPosition )
{
    if( !m_schematic )
        return false;

    // The items of the clipboard, in a sheet of their own
    const std::string  text = aText.toStdString();
    STRING_LINE_READER reader( text, wxT( "Clipboard" ) );
    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET          clipSheet;
    SCH_SCREEN*        clipScreen = new SCH_SCREEN( m_schematic.get() );

    clipSheet.SetScreen( clipScreen );

    try
    {
        io.LoadContent( reader, &clipSheet );
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    clipScreen->MigrateSimModels();

    std::vector<SCH_ITEM*> items;
    bool                   sheetsDropped = false;

    for( SCH_ITEM* item : clipScreen->Items() )
        items.push_back( item );

    // The items leave the clipboard screen
    clipScreen->Clear( false );

    // The references in the schematic: a pasted symbol keeps its reference if it is new
    const wxString     projectName = m_schematic->Project().GetProjectName();
    SCH_SHEET_LIST     hierarchy = m_schematic->Hierarchy();
    SCH_SHEET_LIST     pastePaths = hierarchy.FindAllSheetsForScreen( currentScreen() );
    SCH_REFERENCE_LIST existingRefs;
    std::set<wxString> existing;

    hierarchy.GetSymbols( existingRefs, SYMBOL_FILTER_ALL );

    for( const SCH_REFERENCE& ref : existingRefs )
        existing.insert( ref.GetRef() );

    std::vector<SCH_ITEM*> pasted;

    for( SCH_ITEM* item : items )
    {
        // The shell pastes the items of a sheet, not sheets (their files and hierarchy)
        if( item->Type() == SCH_SHEET_T )
        {
            sheetsDropped = true;
            delete item;
            continue;
        }

        // The clipboard screen goes away: the items reach the schematic through this one
        item->SetParent( currentScreen() );

        // New items: new UUIDs, for them and their fields and pins
        const_cast<KIID&>( item->m_Uuid ) = KIID();
        item->RunOnChildren( []( SCH_ITEM* aChild ) { const_cast<KIID&>( aChild->m_Uuid ) = KIID(); },
                             RECURSE_MODE::RECURSE );

        if( item->IsConnectable() )
            item->SetConnectivityDirty();

        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            if( const LIB_SYMBOL* source = SCH_PASTE::ChooseLibSymbol(
                        clipScreen, currentScreen(), symbol->GetSchSymbolLibraryName() ) )
            {
                symbol->SetLibSymbol( new LIB_SYMBOL( *source ) );
            }

            bool keepAnnotation = true;

            for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
            {
                if( existing.count( instance.m_Reference ) )
                    keepAnnotation = false;
            }

            for( const SCH_SHEET_PATH& path : pastePaths )
            {
                SCH_PASTE::UpdateSymbolInstance( symbol, path, KIID_PATH( wxT( "/" ) ), projectName,
                                                 keepAnnotation );
            }

            SCH_PASTE::PruneSymbolInstances( symbol, projectName );

            for( SCH_PIN* pin : symbol->GetPins() )
                pin->SetConnectivityDirty();
        }

        pasted.push_back( item );
    }

    if( sheetsDropped )
        emit statusMessage( tr( "The sheets of the clipboard are not pasted." ) );

    if( pasted.empty() )
        return false;

    // Around the cursor: the center of their box on the grid goes to it
    BOX2I box;

    for( SCH_ITEM* item : pasted )
        box.Merge( item->GetBoundingBox() );

    const VECTOR2I offset = aPosition - snapPoint( VECTOR2D( box.GetCenter() ) );

    for( SCH_ITEM* item : pasted )
    {
        item->Move( offset );
        view()->AddToPreview( item, false );
    }

    m_moving = std::move( pasted );
    m_pasting = true;
    return true;
}


void SCHEMATIC_DOCUMENT::rotateItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::vector<SCH_ITEM*> items = changeableItems( aItems );

    if( items.empty() )
        return;

    // A single item turns about its position (the anchor of a symbol), several items about
    // the center of their box, on the grid
    VECTOR2I center;

    if( items.size() == 1 )
    {
        center = items.front()->GetPosition();
    }
    else
    {
        BOX2I box;

        for( SCH_ITEM* item : items )
            box.Merge( item->GetBoundingBox() );

        center = snapPoint( VECTOR2D( box.GetCenter() ) );
    }

    std::unique_ptr<SCH_COMMIT> commit = makeCommit();

    for( SCH_ITEM* item : items )
    {
        commit->Modify( item, currentScreen() );
        item->Rotate( center, true /* counterclockwise */ );
    }

    commit->Push( ToWxString( tr( "Rotate" ) ) );
}


void SCHEMATIC_DOCUMENT::deleteItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::unique_ptr<SCH_COMMIT> commit = makeCommit();
    bool                        removed = false;

    for( SCH_ITEM* item : changeableItems( aItems ) )
    {
        // The fields and the pins of a symbol are deleted with it
        if( item->Type() == SCH_FIELD_T || item->Type() == SCH_PIN_T
                || item->Type() == SCH_TABLECELL_T )
        {
            continue;
        }

        commit->Remove( item, currentScreen() );
        removed = true;
    }

    if( removed )
        commit->Push( ToWxString( tr( "Delete" ) ) );
}


void SCHEMATIC_DOCUMENT::markScreensModified( const PICKED_ITEMS_LIST& aEntry )
{
    for( unsigned ii = 0; ii < aEntry.GetCount(); ii++ )
    {
        if( SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( aEntry.GetScreenForItem( ii ) ) )
            screen->SetContentModified();
    }
}


void SCHEMATIC_DOCUMENT::recordChange( PICKED_ITEMS_LIST& aChange )
{
    auto* entry = new PICKED_ITEMS_LIST();
    entry->CopyList( aChange );
    entry->SetDescription( aChange.GetDescription() );

    markScreensModified( *entry );
    m_undo->Push( entry );
}


void SCHEMATIC_DOCUMENT::recalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanUp )
{
    KIGFX::SCH_VIEW* schView = view();

    std::function<void( SCH_ITEM* )> changed =
            [schView]( SCH_ITEM* aItem )
            {
                schView->Update( aItem, KIGFX::REPAINT );
            };

    m_schematic->RecalculateConnections( aCommit, aCleanUp, nullptr, schView, &changed,
                                         m_undo->LastChange() );

    schView->UpdateConnectionChanges( currentScreen() );
}


void SCHEMATIC_DOCUMENT::applyUndoEntry( PICKED_ITEMS_LIST& aEntry )
{
    UNDO_LISTENER listener( *this );

    SCH_UNDO_REDO::RESULT result = SCH_UNDO_REDO::Apply( *m_schematic, aEntry, listener );

    markScreensModified( aEntry );
    view()->ClearHiddenFlags();

    if( result.connectivityChanged )
    {
        // As in the schematic editor, the clean up of the connections is not pushed: it
        // belongs to the change undone or redone
        SCH_COMMIT cleanUp( std::make_unique<COMMIT_LISTENER>( *this ) );
        recalculateConnections( &cleanUp, result.connectivityCleanUp );

        if( result.connectivityCleanUp == GLOBAL_CLEANUP )
            m_schematic->SetSheetNumberAndCount();

        // Restore the hop-over shapes of the wires
        if( m_schematic->Settings().GetHopOverScale() > 0.0 )
        {
            for( SCH_ITEM* item : currentScreen()->Items() )
            {
                if( item->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( line->IsWire() || line->IsBus() )
                    view()->UpdateHopOveredWires( line );
            }
        }
    }

    if( result.hierarchyChanged )
        fillSheets();

    if( result.variantsChanged )
        m_schematic->LoadVariants();
}


void SCHEMATIC_DOCUMENT::undo()
{
    m_undo->Undo( [this]( PICKED_ITEMS_LIST& aEntry ) { applyUndoEntry( aEntry ); } );

    if( std::exchange( m_leaveSheet, false ) )
        showSheet( 0 );
}


void SCHEMATIC_DOCUMENT::redo()
{
    m_undo->Redo( [this]( PICKED_ITEMS_LIST& aEntry ) { applyUndoEntry( aEntry ); } );

    if( std::exchange( m_leaveSheet, false ) )
        showSheet( 0 );
}
