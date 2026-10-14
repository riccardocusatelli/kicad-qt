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
 * @file kiwx_statusbar.h
 * The status bar of the wxWidgets compatibility layer (wxStatusBar): a QStatusBar with fields.
 */

#ifndef KIWX_STATUSBAR_H
#define KIWX_STATUSBAR_H

#include <vector>

#include <QStatusBar>

#include <wx/kiwx_gui.h>

class QLabel;

#define wxSTB_SIZEGRIP         0x0010
#define wxSTB_SHOW_TIPS        0x0020
#define wxSTB_ELLIPSIZE_START  0x0040
#define wxSTB_ELLIPSIZE_MIDDLE 0x0080
#define wxSTB_ELLIPSIZE_END    0x0100
#define wxSTB_DEFAULT_STYLE    ( wxSTB_SIZEGRIP | wxSTB_ELLIPSIZE_END | wxSTB_SHOW_TIPS )

#define wxSB_NORMAL 0x0000
#define wxSB_FLAT   0x0001
#define wxSB_RAISED 0x0002
#define wxSB_SUNKEN 0x0003


/**
 * A status bar (wxStatusBar): its fields are labels, with the widths of wxWidgets (a positive
 * width is fixed, a negative one is a stretch factor).
 */
class KIQT_API wxStatusBar : public wxWindowCompat<QStatusBar>
{
public:
    explicit wxStatusBar( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                          long aStyle = wxSTB_DEFAULT_STYLE,
                          const wxString& aName = wxS( "statusBar" ) );

    void SetFieldsCount( int aNumber = 1, const int* aWidths = nullptr );
    int GetFieldsCount() const { return int( m_fields.size() ); }

    void SetStatusWidths( int aNumber, const int* aWidths );
    void SetStatusStyles( int, const int* ) {}

    void SetStatusText( const wxString& aText, int aField = 0 );
    wxString GetStatusText( int aField = 0 ) const;

    void PushStatusText( const wxString& aText, int aField = 0 );
    void PopStatusText( int aField = 0 );

    bool GetFieldRect( int aField, wxRect& aRect ) const;

private:
    struct FIELD
    {
        QLabel*               m_label = nullptr;
        std::vector<wxString> m_stack;
    };

    std::vector<FIELD> m_fields;
};

#endif // KIWX_STATUSBAR_H
