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

/**
 * @file single_instance_qt.cpp
 * SINGLE_INSTANCE_CHECKER with Qt: a lock file (see common/toolkit/single_instance_wx.cpp).
 */

#include <toolkit/single_instance.h>

#include <QDir>
#include <QLockFile>

#include <qt_strings.h>


class SINGLE_INSTANCE_CHECKER::IMPL
{
public:
    std::unique_ptr<QLockFile> m_lock;
    bool                       m_otherRunning = false;
};


SINGLE_INSTANCE_CHECKER::SINGLE_INSTANCE_CHECKER() :
        m_impl( std::make_unique<IMPL>() )
{
}


SINGLE_INSTANCE_CHECKER::~SINGLE_INSTANCE_CHECKER() = default;


bool SINGLE_INSTANCE_CHECKER::Create( const wxString& aName, const wxString& aPath )
{
    QString path = QDir( ToQString( aPath ) ).filePath( ToQString( aName ) );

    // The lock of a program that ended without releasing it is stale (QLockFile checks the
    // process holding it)
    m_impl->m_lock = std::make_unique<QLockFile>( path );

    if( m_impl->m_lock->tryLock() )
    {
        m_impl->m_otherRunning = false;
        return true;
    }

    // Held by another program: the checker works; any other error: it doesn't
    m_impl->m_otherRunning = m_impl->m_lock->error() == QLockFile::LockFailedError;
    return m_impl->m_otherRunning;
}


bool SINGLE_INSTANCE_CHECKER::IsAnotherRunning() const
{
    return m_impl->m_otherRunning;
}
