/**
 * @file kicad_clipboard.h
 * @brief
 * @author Kristoffer Ödmark
 * @version 1.0
 * @date 2017-05-03
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_CLIPBOARD_H
#define KICAD_CLIPBOARD_H

#include <functional>
#include <vector>

#include <board_item.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>

class PCB_SELECTION;


/**
 * The board items on a clipboard, as KiCad s-expression text.
 *
 * The text goes to and comes from the clipboard through a writer and a reader: the ones of
 * the toolkit of the editor.
 */
class CLIPBOARD_IO : public PCB_IO_KICAD_SEXPR
{
public:
    /// With the clipboard of wxWidgets, and its dialogs to ask the user (kicad_clipboard_gui.cpp).
    CLIPBOARD_IO();

    /// With a clipboard that @a aWriter writes and @a aReader reads.
    CLIPBOARD_IO( std::function<void( const wxString& )> aWriter,
                  std::function<wxString()>                aReader );

    ~CLIPBOARD_IO();

    void SetWriter( std::function<void(const wxString&)> aWriter ) { m_writer = aWriter; }
    void SetReader( std::function<wxString()> aReader ) { m_reader = aReader; }

    /*
     * Saves the entire board to the clipboard formatted using the PCB_IO_KICAD_SEXPR formatting
     */
    void SaveBoard( const wxString& aFileName, BOARD* aBoard,
                    const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    /**
     * Write all the settings of the BOARD* set by SetBoard() and then the board items of
     * @a aItems formatted by PCB_IO_KICAD_SEXPR to the clipboard as sexpr text.
     *
     * @param aRefPoint the point of the items placed at (0, 0) in the clipboard.
     */
    void SaveItems( const std::vector<EDA_ITEM*>& aItems, const VECTOR2I& aRefPoint,
                    bool isFootprintEditor );

    /// SaveItems() for the items of a selection, from its reference point (kicad_clipboard_gui.cpp).
    void SaveSelection( const PCB_SELECTION& selected, bool isFootprintEditor );

    BOARD_ITEM* Parse();

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    void SetBoard( BOARD* aBoard );

private:
    STRING_FORMATTER m_formatter;
    std::function<void(const wxString&)> m_writer;
    std::function<wxString()> m_reader;
};


#endif /* KICAD_CLIPBOARD_H */
