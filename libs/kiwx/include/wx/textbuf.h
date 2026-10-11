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
#pragma GCC visibility push( default )

#include <wx/arrstr.h>
#include <wx/strconv.h>
#include <wx/string.h>

#include <vector>

enum wxTextFileType
{
    wxTextFileType_None,
    wxTextFileType_Unix,
    wxTextFileType_Dos,
    wxTextFileType_Mac,
    wxTextFileType_Os2
};


/// Lines of text with their line ends (the base of wxTextFile).
class wxTextBuffer
{
public:
#ifdef _WIN32
    static const wxTextFileType typeDefault = wxTextFileType_Dos;
#else
    static const wxTextFileType typeDefault = wxTextFileType_Unix;
#endif

    static wxString       Translate( const wxString& aText, wxTextFileType aType = typeDefault );
    static const wxChar*  GetEOL( wxTextFileType aType = typeDefault );

    virtual ~wxTextBuffer() = default;

    bool Exists() const { return OnExists(); }
    bool Create();
    bool Create( const wxString& aName );
    bool Open( const wxMBConv& aConv = wxConvAuto() ) { return Open( m_name, aConv ); }
    bool Open( const wxString& aName, const wxMBConv& aConv = wxConvAuto() );
    bool IsOpened() const { return m_isOpened; }
    bool Close();

    size_t GetLineCount() const { return m_lines.size(); }
    wxString&       GetLine( size_t n ) { return m_lines[n]; }
    const wxString& GetLine( size_t n ) const { return m_lines[n]; }
    wxString&       operator[]( size_t n ) { return m_lines[n]; }
    const wxString& operator[]( size_t n ) const { return m_lines[n]; }
    wxTextFileType  GetLineType( size_t n ) const { return m_types[n]; }
    wxTextFileType  GuessType() const;
    const wxString& GetName() const { return m_name; }

    size_t   GetCurrentLine() const { return m_current; }
    void     GoToLine( size_t n ) { m_current = n; }
    bool     Eof() const { return m_lines.empty() || m_current == m_lines.size() - 1; }
    wxString& GetFirstLine() { m_current = 0; return m_lines[0]; }
    wxString& GetNextLine() { return m_lines[++m_current]; }
    wxString& GetPrevLine() { return m_lines[--m_current]; }
    wxString& GetLastLine() { m_current = m_lines.size() - 1; return m_lines.back(); }

    void AddLine( const wxString& aStr, wxTextFileType aType = typeDefault )
    {
        m_lines.push_back( aStr );
        m_types.push_back( aType );
    }
    void InsertLine( const wxString& aStr, size_t n, wxTextFileType aType = typeDefault )
    {
        m_lines.insert( m_lines.begin() + n, aStr );
        m_types.insert( m_types.begin() + n, aType );
    }
    void RemoveLine( size_t n )
    {
        m_lines.erase( m_lines.begin() + n );
        m_types.erase( m_types.begin() + n );
    }
    void Clear()
    {
        m_lines.clear();
        m_types.clear();
        m_current = 0;
    }

    bool Write( wxTextFileType aType = wxTextFileType_None, const wxMBConv& aConv = wxConvUTF8 );

protected:
    wxTextBuffer() = default;
    explicit wxTextBuffer( const wxString& aName ) : m_name( aName ) {}

    virtual bool OnExists() const = 0;
    virtual bool OnOpen( const wxString& aName, int aMode ) = 0;
    virtual bool OnClose() = 0;
    virtual bool OnRead( const wxMBConv& aConv ) = 0;
    virtual bool OnWrite( wxTextFileType aType, const wxMBConv& aConv ) = 0;

    enum
    {
        ReadAccess,
        WriteAccess
    };

    wxString                    m_name;
    std::vector<wxString>       m_lines;
    std::vector<wxTextFileType> m_types;
    size_t                      m_current = 0;
    bool                        m_isOpened = false;
};


/// Splits text in lines (any line end); used by wxTextFile and the text streams.
void kiwxSplitLines( const wxString& aText, std::vector<wxString>& aLines,
                     std::vector<wxTextFileType>& aTypes );

#pragma GCC visibility pop
