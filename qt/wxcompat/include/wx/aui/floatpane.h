// The wxWidgets compatibility layer of the Qt build: <wx/aui/floatpane.h>. The floating panes
// are floating dock widgets of Qt, not frames with their own manager: wxAuiPaneInfo::frame is
// always null, so there is never a wxAuiFloatingFrame.
#pragma once
#include <wx/aui/framemanager.h>

class wxAuiFloatingFrame : public wxWindow
{
public:
    wxAuiFloatingFrame() = delete;

    wxAuiManager& GetAuiManager() { return *m_manager; }

private:
    wxAuiManager* m_manager = nullptr;
};
