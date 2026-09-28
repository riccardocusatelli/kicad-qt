/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
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

#include <generate_alias_info.h>
#include <ki_exception.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <lib_symbol.h>
#include <libraries/symbol_library_adapter.h>
#include <wx/log.h>
#include <wx/settings.h>

// Component-panel style details: title, muted subtitle line, description, then a two column
// parameter table with alternating row shading.  Colours are derived from the system theme at
// generation time so the page reads correctly in both light and dark mode.
static const wxString DescriptionFormat = wxS(
    "<font size=+1><b>__NAME__</b></font>"
    "__SUBTITLE__"
    "__DESC__"
    "__KEY__"
    "<br><br><font size=-1 color='__MUTED__'><b>__PARAMS_TITLE__</b></font>"
    "<table width='100%' border=0 cellspacing=0 cellpadding=4>"
    "__FIELDS__"
    "</table>" );

static const wxString SubtitleFormat =  wxS( "<br><font color='%s'>%s</font>" );
static const wxString DescFormat =      wxS( "<p>%s</p>" );
static const wxString KeywordsFormat =  wxS( "<font size=-1 color='%s'>%s:</font> <font size=-1>%s</font>" );
static const wxString FieldFormat = wxS(
    "<tr__ROWBG__>"
    "   <td width='32%' valign=top><font color='__MUTED__'>__NAME__</font></td>"
    "   <td valign=top>__VALUE__</td>"
    "</tr>" );
static const wxString LinkFormat = wxS( "<a href=\"__HREF__\">__TEXT__</a>" );


static wxColour blendColour( const wxColour& aA, const wxColour& aB, double aAmount )
{
    auto mix = [&]( unsigned char a, unsigned char b )
               {
                   return (unsigned char) ( a + ( b - a ) * aAmount );
               };

    return wxColour( mix( aA.Red(), aB.Red() ), mix( aA.Green(), aB.Green() ),
                     mix( aA.Blue(), aB.Blue() ) );
}


class FOOTPRINT_INFO_GENERATOR
{
public:
    FOOTPRINT_INFO_GENERATOR( SYMBOL_LIBRARY_ADAPTER* aLibs, LIB_ID const& aLibId, int aUnit ) :
            m_html( DescriptionFormat ),
            m_libs( aLibs ),
            m_lib_id( aLibId ),
            m_symbol( nullptr ),
            m_unit( aUnit ),
            m_rowIndex( 0 )
    {
        wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
        wxColour fg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

        m_muted = blendColour( bg, fg, 0.6 ).GetAsString( wxC2S_HTML_SYNTAX );
        m_stripe = blendColour( bg, fg, 0.05 ).GetAsString( wxC2S_HTML_SYNTAX );
    }

    /**
     * Generate the HTML internally.
     */
    void GenerateHtml()
    {
        wxCHECK_RET( m_libs, "Symbol library manager adapter pointer is not valid" );

        if( !m_lib_id.IsValid() )
        {
            ClearPlaceholders();
            return;
        }

        try
        {
            m_symbol = m_libs->LoadSymbol( m_lib_id );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( _( "Error loading symbol %s from library '%s'." ) + wxS( "\n%s" ),
                        m_lib_id.GetLibItemName().wx_str(),
                        m_lib_id.GetLibNickname().wx_str(),
                        ioe.What() );
            ClearPlaceholders();
            return;
        }

        if( m_symbol )
        {
            SetHtmlName();
            SetHtmlAliasOf();
            SetHtmlDesc();
            SetHtmlKeywords();
            SetHtmlFieldTable();
        }
        else
        {
            ClearPlaceholders();
        }
    }

    /**
     * Return the generated HTML.
     */
    wxString GetHtml() const
    {
        return m_html;
    }

protected:
    void SetHtmlName()
    {
        m_html.Replace( wxS( "__NAME__" ), EscapeHTML( UnescapeString( m_symbol->GetName() ) ) );
    }

    /// Muted line under the name: library, units, power flag, derivation.
    void SetHtmlAliasOf()
    {
        wxArrayString parts;

        parts.Add( EscapeHTML( UnescapeString( m_lib_id.GetUniStringLibNickname() ) ) );

        if( m_symbol->GetUnitCount() > 1 )
            parts.Add( wxString::Format( _( "%d units" ), m_symbol->GetUnitCount() ) );

        if( m_symbol->IsPower() )
            parts.Add( _( "power symbol" ) );

        if( !m_symbol->IsRoot() )
        {
            wxString                    root_name = _( "Unknown" );
            std::shared_ptr<LIB_SYMBOL> parent = m_symbol->GetParent().lock();

            if( parent )
                root_name = parent->GetName();

            parts.Add( wxString::Format( _( "derived from %s" ),
                                         EscapeHTML( UnescapeString( root_name ) ) ) );
        }

        m_html.Replace( wxS( "__SUBTITLE__" ),
                        wxString::Format( SubtitleFormat, m_muted, joinParts( parts ) ) );
    }

    static wxString joinParts( const wxArrayString& aParts )
    {
        wxString out;

        for( size_t i = 0; i < aParts.size(); ++i )
        {
            if( i > 0 )
                out += wxS( " &middot; " );

            out += aParts[i];
        }

        return out;
    }

    void SetHtmlDesc()
    {
        wxString esc_desc = EscapeHTML( UnescapeString( m_symbol->GetShownDescription() ) );

        // Add line breaks
        esc_desc.Replace( wxS( "\n" ), wxS( "<br>" ) );

        // Add links
        esc_desc = LinkifyHTML( esc_desc );

        if( esc_desc.IsEmpty() )
            m_html.Replace( wxS( "__DESC__" ), wxEmptyString );
        else
            m_html.Replace( wxS( "__DESC__" ), wxString::Format( DescFormat, esc_desc ) );
    }

    void SetHtmlKeywords()
    {
        wxString keywords = m_symbol->GetShownKeyWords();

        if( keywords.empty() )
            m_html.Replace( wxS( "__KEY__" ), wxEmptyString );
        else
            m_html.Replace( wxS( "__KEY__" ), wxString::Format( KeywordsFormat, m_muted, _( "Keywords" ),
                                                                EscapeHTML( keywords ) ) );
    }

    wxString GetHtmlFieldRow( const SCH_FIELD& aField ) const
    {
        wxString name = aField.GetCanonicalName();
        wxString text;
        wxString fieldhtml = FieldFormat;

        fieldhtml.Replace( wxS( "__NAME__" ), EscapeHTML( name ) );
        fieldhtml.Replace( wxS( "__MUTED__" ), m_muted );

        switch( aField.GetId() )
        {
        case FIELD_T::DATASHEET:
            text = m_symbol->GetDatasheetField().GetShownText( false );

            if( text.IsEmpty() || text == wxT( "~" ) )
            {
                fieldhtml.Replace( wxS( "__VALUE__" ), text );
            }
            else
            {
                wxString datasheetlink = LinkFormat;
                datasheetlink.Replace( wxS( "__HREF__" ), EscapeHTML( text ) );

                if( text.Length() > 75 )
                    text = text.Left( 72 ) + wxT( "..." );

                datasheetlink.Replace( wxS( "__TEXT__" ), EscapeHTML( text ) );

                fieldhtml.Replace( wxS( "__VALUE__" ), datasheetlink );
            }

            break;

        case FIELD_T::VALUE:
            // showing the value just repeats the name, so that's not much use...
            return wxEmptyString;

        case FIELD_T::REFERENCE:
            text = aField.GetFullText( m_unit > 0 ? m_unit : 1 );
            fieldhtml.Replace( wxS( "__VALUE__" ), EscapeHTML( text ) );
            break;

        default:
            text = aField.GetShownText( false );

            if( aField.HasHypertext() )
            {
                wxString link = LinkFormat;
                link.Replace( wxS( "__HREF__" ), EscapeHTML( text ) );

                if( text.Length() > 75 )
                    text = text.Left( 72 ) + wxT( "..." );

                link.Replace( wxS( "__TEXT__" ), EscapeHTML( text ) );

                fieldhtml.Replace( wxS( "__VALUE__" ), link );
            }
            else
            {
                fieldhtml.Replace( wxS( "__VALUE__" ), EscapeHTML( text ) );
            }
        }

        fieldhtml.Replace( wxS( "__ROWBG__" ),
                           ( m_rowIndex++ % 2 == 0 ) ? wxString::Format( wxS( " bgcolor='%s'" ), m_stripe )
                                                     : wxString() );
        return fieldhtml;
    }


    void SetHtmlFieldTable()
    {
        wxString                fieldtable;
        std::vector<SCH_FIELD*> fields;

        m_symbol->GetFields( fields );

        for( const SCH_FIELD* field: fields )
            fieldtable += GetHtmlFieldRow( *field );

        if( m_symbol->IsDerived() )
        {
            std::shared_ptr<LIB_SYMBOL> parent = m_symbol->GetParent().lock();

            // Append all of the unique parent fields if this is a derived symbol.
            if( parent )
            {
                std::vector<SCH_FIELD*> parentFields;

                parent->GetFields( parentFields );

                for( const SCH_FIELD* parentField : parentFields )
                {
                    if( m_symbol->GetField( parentField->GetCanonicalName() ) )
                        continue;

                    fieldtable += GetHtmlFieldRow( *parentField );
                }
            }
        }

        m_html.Replace( wxS( "__FIELDS__" ), fieldtable );
        m_html.Replace( wxS( "__PARAMS_TITLE__" ), _( "PARAMETERS" ) );
        m_html.Replace( wxS( "__MUTED__" ), m_muted );
    }

    void ClearPlaceholders()
    {
        m_html.Replace( wxS( "__NAME__" ), wxEmptyString );
        m_html.Replace( wxS( "__SUBTITLE__" ), wxEmptyString );
        m_html.Replace( wxS( "__DESC__" ), wxEmptyString );
        m_html.Replace( wxS( "__KEY__" ), wxEmptyString );
        m_html.Replace( wxS( "__FIELDS__" ), wxEmptyString );
        m_html.Replace( wxS( "__PARAMS_TITLE__" ), wxEmptyString );
        m_html.Replace( wxS( "__MUTED__" ), m_muted );
    }

private:
    wxString                        m_html;
    SYMBOL_LIBRARY_ADAPTER* m_libs;
    LIB_ID const                    m_lib_id;
    LIB_SYMBOL*                     m_symbol;
    int                             m_unit;
    mutable int                     m_rowIndex;
    wxString                        m_muted;
    wxString                        m_stripe;
};


wxString GenerateAliasInfo( SYMBOL_LIBRARY_ADAPTER* aLibs, LIB_ID const& aLibId, int aUnit )
{
    FOOTPRINT_INFO_GENERATOR gen( aLibs, aLibId, aUnit );
    gen.GenerateHtml();
    return gen.GetHtml();
}
