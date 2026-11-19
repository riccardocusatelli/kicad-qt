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

#include <widgets/wx_data_view_hyperlink_renderer.h>
#include <hyperlink_markup.h>

#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/uri.h>
#include <wx/utils.h>


namespace
{

void layoutRuns( wxDC& aDC, std::vector<HYPERLINK_DV_RENDERER::RUN>& aRuns, int aX, int aY, int aHeight )
{
    int x = aX;

    for( HYPERLINK_DV_RENDERER::RUN& run : aRuns )
    {
        wxSize ext = aDC.GetTextExtent( run.text );
        run.bounds = wxRect( x, aY, ext.x, aHeight > 0 ? aHeight : ext.y );
        x += ext.x;
    }
}

} // namespace


bool HYPERLINK_DV_RENDERER::IsSafeUrl( const wxString& aHref )
{
    return HYPERLINK_MARKUP::IsSafeUrl( aHref );
}


wxString HYPERLINK_DV_RENDERER::StripMarkup( const wxString& aValue )
{
    return HYPERLINK_MARKUP::StripMarkup( aValue );
}


void HYPERLINK_DV_RENDERER::ParseRuns( const wxString& aValue, std::vector<RUN>& aRuns )
{
    std::vector<HYPERLINK_MARKUP::RUN> runs;
    HYPERLINK_MARKUP::ParseRuns( aValue, runs );

    aRuns.clear();

    for( const HYPERLINK_MARKUP::RUN& run : runs )
        aRuns.push_back( { run.text, run.href, wxRect() } );
}


HYPERLINK_DV_RENDERER::HYPERLINK_DV_RENDERER() :
        wxDataViewCustomRenderer( wxT( "string" ), wxDATAVIEW_CELL_INERT, wxDVR_DEFAULT_ALIGNMENT )
{
}


bool HYPERLINK_DV_RENDERER::SetValue( const wxVariant& aValue )
{
    m_value = aValue.GetString();
    ParseRuns( m_value, m_runs );
    return true;
}


bool HYPERLINK_DV_RENDERER::GetValue( wxVariant& aValue ) const
{
    aValue = m_value;
    return true;
}


wxSize HYPERLINK_DV_RENDERER::GetSize() const
{
    if( m_value.IsEmpty() )
        return wxSize( wxDVC_DEFAULT_RENDERER_SIZE, wxDVC_DEFAULT_RENDERER_SIZE );

    // Sum the visible-text width across runs (markup is hidden, so the raw
    // m_value width over-reports).
    wxSize size( 0, 0 );

    for( const RUN& run : m_runs )
    {
        wxSize ext = GetTextExtent( run.text );
        size.x += ext.x;
        size.y = std::max( size.y, ext.y );
    }

    if( size.y == 0 )
        size.y = wxDVC_DEFAULT_RENDERER_SIZE;

    return size;
}


bool HYPERLINK_DV_RENDERER::Render( wxRect aCell, wxDC* aDC, int aState )
{
    RenderBackground( aDC, aCell );

    layoutRuns( *aDC, m_runs, aCell.x, aCell.y, aCell.height );

    const bool selected = ( aState & wxDATAVIEW_CELL_SELECTED ) != 0;
    int        xoffset = 0;

#ifdef __WXOSX__
    const int textState = 0;
#else
    const int textState = aState;
#endif

    for( const RUN& run : m_runs )
    {
        wxRect runRect = aCell;
        runRect.x += xoffset;
        runRect.width = run.bounds.width;

        if( !run.href.IsEmpty() )
        {
            wxDCTextColourChanger col( *aDC, selected ? wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT )
                                                      : wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT ) );

            wxFont linkFont = aDC->GetFont();
            linkFont.SetUnderlined( true );
            wxDCFontChanger font( *aDC, linkFont );

            RenderText( run.text, 0, runRect, aDC, textState );
        }
        else
        {
#ifdef __WXOSX__
            if( selected )
                aDC->SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT ) );
#endif
            RenderText( run.text, 0, runRect, aDC, textState );
        }

        xoffset += run.bounds.width;
    }

    return true;
}


bool HYPERLINK_DV_RENDERER::HitTestRunsForCell( const wxString& aValue, const wxDataViewItemAttr& aCellAttr,
                                                const wxRect& aCellRect, const wxPoint& aPoint, wxString* aHref ) const
{
    std::vector<RUN> runs;
    ParseRuns( aValue, runs );

    wxClientDC dc( const_cast<wxDataViewCtrl*>( GetView() ) );
    wxFont     font = GetView()->GetFont();

    if( aCellAttr.GetBold() )
        font.MakeBold();

    if( aCellAttr.GetItalic() )
        font.MakeItalic();

    if( aCellAttr.GetStrikethrough() )
        font.MakeStrikethrough();

    dc.SetFont( font );
    layoutRuns( dc, runs, aCellRect.x, aCellRect.y, aCellRect.height );

    for( const RUN& run : runs )
    {
        if( !run.href.IsEmpty() && run.bounds.Contains( aPoint ) )
        {
            if( aHref )
                *aHref = run.href;

            return true;
        }
    }

    return false;
}
