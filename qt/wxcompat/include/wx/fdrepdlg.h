// The wxWidgets compatibility layer of the Qt build: <wx/fdrepdlg.h>.  The find dialogs of KiCad
// are its own (DIALOG_SCH_FIND): only the flags of wxWidgets are part of the layer.
#pragma once
#include <wx/kiwx_gui.h>

/// The flags of the search (wxFindReplaceFlags).
enum wxFindReplaceFlags
{
    wxFR_DOWN = 1,
    wxFR_WHOLEWORD = 2,
    wxFR_MATCHCASE = 4
};

/// The styles of the find dialogs (wxFindReplaceDialogStyles).
enum wxFindReplaceDialogStyles
{
    wxFR_REPLACEDIALOG = 1,
    wxFR_NOUPDOWN = 2,
    wxFR_NOMATCHCASE = 4,
    wxFR_NOWHOLEWORD = 8
};
