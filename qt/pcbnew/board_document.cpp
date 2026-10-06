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

#include "board_document.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

#include <QKeySequence>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include <base_units.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board_undo_redo.h>
#include <kicad_clipboard.h>
#include <footprint.h>
#include <pad.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_board_outline.h>
#include <pcb_display_options.h>
#include <pcb_painter.h>
#include <pcb_track.h>
#include <pcb_view.h>
#include <pcbnew_scripting_helpers.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <zone.h>
#include <zoom_defines.h>

#include <gal_canvas.h>
#include <inspectable_rows.h>
#include <property_panel.h>
#include <qt_strings.h>
#include <undo_stack.h>

#include "board_tools.h"


/// Role of the layer list items holding the PCB_LAYER_ID
static constexpr int LAYER_ROLE = Qt::UserRole;

/// Distance of a click to an item that still hits it, in pixels
static constexpr double HIT_DISTANCE_PX = 4.0;


/// A color as a QColor.
static QColor toQColor( const KIGFX::COLOR4D& aColor )
{
    return QColor::fromRgbF( aColor.r, aColor.g, aColor.b, aColor.a );
}


/// The settings of the PCB editor, if any.
static PCBNEW_SETTINGS* pcbnewSettings()
{
    return GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
}


/// The color theme of the PCB editor.
static COLOR_SETTINGS* boardColors()
{
    if( PCBNEW_SETTINGS* cfg = pcbnewSettings() )
        return ::GetColorSettings( cfg->m_ColorTheme );

    return ::GetColorSettings( DEFAULT_THEME );
}


/// True if @a aItem is an item of the board the user can select.
static bool isSelectable( const BOARD_ITEM* aItem )
{
    return aItem->Type() != PCB_BOARD_OUTLINE_T && aItem->Type() != PCB_NETINFO_T;
}


/**
 * Keeps the view and the selection of a BOARD_DOCUMENT in step with the board when a commit
 * is pushed, and records its changes for undo.
 */
class BOARD_DOCUMENT::COMMIT_LISTENER : public BOARD_COMMIT_LISTENER
{
public:
    explicit COMMIT_LISTENER( BOARD_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    BOARD* Board() const override { return m_doc.m_board.get(); }

    PCB_GROUP* EnteredGroup() const override { return nullptr; }

    bool AutoRefillZones() const override { return false; }

    void ItemAdded( BOARD_ITEM* aItem ) override { m_doc.view()->Add( aItem ); }

    void ItemRemoved( BOARD_ITEM* aItem ) override { m_doc.view()->Remove( aItem ); }

    void ItemChanged( BOARD_ITEM* aItem ) override { m_doc.view()->Update( aItem ); }

    void Deselect( BOARD_ITEM* aItem ) override { m_doc.forgetSelected( aItem ); }

    bool PruneSelection( const std::unordered_set<EDA_ITEM*>& aRemoved ) override
    {
        bool pruned = false;

        for( EDA_ITEM* selected : std::vector<EDA_ITEM*>( m_doc.Selection() ) )
        {
            for( EDA_ITEM* ancestor = selected; ancestor; ancestor = ancestor->GetParent() )
            {
                if( aRemoved.count( ancestor ) )
                {
                    m_doc.forgetSelected( selected );
                    pruned = true;
                    break;
                }
            }
        }

        return pruned;
    }

    // The shell shows neither the ratsnest nor the solder mask of the board editor, nor the
    // clearances of the rule areas
    void RatsnestChanged() override {}
    void SolderMaskChanged() override {}
    void RuleAreasChanged( const std::vector<BOX2I>& aAreas ) override {}

    void BoardOutlineChanged() override { m_doc.updateBoardOutline(); }

    void Pushed( PICKED_ITEMS_LIST& aUndoList, int aCommitFlags,
                 const PUSH_RESULT& aResult ) override
    {
        if( !( aCommitFlags & SKIP_UNDO ) )
            m_doc.recordChange( aUndoList );
    }

    void Reverted() override {}

private:
    BOARD_DOCUMENT& m_doc;
};


/// Keeps the view of a BOARD_DOCUMENT in step with the board during an undo or a redo.
class BOARD_DOCUMENT::UNDO_LISTENER : public BOARD_UNDO_REDO::LISTENER
{
public:
    explicit UNDO_LISTENER( BOARD_DOCUMENT& aDocument ) :
            m_doc( aDocument )
    {
    }

    void ItemRemoved( EDA_ITEM* aItem ) override { m_doc.view()->Remove( aItem ); }

    void ItemAdded( EDA_ITEM* aItem ) override
    {
        m_doc.view()->Add( aItem );
        m_doc.view()->Hide( aItem, false );
    }

    bool ShowRatsnest() const override { return false; }

    void SetOrigin( UNDO_REDO aOrigin, EDA_ITEM* aOriginItem, const VECTOR2I& aPosition ) override
    {
        BOARD_DESIGN_SETTINGS& settings = m_doc.m_board->GetDesignSettings();

        if( aOrigin == UNDO_REDO::DRILLORIGIN )
            settings.SetAuxOrigin( aPosition );
        else
            settings.SetGridOrigin( aPosition );
    }

    void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem ) override
    {
        // The shell does not change the page settings: its changes have no such entries
    }

private:
    BOARD_DOCUMENT& m_doc;
};


QT_DOCUMENT* CreateQtBoardDocument( QWidget* aParent )
{
    return new BOARD_DOCUMENT( aParent );
}


BOARD_DOCUMENT::BOARD_DOCUMENT( QWidget* aParent ) :
        QT_DOCUMENT( aParent ),
        m_undo( std::make_unique<UNDO_STACK>( &BOARD_UNDO_REDO::DeleteItems ) )
{
    m_canvas = new GAL_CANVAS;
    m_layers = new QListWidget;
    m_properties = new PROPERTY_PANEL;

    m_layers->setUniformItemSizes( true );

    auto* side = new QTabWidget;
    side->addTab( m_layers, tr( "Layers" ) );
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

    addTool( tr( "Route Track" ), QKeySequence( Qt::Key_X ), std::make_unique<TRACK_TOOL>( *this ) );
    addTool( tr( "Place Via" ), QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_V ),
             std::make_unique<VIA_TOOL>( *this ) );

    connect( m_canvas, &GAL_CANVAS::cursorMoved, this, &BOARD_DOCUMENT::onCursorMoved );
    connect( m_properties, &PROPERTY_PANEL::valueEdited, this, &BOARD_DOCUMENT::onPropertyEdited );
    connect( m_canvas, &GAL_CANVAS::zoomToFitRequested, this, &BOARD_DOCUMENT::ZoomToFit );
    connect( m_layers, &QListWidget::itemChanged, this, &BOARD_DOCUMENT::onLayerChanged );
    connect( m_layers, &QListWidget::currentItemChanged, this,
             &BOARD_DOCUMENT::onCurrentLayerChanged );
}


BOARD_DOCUMENT::~BOARD_DOCUMENT()
{
    // The tool and the move in progress drop their work while the view is there
    setTool( nullptr );
    endMove( false );

    // The undo entries own items that refer to the board; the view refers to the items of
    // the board
    m_undo->Clear();

    delete m_canvas;
    m_canvas = nullptr;
}


KIGFX::PCB_VIEW* BOARD_DOCUMENT::view() const
{
    return static_cast<KIGFX::PCB_VIEW*>( m_canvas->GetView() );
}


bool BOARD_DOCUMENT::Open( const QString& aFileName, QString& aError )
{
    wxString reason;
    BOARD*   board = LoadBoard( ToWxString( aFileName ), true, &reason );

    if( !board )
    {
        aError = reason.empty() ? tr( "The board could not be loaded." ) : ToQString( reason );
        return false;
    }

    dropSelection();
    m_undo->Clear();
    m_board.reset( board );
    m_fileName = aFileName;

    KIGFX::GAL* gal = m_canvas->GetGAL();
    gal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );

    auto view = std::make_unique<KIGFX::PCB_VIEW>();
    auto painter = std::make_unique<KIGFX::PCB_PAINTER>( gal, FRAME_PCB_EDITOR );

    view->SetScaleLimits( ZOOM_MAX_LIMIT_PCBNEW, ZOOM_MIN_LIMIT_PCBNEW );

    KIGFX::PCB_VIEW* pcbView = view.get();
    m_canvas->SetView( std::move( view ), std::move( painter ) );

    pcbView->SetDefaultLayerOrder();
    pcbView->SetDefaultLayerDeps( false );      // the Cairo GAL does not cache
    pcbView->UpdateDisplayOptions( PCB_DISPLAY_OPTIONS() );
    pcbView->LoadColors( boardColors() );
    pcbView->ShowBoard( m_board.get() );
    pcbView->SyncLayersVisibility( m_board.get() );
    pcbView->SetActiveLayerOnTop( F_Cu );

    m_activeLayer = F_Cu;

    setupGrid();
    fillLayers();
    selectionChanged();
    ZoomToFit();

    emit statusMessage( tr( "%1: %2 footprints, %3 tracks and vias" )
                                .arg( aFileName )
                                .arg( m_board->Footprints().size() )
                                .arg( m_board->Tracks().size() ) );
    emit stateChanged();
    return true;
}


bool BOARD_DOCUMENT::Save( QString& aError )
{
    if( !m_board )
        return false;

    // The board and its project, as the board editor saves them
    wxString fileName = ToWxString( m_fileName );

    if( !SaveBoard( fileName, m_board.get() ) )
    {
        aError = tr( "The board could not be saved to %1." ).arg( m_fileName );
        return false;
    }

    m_undo->SetSaved();
    emit statusMessage( tr( "%1 saved." ).arg( m_fileName ) );
    emit stateChanged();
    return true;
}


bool BOARD_DOCUMENT::IsModified() const
{
    return m_undo->IsModified();
}


bool BOARD_DOCUMENT::CanUndo() const
{
    return m_undo->CanUndo();
}


bool BOARD_DOCUMENT::CanRedo() const
{
    return m_undo->CanRedo();
}


void BOARD_DOCUMENT::ZoomToFit()
{
    if( !m_board )
        return;

    BOX2I box = m_board->ComputeBoundingBox( true );       // the board outline

    if( box.GetWidth() == 0 || box.GetHeight() == 0 )
        box = m_board->GetBoundingBox();

    m_canvas->ZoomToFit( box );
}


void BOARD_DOCUMENT::setupGrid()
{
    m_gridSize = VECTOR2D( pcbIUScale.mmToIU( 0.1 ), pcbIUScale.mmToIU( 0.1 ) );

    const GRID_SETTINGS* settings = nullptr;

    if( auto* cfg = pcbnewSettings() )
    {
        settings = &cfg->m_Window.grid;

        if( settings->last_size_idx >= 0
                && settings->last_size_idx < static_cast<int>( settings->grids.size() ) )
        {
            m_gridSize = settings->grids[settings->last_size_idx].ToDouble( pcbIUScale );
        }
    }

    m_canvas->SetGrid( m_gridSize, VECTOR2D( m_board->GetDesignSettings().GetGridOrigin() ),
                       settings );
}


void BOARD_DOCUMENT::fillLayers()
{
    const QSignalBlocker blocker( m_layers );
    m_layers->clear();

    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view()->GetPainter()->GetSettings() );

    for( PCB_LAYER_ID layer : m_board->GetEnabledLayers().UIOrder() )
    {
        QPixmap swatch( 14, 14 );
        swatch.fill( Qt::transparent );

        {
            QPainter painter( &swatch );
            painter.setRenderHint( QPainter::Antialiasing );
            painter.setPen( QPen( palette().mid().color() ) );
            painter.setBrush( toQColor( settings->GetColor( nullptr, layer ).WithAlpha( 1.0 ) ) );
            painter.drawRoundedRect( QRectF( 0.5, 0.5, 13, 13 ), 3, 3 );
        }

        auto* item = new QListWidgetItem( QIcon( swatch ), ToQString( m_board->GetLayerName( layer ) ),
                                          m_layers );
        item->setData( LAYER_ROLE, static_cast<int>( layer ) );
        item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
        item->setCheckState( m_board->IsLayerVisible( layer ) ? Qt::Checked : Qt::Unchecked );

        if( layer == F_Cu )
            m_layers->setCurrentItem( item );
    }
}


void BOARD_DOCUMENT::onLayerChanged( QListWidgetItem* aItem )
{
    if( !m_board )
        return;

    PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aItem->data( LAYER_ROLE ).toInt() );
    bool         visible = aItem->checkState() == Qt::Checked;

    LSET visibleLayers = m_board->GetVisibleLayers();
    visibleLayers.set( layer, visible );
    m_board->SetVisibleLayers( visibleLayers );

    view()->SetLayerVisible( layer, visible );
    m_canvas->update();
}


void BOARD_DOCUMENT::onCurrentLayerChanged( QListWidgetItem* aCurrent )
{
    if( !m_board || !aCurrent )
        return;

    const PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aCurrent->data( LAYER_ROLE ).toInt() );

    // The tools draw on the copper layer chosen last
    if( IsCopperLayer( layer ) )
        m_activeLayer = layer;

    view()->SetActiveLayerOnTop( layer );
    m_canvas->update();
}


void BOARD_DOCUMENT::onCursorMoved( double aX, double aY )
{
    emit cursorPositionChanged( QStringLiteral( "X %1  Y %2 mm" )
                                        .arg( pcbIUScale.IUTomm( aX ), 0, 'f', 4 )
                                        .arg( pcbIUScale.IUTomm( aY ), 0, 'f', 4 ) );
}


EDA_ITEM* BOARD_DOCUMENT::itemAt( const VECTOR2I& aPoint, Qt::KeyboardModifiers aModifiers )
{
    if( !m_board )
        return nullptr;

    KIGFX::PCB_VIEW* pcbView = view();

    const int accuracy = static_cast<int>( pcbView->ToWorld( HIT_DISTANCE_PX ) );
    BOX2I     area( aPoint - VECTOR2I( accuracy, accuracy ), VECTOR2I( 2 * accuracy,
                                                                       2 * accuracy ) );

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> found;
    pcbView->Query( area, found );

    BOARD_ITEM* best = nullptr;
    int         bestOrder = std::numeric_limits<int>::max();

    for( const auto& [viewItem, layer] : found )
    {
        BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( viewItem );

        if( !item || !isSelectable( item ) || !pcbView->IsLayerVisible( layer )
                || !pcbView->IsVisible( item ) )
        {
            continue;
        }

        if( !item->HitTest( aPoint, accuracy ) )
            continue;

        // The view draws the layers of lower order on top
        int order = pcbView->GetLayerOrder( layer );

        if( order < bestOrder )
        {
            best = item;
            bestOrder = order;
        }
    }

    // A click on a pad or a field of a footprint selects the footprint, as in the editor;
    // Ctrl selects the pad or the field
    if( best && best->GetParentFootprint() && !( aModifiers & Qt::ControlModifier ) )
        best = best->GetParentFootprint();

    return best;
}


std::vector<EDA_ITEM*> BOARD_DOCUMENT::itemsInArea( const BOX2I& aArea, bool aCrossing )
{
    std::vector<EDA_ITEM*> items;

    if( !m_board )
        return items;

    KIGFX::PCB_VIEW* pcbView = view();

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> found;
    pcbView->Query( aArea, found );

    std::unordered_set<BOARD_ITEM*> seen;

    for( const auto& [viewItem, layer] : found )
    {
        BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( viewItem );

        if( !item || !isSelectable( item ) || !pcbView->IsLayerVisible( layer )
                || !pcbView->IsVisible( item ) )
        {
            continue;
        }

        // An area selects whole footprints
        if( FOOTPRINT* footprint = item->GetParentFootprint() )
            item = footprint;

        if( !seen.insert( item ).second )
            continue;

        if( item->HitTest( aArea, !aCrossing ) )
            items.push_back( item );
    }

    return items;
}


std::vector<EDA_ITEM*> BOARD_DOCUMENT::allItems()
{
    std::vector<EDA_ITEM*> items;

    if( !m_board )
        return items;

    const LSET visible = m_board->GetVisibleLayers();

    auto add =
            [&]( BOARD_ITEM* aItem )
            {
                if( ( aItem->GetLayerSet() & visible ).any() )
                    items.push_back( aItem );
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
        add( footprint );

    for( PCB_TRACK* track : m_board->Tracks() )
        add( track );

    for( BOARD_ITEM* drawing : m_board->Drawings() )
        add( drawing );

    for( ZONE* zone : m_board->Zones() )
        add( zone );

    return items;
}


void BOARD_DOCUMENT::setItemSelected( EDA_ITEM* aItem, bool aSelected )
{
    BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( aItem );

    auto mark =
            [aSelected]( BOARD_ITEM* aMarked )
            {
                if( aSelected )
                    aMarked->SetSelected();
                else
                    aMarked->ClearSelected();
            };

    mark( boardItem );

    // The children of a footprint are drawn selected with it
    if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( boardItem ) )
        footprint->RunOnChildren( mark, RECURSE_MODE::RECURSE );

    view()->Update( boardItem );
}


void BOARD_DOCUMENT::selectionChanged()
{
    const std::vector<EDA_ITEM*>& selection = Selection();
    QString                       title;

    if( selection.size() == 1 )
        title = ToQString( selection.front()->GetFriendlyName() );
    else if( selection.size() > 1 )
        title = tr( "%1 items" ).arg( selection.size() );

    m_properties->ShowProperties( title, PropertyRows( selection, pcbIUScale, EDA_UNITS::MM ) );
}


void BOARD_DOCUMENT::onPropertyEdited( const void* aKey, const QString& aValue )
{
    if( Selection().empty() )
        return;

    auto    commit = makeCommit();
    QString error;

    for( EDA_ITEM* item : Selection() )
    {
        commit->Modify( item );

        if( !SetPropertyValue( item, aKey, aValue, pcbIUScale, EDA_UNITS::MM, error ) )
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


VECTOR2I BOARD_DOCUMENT::snapToGrid( const VECTOR2D& aDelta ) const
{
    return VECTOR2I( KiROUND( std::round( aDelta.x / m_gridSize.x ) * m_gridSize.x ),
                     KiROUND( std::round( aDelta.y / m_gridSize.y ) * m_gridSize.y ) );
}


VECTOR2I BOARD_DOCUMENT::snapPoint( const VECTOR2D& aPoint ) const
{
    const VECTOR2D origin( m_board ? m_board->GetDesignSettings().GetGridOrigin() : VECTOR2I() );

    return VECTOR2I( origin ) + snapToGrid( aPoint - origin );
}


std::unique_ptr<BOARD_COMMIT> BOARD_DOCUMENT::MakeCommit()
{
    return makeCommit();
}


std::unique_ptr<BOARD_COMMIT> BOARD_DOCUMENT::makeCommit()
{
    return std::make_unique<BOARD_COMMIT>( std::make_unique<COMMIT_LISTENER>( *this ),
                                           true /* board editor */, false );
}


std::vector<BOARD_ITEM*> BOARD_DOCUMENT::changeableItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::unordered_set<EDA_ITEM*> all( aItems.begin(), aItems.end() );
    std::vector<BOARD_ITEM*>      items;

    for( EDA_ITEM* item : aItems )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( boardItem->IsLocked() )
            continue;

        // A child changes with its footprint
        if( FOOTPRINT* footprint = boardItem->GetParentFootprint() )
        {
            if( all.count( footprint ) )
                continue;
        }

        items.push_back( boardItem );
    }

    if( items.size() < aItems.size() )
        emit statusMessage( tr( "Locked items are not changed." ) );

    return items;
}


bool BOARD_DOCUMENT::beginMove( bool aWithConnections )
{
    m_moving = changeableItems( Selection() );

    if( m_moving.empty() )
        return false;

    m_moveCommit = makeCommit();

    for( BOARD_ITEM* item : m_moving )
        m_moveCommit->Modify( item );

    if( aWithConnections )
        findConnectedTrackEnds();

    return true;
}


void BOARD_DOCUMENT::findConnectedTrackEnds()
{
    m_followingEnds.clear();

    std::unordered_set<BOARD_ITEM*> moving( m_moving.begin(), m_moving.end() );

    // The points the moving items connect at: the pads of the footprints, the ends of the
    // tracks, the vias
    std::vector<PAD*>     pads;
    std::vector<VECTOR2I> points;

    for( BOARD_ITEM* item : m_moving )
    {
        switch( item->Type() )
        {
        case PCB_FOOTPRINT_T:
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                pads.push_back( pad );

            break;

        case PCB_PAD_T:
            pads.push_back( static_cast<PAD*>( item ) );
            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
            points.push_back( static_cast<PCB_TRACK*>( item )->GetStart() );
            points.push_back( static_cast<PCB_TRACK*>( item )->GetEnd() );
            break;

        case PCB_VIA_T:
            points.push_back( static_cast<PCB_VIA*>( item )->GetPosition() );
            break;

        default:
            break;
        }
    }

    auto connects =
            [&]( PCB_TRACK* aTrack, const VECTOR2I& aEnd ) -> bool
            {
                for( PAD* pad : pads )
                {
                    if( pad->GetNetCode() == aTrack->GetNetCode() && pad->IsOnLayer( aTrack->GetLayer() )
                            && pad->HitTest( aEnd ) )
                    {
                        return true;
                    }
                }

                return std::find( points.begin(), points.end(), aEnd ) != points.end();
            };

    // The straight tracks with an end there follow it; the ones with both ends there move
    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_TRACE_T || moving.count( track ) || track->IsLocked() )
            continue;

        const bool start = connects( track, track->GetStart() );
        const bool end = connects( track, track->GetEnd() );

        if( !start && !end )
            continue;

        m_moveCommit->Modify( track );

        if( start && end )
        {
            m_moving.push_back( track );
            continue;
        }

        m_followingEnds.push_back( { track, start } );
    }
}


void BOARD_DOCUMENT::moveBy( const VECTOR2I& aDelta )
{
    for( BOARD_ITEM* item : m_moving )
    {
        item->Move( aDelta );

        // The pasted items are drawn by the preview of the view: they are not in it
        if( !m_pasting )
            view()->Update( item );
    }

    for( const TRACK_END& end : m_followingEnds )
    {
        if( end.m_Start )
            end.m_Track->SetStart( end.m_Track->GetStart() + aDelta );
        else
            end.m_Track->SetEnd( end.m_Track->GetEnd() + aDelta );

        view()->Update( end.m_Track );
    }

    if( m_pasting )
        view()->MarkDirty();
}


void BOARD_DOCUMENT::endMove( bool aCommit )
{
    if( m_pasting )
    {
        // The pasted items go to the board, or are dropped
        m_pasting = false;
        view()->ClearPreview();

        if( aCommit )
        {
            std::unique_ptr<BOARD_COMMIT> commit = makeCommit();

            for( BOARD_ITEM* item : m_moving )
                commit->Add( item );

            commit->Push( ToWxString( tr( "Paste" ) ) );

            std::vector<EDA_ITEM*> pasted( m_moving.begin(), m_moving.end() );
            m_moving.clear();
            setSelection( pasted );
        }
        else
        {
            for( BOARD_ITEM* item : m_moving )
                delete item;

            m_moving.clear();
        }

        m_canvas->update();
        return;
    }

    if( !m_moveCommit )
        return;

    if( aCommit )
        m_moveCommit->Push( ToWxString( m_followingEnds.empty() ? tr( "Move" ) : tr( "Drag" ) ) );
    else
        m_moveCommit->Revert();

    m_moveCommit.reset();
    m_moving.clear();
    m_followingEnds.clear();
    m_canvas->update();
}


QString BOARD_DOCUMENT::copyItems( const std::vector<EDA_ITEM*>& aItems )
{
    if( !m_board || aItems.empty() )
        return QString();

    // The items are placed around the center of their box, on the grid, at (0, 0)
    BOX2I box;

    for( EDA_ITEM* item : aItems )
        box.Merge( item->GetBoundingBox() );

    wxString     text;
    CLIPBOARD_IO io( [&text]( const wxString& aText ) { text = aText; },
                     []() { return wxString(); } );

    io.SetBoard( m_board.get() );
    io.SaveItems( aItems, snapPoint( VECTOR2D( box.GetCenter() ) ), false );

    return ToQString( text );
}


bool BOARD_DOCUMENT::beginPaste( const QString& aText, const VECTOR2I& aPosition )
{
    if( !m_board )
        return false;

    const wxString text = ToWxString( aText );
    CLIPBOARD_IO   io( []( const wxString& ) {}, [text]() { return text; } );
    BOARD_ITEM*    clip = io.Parse();

    if( !clip )
        return false;

    std::vector<BOARD_ITEM*> items;

    if( clip->Type() == PCB_T )
    {
        // A board: its items, with their nets mapped to the ones of this board by name
        BOARD* clipBoard = static_cast<BOARD*>( clip );
        clipBoard->MapNets( m_board.get() );

        for( BOARD_ITEM* item : clipBoard->GetItemSet() )
        {
            if( item->Type() != PCB_MARKER_T )
                items.push_back( item );
        }

        clipBoard->RemoveAll();
        delete clipBoard;
    }
    else if( clip->Type() == PCB_FOOTPRINT_T )
    {
        items.push_back( clip );
    }
    else
    {
        delete clip;
        return false;
    }

    for( BOARD_ITEM* item : items )
    {
        // New items, as the board editor pastes them
        item->SetParent( m_board.get() );
        item->ResetUuid();
        item->RunOnChildren( []( BOARD_ITEM* aChild ) { aChild->ResetUuid(); },
                             RECURSE_MODE::RECURSE );

        if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( item ) )
        {
            footprint->ResolveComponentClassNames( m_board.get(),
                                                   footprint->GetTransientComponentClassNames() );
            footprint->ClearTransientComponentClassNames();
            footprint->SetPath( KIID_PATH() );
        }
        else if( ZONE* zone = dynamic_cast<ZONE*>( item ) )
        {
            // A pasted zone must not reuse a name already on the board
            if( !zone->GetZoneName().IsEmpty() )
                zone->SetZoneName( m_board->GetUniqueZoneName( zone->GetZoneName() ) );
        }

        // The clipboard has its items around (0, 0)
        item->Move( aPosition );
    }

    if( items.empty() )
        return false;

    // The items follow the cursor in the preview of the view until they are placed
    for( BOARD_ITEM* item : items )
    {
        view()->AddToPreview( item, false );

        item->RunOnChildren( [this]( BOARD_ITEM* aChild ) { view()->AddToPreview( aChild, false ); },
                             RECURSE_MODE::RECURSE );
    }

    m_moving = std::move( items );
    m_pasting = true;
    return true;
}


BOARD_CONNECTED_ITEM* BOARD_DOCUMENT::ConnectionAt( const VECTOR2I& aPoint,
                                                    PCB_LAYER_ID aLayer ) const
{
    if( !m_board )
        return nullptr;

    auto onLayer =
            [aLayer]( const BOARD_ITEM* aItem )
            {
                return aLayer == UNDEFINED_LAYER ? aItem->IsOnCopperLayer()
                                                 : aItem->IsOnLayer( aLayer );
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( onLayer( pad ) && pad->HitTest( aPoint ) )
                return pad;
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( !onLayer( track ) )
            continue;

        if( track->Type() == PCB_VIA_T ? track->HitTest( aPoint )
                                       : track->GetStart() == aPoint || track->GetEnd() == aPoint )
        {
            return track;
        }
    }

    return nullptr;
}


void BOARD_DOCUMENT::rotateItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::vector<BOARD_ITEM*> items = changeableItems( aItems );

    if( items.empty() )
        return;

    // A single item turns about its position (the anchor of a footprint), several items
    // about the center of their box, on the grid
    VECTOR2I center;

    if( items.size() == 1 )
    {
        center = items.front()->GetPosition();
    }
    else
    {
        BOX2I box;

        for( BOARD_ITEM* item : items )
            box.Merge( item->GetBoundingBox() );

        center = snapPoint( VECTOR2D( box.GetCenter() ) );
    }

    std::unique_ptr<BOARD_COMMIT> commit = makeCommit();

    for( BOARD_ITEM* item : items )
    {
        commit->Modify( item );
        item->Rotate( center, ANGLE_90 );
    }

    commit->Push( ToWxString( tr( "Rotate" ) ) );
}


void BOARD_DOCUMENT::deleteItems( const std::vector<EDA_ITEM*>& aItems )
{
    std::unique_ptr<BOARD_COMMIT> commit = makeCommit();
    bool                          removed = false;

    for( BOARD_ITEM* item : changeableItems( aItems ) )
    {
        // The pads and the fields of a footprint are deleted with it
        if( item->GetParentFootprint() )
            continue;

        commit->Remove( item );
        removed = true;
    }

    if( removed )
        commit->Push( ToWxString( tr( "Delete" ) ) );
}


void BOARD_DOCUMENT::recordChange( PICKED_ITEMS_LIST& aChange )
{
    auto* entry = new PICKED_ITEMS_LIST();
    entry->SetDescription( aChange.GetDescription() );

    for( unsigned ii = 0; ii < aChange.GetCount(); ii++ )
        entry->PushItem( aChange.GetItemWrapper( ii ) );

    BOARD_UNDO_REDO::CompleteEntry( *entry, 0, UNDO_REDO::UNSPECIFIED );

    if( entry->GetCount() )
        m_undo->Push( entry );
    else
        delete entry;
}


void BOARD_DOCUMENT::applyUndoEntry( PICKED_ITEMS_LIST& aEntry )
{
    UNDO_LISTENER listener( *this );

    BOARD_UNDO_REDO::RESULT result = BOARD_UNDO_REDO::Apply( *m_board, aEntry, listener, true );

    if( result.shapesChanged )
    {
        m_board->UpdateBoardOutline();
        updateBoardOutline();
    }

    if( result.itemsNotFound )
        emit statusMessage( tr( "Incomplete undo/redo operation: some items not found." ) );
}


void BOARD_DOCUMENT::undo()
{
    m_undo->Undo( [this]( PICKED_ITEMS_LIST& aEntry ) { applyUndoEntry( aEntry ); } );
}


void BOARD_DOCUMENT::redo()
{
    m_undo->Redo( [this]( PICKED_ITEMS_LIST& aEntry ) { applyUndoEntry( aEntry ); } );
}


void BOARD_DOCUMENT::updateBoardOutline()
{
    PCB_BOARD_OUTLINE* outline = m_board->BoardOutline();

    if( !outline )
        return;

    if( view()->HasItem( outline ) )
        view()->Update( outline );
    else
        view()->Add( outline );
}
