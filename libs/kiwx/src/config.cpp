/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/config.cpp, wxWindows Library Licence 3.1),
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

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include <wx/config.h>

#include <wx/app.h>
#include <wx/crt.h>
#include <wx/file.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/utils.h>

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>     // for INT_MAX
#include <float.h>      // for FLT_MAX
#include <math.h>

// ----------------------------------------------------------------------------
// global and class static variables
// ----------------------------------------------------------------------------

wxConfigBase *wxConfigBase::ms_pConfig     = NULL;
bool          wxConfigBase::ms_bAutoCreate = true;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxConfigBase
// ----------------------------------------------------------------------------

// Not all args will always be used by derived classes, but including them all
// in each class ensures compatibility.
wxConfigBase::wxConfigBase(const wxString& appName,
                           const wxString& vendorName,
                           const wxString& WXUNUSED(localFilename),
                           const wxString& WXUNUSED(globalFilename),
                           long style)
            : m_appName(appName), m_vendorName(vendorName), m_style(style)
{
    m_bExpandEnvVars = true;
    m_bRecordDefaults = false;
}

wxConfigBase::~wxConfigBase()
{
    // required here for Darwin
}

wxConfigBase *wxConfigBase::Set(wxConfigBase *pConfig)
{
  wxConfigBase *pOld = ms_pConfig;
  ms_pConfig = pConfig;
  return pOld;
}

wxConfigBase *wxConfigBase::Create()
{
  if ( ms_bAutoCreate && ms_pConfig == NULL ) {
    // what the wxWidgets application traits create on Unix
    ms_pConfig = new wxFileConfig( wxTheApp ? wxTheApp->GetAppName() : wxString() );
  }

  return ms_pConfig;
}

// ----------------------------------------------------------------------------
// wxConfigBase reading entries
// ----------------------------------------------------------------------------

// implement both Read() overloads for the given type in terms of DoRead()
#define IMPLEMENT_READ_FOR_TYPE(name, type, deftype, extra)                 \
    bool wxConfigBase::Read(const wxString& key, type *val) const           \
    {                                                                       \
        wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        if ( !DoRead##name(key, val) )                                      \
            return false;                                                   \
                                                                            \
        *val = (extra)(*val);                                               \
                                                                            \
        return true;                                                        \
    }                                                                       \
                                                                            \
    bool wxConfigBase::Read(const wxString& key,                            \
                            type *val,                                      \
                            deftype defVal) const                           \
    {                                                                       \
        wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        bool read = DoRead##name(key, val);                                 \
        if ( !read )                                                        \
        {                                                                   \
            if ( IsRecordingDefaults() )                                    \
            {                                                               \
                const_cast<wxConfigBase*>(this)->DoWrite##name(key, defVal);\
            }                                                               \
                                                                            \
            *val = defVal;                                                  \
        }                                                                   \
                                                                            \
        *val = (extra)(*val);                                               \
                                                                            \
        return read;                                                        \
    }

IMPLEMENT_READ_FOR_TYPE(String, wxString, const wxString&, ExpandEnvVars)
IMPLEMENT_READ_FOR_TYPE(Long, long, long, long)
#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
IMPLEMENT_READ_FOR_TYPE(LongLong, wxLongLong_t, wxLongLong_t, wxLongLong_t)
#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
IMPLEMENT_READ_FOR_TYPE(Double, double, double, double)
IMPLEMENT_READ_FOR_TYPE(Bool, bool, bool, bool)

#undef IMPLEMENT_READ_FOR_TYPE

// int is stored as long
bool wxConfigBase::Read(const wxString& key, int *pi) const
{
    long l = *pi;
    bool r = Read(key, &l);
    wxASSERT_MSG( l < INT_MAX, wxT("int overflow in wxConfig::Read") );
    *pi = (int)l;
    return r;
}

bool wxConfigBase::Read(const wxString& key, int *pi, int defVal) const
{
    long l = *pi;
    bool r = Read(key, &l, defVal);
    wxASSERT_MSG( l < INT_MAX, wxT("int overflow in wxConfig::Read") );
    *pi = (int)l;
    return r;
}

// size_t is stored either as long or long long (Win64)
typedef long SizeSameSizeAsSizeT;

bool wxConfigBase::Read(const wxString& key, size_t* val) const
{
    wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );

    SizeSameSizeAsSizeT tmp;
    if ( !Read(key, &tmp) )
        return false;

    *val = static_cast<size_t>(tmp);
    return true;
}

bool wxConfigBase::Read(const wxString& key, size_t* val, size_t defVal) const
{
    wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );

    if ( !Read(key, val) )
    {
        *val = defVal;
        return false;
    }

    return true;
}

// Read floats as doubles then just type cast it down.
bool wxConfigBase::Read(const wxString& key, float* val) const
{
    wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );

    double temp;
    if ( !Read(key, &temp) )
        return false;

    wxCHECK_MSG( fabs(temp) <= double(FLT_MAX), false,
                     wxT("float overflow in wxConfig::Read") );
    wxCHECK_MSG( temp == 0.0 || fabs(temp) >= double(FLT_MIN), false,
                     wxT("float underflow in wxConfig::Read") );

    *val = static_cast<float>(temp);

    return true;
}

bool wxConfigBase::Read(const wxString& key, float* val, float defVal) const
{
    wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );

    if ( Read(key, val) )
        return true;

    *val = defVal;
    return false;
}

// the DoReadXXX() for the other types have implementation in the base class
// but can be overridden in the derived ones
bool wxConfigBase::DoReadBool(const wxString& key, bool* val) const
{
    wxCHECK_MSG( val, false, wxT("wxConfig::Read(): NULL parameter") );

    long l;
    if ( !DoReadLong(key, &l) )
        return false;

    if ( l != 0 && l != 1 )
    {
        // Don't assert here as this could happen in the result of user editing
        // the file directly and this not indicate a bug in the program but
        // still complain that something is wrong.
        wxLogWarning(_("Invalid value %ld for a boolean key \"%s\" in "
                       "config file."),
                     l, key);
    }

    *val = l != 0;

    return true;
}

#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG

bool wxConfigBase::DoReadLongLong(const wxString& key, wxLongLong_t *pll) const
{
    wxString str;
    if ( !Read(key, &str) )
        return false;

    str.Trim();

    return str.ToLongLong(pll);
}

#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG

bool wxConfigBase::DoReadDouble(const wxString& key, double* val) const
{
    wxString str;
    if ( Read(key, &str) )
    {
        if ( str.ToCDouble(val) )
            return true;

        // Previous versions of wxFileConfig wrote the numbers out using the
        // current locale and not the C one as now, so attempt to parse the
        // string as a number in the current locale too, for compatibility.
        if ( str.ToDouble(val) )
            return true;
    }

    return false;
}

// string reading helper
wxString wxConfigBase::ExpandEnvVars(const wxString& str) const
{
    wxString tmp; // Required for BC++
    if (IsExpandingEnvVars())
        tmp = wxExpandEnvVars(str);
    else
        tmp = str;
    return tmp;
}

// ----------------------------------------------------------------------------
// wxConfigBase writing
// ----------------------------------------------------------------------------

#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG

bool wxConfigBase::DoWriteLongLong(const wxString& key, wxLongLong_t llValue)
{
  return Write(key, wxString::Format("%" wxLongLongFmtSpec "d", llValue));
}

#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG

bool wxConfigBase::DoWriteDouble(const wxString& key, double val)
{
    // Notice that we always write out the numbers in C locale and not the
    // current one. This makes the config files portable between machines using
    // different locales.
    return DoWriteString(key, wxString::FromCDouble(val));
}

bool wxConfigBase::DoWriteBool(const wxString& key, bool value)
{
    return DoWriteLong(key, value ? 1l : 0l);
}

// ----------------------------------------------------------------------------
// wxConfigPathChanger
// ----------------------------------------------------------------------------

wxConfigPathChanger::wxConfigPathChanger(const wxConfigBase *pContainer,
                                         const wxString& strEntry)
{
  m_bChanged = false;
  m_pContainer = const_cast<wxConfigBase *>(pContainer);

  // the path is everything which precedes the last slash and the name is
  // everything after it -- and this works correctly if there is no slash too
  wxString strPath = strEntry.BeforeLast(wxCONFIG_PATH_SEPARATOR, &m_strName);

  // except in the special case of "/keyname" when there is nothing before "/"
  if ( strPath.empty() &&
       ((!strEntry.empty()) && strEntry[0] == wxCONFIG_PATH_SEPARATOR) )
  {
    strPath = wxCONFIG_PATH_SEPARATOR;
  }

  if ( !strPath.empty() )
  {
    if ( m_pContainer->GetPath() != strPath )
    {
        // we do change the path so restore it later
        m_bChanged = true;

        /* JACS: work around a memory bug that causes an assert
           when using wxRegConfig, related to reference-counting.
           Can be reproduced by removing .wc_str() below and
           adding the following code to the config sample OnInit under
           Windows:

           pConfig->SetPath(wxT("MySettings"));
           pConfig->SetPath(wxT(".."));
           int value;
           pConfig->Read(wxT("MainWindowX"), & value);
        */
        m_strOldPath = m_pContainer->GetPath().wc_str();
        if ( *m_strOldPath.c_str() != wxCONFIG_PATH_SEPARATOR )
          m_strOldPath += wxCONFIG_PATH_SEPARATOR;
        m_pContainer->SetPath(strPath);
    }
  }
}

void wxConfigPathChanger::UpdateIfDeleted()
{
    // we don't have to do anything at all if we didn't change the path
    if ( !m_bChanged )
        return;

    // find the deepest still existing parent path of the original path
    while ( !m_pContainer->HasGroup(m_strOldPath) )
    {
        m_strOldPath = m_strOldPath.BeforeLast(wxCONFIG_PATH_SEPARATOR);
        if ( m_strOldPath.empty() )
            m_strOldPath = wxCONFIG_PATH_SEPARATOR;
    }
}

wxConfigPathChanger::~wxConfigPathChanger()
{
  // only restore path if it was changed
  if ( m_bChanged ) {
    m_pContainer->SetPath(m_strOldPath);
  }
}

// this is a wxConfig method but it's mainly used with wxConfigPathChanger
/* static */
wxString wxConfigBase::RemoveTrailingSeparator(const wxString& key)
{
    wxString path(key);

    // don't remove the only separator from a root group path!
    while ( path.length() > 1 )
    {
        if ( *path.rbegin() != wxCONFIG_PATH_SEPARATOR )
            break;

        path.erase(path.end() - 1);
    }

    return path;
}


// this function is used to properly interpret '..' in path
void wxSplitPath(wxArrayString& aParts, const wxString& path)
{
  aParts.clear();

  wxString strCurrent;
  wxString::const_iterator pc = path.begin();
  for ( ;; ) {
    if ( pc == path.end() || *pc == wxCONFIG_PATH_SEPARATOR ) {
      if ( strCurrent == wxT(".") ) {
        // ignore
      }
      else if ( strCurrent == wxT("..") ) {
        // go up one level
        if ( aParts.size() == 0 )
        {
          wxLogWarning(_("'%s' has extra '..', ignored."), path);
        }
        else
        {
          aParts.erase(aParts.end() - 1);
        }

        strCurrent.Empty();
      }
      else if ( !strCurrent.empty() ) {
        aParts.push_back(strCurrent);
        strCurrent.Empty();
      }
      //else:
        // could log an error here, but we prefer to ignore extra '/'

      if ( pc == path.end() )
        break;
    }
    else
      strCurrent += *pc;

    ++pc;
  }
}
