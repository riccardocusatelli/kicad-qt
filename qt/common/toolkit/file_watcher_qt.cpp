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

#include <QFileInfo>
#include <QFileSystemWatcher>

#include <qt_strings.h>


/**
 * QFileSystemWatcher reports the changes of a file it watches, and stops watching a file that
 * is removed or replaced: the directory is watched too, so that the file is watched again when
 * it comes back (saved by writing a new file and renaming it).
 */
class FILE_WATCHER::IMPL
{
public:
    IMPL( std::function<void( const wxString& )> aHandler ) :
            m_handler( std::move( aHandler ) )
    {
        QObject::connect( &m_watcher, &QFileSystemWatcher::fileChanged, &m_watcher,
                          [this]( const QString& )
                          {
                              rewatch();
                              notify();
                          } );

        QObject::connect( &m_watcher, &QFileSystemWatcher::directoryChanged, &m_watcher,
                          [this]( const QString& )
                          {
                              // The file came back (or went away)
                              if( rewatch() )
                                  notify();
                          } );
    }

    bool Watch( const wxString& aPath )
    {
        Clear();

        m_path = ToQString( aPath );

        QFileInfo info( m_path );

        if( !m_watcher.addPath( info.absolutePath() ) )
            return false;

        rewatch();
        return true;
    }

    void Clear()
    {
        if( !m_watcher.files().isEmpty() )
            m_watcher.removePaths( m_watcher.files() );

        if( !m_watcher.directories().isEmpty() )
            m_watcher.removePaths( m_watcher.directories() );

        m_path.clear();
    }

private:
    /// Watch the file again if it exists and is not watched; true if it is watched again.
    bool rewatch()
    {
        if( m_path.isEmpty() || m_watcher.files().contains( m_path )
                || !QFileInfo::exists( m_path ) )
        {
            return false;
        }

        return m_watcher.addPath( m_path );
    }

    void notify()
    {
        if( m_handler && !m_path.isEmpty() )
            m_handler( ToWxString( m_path ) );
    }

    std::function<void( const wxString& )> m_handler;
    QFileSystemWatcher                     m_watcher;
    QString                                m_path;
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
