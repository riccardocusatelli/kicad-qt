/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#pragma once

#include <widgets/wx_progress_reporters.h>


/// The progress dialog of the jobsets run from the project manager.
class JOBS_PROGRESS_REPORTER : public WX_PROGRESS_REPORTER
{
public:
    JOBS_PROGRESS_REPORTER(wxWindow* aParent, const wxString& aTitle ) :
            WX_PROGRESS_REPORTER( aParent, aTitle, 1000, PR_NO_ABORT )
    {}

    void AdvancePhase() override
    {
    }

    void AdvanceJob( const wxString& aMessage )
    {
        m_jobStatus = aMessage;
        WX_PROGRESS_REPORTER::Report( aMessage );
    }

    void Report( const wxString& aMessage ) override
    {
        WX_PROGRESS_REPORTER::Report( m_jobStatus + wxS( "\n" ) + aMessage );
    }

private:
    wxString m_jobStatus;
};
