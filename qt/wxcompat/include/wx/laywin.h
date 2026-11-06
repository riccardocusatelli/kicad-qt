// The wxWidgets compatibility layer of the Qt build: <wx/laywin.h>, the windows laid out by a
// wxLayoutAlgorithm.  KiCad uses a wxSashLayoutWindow as a panel of an AUI manager only: it is a
// wxPanel here.
#pragma once
#include <wx/kiwx_gui.h>

class wxSashLayoutWindow : public wxPanel
{
public:
    using wxPanel::wxPanel;
};
