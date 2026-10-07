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


#include <toolkit/file_watcher.h>

#include <wx/filename.h>
#include <wx/fswatcher.h>
#include <wx/log.h>


/**
 * The watcher of wxWidgets watches the directory of the file (so that a file replaced by a
 * new one is still seen) and reports the events of the file only.
 */
class FILE_WATCHER::IMPL : public wxEvtHandler
{
public:
    IMPL( std::function<void( const wxString& )> aHandler ) :
            m_handler( std::move( aHandler ) )
    {
        Bind( wxEVT_FSWATCHER, &IMPL::onFileSystemEvent, this );
    }

    ~IMPL() override { Clear(); }

    bool Watch( const wxString& aPath )
    {
        Clear();

        m_path = aPath;
        m_watcher = std::make_unique<wxFileSystemWatcher>();
        m_watcher->SetOwner( this );

        wxFileName dir;
        dir.AssignDir( wxFileName( aPath ).GetPath() );
        dir.DontFollowLink();

        // Silence OS errors that come from the watcher
        wxLogNull silence;
        return m_watcher->Add( dir );
    }

    void Clear()
    {
        if( m_watcher )
        {
            m_watcher->RemoveAll();
            m_watcher->SetOwner( nullptr );
            m_watcher.reset();
        }

        m_path.clear();
    }

private:
    void onFileSystemEvent( wxFileSystemWatcherEvent& aEvent )
    {
        if( m_handler && !m_path.empty() && aEvent.GetPath().GetFullPath() == m_path )
            m_handler( m_path );
    }

    std::function<void( const wxString& )> m_handler;
    std::unique_ptr<wxFileSystemWatcher>   m_watcher;
    wxString                               m_path;
};


FILE_WATCHER::FILE_WATCHER( std::function<void( const wxString& aPath )> aHandler ) :
        m_impl( std::make_unique<IMPL>( std::move( aHandler ) ) )
{
}


FILE_WATCHER::~FILE_WATCHER() = default;


bool FILE_WATCHER::WatchFile( const wxString& aPath )
{
    return m_impl->Watch( aPath );
}


void FILE_WATCHER::Clear()
{
    m_impl->Clear();
}
