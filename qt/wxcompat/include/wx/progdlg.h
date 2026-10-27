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
 * @file progdlg.h
 * The progress dialog (wxProgressDialog) of the wxWidgets compatibility layer, on
 * QProgressDialog.
 */

#pragma once

#include <wx/kiwx_gui.h>

#include <QApplication>
#include <QProgressDialog>

#include <qt_strings.h>


#define wxPD_CAN_ABORT      0x0001
#define wxPD_APP_MODAL      0x0002
#define wxPD_AUTO_HIDE      0x0004
#define wxPD_ELAPSED_TIME   0x0008
#define wxPD_ESTIMATED_TIME 0x0010
#define wxPD_SMOOTH         0x0020
#define wxPD_REMAINING_TIME 0x0040
#define wxPD_CAN_SKIP       0x0080


/**
 * A dialog showing the progress of a long operation (wxProgressDialog).  As wxWidgets, Update()
 * shows the dialog, lets the user interface process its events and tells whether the user
 * cancelled; with wxPD_AUTO_HIDE the dialog hides when the value reaches the maximum.
 */
class wxProgressDialog : public QProgressDialog
{
public:
    wxProgressDialog( const wxString& aTitle, const wxString& aMessage, int aMaximum = 100,
                      wxWidgetArgCompat aParent = nullptr,
                      int aStyle = wxPD_APP_MODAL | wxPD_AUTO_HIDE ) :
            QProgressDialog( ToQString( aMessage ), QString(), 0, aMaximum, aParent ),
            m_style( aStyle )
    {
        setWindowTitle( ToQString( aTitle ) );
        setAutoClose( false );
        setAutoReset( false );
        setMinimumDuration( 0 );

        if( aStyle & wxPD_APP_MODAL )
            setWindowModality( Qt::ApplicationModal );

        if( aStyle & wxPD_CAN_ABORT )
            setCancelButtonText( QObject::tr( "Cancel" ) );

        setValue( 0 );
        show();
        QApplication::processEvents();
    }

    virtual ~wxProgressDialog() = default;

    virtual bool Update( int aValue, const wxString& aNewMsg = wxEmptyString,
                         bool* aSkip = nullptr )
    {
        if( aSkip )
            *aSkip = false;

        if( !aNewMsg.IsEmpty() )
            setLabelText( ToQString( aNewMsg ) );

        setValue( std::min( aValue, maximum() ) );

        if( aValue >= maximum() && ( m_style & wxPD_AUTO_HIDE ) )
            hide();

        QApplication::processEvents();
        return !wasCanceled();
    }

    virtual bool Pulse( const wxString& aNewMsg = wxEmptyString, bool* aSkip = nullptr )
    {
        // A busy indicator: no range
        setRange( 0, 0 );
        return Update( 0, aNewMsg, aSkip );
    }

    void Resume() { reset(); }

    int GetValue() const { return value(); }
    int GetRange() const { return maximum(); }
    void SetRange( int aMaximum ) { setMaximum( aMaximum ); }
    wxString GetMessage() const { return ToWxString( labelText() ); }

    bool WasCancelled() const { return wasCanceled(); }
    bool WasSkipped() const { return false; }

    /// The layout is the one of the Qt dialog: no sizer.
    void Layout() {}
    wxSizer* GetSizer() const { return nullptr; }

private:
    int m_style;
};


/// The generic progress dialog: the same dialog.
using wxGenericProgressDialog = wxProgressDialog;
