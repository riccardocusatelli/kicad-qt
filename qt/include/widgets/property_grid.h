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
 * @file property_grid.h
 * The property grid of the properties panels with Qt: the rows of the PG_PROPERTY properties
 * (by category), their values and their editors (the wxPropertyGrid of the wxWidgets build).
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <QTreeWidget>

#include <wx/string.h>
#include <wx/variant.h>

#include <properties/pg_properties.h>

class EDA_DRAW_FRAME;
class PROPERTY_GRID;


/**
 * An editor of the properties given to the grid by a panel, for the values the grid cannot
 * edit itself (the nets of the board).
 */
class PG_CUSTOM_EDITOR
{
public:
    virtual ~PG_CUSTOM_EDITOR() = default;

    /// The editor of @a aProperty, showing its value.
    virtual QWidget* CreateEditor( QWidget* aParent, PROPERTY_GRID* aGrid,
                                   PG_PROPERTY* aProperty ) = 0;

    /// The value of the editor; false if it has none.
    virtual bool GetValue( QWidget* aEditor, PG_PROPERTY* aProperty, wxVariant& aValue ) = 0;
};


/**
 * The grid of properties: two columns (the name and the value), the properties grouped under
 * their categories.  The values are edited in place, as their PG_PROPERTY::EDITOR says; a new
 * value goes through OnChanging (which may refuse it) and then to OnChanged.
 */
class PROPERTY_GRID : public QTreeWidget
{
public:
    explicit PROPERTY_GRID( QWidget* aParent = nullptr );
    ~PROPERTY_GRID() override;

    /// Remove all the properties (an edit in progress is committed first).
    void Clear();

    /// Append a property (the grid owns it); the properties follow the last category.
    void Append( PG_PROPERTY* aProperty );

    /// The properties, categories included, in their order.
    std::vector<PG_PROPERTY*> Properties() const;

    PG_PROPERTY* GetSelectedProperty() const;

    /// Show the current state of @a aProperty (its value, choices, read-only state).
    void RefreshProperty( PG_PROPERTY* aProperty );

    /**
     * Change the value of a property as the user does: OnChanging may refuse the value, then
     * OnChanged is called (once the grid is back in the event loop, since it may rebuild the
     * grid).
     *
     * @return false if the value was refused.
     */
    bool ChangePropertyValue( PG_PROPERTY* aProperty, const wxVariant& aValue );

    /// Commit the value of the editor that is open, if any.
    void CommitChangesFromEditor();
    bool IsEditorFocused() const;

    void SetSplitterPosition( int aPosition );
    int  GetSplitterPosition() const;
    void CenterSplitter();

    /// The frame of the unit editors (its units and origin) and of the custom editors.
    void            SetEditorFrame( EDA_DRAW_FRAME* aFrame ) { m_frame = aFrame; }
    EDA_DRAW_FRAME* GetEditorFrame() const { return m_frame; }

    /// The editor of the properties of a kind that the grid doesn't edit itself (NET).
    void SetCustomEditor( PG_PROPERTY::EDITOR aKind, std::unique_ptr<PG_CUSTOM_EDITOR> aEditor );
    PG_CUSTOM_EDITOR* GetCustomEditor( PG_PROPERTY::EDITOR aKind ) const;

    /// A property is about to change (name, new value); false refuses the value.
    std::function<bool( const wxString&, const wxVariant& )> OnChanging;

    /// A property changed (name, new value).
    std::function<void( const wxString&, const wxVariant& )> OnChanged;

    /// The user moved the splitter between the names and the values.
    std::function<void()> OnSplitterMoved;

    /// The property of a row (nullptr for no row).
    PG_PROPERTY* PropertyOf( const QModelIndex& aIndex ) const;

    /// The editor that is open, if any (set by the editors of the grid).
    void SetActiveEditor( QWidget* aEditor ) { m_activeEditor = aEditor; }

    /// Choose a value for the properties edited with a dialog (colors) or a button (FPID, URL).
    void RunDialogEditor( PG_PROPERTY* aProperty, const wxString& aCurrentText = wxString() );

protected:
    void keyPressEvent( QKeyEvent* aEvent ) override;

private:
    QTreeWidgetItem* itemOf( PG_PROPERTY* aProperty ) const;

    /// Select the next (or previous) property that can be edited, and edit it.
    void selectNextEditable( bool aForward );

    void onItemClicked( QTreeWidgetItem* aItem, int aColumn );

    std::vector<std::unique_ptr<PG_PROPERTY>>                         m_properties;
    std::map<PG_PROPERTY*, QTreeWidgetItem*>                          m_items;
    std::map<PG_PROPERTY::EDITOR, std::unique_ptr<PG_CUSTOM_EDITOR>>  m_customEditors;
    QTreeWidgetItem*                                                  m_category;
    EDA_DRAW_FRAME*                                                   m_frame;
    QWidget*                                                          m_activeEditor;
    bool                                                              m_filling;
};
