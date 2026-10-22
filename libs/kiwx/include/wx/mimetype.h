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

/**
 * @file mimetype.h
 * The file types (wxMimeTypesManager, wxFileType): a document is opened by the opener of the
 * system (xdg-open, open, the shell of Windows), that knows the application of each type.
 */

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>
#include <wx/arrstr.h>
#include <wx/string.h>

#include <cstddef>
#include <vector>

#ifndef wxNullPtr
#define wxNullPtr nullptr
#endif


/// The description of a file type, given as a fallback to wxMimeTypesManager.
class wxFileTypeInfo
{
public:
    wxFileTypeInfo() = default;

    /// The extensions follow the description, the list ends with wxNullPtr.
    template <typename... EXTENSIONS>
    wxFileTypeInfo( const wxString& aMimeType, const wxString& aOpenCmd,
                    const wxString& aPrintCmd, const wxString& aDescription,
                    EXTENSIONS... aExtensions ) :
            m_mimeType( aMimeType ),
            m_openCmd( aOpenCmd ),
            m_printCmd( aPrintCmd ),
            m_description( aDescription )
    {
        ( addExtension( aExtensions ), ... );
    }

    bool IsValid() const { return !m_mimeType.empty(); }

    const wxString&      GetMimeType() const { return m_mimeType; }
    const wxString&      GetOpenCommand() const { return m_openCmd; }
    const wxString&      GetPrintCommand() const { return m_printCmd; }
    const wxString&      GetDescription() const { return m_description; }
    const wxArrayString& GetExtensions() const { return m_extensions; }

private:
    void addExtension( const wxString& aExtension ) { m_extensions.Add( aExtension ); }
    void addExtension( const wchar_t* aExtension )
    {
        if( aExtension )
            m_extensions.Add( aExtension );
    }
    void addExtension( std::nullptr_t ) {}

    wxString      m_mimeType;
    wxString      m_openCmd;
    wxString      m_printCmd;
    wxString      m_description;
    wxArrayString m_extensions;
};


/// A file type: the commands that open a file of the type.
class wxFileType
{
public:
    /// The file of a command, and its type.
    class MessageParameters
    {
    public:
        MessageParameters( const wxString& aFileName = wxEmptyString,
                           const wxString& aMimeType = wxEmptyString ) :
                m_fileName( aFileName ),
                m_mimeType( aMimeType )
        {
        }

        const wxString& GetFileName() const { return m_fileName; }
        const wxString& GetMimeType() const { return m_mimeType; }

    private:
        wxString m_fileName;
        wxString m_mimeType;
    };

    explicit wxFileType( const wxString& aExtension, const wxString& aMimeType = wxEmptyString );

    bool GetMimeType( wxString* aMimeType ) const;
    bool GetExtensions( wxArrayString& aExtensions ) const;

    /// The command that opens the file with the opener of the system.
    bool GetOpenCommand( wxString* aCommand, const MessageParameters& aParams ) const;
    wxString GetOpenCommand( const wxString& aFileName ) const;

private:
    wxString m_extension;
    wxString m_mimeType;
};


/// The file types: every extension is known (to the opener of the system).
class wxMimeTypesManager
{
public:
    /// A new file type (deleted by the caller), nullptr for no extension.
    wxFileType* GetFileTypeFromExtension( const wxString& aExtension );
    wxFileType* GetFileTypeFromMimeType( const wxString& aMimeType );

    /// The fallbacks give the MIME types of their extensions.
    void AddFallbacks( const wxFileTypeInfo* aFallbacks );
    void AddFallback( const wxFileTypeInfo& aFallback ) { m_fallbacks.push_back( aFallback ); }

private:
    std::vector<wxFileTypeInfo> m_fallbacks;
};


extern WXDLLIMPEXP_DATA_BASE( wxMimeTypesManager* ) wxTheMimeTypesManager;

#ifndef _WIN32
#pragma GCC visibility pop
#endif
