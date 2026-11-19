/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_WIZARD_PROPERTIES_PANEL_H
#define FOOTPRINT_WIZARD_PROPERTIES_PANEL_H

#include <widgets/properties_panel.h>

#include <map>
#include <memory>
#include <vector>

#include <toolkit/toolkit_types.h>
#include <wx/any.h>

class FOOTPRINT_WIZARD_FRAME;
class FOOTPRINT_WIZARD;
class PG_UNIT_EDITOR;
class PG_CHECKBOX_EDITOR;
class PG_RATIO_EDITOR;
class WIZARD_PARAMETER;

class FOOTPRINT_WIZARD_PROPERTIES_PANEL : public PROPERTIES_PANEL
{
public:
    FOOTPRINT_WIZARD_PROPERTIES_PANEL( TOOLKIT_WINDOW* aParent, FOOTPRINT_WIZARD_FRAME* aFrame );
    ~FOOTPRINT_WIZARD_PROPERTIES_PANEL() override;

    void UpdateData() override;

    void RebuildParameters( FOOTPRINT_WIZARD* aWizard );

protected:
#ifdef KICAD_WX_FREE
    TOOLKIT_PG_PROPERTY* createPGProperty( const PROPERTY_BASE* aProperty ) const override
    {
        return nullptr;
    }

    TOOLKIT_PG_PROPERTY* createPGProperty( WIZARD_PARAMETER* aParam ) const;
    bool valueChanging( const wxString& aName, const wxVariant& aValue ) override;
    void valueChanged( const wxString& aName, const wxVariant& aValue ) override;

    /// The parameter of a property of the grid (by its name, the identifier of the parameter).
    WIZARD_PARAMETER* getParamFromName( const wxString& aName ) const;
#else
    wxPGProperty* createPGProperty( const PROPERTY_BASE* aProperty ) const override { return nullptr; }
    wxPGProperty* createPGProperty( WIZARD_PARAMETER* aParam ) const;
    void valueChanging( wxPropertyGridEvent& aEvent ) override;
    void valueChanged( wxPropertyGridEvent& aEvent ) override;
    static WIZARD_PARAMETER* getParamFromEvent( const wxPropertyGridEvent& aEvent );
#endif

    /// Check a new value of a parameter: false (with the reason in the build messages) to refuse it.
    bool checkValue( WIZARD_PARAMETER* aParam, const wxAny& aValue ) const;

    /// Store a new value in a parameter.
    void applyValue( WIZARD_PARAMETER* aParam, const wxAny& aValue );

    wxString formatConstraintValue( const WIZARD_PARAMETER* aParam, int aValue ) const;
    wxString formatConstraintValue( const WIZARD_PARAMETER* aParam, double aValue ) const;

private:
    struct WIZARD_PARAM_INFO
    {
        int      page = -1;
        int      index = -1;
        wxString units;
    };

    FOOTPRINT_WIZARD_FRAME* m_frame;
    std::vector<std::unique_ptr<WIZARD_PARAM_INFO>> m_paramInfos;

#ifdef KICAD_WX_FREE
    /// The parameters shown in the grid, by their identifier
    std::map<wxString, WIZARD_PARAMETER*> m_params;
#endif

    PG_UNIT_EDITOR*     m_unitEditorInstance;
    PG_CHECKBOX_EDITOR* m_checkboxEditorInstance;
    PG_RATIO_EDITOR*    m_ratioEditorInstance;
};

#endif // FOOTPRINT_WIZARD_PROPERTIES_PANEL_H
