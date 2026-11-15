// The wxWidgets compatibility layer of the Qt build: <wx/dnd.h>, the drag and drop (its targets
// are part of wx/kiwx_gui.h).
#pragma once
#include <wx/kiwx_gui.h>

/// The result of a drag and drop (wxDragResult).
enum wxDragResult
{
    wxDragError,
    wxDragNone,
    wxDragCopy,
    wxDragMove,
    wxDragLink,
    wxDragCancel
};

/// The flags of a drag (wxDrag_CopyOnly...).
enum
{
    wxDrag_CopyOnly = 0,
    wxDrag_AllowMove = 1,
    wxDrag_DefaultMove = 3
};
