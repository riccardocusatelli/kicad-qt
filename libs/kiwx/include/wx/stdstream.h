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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/stream.h>

#include <istream>
#include <ostream>
#include <streambuf>


/// A std::streambuf reading from a wxInputStream.
class wxStdInputStreamBuffer : public std::streambuf
{
public:
    explicit wxStdInputStreamBuffer( wxInputStream& aStream ) : m_stream( aStream ) {}

protected:
    int_type underflow() override;
    pos_type seekoff( off_type aOff, std::ios_base::seekdir aDir, std::ios_base::openmode aMode ) override;
    pos_type seekpos( pos_type aPos, std::ios_base::openmode aMode ) override;
    std::streamsize showmanyc() override;

private:
    wxInputStream& m_stream;
    char           m_buffer[4096];
};


/// A std::streambuf writing to a wxOutputStream.
class wxStdOutputStreamBuffer : public std::streambuf
{
public:
    explicit wxStdOutputStreamBuffer( wxOutputStream& aStream ) : m_stream( aStream ) {}

protected:
    int_type        overflow( int_type aCh ) override;
    std::streamsize xsputn( const char* aData, std::streamsize aCount ) override;
    pos_type seekoff( off_type aOff, std::ios_base::seekdir aDir, std::ios_base::openmode aMode ) override;
    pos_type seekpos( pos_type aPos, std::ios_base::openmode aMode ) override;

private:
    wxOutputStream& m_stream;
};


class wxStdInputStream : public std::istream
{
public:
    explicit wxStdInputStream( wxInputStream& aStream ) : std::istream( nullptr ), m_buffer( aStream )
    {
        init( &m_buffer );
    }

private:
    wxStdInputStreamBuffer m_buffer;
};


class wxStdOutputStream : public std::ostream
{
public:
    explicit wxStdOutputStream( wxOutputStream& aStream ) : std::ostream( nullptr ), m_buffer( aStream )
    {
        init( &m_buffer );
    }

private:
    wxStdOutputStreamBuffer m_buffer;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
