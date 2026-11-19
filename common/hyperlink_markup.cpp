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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <hyperlink_markup.h>

#include <wx/uri.h>


namespace
{

// Parse [label](url) starting at aStart
bool tryParseMarkdown( const wxString& aText, size_t aStart, size_t& aEnd, wxString& aLabel, wxString& aHref )
{
    if( aStart >= aText.length() || aText[aStart] != '[' )
        return false;

    size_t labelEnd = aText.find( ']', aStart + 1 );

    if( labelEnd == wxString::npos || labelEnd + 1 >= aText.length() || aText[labelEnd + 1] != '(' )
        return false;

    // Match balanced parens so URLs like "file:///foo (1).pdf" survive.
    size_t hrefStart = labelEnd + 2;
    size_t hrefEnd = wxString::npos;
    int    depth = 1;

    for( size_t i = hrefStart; i < aText.length(); ++i )
    {
        wxChar c = aText[i];

        if( c == '(' )
            ++depth;
        else if( c == ')' && --depth == 0 )
        {
            hrefEnd = i;
            break;
        }
    }

    if( hrefEnd == wxString::npos )
        return false;

    aLabel = aText.SubString( aStart + 1, labelEnd - 1 );
    aHref = aText.SubString( hrefStart, hrefEnd - 1 );
    aEnd = hrefEnd + 1;
    return true;
}

} // namespace


namespace HYPERLINK_MARKUP
{

bool IsSafeUrl( const wxString& aHref )
{
    wxString lower = aHref.Lower();
    wxURI    uri( lower );

    if( uri.GetScheme() == wxT( "http" ) || uri.GetScheme() == wxT( "https" ) )
        return true;

    if( uri.GetScheme() == wxT( "file" ) || lower.StartsWith( wxT( "\\\\" ) ) )
    {
        static const wxString blockedExtensions[] = { wxT( ".exe" ), wxT( ".com" ), wxT( ".bat" ), wxT( ".cmd" ),
                                                      wxT( ".ps1" ), wxT( ".vbs" ), wxT( ".js" ),  wxT( ".jar" ),
                                                      wxT( ".msi" ), wxT( ".scr" ), wxT( ".pif" ), wxT( ".lnk" ),
                                                      wxT( ".hta" ) };

        // The OS launches the decoded name, and Windows drops trailing dots and spaces.
        wxString path = wxURI::Unescape( uri.GetPath() );

        while( path.EndsWith( wxT( "." ) ) || path.EndsWith( wxT( " " ) ) )
            path.RemoveLast();

        for( const wxString& ext : blockedExtensions )
        {
            if( path.EndsWith( ext ) )
                return false;
        }

        return true;
    }

    return false;
}


wxString StripMarkup( const wxString& aValue )
{
    std::vector<RUN> runs;
    ParseRuns( aValue, runs );

    wxString result;

    for( const RUN& run : runs )
        result += run.text;

    return result;
}


void ParseRuns( const wxString& aValue, std::vector<RUN>& aRuns )
{
    aRuns.clear();

    size_t   pos = 0;
    wxString plain;

    while( pos < aValue.length() )
    {
        size_t   end = pos;
        wxString label;
        wxString href;

        if( aValue[pos] == '[' && tryParseMarkdown( aValue, pos, end, label, href ) )
        {
            if( !plain.IsEmpty() )
            {
                aRuns.push_back( { plain, wxEmptyString } );
                plain.clear();
            }

            if( IsSafeUrl( href ) )
                aRuns.push_back( { label, href } );
            else
                aRuns.push_back( { label, wxEmptyString } );

            pos = end;
        }
        else
        {
            plain += aValue[pos];
            ++pos;
        }
    }

    if( !plain.IsEmpty() )
        aRuns.push_back( { plain, wxEmptyString } );
}

} // namespace HYPERLINK_MARKUP
