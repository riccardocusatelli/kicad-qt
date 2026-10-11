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
 * @file design_block_preview_widget.h
 * DESIGN_BLOCK_PREVIEW_WIDGET of the Qt build: the base of the previews of the design blocks of
 * the editors (a QWidget, a wxPanel in common/widgets/design_block_preview_widget.h).
 */

#ifndef DESIGN_BLOCK_PREVIEW_WIDGET_H
#define DESIGN_BLOCK_PREVIEW_WIDGET_H

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QWidget>

#include <kiway.h>
#include <gal_display_options_common.h>
#include <class_draw_panel_gal.h>

class LIB_ID;
class DESIGN_BLOCK;


class DESIGN_BLOCK_PREVIEW_WIDGET : public QWidget
{
public:
    /**
     * Construct a design block preview widget.
     *
     * @param aParent - parent window
     */
    DESIGN_BLOCK_PREVIEW_WIDGET( QWidget* aParent ) : QWidget( aParent ) {}

    ~DESIGN_BLOCK_PREVIEW_WIDGET() override = default;

    /**
     * Return whether the widget initialized properly. This could return false if Kiway is
     * not available. If this returns false, no other methods should be called.
     */
    virtual bool IsInitialized() const = 0;

    /**
     * Set the contents of the status label and display it.
     */
    virtual void SetStatusText( const wxString& aText ) = 0;

    /**
     * Set the currently displayed design block.
     */
    virtual void DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock ) = 0;
};

#endif // DESIGN_BLOCK_PREVIEW_WIDGET_H
