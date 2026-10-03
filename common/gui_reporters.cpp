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

#include <wx/statusbr.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#include <wx/weakref.h>

#include <core/ki_assert.h>
#include <gui_reporters.h>
#include <string_utils.h>
#include <widgets/kistatusbar.h>


class STATUSBAR_WARNING_REPORTER_IMPL
{
public:
    STATUSBAR_WARNING_REPORTER_IMPL( KISTATUSBAR* aStatusBar, const wxString& aSource ) :
            m_statusBar( aStatusBar ),
            m_source( aSource )
    {
    }

    KISTATUSBAR* GetStatusBar() const
    {
        KISTATUSBAR* statusBar = m_statusBar.get();

        if( statusBar && !statusBar->IsBeingDeleted() )
            return statusBar;

        return nullptr;
    }

    wxWeakRef<KISTATUSBAR> m_statusBar;
    wxString               m_source;
};


REPORTER& WX_TEXT_CTRL_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    KI_CHECK_MSG( m_textCtrl != nullptr, *this,
                 wxT( "No wxTextCtrl object defined in WX_TEXT_CTRL_REPORTER." ) );

    m_textCtrl->AppendText( aText + wxS( "\n" ) );
    return *this;
}


REPORTER& STATUSBAR_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    if( m_statusBar )
    {
        // The message will be ellipsized automatically
        m_statusBar->SetStatusText( aText, m_position );
    }

    return *this;
}


STATUSBAR_WARNING_REPORTER::STATUSBAR_WARNING_REPORTER( KISTATUSBAR* aStatusBar,
                                                        const wxString& aSource ) :
        m_impl( std::make_shared<STATUSBAR_WARNING_REPORTER_IMPL>( aStatusBar, aSource ) )
{
}


STATUSBAR_WARNING_REPORTER::~STATUSBAR_WARNING_REPORTER() = default;


REPORTER& STATUSBAR_WARNING_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    KISTATUSBAR* statusBar = m_impl ? m_impl->GetStatusBar() : nullptr;

    if( !statusBar || aText.IsEmpty() )
        return *this;

    std::vector<LOAD_MESSAGE> messages;
    wxStringTokenizer tokenizer( aText, wxS( "\n" ), wxTOKEN_STRTOK );
    SEVERITY severity = aSeverity == RPT_SEVERITY_UNDEFINED ? RPT_SEVERITY_WARNING : aSeverity;

    while( tokenizer.HasMoreTokens() )
    {
        LOAD_MESSAGE message;
        message.message = tokenizer.GetNextToken();
        message.severity = severity;
        messages.emplace_back( std::move( message ) );
    }

    if( messages.empty() )
    {
        LOAD_MESSAGE message;
        message.message = aText;
        message.severity = severity;
        messages.emplace_back( std::move( message ) );
    }

    statusBar->AddWarningMessages( m_impl->m_source, messages );
    return *this;
}
