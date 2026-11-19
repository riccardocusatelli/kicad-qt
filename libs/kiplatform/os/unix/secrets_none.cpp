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


// The secrets without a keyring (KICAD_HEADLESS_ONLY, no libsecret): nothing is stored, nothing
// is found.  The callers (git credentials, OAuth tokens) then ask or work without them.

#include <kiplatform/secrets.h>

namespace KIPLATFORM
{
    namespace SECRETS
    {
        bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret )
        {
            return false;
        }

        void StoreSecretAsync( const wxString& aService, const wxString& aKey, const wxString& aSecret,
                               std::function<void( bool )> aDone )
        {
            if( aDone )
                aDone( false );
        }

        bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret )
        {
            return false;
        }

        void GetSecretAsync( const wxString& aService, const wxString& aKey,
                             std::function<void( bool, const wxString& )> aDone )
        {
            if( aDone )
                aDone( false, wxString() );
        }

        bool GetSecretNoPrompt( const wxString& aService, const wxString& aKey, wxString& aSecret )
        {
            return false;
        }

        bool DeleteSecret( const wxString& aService, const wxString& aKey )
        {
            return false;
        }
    }
}
