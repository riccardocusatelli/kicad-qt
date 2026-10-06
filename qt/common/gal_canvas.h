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

#pragma once

#include <memory>
#include <optional>

#include <QImage>
#include <QPoint>
#include <QWidget>

#include <gal/gal_display_options.h>
#include <math/box2.h>
#include <math/vector2d.h>

#include "kiqt.h"

struct GRID_SETTINGS;

namespace KIGFX
{
class GAL;
class PAINTER;
class QT_CAIRO_GAL;
class VIEW;
}


/**
 * A canvas that shows a KiCad VIEW, drawn by the Cairo GAL.
 *
 * The owner gives the canvas its view and painter (SetView()).  The canvas zooms with the
 * wheel (around the cursor), pans with the middle or right button, and reports the cursor
 * position and the clicks in world coordinates.
 */
class KIQT_API GAL_CANVAS : public QWidget
{
    Q_OBJECT

public:
    explicit GAL_CANVAS( QWidget* aParent = nullptr );
    ~GAL_CANVAS() override;

    KIGFX::GAL*  GetGAL() const;
    KIGFX::VIEW* GetView() const { return m_view.get(); }

    /// Show a view drawn by a painter; the canvas owns them.
    void SetView( std::unique_ptr<KIGFX::VIEW> aView, std::unique_ptr<KIGFX::PAINTER> aPainter );

    /// Show the whole of a box (in world units), with a margin.
    void ZoomToFit( const BOX2I& aBox );

    void ZoomIn();
    void ZoomOut();

    /// Redraw the whole view (after a change of the items, layers or colors).
    void Refresh();

    /**
     * Show a grid of @a aSize from @a aOrigin (in world units), drawn as the settings of an
     * editor say (style, line width, minimum spacing, axes, visibility), or with the defaults
     * if @a aSettings is null.
     */
    void SetGrid( const VECTOR2D& aSize, const VECTOR2D& aOrigin, const GRID_SETTINGS* aSettings );

    /**
     * Show the area of a selection by dragging (in world units), or nothing.
     * @param aCrossing the area selects the items it touches (dashed), not only the ones
     *                  inside it.
     */
    void SetSelectionArea( const std::optional<BOX2D>& aArea, bool aCrossing );

signals:
    /// The cursor moved, at a point in world units.
    void cursorMoved( double aX, double aY );

    /// The left button was pressed, at a point in world units.
    void leftPressed( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers );

    /// The cursor moved by a few pixels with the left button pressed at @a aStart: a drag.
    void leftDragStarted( const VECTOR2D& aStart, Qt::KeyboardModifiers aModifiers );

    /// The cursor moved during a drag.
    void leftDragged( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers );

    /// The left button was released; @a aDragged if it was a drag, not a click.
    void leftReleased( const VECTOR2D& aPoint, Qt::KeyboardModifiers aModifiers, bool aDragged );

    /// The Escape key.
    void cancelRequested();

    /// The zoom changed (VIEW scale).
    void zoomChanged( double aScale );

    /// The user asked to see everything (Home key).
    void zoomToFitRequested();

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void resizeEvent( QResizeEvent* aEvent ) override;
    void wheelEvent( QWheelEvent* aEvent ) override;
    void mousePressEvent( QMouseEvent* aEvent ) override;
    void mouseMoveEvent( QMouseEvent* aEvent ) override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;
    void keyPressEvent( QKeyEvent* aEvent ) override;

private:
    /// Draw the view into m_image.
    void render();

    /// Copy the raster of the GAL (called at the end of each drawing).
    void present();

    /// A point of the widget in device pixels.
    VECTOR2D toDevice( const QPointF& aPoint ) const;

    void zoomAt( double aFactor, const VECTOR2D& aDevicePoint );

    KIGFX::GAL_DISPLAY_OPTIONS            m_options;
    std::unique_ptr<KIGFX::QT_CAIRO_GAL>  m_gal;
    std::unique_ptr<KIGFX::PAINTER>       m_painter;
    std::unique_ptr<KIGFX::VIEW>          m_view;     ///< Destroyed before the painter

    QImage                                m_image;    ///< The last drawing
    /// The box of the last ZoomToFit(): shown again when the canvas is resized, until the user
    /// zooms or pans
    std::optional<BOX2I>                  m_fitBox;
    bool                                  m_panning = false;
    QPointF                               m_panStart;

    bool                                  m_leftDown = false;
    bool                                  m_dragging = false;
    QPointF                               m_leftStart;    ///< Where the left button was pressed

    std::optional<BOX2D>                  m_selectionArea;
    bool                                  m_selectionCrossing = false;
};
