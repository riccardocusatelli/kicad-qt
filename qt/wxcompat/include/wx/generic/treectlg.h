// The wxWidgets compatibility layer of the Qt build: <wx/generic/treectlg.h>, the generic tree
// control of wxWidgets, is the tree control of the layer (wx/kiwx_treectrl.h).
#pragma once
#include <wx/kiwx_treectrl.h>

// Only in this header: not exported by libkiqt (with Windows, dllimport would ask for its members)
class wxGenericTreeCtrl : public wxTreeCtrl
{
public:
    using wxTreeCtrl::wxTreeCtrl;
};
