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

#include "gal_canvas.h"

#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>

#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <settings/grid_settings.h>
#include <view/view.h>

#include "qt_cairo_gal.h"


/// Zoom factor of a wheel notch and of ZoomIn() / ZoomOut()
static constexpr double ZOOM_STEP = 1.25;

/// Distance the cursor moves with the button pressed before a click becomes a drag, in pixels
static constexpr double DRAG_THRESHOLD_PX = 4.0;


GAL_CANVAS::GAL_CANVAS( QWidget* aParent ) :
        QWidget( aParent )
{
    m_gal = std::make_unique<KIGFX::QT_CAIRO_GAL>( m_options, [this]() { present(); } );
    m_gal->SetCursorEnabled( false );

    setMouseTracking( true );
    setFocusPolicy( Qt::StrongFocus );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setMinimumSize( 64, 64 );
}


GAL_CANVAS::~GAL_CANVAS()
{
    // The items shown may outlive the view: detach them.  The view refers to the painter and
    // the GAL.
    if( m_view )
        m_view->Clear();

    m_view.reset();
    m_painter.reset();
}


KIGFX::GAL* GAL_CANVAS::GetGAL() const
{
    return m_gal.get();
}


void GAL_CANVAS::SetView( std::unique_ptr<KIGFX::VIEW> aView,
                          std::unique_ptr<KIGFX::PAINTER> aPainter )
{
    if( m_view )
        m_view->Clear();

    m_view.reset();
    m_painter = std::move( aPainter );
    m_view = std::move( aView );

    m_view->SetGAL( m_gal.get() );
    m_view->SetPainter( m_painter.get() );
    m_view->MarkDirty();
    update();
}


void GAL_CANVAS::ZoomToFit( const BOX2I& aBox )
{
    if( !m_view || aBox.GetWidth() <= 0 || aBox.GetHeight() <= 0 )
        return;

    m_fitBox = aBox;

    // Without a size, the view cannot compute the scale: the resize will do it
    if( m_gal->GetScreenPixelSize().x <= 0 || m_gal->GetScreenPixelSize().y <= 0 )
        return;

    BOX2D box( aBox.GetOrigin(), aBox.GetSize() );
    box.Inflate( box.GetWidth() * 0.05, box.GetHeight() * 0.05 );

    m_view->SetViewport( box );
    m_view->MarkDirty();
    Q_EMIT zoomChanged( m_view->GetScale() );
    update();
}


void GAL_CANVAS::ZoomIn()
{
    zoomAt( ZOOM_STEP, VECTOR2D( m_gal->GetScreenPixelSize() ) / 2 );
}


void GAL_CANVAS::ZoomOut()
{
    zoomAt( 1.0 / ZOOM_STEP, VECTOR2D( m_gal->GetScreenPixelSize() ) / 2 );
}


void GAL_CANVAS::Refresh()
{
    if( m_view )
    {
        m_view->UpdateAllItems( KIGFX::ALL );
        m_view->MarkDirty();
    }

    update();
}


void GAL_CANVAS::SetGrid( const VECTOR2D& aSize, const VECTOR2D& aOrigin,
                          const GRID_SETTINGS* aSettings )
{
    bool show = true;

    if( aSettings )
    {
        // The values of the style setting, as the editors read them
        switch( aSettings->style )
        {
        case 1:  m_options.m_gridStyle = KIGFX::GRID_STYLE::LINES;       break;
        case 2:  m_options.m_gridStyle = KIGFX::GRID_STYLE::SMALL_CROSS; break;
        default: m_options.m_gridStyle = KIGFX::GRID_STYLE::DOTS;        break;
        }

        m_options.m_gridLineWidth = aSettings->line_width;
        m_options.m_gridMinSpacing = aSettings->min_spacing;
        m_options.m_axesEnabled = aSettings->axes_enabled;
        show = aSettings->show;
    }

    m_options.NotifyChanged();

    m_gal->SetGridSize( aSize );
    m_gal->SetGridOrigin( aOrigin );
    m_gal->SetGridVisibility( show );

    if( m_view )
        m_view->MarkDirty();

    update();
}


void GAL_CANVAS::SetSelectionArea( const std::optional<BOX2D>& aArea, bool aCrossing )
{
    m_selectionArea = aArea;
    m_selectionCrossing = aCrossing;
    update();
}


void GAL_CANVAS::SetCrosshair( const std::optional<VECTOR2D>& aPoint )
{
    if( aPoint == m_crosshair )
        return;

    m_crosshair = aPoint;
    update();
}


void GAL_CANVAS::render()
{
    const VECTOR2I size = m_gal->GetScreenPixelSize();

    if( !m_view || size.x <= 0 || size.y <= 0 )
        return;

    m_view->UpdateItems();

    KIGFX::RENDER_SETTINGS* settings = m_painter->GetSettings();
    KIGFX::GAL_DRAWING_CONTEXT ctx( m_gal.get() );

    m_gal->SetClearColor( settings->GetBackgroundColor() );
    m_gal->SetGridColor( settings->GetGridColor() );

    if( m_view->IsDirty() )
    {
        const bool redrawAll = m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED );

        m_view->ClearTargets();

        // The main buffer of the compositor holds the background: fill it once cleared
        if( redrawAll )
        {
            m_gal->ClearScreen();
            m_gal->DrawGrid();
        }

        m_view->Redraw();
    }
}


void GAL_CANVAS::present()
{
    const VECTOR2I size = m_gal->GetScreenPixelSize();

    // The raster is ARGB32 with premultiplied alpha in native byte order, as QImage's
    // Format_ARGB32_Premultiplied
    QImage raster( m_gal->Raster(), size.x, size.y, m_gal->RasterStride(),
                   QImage::Format_ARGB32_Premultiplied );

    m_image = raster.copy();
    m_image.setDevicePixelRatio( devicePixelRatioF() );
}


VECTOR2D GAL_CANVAS::toDevice( const QPointF& aPoint ) const
{
    const double ratio = devicePixelRatioF();
    return VECTOR2D( aPoint.x() * ratio, aPoint.y() * ratio );
}


void GAL_CANVAS::zoomAt( double aFactor, const VECTOR2D& aDevicePoint )
{
    if( !m_view )
        return;

    const VECTOR2D anchor = m_view->ToWorld( aDevicePoint );
    m_fitBox.reset();

    m_view->SetScale( m_view->GetScale() * aFactor, anchor );
    m_view->MarkDirty();
    Q_EMIT zoomChanged( m_view->GetScale() );
    update();
}


void GAL_CANVAS::paintEvent( QPaintEvent* aEvent )
{
    render();

    QPainter painter( this );

    if( m_image.isNull() )
        painter.fillRect( rect(), palette().window() );
    else
        painter.drawImage( QPointF( 0, 0 ), m_image );

    if( m_selectionArea && m_view )
    {
        // As in the editors: a window area is solid, a crossing area is dashed
        const double   ratio = devicePixelRatioF();
        const VECTOR2D a = m_view->ToScreen( m_selectionArea->GetOrigin() ) / ratio;
        const VECTOR2D b = m_view->ToScreen( m_selectionArea->GetEnd() ) / ratio;
        const QRectF   area = QRectF( QPointF( a.x, a.y ), QPointF( b.x, b.y ) ).normalized();
        const QColor   color = m_selectionCrossing ? QColor( 80, 200, 120 ) : QColor( 70, 140, 255 );

        QColor fill = color;
        fill.setAlpha( 40 );

        QPen pen( color, 1.0, m_selectionCrossing ? Qt::DashLine : Qt::SolidLine );
        pen.setCosmetic( true );

        painter.setPen( pen );
        painter.setBrush( fill );
        painter.drawRect( area );
    }

    if( m_crosshair && m_view )
    {
        // A small cross, in the color of the cursor of the editors
        const VECTOR2D at = m_view->ToScreen( *m_crosshair ) / devicePixelRatioF();
        const double   arm = 10.0;
        const QPointF  center( at.x, at.y );

        const KIGFX::COLOR4D& color = m_painter->GetSettings()->GetCursorColor();

        QPen pen( QColor::fromRgbF( color.r, color.g, color.b ), 1.0 );
        pen.setCosmetic( true );

        painter.setPen( pen );
        painter.drawLine( center - QPointF( arm, 0 ), center + QPointF( arm, 0 ) );
        painter.drawLine( center - QPointF( 0, arm ), center + QPointF( 0, arm ) );
    }
}


void GAL_CANVAS::resizeEvent( QResizeEvent* aEvent )
{
    const double ratio = devicePixelRatioF();
    const int    width = std::max( 1, (int) std::lround( aEvent->size().width() * ratio ) );
    const int    height = std::max( 1, (int) std::lround( aEvent->size().height() * ratio ) );

    m_gal->ResizeScreen( width, height );

    if( m_view )
        m_view->MarkDirty();

    // Still showing the whole of a box: show it at the new size
    if( m_fitBox )
        ZoomToFit( *m_fitBox );

    update();
}


void GAL_CANVAS::wheelEvent( QWheelEvent* aEvent )
{
    const double notches = aEvent->angleDelta().y() / 120.0;

    if( notches != 0.0 )
        zoomAt( std::pow( ZOOM_STEP, notches ), toDevice( aEvent->position() ) );

    aEvent->accept();
}


void GAL_CANVAS::mousePressEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::MiddleButton || aEvent->button() == Qt::RightButton )
    {
        m_panning = true;
        m_panStart = aEvent->position();
        setCursor( Qt::ClosedHandCursor );
    }
    else if( aEvent->button() == Qt::LeftButton && m_view && !m_panning )
    {
        m_leftDown = true;
        m_dragging = false;
        m_leftStart = aEvent->position();

        Q_EMIT leftPressed( m_view->ToWorld( toDevice( m_leftStart ) ), aEvent->modifiers() );
    }

    aEvent->accept();
}


void GAL_CANVAS::mouseMoveEvent( QMouseEvent* aEvent )
{
    if( !m_view )
        return;

    if( m_panning )
    {
        const VECTOR2D delta = toDevice( aEvent->position() ) - toDevice( m_panStart );

        m_view->SetCenter( m_view->GetCenter() - m_view->ToWorld( delta, false ) );
        m_fitBox.reset();
        m_view->MarkDirty();
        m_panStart = aEvent->position();
        update();
    }

    const VECTOR2D world = m_view->ToWorld( toDevice( aEvent->position() ) );

    if( m_leftDown && ( aEvent->buttons() & Qt::LeftButton ) )
    {
        if( !m_dragging )
        {
            const QPointF moved = aEvent->position() - m_leftStart;

            if( std::hypot( moved.x(), moved.y() ) >= DRAG_THRESHOLD_PX )
            {
                m_dragging = true;
                Q_EMIT leftDragStarted( m_view->ToWorld( toDevice( m_leftStart ) ),
                                      aEvent->modifiers() );
            }
        }

        if( m_dragging )
            Q_EMIT leftDragged( world, aEvent->modifiers() );
    }

    Q_EMIT cursorMoved( world.x, world.y );
}


void GAL_CANVAS::mouseReleaseEvent( QMouseEvent* aEvent )
{
    if( m_panning
        && ( aEvent->button() == Qt::MiddleButton || aEvent->button() == Qt::RightButton ) )
    {
        m_panning = false;
        unsetCursor();
    }
    else if( aEvent->button() == Qt::LeftButton && m_leftDown && m_view )
    {
        const bool dragged = m_dragging;

        m_leftDown = false;
        m_dragging = false;

        Q_EMIT leftReleased( m_view->ToWorld( toDevice( aEvent->position() ) ), aEvent->modifiers(),
                           dragged );
    }

    aEvent->accept();
}


void GAL_CANVAS::mouseDoubleClickEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::LeftButton && m_view && !m_panning )
    {
        // The second press of a double click: no drag can start from it
        m_leftDown = false;
        m_dragging = false;

        Q_EMIT leftDoubleClicked( m_view->ToWorld( toDevice( aEvent->position() ) ),
                                aEvent->modifiers() );
    }

    aEvent->accept();
}


void GAL_CANVAS::keyPressEvent( QKeyEvent* aEvent )
{
    switch( aEvent->key() )
    {
    case Qt::Key_Home:   Q_EMIT zoomToFitRequested(); break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:  ZoomIn();                  break;
    case Qt::Key_Minus:  ZoomOut();                 break;
    case Qt::Key_Escape: Q_EMIT cancelRequested();    break;
    default:             QWidget::keyPressEvent( aEvent ); return;
    }

    aEvent->accept();
}
