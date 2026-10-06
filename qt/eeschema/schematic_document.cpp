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

#include <limits>

#include <QHeaderView>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <base_units.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <eeschema_helpers.h>
#include <eeschema_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/util.h>
#include <sch_item.h>
#include <sch_painter.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <zoom_defines.h>

#include <gal_canvas.h>
#include <inspectable_rows.h>
#include <property_panel.h>
#include <qt_strings.h>


/// Role of the sheet list items holding the page index
static constexpr int PAGE_ROLE = Qt::UserRole;

/// Distance of a click to an item that still hits it, in pixels
static constexpr double HIT_DISTANCE_PX = 4.0;


/// The color theme of the schematic editor.
static COLOR_SETTINGS* schematicColors()
{
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        return ::GetColorSettings( cfg->m_ColorTheme );

    return ::GetColorSettings( DEFAULT_THEME );
}


QT_DOCUMENT* CreateQtSchematicDocument( QWidget* aParent )
{
    return new SCHEMATIC_DOCUMENT( aParent );
}


SCHEMATIC_DOCUMENT::SCHEMATIC_DOCUMENT( QWidget* aParent ) :
        QT_DOCUMENT( aParent )
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

    connect( m_canvas, &GAL_CANVAS::cursorMoved, this, &SCHEMATIC_DOCUMENT::onCursorMoved );
    connect( m_canvas, &GAL_CANVAS::clicked, this, &SCHEMATIC_DOCUMENT::onClicked );
    connect( m_canvas, &GAL_CANVAS::zoomToFitRequested, this, &SCHEMATIC_DOCUMENT::ZoomToFit );
    connect( m_sheets, &QTreeWidget::itemActivated, this,
             &SCHEMATIC_DOCUMENT::onSheetActivated );
}


SCHEMATIC_DOCUMENT::~SCHEMATIC_DOCUMENT()
{
    // The view refers to the items of the schematic
    delete m_canvas;
    m_canvas = nullptr;
}


KIGFX::SCH_VIEW* SCHEMATIC_DOCUMENT::view() const
{
    return static_cast<KIGFX::SCH_VIEW*>( m_canvas->GetView() );
}


bool SCHEMATIC_DOCUMENT::Open( const QString& aFileName, QString& aError )
{
    SCHEMATIC* schematic = EESCHEMA_HELPERS::LoadSchematic( ToWxString( aFileName ), true, false );

    if( !schematic )
    {
        aError = tr( "The schematic could not be loaded." );
        return false;
    }

    m_schematic.reset( schematic );
    m_fileName = aFileName;
    m_selected = nullptr;

    KIGFX::GAL* gal = m_canvas->GetGAL();
    gal->SetWorldUnitLength( SCH_WORLD_UNIT );

    auto view = std::make_unique<KIGFX::SCH_VIEW>( this );
    auto painter = std::make_unique<KIGFX::SCH_PAINTER>( gal );

    painter->SetSchematic( m_schematic.get() );
    painter->GetSettings()->LoadColors( schematicColors() );

    view->SetScaleLimits( ZOOM_MAX_LIMIT_EESCHEMA, ZOOM_MIN_LIMIT_EESCHEMA );

    KIGFX::SCH_VIEW* schView = view.get();
    m_canvas->SetView( std::move( view ), std::move( painter ) );

    schView->SetMirror( false, false );

    schView->SetDefaultLayerOrder();
    schView->SetDefaultLayerDeps( false );      // the Cairo GAL does not cache
    schView->UpdateAllLayersOrder();

    fillSheets();
    showSheet( 0 );

    emit statusMessage( tr( "%1: %2 sheets" ).arg( aFileName ).arg( m_sheets->topLevelItemCount() ) );
    return true;
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
    }
}


void SCHEMATIC_DOCUMENT::showSheet( int aPage )
{
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    if( aPage < 0 || aPage >= static_cast<int>( sheets.size() ) )
        return;

    select( nullptr );

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

    for( SCH_ITEM* item : m_schematic->GetCurrentScreen()->Items() )
        box.Merge( item->GetBoundingBox() );

    m_canvas->ZoomToFit( box );
}


void SCHEMATIC_DOCUMENT::onCursorMoved( double aX, double aY )
{
    emit cursorPositionChanged( QStringLiteral( "X %1  Y %2 mm" )
                                        .arg( schIUScale.IUTomm( aX ), 0, 'f', 2 )
                                        .arg( schIUScale.IUTomm( aY ), 0, 'f', 2 ) );
}


SCH_ITEM* SCHEMATIC_DOCUMENT::itemAt( const VECTOR2I& aPosition ) const
{
    KIGFX::SCH_VIEW* schView = view();

    const int accuracy = static_cast<int>( schView->ToWorld( HIT_DISTANCE_PX ) );
    BOX2I     area( aPosition - VECTOR2I( accuracy, accuracy ), VECTOR2I( 2 * accuracy,
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

        if( !item->HitTest( aPosition, accuracy ) )
            continue;

        // The view draws the layers of lower order on top
        int order = schView->GetLayerOrder( layer );

        if( order < bestOrder )
        {
            best = item;
            bestOrder = order;
        }
    }

    return best;
}


void SCHEMATIC_DOCUMENT::select( SCH_ITEM* aItem )
{
    KIGFX::SCH_VIEW* schView = view();

    if( m_selected )
    {
        m_selected->ClearSelected();
        schView->Update( m_selected );
    }

    m_selected = aItem;

    if( m_selected )
    {
        m_selected->SetSelected();
        schView->Update( m_selected );
        m_properties->ShowProperties( ToQString( m_selected->GetFriendlyName() ),
                                      PropertyRows( m_selected, schIUScale, EDA_UNITS::MM ) );
    }
    else
    {
        m_properties->ShowProperties( QString(), {} );
    }

    m_canvas->update();
}


void SCHEMATIC_DOCUMENT::onClicked( double aX, double aY, Qt::KeyboardModifiers aModifiers )
{
    if( !m_schematic )
        return;

    SCH_ITEM* item = itemAt( VECTOR2I( KiROUND( aX ), KiROUND( aY ) ) );

    // A click on a field or a pin of a symbol selects the symbol, as in the editor
    if( item && !( aModifiers & Qt::ControlModifier ) )
    {
        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() ) )
            item = symbol;
    }

    select( item );
}
