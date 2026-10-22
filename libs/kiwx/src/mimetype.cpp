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

#include <wx/mimetype.h>


static wxMimeTypesManager s_mimeTypesManager;

wxMimeTypesManager* wxTheMimeTypesManager = &s_mimeTypesManager;


wxFileType::wxFileType( const wxString& aExtension, const wxString& aMimeType ) :
        m_extension( aExtension ),
        m_mimeType( aMimeType )
{
}


bool wxFileType::GetMimeType( wxString* aMimeType ) const
{
    if( m_mimeType.empty() )
        return false;

    *aMimeType = m_mimeType;
    return true;
}


bool wxFileType::GetExtensions( wxArrayString& aExtensions ) const
{
    aExtensions.Clear();

    if( !m_extension.empty() )
        aExtensions.Add( m_extension );

    return !aExtensions.empty();
}


wxString wxFileType::GetOpenCommand( const wxString& aFileName ) const
{
#if defined( _WIN32 )
    // the association of the shell, as a double click in the Explorer
    return wxT( "rundll32 url.dll,FileProtocolHandler \"" ) + aFileName + wxT( "\"" );
#else
    wxString quoted;

    for( wxUniChar c : aFileName )
    {
        if( c == '"' || c == '\\' )
            quoted += '\\';

        quoted += c;
    }

#if defined( __APPLE__ )
    return wxT( "open \"" ) + quoted + wxT( "\"" );
#else
    return wxT( "xdg-open \"" ) + quoted + wxT( "\"" );
#endif
#endif
}


bool wxFileType::GetOpenCommand( wxString* aCommand, const MessageParameters& aParams ) const
{
    if( aParams.GetFileName().empty() )
        return false;

    *aCommand = GetOpenCommand( aParams.GetFileName() );
    return true;
}


wxFileType* wxMimeTypesManager::GetFileTypeFromExtension( const wxString& aExtension )
{
    if( aExtension.empty() )
        return nullptr;

    for( const wxFileTypeInfo& info : m_fallbacks )
    {
        for( const wxString& ext : info.GetExtensions() )
        {
            if( ext.CmpNoCase( aExtension ) == 0 )
                return new wxFileType( aExtension, info.GetMimeType() );
        }
    }

    return new wxFileType( aExtension );
}


wxFileType* wxMimeTypesManager::GetFileTypeFromMimeType( const wxString& aMimeType )
{
    for( const wxFileTypeInfo& info : m_fallbacks )
    {
        if( info.GetMimeType().CmpNoCase( aMimeType ) == 0 && !info.GetExtensions().empty() )
            return new wxFileType( info.GetExtensions()[0], info.GetMimeType() );
    }

    return nullptr;
}


void wxMimeTypesManager::AddFallbacks( const wxFileTypeInfo* aFallbacks )
{
    // the table ends with an invalid (default) entry
    for( ; aFallbacks && aFallbacks->IsValid(); ++aFallbacks )
        m_fallbacks.push_back( *aFallbacks );
}
