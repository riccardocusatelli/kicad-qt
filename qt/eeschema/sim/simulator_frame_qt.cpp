/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file simulator_frame_qt.cpp
 * SIMULATOR_FRAME of the Qt build: the simulator (its plots, with mathplot, and its property grids)
 * isn't on Qt yet.  Its constructor throws SIMULATOR_INIT_ERR before building the frame, as when
 * ngspice can't be loaded: the KIFACE of eeschema then has no simulator frame, and the schematic
 * editor works without it.
 */

#include <sim/simulator_frame.h>

#include <eda_units.h>


namespace
{

KIWAY* simulatorUnavailable( KIWAY* aKiway )
{
    throw SIMULATOR_INIT_ERR( "The simulator is not available on Qt yet" );
    return aKiway;
}

} // namespace


SIMULATOR_FRAME::SIMULATOR_FRAME( KIWAY* aKiway, TOOLKIT_WINDOW* aParent ) :
        KIWAY_PLAYER( simulatorUnavailable( aKiway ), aParent, FRAME_SIMULATOR, _( "Simulator" ),
                      wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, wxT( "simulator" ),
                      unityScale ),
        m_schematicFrame( nullptr ),
        m_toolBar( nullptr ),
        m_ui( nullptr ),
        m_consoleReporter( nullptr ),
        m_stateListener( nullptr ),
        m_simFinished( false ),
        m_workbookModified( false )
{
}


SIMULATOR_FRAME::~SIMULATOR_FRAME() = default;

bool SIMULATOR_FRAME::LoadSimulator( const wxString&, unsigned ) { return false; }
void SIMULATOR_FRAME::ReloadSimulator( const wxString&, unsigned ) {}
void SIMULATOR_FRAME::StartSimulation() {}
SIM_TAB* SIMULATOR_FRAME::NewSimTab( const wxString& ) { return nullptr; }
bool SIMULATOR_FRAME::EditAnalysis() { return false; }
const std::vector<wxString> SIMULATOR_FRAME::SimPlotVectors() { return {}; }
const std::vector<wxString> SIMULATOR_FRAME::Signals() { return {}; }


const std::map<int, wxString>& SIMULATOR_FRAME::UserDefinedSignals()
{
    static const std::map<int, wxString> none;
    return none;
}


void SIMULATOR_FRAME::SetUserDefinedSignals( const std::map<int, wxString>& ) {}
void SIMULATOR_FRAME::AddVoltageTrace( const wxString& ) {}
void SIMULATOR_FRAME::AddCurrentTrace( const wxString& ) {}
void SIMULATOR_FRAME::AddTuner( const SCH_SHEET_PATH&, SCH_SYMBOL* ) {}
SIM_TAB* SIMULATOR_FRAME::GetCurrentSimTab() const { return nullptr; }
void SIMULATOR_FRAME::ToggleSimConsole() {}
void SIMULATOR_FRAME::ToggleSimSidePanel() {}
void SIMULATOR_FRAME::ToggleDarkModePlots() {}
void SIMULATOR_FRAME::ShowChangedLanguage() { KIWAY_PLAYER::ShowChangedLanguage(); }
bool SIMULATOR_FRAME::LoadWorkbook( const wxString& ) { return false; }
bool SIMULATOR_FRAME::SaveWorkbook( const wxString& ) { return false; }
void SIMULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* ) {}
void SIMULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* ) {}
void SIMULATOR_FRAME::CommonSettingsChanged( int aFlags ) { KIWAY_PLAYER::CommonSettingsChanged( aFlags ); }
WINDOW_SETTINGS* SIMULATOR_FRAME::GetWindowSettings( APP_SETTINGS_BASE* ) { return nullptr; }
wxString SIMULATOR_FRAME::GetCurrentSimCommand() const { return wxEmptyString; }
SIM_TYPE SIMULATOR_FRAME::GetCurrentSimType() const { return ST_UNKNOWN; }
int SIMULATOR_FRAME::GetCurrentOptions() const { return 0; }
wxString SIMULATOR_FRAME::TakeSimReportMessages() { return wxEmptyString; }
void SIMULATOR_FRAME::UpdateTitle() {}
void SIMULATOR_FRAME::OnModify() { KIWAY_PLAYER::OnModify(); }
void SIMULATOR_FRAME::setupTools() {}
void SIMULATOR_FRAME::doReCreateMenuBar() {}
void SIMULATOR_FRAME::setupUIConditions() {}
void SIMULATOR_FRAME::showNetlistErrors( const WX_STRING_REPORTER& ) {}
void SIMULATOR_FRAME::destroyTabPlot( SIM_TAB* ) {}
bool SIMULATOR_FRAME::canCloseWindow( CLOSE_REQUEST& ) { return true; }
void SIMULATOR_FRAME::doCloseWindow() {}
void SIMULATOR_FRAME::onUpdateSim( wxCommandEvent& ) {}
void SIMULATOR_FRAME::onSimStarted( wxCommandEvent& ) {}
void SIMULATOR_FRAME::onSimFinished( wxCommandEvent& ) {}
void SIMULATOR_FRAME::onExit( wxCommandEvent& ) {}


wxDEFINE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );
