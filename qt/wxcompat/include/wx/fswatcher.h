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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file fswatcher.h
 * The watcher of the file system (wxFileSystemWatcher) of the wxWidgets compatibility layer, on
 * QFileSystemWatcher.
 *
 * Qt tells only that a watched directory or file changed: the watcher keeps the entries of the
 * watched directories and sends, as wxWidgets, an event for each entry created, deleted or
 * modified (the files of a watched directory are watched too, for their modifications).
 */

#pragma once

#include <wx/filename.h>
#include <wx/kiwx_events.h>

#include <map>
#include <memory>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include <qt_strings.h>


/// The changes of the file system (wxEVT_FSWATCHER).
inline const wxEventType wxEVT_FSWATCHER = wxNewEventType();

enum
{
    wxFSW_EVENT_CREATE = 0x01,
    wxFSW_EVENT_DELETE = 0x02,
    wxFSW_EVENT_RENAME = 0x04,
    wxFSW_EVENT_MODIFY = 0x08,
    wxFSW_EVENT_ACCESS = 0x10,
    wxFSW_EVENT_ATTRIB = 0x20,
    wxFSW_EVENT_UNMOUNT = 0x2000,
    wxFSW_EVENT_WARNING = 0x40,
    wxFSW_EVENT_ERROR = 0x80,
    wxFSW_EVENT_ALL = wxFSW_EVENT_CREATE | wxFSW_EVENT_DELETE | wxFSW_EVENT_RENAME
                      | wxFSW_EVENT_MODIFY | wxFSW_EVENT_ACCESS | wxFSW_EVENT_ATTRIB
                      | wxFSW_EVENT_WARNING | wxFSW_EVENT_ERROR
};

enum wxFSWWarningType
{
    wxFSW_WARNING_NONE,
    wxFSW_WARNING_GENERAL,
    wxFSW_WARNING_OVERFLOW
};


class wxFileSystemWatcherEvent : public wxEvent
{
public:
    explicit wxFileSystemWatcherEvent( int aChangeType = 0, const wxFileName& aPath = wxFileName(),
                                       const wxFileName& aNewPath = wxFileName() ) :
            wxEvent( wxID_ANY, wxEVT_FSWATCHER ),
            m_changeType( aChangeType ),
            m_path( aPath ),
            m_newPath( aNewPath )
    {
    }

    wxEvent* Clone() const override { return new wxFileSystemWatcherEvent( *this ); }

    int GetChangeType() const { return m_changeType; }
    const wxFileName& GetPath() const { return m_path; }
    void SetPath( const wxFileName& aPath ) { m_path = aPath; }
    const wxFileName& GetNewPath() const { return m_newPath; }
    void SetNewPath( const wxFileName& aPath ) { m_newPath = aPath; }

    bool IsError() const { return m_changeType & ( wxFSW_EVENT_ERROR | wxFSW_EVENT_WARNING ); }
    wxString GetErrorDescription() const { return wxEmptyString; }
    wxFSWWarningType GetWarningType() const { return wxFSW_WARNING_NONE; }

    wxString ToString() const { return m_path.GetFullPath(); }

private:
    int        m_changeType;
    wxFileName m_path;
    wxFileName m_newPath;
};


using wxFileSystemWatcherEventFunction = void ( wxEvtHandler::* )( wxFileSystemWatcherEvent& );

#define wxFileSystemWatcherEventHandler( func ) \
    ( reinterpret_cast<wxFileSystemWatcherEventFunction>( &func ) )


class wxFileSystemWatcher : public wxEvtHandler
{
public:
    wxFileSystemWatcher() :
            m_owner( this ),
            m_watcher( std::make_unique<QFileSystemWatcher>() )
    {
        QObject::connect( m_watcher.get(), &QFileSystemWatcher::directoryChanged, m_watcher.get(),
                          [this]( const QString& aPath ) { directoryChanged( aPath ); } );
        QObject::connect( m_watcher.get(), &QFileSystemWatcher::fileChanged, m_watcher.get(),
                          [this]( const QString& aPath ) { fileChanged( aPath ); } );
    }

    ~wxFileSystemWatcher() override = default;

    void SetOwner( wxEvtHandler* aOwner ) { m_owner = aOwner; }
    wxEvtHandler* GetOwner() const { return m_owner; }

    /// Watch a directory (its entries) or a file.
    bool Add( const wxFileName& aPath, int aEvents = wxFSW_EVENT_ALL )
    {
        QString path = QDir::cleanPath( ToQString( aPath.GetFullPath() ) );
        QFileInfo info( path );

        if( !info.exists() )
            return false;

        if( info.isDir() )
        {
            m_dirs[path] = entries( path );

            for( const auto& [file, stamp] : m_dirs[path] )
            {
                if( QFileInfo( file ).isFile() )
                    m_watcher->addPath( file );
            }
        }

        return m_watcher->addPath( path ) || m_watcher->files().contains( path )
               || m_watcher->directories().contains( path );
    }

    /// Watch a directory and its subdirectories.
    bool AddTree( const wxFileName& aPath, int aEvents = wxFSW_EVENT_ALL,
                  const wxString& aFilter = wxEmptyString )
    {
        QString root = ToQString( aPath.GetPath() );

        if( !Add( wxFileName::DirName( aPath.GetPath() ), aEvents ) )
            return false;

        QDir dir( root );

        for( const QFileInfo& sub : dir.entryInfoList( QDir::Dirs | QDir::NoDotAndDotDot ) )
            AddTree( wxFileName::DirName( ToWxString( sub.absoluteFilePath() ) ), aEvents, aFilter );

        return true;
    }

    bool Remove( const wxFileName& aPath )
    {
        QString path = QDir::cleanPath( ToQString( aPath.GetFullPath() ) );

        if( auto it = m_dirs.find( path ); it != m_dirs.end() )
        {
            for( const auto& [file, stamp] : it->second )
                m_watcher->removePath( file );

            m_dirs.erase( it );
        }

        return m_watcher->removePath( path );
    }

    bool RemoveTree( const wxFileName& aPath ) { return Remove( aPath ); }

    bool RemoveAll()
    {
        QStringList paths = m_watcher->files() + m_watcher->directories();

        if( !paths.isEmpty() )
            m_watcher->removePaths( paths );

        m_dirs.clear();
        return true;
    }

    int GetWatchedPathsCount() const
    {
        return int( m_watcher->directories().size() + m_watcher->files().size() );
    }

    int GetWatchedPaths( wxArrayString* aPaths ) const
    {
        for( const QString& path : m_watcher->directories() + m_watcher->files() )
            aPaths->Add( ToWxString( path ) );

        return GetWatchedPathsCount();
    }

private:
    /// The modification time and the size of an entry.
    using STAMP = std::pair<qint64, qint64>;

    static std::map<QString, STAMP> entries( const QString& aDir )
    {
        std::map<QString, STAMP> result;
        QDir dir( aDir );

        for( const QFileInfo& info : dir.entryInfoList( QDir::AllEntries | QDir::NoDotAndDotDot
                                                        | QDir::Hidden | QDir::System ) )
        {
            result[QDir::cleanPath( info.absoluteFilePath() )] =
                    { info.lastModified().toMSecsSinceEpoch(), info.size() };
        }

        return result;
    }

    void send( int aChangeType, const QString& aPath )
    {
        if( !m_owner )
            return;

        wxFileSystemWatcherEvent event( aChangeType, wxFileName( ToWxString( aPath ) ) );
        event.SetEventObject( this );
        m_owner->ProcessEvent( event );
    }

    void directoryChanged( const QString& aPath )
    {
        auto it = m_dirs.find( QDir::cleanPath( aPath ) );

        if( it == m_dirs.end() )
        {
            send( wxFSW_EVENT_MODIFY, aPath );
            return;
        }

        std::map<QString, STAMP> before = it->second;
        std::map<QString, STAMP> now = entries( it->first );
        it->second = now;

        for( const auto& [file, stamp] : before )
        {
            if( !now.count( file ) )
            {
                m_watcher->removePath( file );
                send( wxFSW_EVENT_DELETE, file );
            }
        }

        for( const auto& [file, stamp] : now )
        {
            auto old = before.find( file );

            if( old == before.end() )
            {
                if( QFileInfo( file ).isFile() )
                    m_watcher->addPath( file );

                send( wxFSW_EVENT_CREATE, file );
            }
            else if( old->second != stamp )
            {
                send( wxFSW_EVENT_MODIFY, file );
            }
        }
    }

    void fileChanged( const QString& aPath )
    {
        QString dir = QFileInfo( aPath ).absolutePath();

        // The entries of the directory of the file are up to date
        if( auto it = m_dirs.find( QDir::cleanPath( dir ) ); it != m_dirs.end() )
        {
            QFileInfo info( aPath );

            if( !info.exists() )
                return;     // Deleted: the directory tells it

            it->second[QDir::cleanPath( aPath )] = { info.lastModified().toMSecsSinceEpoch(),
                                                     info.size() };

            // An editor that saves by replacing the file: watch the new one
            if( !m_watcher->files().contains( aPath ) )
                m_watcher->addPath( aPath );
        }

        send( QFileInfo::exists( aPath ) ? wxFSW_EVENT_MODIFY : wxFSW_EVENT_DELETE, aPath );
    }

    wxEvtHandler*                                    m_owner;
    std::unique_ptr<QFileSystemWatcher>              m_watcher;
    std::map<QString, std::map<QString, STAMP>>      m_dirs;
};
