/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @file properties_panel.h
 * The panel of the properties of the selected items with Qt: the Qt version of
 * common/widgets/properties_panel.h (the logic of the panel is the toolkit-independent
 * common/widgets/properties_panel.cpp; the grid is a PROPERTY_GRID).
 */

#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include <QWidget>

#include <properties/pg_factory.h>
#include <properties/property_choices.h>

#include <wx/string.h>
#include <wx/variant.h>

#include <vector>

class EDA_BASE_FRAME;
class EDA_ITEM;
class PROPERTY_BASE;
class PROPERTY_GRID;
class QLabel;
class SELECTION;


/**
 * The panel of the properties of the selected items: a property grid, with a row for each
 * property common to the items.
 */
class PROPERTIES_PANEL : public QWidget
{
public:
    PROPERTIES_PANEL( QWidget* aParent, EDA_BASE_FRAME* aFrame );

    virtual ~PROPERTIES_PANEL();

    virtual void UpdateData() = 0;

    virtual void AfterCommit() {}

    PROPERTY_GRID* GetPropertyGrid() { return m_grid; }

    int PropertiesCount() const { return m_displayed.size(); }

    const std::vector<PROPERTY_BASE*>& Properties() const { return m_displayed; }

    void RecalculateSplitterPos();

    void  SetSplitterProportion( float aProportion );
    float SplitterProportion() const { return m_splitter_key_proportion; }

    /// Rebuild the grid in the language of the user interface.
    void OnLanguageChanged();

protected:
    /**
     * Generates the property grid for a given selection of items.
     *
     * @param aSelection is a set of items to show properties for.
     */
    virtual void rebuildProperties( const SELECTION& aSelection );

    virtual TOOLKIT_PG_PROPERTY* createPGProperty( const PROPERTY_BASE* aProperty ) const = 0;

    /**
     * The user is changing the property @a aName to @a aValue.
     *
     * @return false to refuse the value.
     */
    virtual bool valueChanging( const wxString& aName, const wxVariant& aValue ) { return true; }

    /// The user changed the property @a aName to @a aValue.
    virtual void valueChanged( const wxString& aName, const wxVariant& aValue ) {}

    /**
     * Utility to fetch a property value and convert to wxVariant
     * Precondition: aItem is known to have property aProperty
     * @return true if conversion succeeded
     */
    virtual bool getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue );

    /**
     * Processes a selection and determines whether the given property should be available or not
     * and what the common value should be for the items in the selection.
     * @param aSelection is a set of EDA_ITEMs to process
     * @param aProperty is the property to look up
     * @param aValue will be filled with the value common to the selection, or null if different
     * @param aWritable will be set to whether or not the property can be written for the selection
     * @return true if the property is available for all the items in the selection
     */
    bool extractValueAndWritability( const SELECTION& aSelection, const wxString& aPropName,
                                     wxVariant& aValue, bool& aWritable, PROPERTY_CHOICES& aChoices );

    void showEvent( QShowEvent* aEvent ) override;
    void resizeEvent( QResizeEvent* aEvent ) override;

private:
    /// The grid of the GUI toolkit, for the toolkit-independent logic of the panel
    void                              setCaption( const wxString& aCaption );
    void                              gridReset();
    std::vector<TOOLKIT_PG_PROPERTY*> gridProperties() const;
    void                              gridAppendCategory( const wxString& aCaption );
    void                              gridAppend( TOOLKIT_PG_PROPERTY* aProperty );

public:
    int                         m_SuppressGridChangeEvents;

protected:
    std::vector<PROPERTY_BASE*> m_displayed;    // no ownership of pointers
    PROPERTY_GRID*              m_grid;
    EDA_BASE_FRAME*             m_frame;
    QLabel*                     m_caption;

    /// Proportion of the grid column splitter that is used for the key column (0.0 - 1.0)
    float m_splitter_key_proportion;
};


#endif /* PROPERTIES_PANEL_H */
