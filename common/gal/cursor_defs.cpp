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


#include <gal/cursor_defs.h>

// Cursor files
#include <cursors/cursor-add.xpm>
#include <cursors/cursor-component.xpm>
#include <cursors/cursor-eraser.xpm>
#include <cursors/cursor-label-global.xpm>
#include <cursors/cursor-label-hier.xpm>
#include <cursors/cursor-label-net.xpm>
#include <cursors/cursor-line-bus.xpm>
#include <cursors/cursor-line-graphic.xpm>
#include <cursors/cursor-line-wire.xpm>
#include <cursors/cursor-measure.xpm>
#include <cursors/cursor-pencil.xpm>
#include <cursors/cursor-select-lasso.xpm>
#include <cursors/cursor-select-window.xpm>
#include <cursors/cursor-subtract.xpm>
#include <cursors/cursor-text.xpm>
#include <cursors/cursor-xor.xpm>
#include <cursors/cursor-zoom-in.xpm>
#include <cursors/cursor-zoom-out.xpm>
// Under MSW, the standard cursor is white on black.  Elsewhere it is black on white
#ifdef __WINDOWS__
#include <cursors/cursor_tune.xpm>
#include <cursors/voltage_probe_win.xpm>
#include <cursors/current_probe_win.xpm>
#else
#include <cursors/cursor_tune_black.xpm>
#include <cursors/voltage_probe_black.xpm>
#include <cursors/current_probe_black.xpm>
#endif

// HiDPI cursor files
#include <cursors/cursor-add64.xpm>
#include <cursors/cursor-component64.xpm>
#include <cursors/cursor-eraser64.xpm>
#include <cursors/cursor-label-global64.xpm>
#include <cursors/cursor-label-hier64.xpm>
#include <cursors/cursor-label-net64.xpm>
#include <cursors/cursor-line-bus64.xpm>
#include <cursors/cursor-line-graphic64.xpm>
#include <cursors/cursor-line-wire64.xpm>
#include <cursors/cursor-measure64.xpm>
#include <cursors/cursor-pencil64.xpm>
#include <cursors/cursor-select-lasso64.xpm>
#include <cursors/cursor-select-window64.xpm>
#include <cursors/cursor-subtract64.xpm>
#include <cursors/cursor-text64.xpm>
#include <cursors/cursor-xor64.xpm>
#include <cursors/cursor-zoom-in64.xpm>
#include <cursors/cursor-zoom-out64.xpm>
#ifdef __WINDOWS__
#include <cursors/cursor_tune64.xpm>
#include <cursors/voltage_probe_win64.xpm>
#include <cursors/current_probe_win64.xpm>
#else
#include <cursors/cursor_tune_black64.xpm>
#include <cursors/voltage_probe_black64.xpm>
#include <cursors/current_probe_black64.xpm>
#endif


// Under MSW, the standard cursor is white on black.  Elsewhere it is black on white
#ifdef __WINDOWS__
#include <cursors/cursor-place.xpm>
#include <cursors/cursor-place64.xpm>
#include <cursors/cursor-select-m.xpm>
#include <cursors/cursor-select-m64.xpm>
#include <cursors/cursor-warning.xpm>
#include <cursors/cursor-warning64.xpm>
#else
#include <cursors/cursor-place-black.xpm>
#include <cursors/cursor-place-black64.xpm>
#include <cursors/cursor-select-m-black.xpm>
#include <cursors/cursor-select-m-black64.xpm>
#include <cursors/cursor-warning-black.xpm>
#include <cursors/cursor-warning-black64.xpm>
#endif


static const std::map<KICURSOR, std::vector<CURSOR_DEF>> cursors_defs = {
    {
        KICURSOR::VOLTAGE_PROBE,
        {
            { voltage_probe_xpm, { 1, 31 } },
            { voltage_probe64_xpm, { 1, 62 } }
        }
    },
    {
        KICURSOR::CURRENT_PROBE,
        {
            { current_probe_xpm, { 4, 27 } },
            { current_probe64_xpm, { 8, 54 } }
        }
    },
    {
        KICURSOR::TUNE,
        {
            { cursor_tune_xpm, { 1, 30 } },
            { cursor_tune64_xpm, { 2, 60 } }
        }
    },
    {
        KICURSOR::PENCIL,
        {
            { cursor_pencil_xpm, { 4, 27 } },
            { cursor_pencil64_xpm, { 8, 54 } }
        }
    },
    {
        KICURSOR::MOVING,
        {
            {
#ifdef __WINDOWS__
                cursor_select_m_xpm,
#else
                cursor_select_m_black_xpm,
#endif
                { 1, 1 }
            },
            {
#ifdef __WINDOWS__
                cursor_select_m64_xpm,
#else
                cursor_select_m_black64_xpm,
#endif
                { 2, 2 }
            }
        }
    },
    {
        KICURSOR::WARNING,
        {
            {
#ifdef __WINDOWS__
                cursor_warning_xpm,
#else
                cursor_warning_black_xpm,
#endif
                { 1, 1 }
            },
            {
#ifdef __WINDOWS__
                cursor_warning64_xpm,
#else
                cursor_warning_black64_xpm,
#endif
                { 2, 2 }
            }
        }
    },
    {
        KICURSOR::REMOVE,
        {
            { cursor_eraser_xpm, { 4, 4 } },
            { cursor_eraser64_xpm, { 8, 8 } }
        }
    },
    {
        KICURSOR::TEXT,
        {
            { cursor_text_xpm, { 7, 7 } },
            { cursor_text64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::MEASURE,
        {
            { cursor_measure_xpm, { 4, 4 } },
            { cursor_measure64_xpm, { 8, 8 } }
        }
    },
    {
        KICURSOR::ADD,
        {
            { cursor_add_xpm, { 7, 7 } },
            { cursor_add64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::SUBTRACT,
        {
            { cursor_subtract_xpm, { 7, 7 } },
            { cursor_subtract64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::XOR,
        {
            { cursor_xor_xpm, { 7, 7 } },
            { cursor_xor64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::ZOOM_IN,
        {
            { cursor_zoom_in_xpm, { 6, 6 } },
            { cursor_zoom_in64_xpm, { 12, 12 } }
        }
    },
    {
        KICURSOR::ZOOM_OUT,
        {
            { cursor_zoom_out_xpm, { 6, 6 } },
            { cursor_zoom_out64_xpm, { 12, 12 } }
        }
    },
    {
        KICURSOR::LABEL_NET,
        {
            { cursor_label_net_xpm, { 7, 7 } },
            { cursor_label_net64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::LABEL_GLOBAL,
        {
            { cursor_label_global_xpm, { 7, 7 } },
            { cursor_label_global64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::COMPONENT,
        {
            { cursor_component_xpm, { 7, 7 } },
            { cursor_component64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::SELECT_LASSO,
        {
            { cursor_select_lasso_xpm, { 7, 7 } },
            { cursor_select_lasso64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::SELECT_WINDOW,
        {
            { cursor_select_window_xpm, { 7, 10 } },
            { cursor_select_window64_xpm, { 14, 20 } }
        }
    },
    {
        KICURSOR::LINE_BUS,
        {
            { cursor_line_bus_xpm, { 5, 26 } },
            { cursor_line_bus64_xpm, { 10, 52 } }
        }
    },
    {
        KICURSOR::LINE_WIRE,
        {
            { cursor_line_wire_xpm, { 5, 26 } },
            { cursor_line_wire64_xpm, { 10, 52 } }
        }
    },
    {
        KICURSOR::LINE_GRAPHIC,
        {
            { cursor_line_graphic_xpm, { 5, 26 } },
            { cursor_line_graphic64_xpm, { 10, 52 } }
        }
    },
    {
        KICURSOR::LABEL_HIER,
        {
            { cursor_label_hier_xpm, { 7, 7 } },
            { cursor_label_hier64_xpm, { 14, 14 } }
        }
    },
    {
        KICURSOR::PLACE,
        {
            {
#ifdef __WINDOWS__
                cursor_place_xpm,
#else
                cursor_place_black_xpm,
#endif
                { 1, 1 }
            },
            {
#ifdef __WINDOWS__
                cursor_place64_xpm,
#else
                cursor_place_black64_xpm,
#endif
                { 2, 2 }
            }
        }
    }
};


const std::map<KICURSOR, std::vector<CURSOR_DEF>>& CursorDefinitions()
{
    return cursors_defs;
}
