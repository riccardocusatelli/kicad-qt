/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file  trace_helpers.cpp
 * @brief wxLogTrace helper implementation.
 */

#include <trace_helpers.h>
#include <wx/utils.h>

#include <wx/tokenzr.h>

const wxChar* const traceFindReplace = wxT( "KICAD_FIND_REPLACE" );
const wxChar* const kicadTraceCoords = wxT( "KICAD_COORDS" );
const wxChar* const kicadTraceKeyEvent = wxT( "KICAD_KEY_EVENTS" );
const wxChar* const kicadTraceToolStack = wxT( "KICAD_TOOL_STACK" );
const wxChar* const kicadTraceCoroutineStack = wxT( "KICAD_COROUTINE_STACK" );
const wxChar* const traceSchLibMem = wxT( "KICAD_SCH_LIB_MEM" );
const wxChar* const traceFindItem = wxT( "KICAD_FIND_ITEM" );
const wxChar* const traceSchLegacyPlugin = wxT( "KICAD_SCH_LEGACY_PLUGIN" );
const wxChar* const traceSchPlugin = wxT( "KICAD_SCH_PLUGIN" );
const wxChar* const traceGedaPcbPlugin = wxT( "KICAD_GEDA_PLUGIN" );
const wxChar* const traceKicadPcbPlugin = wxT( "KICAD_PCB_PLUGIN" );
const wxChar* const tracePrinting = wxT( "KICAD_PRINT" );
const wxChar* const traceAutoSave = wxT( "KICAD_AUTOSAVE" );
const wxChar* const tracePathsAndFiles = wxT( "KICAD_PATHS_AND_FILES" );
const wxChar* const traceLocale = wxT( "KICAD_LOCALE" );
const wxChar* const traceFonts = wxT( "KICAD_FONTS" );
const wxChar* const traceScreen = wxT( "KICAD_SCREEN" );
const wxChar* const traceZoomScroll = wxT( "KICAD_ZOOM_SCROLL" );
const wxChar* const traceSymbolResolver = wxT( "KICAD_SYM_RESOLVE" );
const wxChar* const traceDisplayLocation = wxT( "KICAD_DISPLAY_LOCATION" );
const wxChar* const traceSchSheetPaths = wxT( "KICAD_SCH_SHEET_PATHS" );
const wxChar* const traceSchCurrentSheet = wxT( "KICAD_SCH_CURRENT_SHEET" );
const wxChar* const traceSchFieldRendering = wxT( "KICAD_SCH_FIELD_RENDERING" );
const wxChar* const traceSchPainter = wxT( "KICAD_SCH_PAINTER" );
const wxChar* const traceSchSymbolRef = wxT( "KICAD_SCH_SYMBOL_REF" );
const wxChar* const traceEnvVars = wxT( "KICAD_ENV_VARS" );
const wxChar* const traceSchDragNetCollision = wxT( "KICAD_SCH_DRAG_NET_COLLISION" );
const wxChar* const traceCrossProbeFlash = wxT( "CROSS_PROBE_FLASH" );
const wxChar* const traceGalProfile = wxT( "KICAD_GAL_PROFILE" );
const wxChar* const traceGalContext = wxT( "KICAD_GAL_CONTEXT" );
const wxChar* const traceGalGroups = wxT( "KICAD_GAL_GROUPS" );
const wxChar* const traceRhi = wxT( "KICAD_RHI" );
const wxChar* const traceStackedPins = wxT( "KICAD_STACKED_PINS" );
const wxChar* const traceLibWatch = wxT( "KICAD_LIB_WATCH" );
const wxChar* const traceKiCad2Step = wxT( "KICAD2STEP" );
const wxChar* const traceUiProfile = wxT( "KICAD_UI_PROFILE" );
const wxChar* const traceGit = wxT( "KICAD_GIT" );
const wxChar* const traceModelPreview = wxT( "KICAD_MODEL_PREVIEW" );
const wxChar* const traceDiffMerge = wxT( "KICAD_DIFF_MERGE" );
const wxChar* const traceEagleIo = wxT( "KICAD_EAGLE_IO" );
const wxChar* const traceDiptraceIo = wxT( "KICAD_DIPTRACE_IO" );
const wxChar* const traceEasyPc = wxT( "KICAD_EASYPC_IO" );
const wxChar* const traceCadstarIo = wxT( "KICAD_CADSTAR_IO" );
const wxChar* const traceFabmaster = wxT( "KICAD_FABMASTER" );
const wxChar* const traceAltiumIo = wxT( "KICAD_ALTIUM_IO" );
const wxChar* const traceOdbppIo = wxT( "KICAD_ODBPP_IO" );
const wxChar* const traceEasyEdaIo = wxT( "KICAD_EASYEDA_IO" );
const wxChar* const tracePadsIo = wxT( "KICAD_PADS_IO" );
const wxChar* const tracePcadIo = wxT( "KICAD_PCAD_IO" );
const wxChar* const traceDesignBlocks = wxT( "KICAD_DESIGN_BLOCK" );
const wxChar* const traceLibFieldTable = wxT( "KICAD_LIB_FIELD_TABLE" );
const wxChar* const tracePdfPlotter = wxT( "KICAD_PDF_PLOTTER" );
const wxChar* const traceSnap = wxT( "KICAD_SNAP" );
const wxChar* const traceLibraries = wxT( "KICAD_LIBRARIES" );
const wxChar* const traceDatabase = wxT( "KICAD_DATABASE" );
const wxChar* const traceSchMove = wxT( "KICAD_SCH_MOVE" );
const wxChar* const traceSymbolInheritance = wxT( "KICAD_SYMBOL_INHERITANCE" );
const wxChar* const traceSchNetChain = wxT( "KICAD_SCH_NETCHAIN" );
const wxChar* const traceImportNetNames = wxT( "KICAD_IMPORT_NET_NAMES" );

#ifdef KICAD_GAL_PROFILE
LATENCY_PROBE latencyProbeZoomToRender( "zoom-to-render", 16 );
LATENCY_PROBE latencyProbeRepaintToMotion( "repaint-to-motion", 16 );
#endif

wxString dump( const wxArrayString& aArray )
{
    wxString tmp;

    for( unsigned i = 0;  i < aArray.GetCount();  i++ )
    {
        if( aArray[i].IsEmpty() )
            tmp << wxT( "\"\" " );
        else
            tmp << aArray[i] << wxT( " " );
    }

    return tmp;
}


// The following code was shamelessly copied from the wxWidgets keyboard sample
// at https://github.com/wxWidgets/wxWidgets/blob/master/samples/keyboard/keyboard.cpp.

/////////////////////////////////////////////////////////////////////////////
// Author:      Vadim Zeitlin
// Modified by: Marcin Wojdyr
// Created:     07.04.02
// Copyright:   (c) 2002 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// helper function that returns textual description of wx virtual keycode



// helper function that returns textual description of key in the event



TRACE_MANAGER& TRACE_MANAGER::Instance()
{
    static TRACE_MANAGER* self = nullptr;

    if( !self )
    {
        self = new TRACE_MANAGER;
        self->init();
    }

    return *self;
}


bool TRACE_MANAGER::IsTraceEnabled( const wxString& aWhat )
{
    if( !m_printAllTraces )
    {
        if( !m_globalTraceEnabled )
            return false;

        if( m_enabledTraces.find( aWhat ) == m_enabledTraces.end() )
            return false;
    }

    return true;
}


void TRACE_MANAGER::printTrace( const wxString& aWhat, const wxString& aMessage )
{
#if defined( __UNIX__ ) || defined( _WIN32 )
    fprintf( stderr, " %-30s | %s", aWhat.c_str().AsChar(), aMessage.c_str().AsChar() );
#endif
}


void TRACE_MANAGER::init()
{
    wxString traceVars;
    m_globalTraceEnabled = wxGetEnv( wxT( "KICAD_TRACE" ), &traceVars );
    m_printAllTraces = false;

    if( !m_globalTraceEnabled )
        return;

    wxStringTokenizer tokenizer( traceVars, "," );

    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        m_enabledTraces[token] = true;

        if( token.Lower() == wxT( "all" ) )
            m_printAllTraces = true;
    }
}
