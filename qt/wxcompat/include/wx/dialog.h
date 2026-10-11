// The wxWidgets compatibility layer of the Qt build: the dialogs of KiCad derive from
// DIALOG_SHIM (a QDialog), wxDialog is DIALOG_SHIM.
#pragma once
#include <wx/kiwx_gui.h>
#include <dialog_shim.h>

using wxDialog = DIALOG_SHIM;
