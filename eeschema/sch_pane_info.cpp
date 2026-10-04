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

#include <sch_pane_info.h>

#include <eda_draw_frame.h>
#include <wx/aui/framemanager.h>
#include <wx/window.h>


const wxAuiPaneInfo& defaultNetNavigatorPaneInfo()
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( wxS( "NetNavigator" ) )
            .Caption( _( "Net Navigator" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Left().Layer( 3 ).Position( 0 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( 120, 60 )
            .BestSize( 200, 200 )
            .FloatingSize( 200, 200 )
            .FloatingPosition( 50, 200 )
            .Show( false );

    return paneInfo;
}


const wxAuiPaneInfo& defaultPropertiesPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( EDA_DRAW_FRAME::PropertiesPaneName() )
            .Caption( _( "Properties" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Left().Layer( 3 ).Position( 2 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 240, 60 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 300, 200 ) ) )
            .FloatingSize( aWindow->FromDIP( wxSize( 300, 400 ) ) )
            .FloatingPosition( aWindow->FromDIP( wxPoint( 50, 200 ) ) )
            .Show( true );

    return paneInfo;
}


const wxAuiPaneInfo& defaultSchSelectionFilterPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( wxS( "SelectionFilter" ) )
            .Caption( _( "Selection Filter" ) )
            .CaptionVisible( true )
            .PaneBorder( false )
            .Left().Layer( 3 ).Position( 4 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            // Fixed-size pane; -1 for MinSize height is required
            .MinSize( aWindow->FromDIP( wxSize( 180, -1 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 180, -1 ) ) )
            .Show( true );

    return paneInfo;
}


const wxAuiPaneInfo& defaultDesignBlocksPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( EDA_DRAW_FRAME::DesignBlocksPaneName() )
            .Caption( _( "Design Blocks" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Right().Layer( 3 ).Position( 2 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 240, 60 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 300, 200 ) ) )
            .FloatingSize( aWindow->FromDIP( wxSize( 800, 600 ) ) )
            .FloatingPosition( aWindow->FromDIP( wxPoint( 50, 200 ) ) )
            .Show( true );

    return paneInfo;
}


const wxAuiPaneInfo& defaultRemoteSymbolPaneInfo( wxWindow* aWindow )
{
    static wxAuiPaneInfo paneInfo;

    paneInfo.Name( EDA_DRAW_FRAME::RemoteSymbolPaneName() )
            .Caption( _( "Remote Symbols" ) )
            .CaptionVisible( true )
            .PaneBorder( true )
            .Right().Layer( 3 ).Position( 3 )
            .TopDockable( false )
            .BottomDockable( false )
            .CloseButton( true )
            .MinSize( aWindow->FromDIP( wxSize( 240, 60 ) ) )
            .BestSize( aWindow->FromDIP( wxSize( 300, 200 ) ) )
            .FloatingSize( aWindow->FromDIP( wxSize( 800, 600 ) ) )
            .FloatingPosition( aWindow->FromDIP( wxPoint( 80, 220 ) ) )
            .Show( false );

    return paneInfo;
}
