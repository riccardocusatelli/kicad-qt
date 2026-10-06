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

#include <limits>

#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_display_options.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew_scripting_helpers.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <zoom_defines.h>

#include <gal_canvas.h>
#include <inspectable_rows.h>
#include <property_panel.h>
#include <qt_strings.h>


/// Role of the layer list items holding the PCB_LAYER_ID
static constexpr int LAYER_ROLE = Qt::UserRole;

/// Distance of a click to an item that still hits it, in pixels
static constexpr double HIT_DISTANCE_PX = 4.0;


/// A color as a QColor.
static QColor toQColor( const KIGFX::COLOR4D& aColor )
{
    return QColor::fromRgbF( aColor.r, aColor.g, aColor.b, aColor.a );
}


/// The color theme of the PCB editor.
static COLOR_SETTINGS* boardColors()
{
    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        return ::GetColorSettings( cfg->m_ColorTheme );

    return ::GetColorSettings( DEFAULT_THEME );
}


QT_DOCUMENT* CreateQtBoardDocument( QWidget* aParent )
{
    return new BOARD_DOCUMENT( aParent );
}


BOARD_DOCUMENT::BOARD_DOCUMENT( QWidget* aParent ) :
        QT_DOCUMENT( aParent )
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

    connect( m_canvas, &GAL_CANVAS::cursorMoved, this, &BOARD_DOCUMENT::onCursorMoved );
    connect( m_canvas, &GAL_CANVAS::clicked, this, &BOARD_DOCUMENT::onClicked );
    connect( m_canvas, &GAL_CANVAS::zoomToFitRequested, this, &BOARD_DOCUMENT::ZoomToFit );
    connect( m_layers, &QListWidget::itemChanged, this, &BOARD_DOCUMENT::onLayerChanged );
    connect( m_layers, &QListWidget::currentItemChanged, this,
             &BOARD_DOCUMENT::onCurrentLayerChanged );
}


BOARD_DOCUMENT::~BOARD_DOCUMENT()
{
    // The view refers to the items of the board
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

    m_board.reset( board );
    m_fileName = aFileName;
    m_selected = nullptr;

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

    fillLayers();
    select( nullptr );
    ZoomToFit();

    emit statusMessage( tr( "%1: %2 footprints, %3 tracks and vias" )
                                .arg( aFileName )
                                .arg( m_board->Footprints().size() )
                                .arg( m_board->Tracks().size() ) );
    return true;
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

    view()->SetActiveLayerOnTop( static_cast<PCB_LAYER_ID>( aCurrent->data( LAYER_ROLE ).toInt() ) );
    m_canvas->update();
}


void BOARD_DOCUMENT::onCursorMoved( double aX, double aY )
{
    emit cursorPositionChanged( QStringLiteral( "X %1  Y %2 mm" )
                                        .arg( pcbIUScale.IUTomm( aX ), 0, 'f', 4 )
                                        .arg( pcbIUScale.IUTomm( aY ), 0, 'f', 4 ) );
}


BOARD_ITEM* BOARD_DOCUMENT::itemAt( const VECTOR2I& aPosition ) const
{
    KIGFX::PCB_VIEW* pcbView = view();

    const int accuracy = static_cast<int>( pcbView->ToWorld( HIT_DISTANCE_PX ) );
    BOX2I     area( aPosition - VECTOR2I( accuracy, accuracy ), VECTOR2I( 2 * accuracy,
                                                                          2 * accuracy ) );

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> found;
    pcbView->Query( area, found );

    BOARD_ITEM* best = nullptr;
    int         bestOrder = std::numeric_limits<int>::max();

    for( const auto& [viewItem, layer] : found )
    {
        BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( viewItem );

        if( !item || !pcbView->IsLayerVisible( layer ) || !pcbView->IsVisible( item ) )
            continue;

        if( !item->HitTest( aPosition, accuracy ) )
            continue;

        // The view draws the layers of lower order on top
        int order = pcbView->GetLayerOrder( layer );

        if( order < bestOrder )
        {
            best = item;
            bestOrder = order;
        }
    }

    return best;
}


void BOARD_DOCUMENT::select( BOARD_ITEM* aItem )
{
    KIGFX::PCB_VIEW* pcbView = view();

    auto setSelected =
            [&]( BOARD_ITEM* aBoardItem, bool aSelected )
            {
                if( aSelected )
                    aBoardItem->SetSelected();
                else
                    aBoardItem->ClearSelected();

                if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aBoardItem ) )
                {
                    footprint->RunOnChildren(
                            [&]( BOARD_ITEM* aChild )
                            {
                                if( aSelected )
                                    aChild->SetSelected();
                                else
                                    aChild->ClearSelected();
                            },
                            RECURSE_MODE::RECURSE );
                }

                pcbView->Update( aBoardItem );
            };

    if( m_selected )
        setSelected( m_selected, false );

    m_selected = aItem;

    if( m_selected )
    {
        setSelected( m_selected, true );
        m_properties->ShowProperties( ToQString( m_selected->GetFriendlyName() ),
                                      PropertyRows( m_selected, pcbIUScale, EDA_UNITS::MM ) );
    }
    else
    {
        m_properties->ShowProperties( QString(), {} );
    }

    m_canvas->update();
}


void BOARD_DOCUMENT::onClicked( double aX, double aY, Qt::KeyboardModifiers aModifiers )
{
    if( !m_board )
        return;

    BOARD_ITEM* item = itemAt( VECTOR2I( KiROUND( aX ), KiROUND( aY ) ) );

    // A click on a pad or a field of a footprint selects the footprint, as in the editor
    if( item && item->GetParentFootprint() && !( aModifiers & Qt::ControlModifier ) )
        item = item->GetParentFootprint();

    select( item );
}
