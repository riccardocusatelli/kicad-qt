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

#ifndef GUI_REPORTERS_H
#define GUI_REPORTERS_H

#include <memory>

#include <reporter.h>

class wxStatusBar;
class wxTextEntry;
class KISTATUSBAR;
class STATUSBAR_WARNING_REPORTER_IMPL;


/**
 * A wrapper for reporting to a wxTextCtrl object (any text entry: a multi-line text control
 * of the Qt compatibility layer is not a wxTextCtrl).
 */
class KICOMMON_API WX_TEXT_CTRL_REPORTER : public REPORTER
{
public:
    WX_TEXT_CTRL_REPORTER( wxTextEntry* aTextCtrl ) :
        REPORTER(),
        m_textCtrl( aTextCtrl )
    {
    }

    virtual ~WX_TEXT_CTRL_REPORTER()
    {
    }

    REPORTER& Report( const wxString& aText,
                      SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

private:
    wxTextEntry* m_textCtrl;
};


/**
 * A wrapper for reporting to a specific text location in a statusbar.
 */
class KICOMMON_API STATUSBAR_REPORTER : public REPORTER
{
public:
    STATUSBAR_REPORTER( wxStatusBar* aStatusBar, int aPosition = 0 )
            : REPORTER(),
              m_statusBar( aStatusBar ),
              m_position( aPosition )
    { }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

private:
    wxStatusBar* m_statusBar;
    int          m_position;
};


class KICOMMON_API STATUSBAR_WARNING_REPORTER : public REPORTER
{
public:
    STATUSBAR_WARNING_REPORTER( KISTATUSBAR* aStatusBar, const wxString& aSource );
    ~STATUSBAR_WARNING_REPORTER() override;

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

private:
    std::shared_ptr<STATUSBAR_WARNING_REPORTER_IMPL> m_impl;
};

#endif // GUI_REPORTERS_H
