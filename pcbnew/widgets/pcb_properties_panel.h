/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCB_PROPERTIES_PANEL_H
#define PCB_PROPERTIES_PANEL_H

#include <widgets/properties_panel.h>
#include <toolkit/toolkit_types.h>
#include <memory>
#include <set>

class SELECTION;
class BOARD;
class PCB_BASE_EDIT_FRAME;
class PROPERTY_MANAGER;

class PCB_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    PCB_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, PCB_BASE_EDIT_FRAME* aFrame );

    virtual ~PCB_PROPERTIES_PANEL();

    void UpdateData() override;

    void AfterCommit() override;

    PCB_BASE_EDIT_FRAME* GetFrame() const { return m_frame; }

protected:
    void rebuildProperties( const SELECTION& aSelection ) override;
    TOOLKIT_PG_PROPERTY* createPGProperty( const PROPERTY_BASE* aProperty ) const override;
    bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue ) override;

    /// @return the property @a aName of the front item of the selection.
    PROPERTY_BASE* getPropertyByName( const wxString& aName ) const;

    bool valueChanging( const wxString& aName, const wxVariant& aValue ) override;
    void valueChanged( const wxString& aName, const wxVariant& aValue ) override;

    ///< Regenerates caches storing layer and net names
    void updateLists( const BOARD* aBoard );

    /**
     * Get the current selection from the selection tool.
     * If the selection is empty and we're in the footprint editor, returns the footprint instead.
     *
     * @param aSelection [out] reference to a SELECTION pointer that will be set to the selection
     * @param aFallbackSelection [out] local SELECTION object for fallback footprint selection
     * @return const SELECTION& reference to the selection (either real selection or fallback)
     */
    const SELECTION& getSelection( SELECTION& aFallbackSelection );

    /**
     * Get the front item of the current selection.
     * If the selection is empty and we're in the footprint editor, returns the footprint instead.
     *
     * @return EDA_ITEM* pointer to the front item, or nullptr if no selection
     */
    EDA_ITEM* getFrontItem();

private:
    /// Register the custom editors of the properties with the grid of the GUI toolkit
    /// (pcb_properties_panel_wx.cpp, qt/pcbnew/widgets/pcb_properties_panel_qt.cpp).
    void registerEditors();

    /// The editors of the grid of the GUI toolkit, defined with the toolkit parts.
    struct TOOLKIT_STATE;

    struct TOOLKIT_STATE_DELETER
    {
        void operator()( TOOLKIT_STATE* aState ) const;
    };

    std::unique_ptr<TOOLKIT_STATE, TOOLKIT_STATE_DELETER> m_toolkitState;

protected:
    PCB_BASE_EDIT_FRAME* m_frame;
    PROPERTY_MANAGER&    m_propMgr;

    static std::set<wxString> m_currentFieldNames;
};

#endif /* PCB_PROPERTIES_PANEL_H */
