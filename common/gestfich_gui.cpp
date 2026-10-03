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
 * @file gestfich_gui.cpp
 * Launching external programs from the GUI (with error message boxes): ExecuteFile(),
 * OpenPDF().  The other file helpers of gestfich.h are in gestfich.cpp.
 */

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <confirm.h>
#include <gestfich.h>
#include <launch_ext.h>
#include <pgm_base.h>


int ExecuteFile( const wxString& aEditorName, const wxString& aFileName, wxProcess* aCallback,
                 bool aFileForKicad )
{
    wxString              fullEditorName;
    std::vector<wxString> params;

#ifdef __UNIX__
    wxString param;
    bool     inSingleQuotes = false;
    bool     inDoubleQuotes = false;

    auto pushParam =
            [&]()
            {
                if( !param.IsEmpty() )
                {
                    params.push_back( param );
                    param.clear();
                }
            };

    for( wxUniChar ch : aEditorName )
    {
        if( inSingleQuotes )
        {
            if( ch == '\'' )
            {
                pushParam();
                inSingleQuotes = false;
                continue;
            }
            else
            {
                param += ch;
            }
        }
        else if( inDoubleQuotes )
        {
            if( ch == '"' )
            {
                pushParam();
                inDoubleQuotes = false;
            }
            else
            {
                param += ch;
            }
        }
        else if( ch == '\'' )
        {
            pushParam();
            inSingleQuotes = true;
        }
        else if( ch == '"' )
        {
            pushParam();
            inDoubleQuotes = true;
        }
        else if( ch == ' ' )
        {
            pushParam();
        }
        else
        {
            param += ch;
        }
    }

    pushParam();

    if( aFileForKicad )
        fullEditorName = FindKicadFile( params[0] );
    else
        fullEditorName = params[0];

    params.erase( params.begin() );
#else

    if( aFileForKicad )
        fullEditorName = FindKicadFile( aEditorName );
    else
        fullEditorName = aEditorName;
#endif

    if( wxFileExists( fullEditorName ) )
    {
        std::vector<const wchar_t*> args;

        args.emplace_back( fullEditorName.wc_str() );

        if( !params.empty() )
        {
            for( const wxString& p : params )
                args.emplace_back( p.wc_str() );
        }

        if( !aFileName.IsEmpty() )
            args.emplace_back( aFileName.wc_str() );

        args.emplace_back( nullptr );

        return wxExecute( const_cast<wchar_t**>( args.data() ), wxEXEC_ASYNC, aCallback );
    }

    wxString msg;
    msg.Printf( _( "Command '%s' could not be found." ), fullEditorName );
    DisplayErrorMessage( nullptr, msg );
    return -1;
}


bool OpenPDF( const wxString& file )
{
    wxString msg;
    wxString filename = file;

    Pgm().ReadPdfBrowserInfos();

    if( Pgm().UseSystemPdfBrowser() )
    {
        if( !LaunchExternal( filename ) )
        {
            msg.Printf( _( "Unable to find a PDF viewer for '%s'." ), filename );
            DisplayErrorMessage( nullptr, msg );
            return false;
        }
    }
    else
    {
        const wchar_t* args[3];

        args[0] = Pgm().GetPdfBrowserName().wc_str();
        args[1] = filename.wc_str();
        args[2] = nullptr;

        if( wxExecute( const_cast<wchar_t**>( args ) ) == -1 )
        {
            msg.Printf( _( "Problem while running the PDF viewer '%s'." ), args[0] );
            DisplayErrorMessage( nullptr, msg );
            return false;
        }
    }

    return true;
}
