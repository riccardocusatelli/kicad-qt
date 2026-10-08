/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#pragma once

#include <widgets/properties_panel.h>
#include <toolkit/toolkit_types.h>
#include <memory>
#include <set>
#include <string>

class SELECTION;
class SCHEMATIC;
class SCH_BASE_FRAME;
class SCH_COMMIT;
class SCH_EDIT_FRAME;
class SCH_SHEET;
class PROPERTY_MANAGER;

class SCH_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    SCH_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, SCH_BASE_FRAME* aFrame );

    virtual ~SCH_PROPERTIES_PANEL();

    void UpdateData() override;

    void AfterCommit() override;

protected:
    void rebuildProperties( const SELECTION& aSelection ) override;
    TOOLKIT_PG_PROPERTY* createPGProperty( const PROPERTY_BASE* aProperty ) const override;
    bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue ) override;

    /// @return the property @a aName of the front item of the selection.
    PROPERTY_BASE* getPropertyByName( const wxString& aName ) const;

    bool valueChanging( const wxString& aName, const wxVariant& aValue ) override;
    void valueChanged( const wxString& aName, const wxVariant& aValue ) override;

    bool handleSheetFilenameChange( SCH_EDIT_FRAME* aFrame, SCH_SHEET* aSheet,
                                    SCH_COMMIT& aChanges, const wxString& aNewFilename );

    /**
     * Get the current selection from the selection tool.
     * If the selection is empty and we're in the symbol editor, returns the current symbol instead.
     *
     * @param aFallbackSelection [out] local SELECTION object for fallback symbol selection
     * @return const SELECTION& reference to the selection (either real selection or fallback)
     */
    const SELECTION& getSelection( SELECTION& aFallbackSelection );

    /**
     * Get the front item of the current selection.
     * If the selection is empty and we're in the symbol editor, returns the current symbol instead.
     *
     * @return EDA_ITEM* pointer to the front item, or nullptr if no selection
     */
    EDA_ITEM* getFrontItem();

    /**
     * The netlist of the selected symbols for the footprint chooser (their pins and their
     * footprint filters), or an empty string if they aren't the same library symbol.
     */
    std::string symbolNetlist() const;

private:
    /// Register the custom editors of the properties with the grid of the GUI toolkit
    /// (sch_properties_panel_wx.cpp).
    void registerEditors();

    /// The editors of the grid of the GUI toolkit, defined with the toolkit parts.
    struct TOOLKIT_STATE;

    struct TOOLKIT_STATE_DELETER
    {
        void operator()( TOOLKIT_STATE* aState ) const;
    };

    std::unique_ptr<TOOLKIT_STATE, TOOLKIT_STATE_DELETER> m_toolkitState;

protected:
    SCH_BASE_FRAME*     m_frame;
    PROPERTY_MANAGER&   m_propMgr;

    static std::set<wxString> m_currentSymbolFieldNames;
    static std::set<wxString> m_currentSheetFieldNames;
};
