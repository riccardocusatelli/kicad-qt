/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/stdpaths.h and include/wx/unix/stdpaths.h, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

#include <wx/defs.h>
#include <wx/filefn.h>
#include <wx/string.h>

class wxStandardPaths;


class wxStandardPathsBase
{
public:
    enum ResourceCat
    {
        ResourceCat_None,
        ResourceCat_Messages,
        ResourceCat_Max
    };

    enum
    {
        AppInfo_None = 0,
        AppInfo_AppName = 1,
        AppInfo_VendorName = 2
    };

    enum Dir
    {
        Dir_Cache,
        Dir_Documents,
        Dir_Desktop,
        Dir_Downloads,
        Dir_Music,
        Dir_Pictures,
        Dir_Videos
    };

    enum FileLayout
    {
        FileLayout_Classic,
        FileLayout_XDG
    };

    enum ConfigFileConv
    {
        ConfigFileConv_Dot,
        ConfigFileConv_Ext
    };

    static wxStandardPaths& Get();

    virtual wxString GetExecutablePath() const;
    virtual wxString GetConfigDir() const = 0;
    virtual wxString GetUserConfigDir() const = 0;
    virtual wxString GetDataDir() const = 0;
    virtual wxString GetLocalDataDir() const;
    virtual wxString GetUserDataDir() const = 0;
    virtual wxString GetUserLocalDataDir() const;
    virtual wxString GetPluginsDir() const = 0;
    virtual wxString GetResourcesDir() const { return GetDataDir(); }
    virtual wxString GetLocalizedResourcesDir( const wxString& aLang,
                                               ResourceCat = ResourceCat_None ) const
    {
        return GetResourcesDir() + wxFILE_SEP_PATH + aLang;
    }
    virtual wxString GetDocumentsDir() const { return GetUserDir( Dir_Documents ); }
    virtual wxString GetAppDocumentsDir() const;
    virtual wxString GetTempDir() const;
    virtual wxString GetUserDir( Dir aUserDir ) const;
    virtual wxString MakeConfigFileName( const wxString& aBasename,
                                         ConfigFileConv aConv = ConfigFileConv_Ext ) const = 0;

    virtual ~wxStandardPathsBase() = default;

    void       UseAppInfo( int aInfo ) { m_usedAppInfo = aInfo; }
    bool       UsesAppInfo( int aInfo ) const { return ( m_usedAppInfo & aInfo ) != 0; }
    void       SetFileLayout( FileLayout aLayout ) { m_fileLayout = aLayout; }
    FileLayout GetFileLayout() const { return m_fileLayout; }

protected:
    wxStandardPathsBase() = default;

    static wxString AppendPathComponent( const wxString& aDir, const wxString& aComponent );
    wxString        AppendAppInfo( const wxString& aDir ) const;

    int        m_usedAppInfo = AppInfo_AppName;
    FileLayout m_fileLayout = FileLayout_Classic;
};


#define wxHAS_NATIVE_STDPATHS
#define wxHAS_STDPATHS_INSTALL_PREFIX

/// The Unix implementation.
class wxStandardPaths : public wxStandardPathsBase
{
public:
    void     DetectPrefix();
    void     SetInstallPrefix( const wxString& aPrefix ) { m_prefix = aPrefix; }
    wxString GetInstallPrefix() const;

    wxString GetExecutablePath() const override;
    wxString GetConfigDir() const override;
    wxString GetUserConfigDir() const override;
    wxString GetDataDir() const override;
    wxString GetLocalDataDir() const override;
    wxString GetUserDataDir() const override;
    wxString GetPluginsDir() const override;
    wxString GetLocalizedResourcesDir( const wxString& aLang, ResourceCat aCategory ) const override;
    wxString GetUserDir( Dir aUserDir ) const override;
    wxString MakeConfigFileName( const wxString& aBasename,
                                 ConfigFileConv aConv = ConfigFileConv_Ext ) const override;

protected:
    wxStandardPaths() = default;

private:
    friend class wxStandardPathsBase;

    wxString m_prefix;
};

#pragma GCC visibility pop
