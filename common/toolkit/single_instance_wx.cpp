/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <toolkit/single_instance.h>

#include <wx/snglinst.h>


class SINGLE_INSTANCE_CHECKER::IMPL
{
public:
    wxSingleInstanceChecker m_checker;
};


SINGLE_INSTANCE_CHECKER::SINGLE_INSTANCE_CHECKER() :
        m_impl( std::make_unique<IMPL>() )
{
}


SINGLE_INSTANCE_CHECKER::~SINGLE_INSTANCE_CHECKER() = default;


bool SINGLE_INSTANCE_CHECKER::Create( const wxString& aName, const wxString& aPath )
{
    return m_impl->m_checker.Create( aName, aPath );
}


bool SINGLE_INSTANCE_CHECKER::IsAnotherRunning() const
{
    return m_impl->m_checker.IsAnotherRunning();
}
