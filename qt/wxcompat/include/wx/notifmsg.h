// The wxWidgets compatibility layer of the Qt build: <wx/notifmsg.h>, the notifications of the
// system (wxNotificationMessage): KiCad shows its notifications in its status bars, this one is
// not shown.
#pragma once
#include <wx/kiwx_gui.h>

class wxNotificationMessage
{
public:
    wxNotificationMessage() = default;
    wxNotificationMessage( const wxString& aTitle, const wxString& aMessage = wxEmptyString,
                           wxWidgetArgCompat aParent = nullptr, int aFlags = 0 ) :
            m_title( aTitle ),
            m_message( aMessage )
    {
    }

    void SetTitle( const wxString& aTitle ) { m_title = aTitle; }
    void SetMessage( const wxString& aMessage ) { m_message = aMessage; }
    void SetParent( wxWidgetArgCompat ) {}
    void SetFlags( int ) {}
    bool Show( int aTimeout = -1 ) { return false; }
    bool Close() { return true; }

private:
    wxString m_title;
    wxString m_message;
};
