// The wxWidgets compatibility layer of the Qt build: <wx/generic/treectlg.h>, the generic tree
// control of wxWidgets, is the tree control of the layer (wx/kiwx_treectrl.h).
#pragma once
#include <wx/kiwx_treectrl.h>

class KIQT_API wxGenericTreeCtrl : public wxTreeCtrl
{
public:
    using wxTreeCtrl::wxTreeCtrl;
};
