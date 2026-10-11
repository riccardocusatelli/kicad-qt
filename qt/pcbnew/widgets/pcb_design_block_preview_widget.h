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

/**
 * @file pcb_design_block_preview_widget.h
 * PCB_DESIGN_BLOCK_PREVIEW_WIDGET of the Qt build: the board of a design block on a
 * PCB_DRAW_PANEL_GAL, or a status in its place.  What the widget does is shared with the
 * wxWidgets build (pcbnew/widgets/pcb_design_block_preview_widget.cpp).
 */

#ifndef PCB_DESIGN_BLOCK_PREVIEW_WIDGET_H
#define PCB_DESIGN_BLOCK_PREVIEW_WIDGET_H

#include <widgets/design_block_preview_widget.h>
#include <kiway.h>
#include <gal_display_options_common.h>
#include <class_draw_panel_gal.h>


class BOARD;
class LIB_ID;
class DESIGN_BLOCK;
class SCHEMATIC;
class PCB_SHEET;
class PCB_EDIT_FRAME;
class QLabel;
class QStackedLayout;
class QResizeEvent;


class PCB_DESIGN_BLOCK_PREVIEW_WIDGET : public DESIGN_BLOCK_PREVIEW_WIDGET
{
public:
    /**
     * Construct a schematic design block preview widget.
     *
     * @param aParent - parent window
     */
    PCB_DESIGN_BLOCK_PREVIEW_WIDGET( QWidget* aParent, PCB_EDIT_FRAME* aFrame );

    ~PCB_DESIGN_BLOCK_PREVIEW_WIDGET() override;

    /**
     * Return whether the widget initialized properly. This could return false if Kiway is
     * not available. If this returns false, no other methods should be called.
     */
    bool IsInitialized() const override { return m_preview != nullptr; }

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( const wxString& aText ) override;

    /**
     * Set the currently displayed symbol.
     */
    void DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock ) override;

protected:
    /// The design block stays fitted to the canvas.
    void resizeEvent( QResizeEvent* aEvent ) override;

    void fitOnDrawArea();    // set the view scale to fit the item on screen and center

    // The widgets of the GUI toolkit (pcb_design_block_preview_widget_qt.cpp)

    /// Load the board of a design block (with a progress reporter).
    BOARD* loadBoard( const wxString& aFileName );

    /// Show the canvas in place of the status.
    void showCanvas();

    GAL_DISPLAY_OPTIONS_IMPL   m_galDisplayOptions;
    EDA_DRAW_PANEL_GAL*        m_preview;

    /// The status (a message in place of the canvas), on the colors of the canvas
    QLabel*                    m_status;
    QWidget*                   m_statusPanel;

    /// The canvas or the status
    QStackedLayout*            m_stack;

    BOARD*                     m_previewItem;

    /// The bounding box of the current item
    BOX2I                      m_itemBBox;
};


#endif
