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
 * @file panel_design_block_chooser.h
 * PANEL_DESIGN_BLOCK_CHOOSER of the Qt build: the LIB_TREE of the design blocks over the details
 * panel with the preview, in a QSplitter.  What the panel does is shared with the wxWidgets build
 * (common/widgets/panel_design_block_chooser.cpp); the widgets are in
 * panel_design_block_chooser_qt.cpp.
 */

#ifndef PANEL_DESIGN_BLOCK_CHOOSER_H
#define PANEL_DESIGN_BLOCK_CHOOSER_H

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QWidget>

#include <functional>
#include <vector>

#include <widgets/lib_tree.h>
#include <design_block_tree_model_adapter.h>
#include <widgets/html_window.h>

class EDA_DRAW_FRAME;
class DESIGN_BLOCK_PANE;
class DESIGN_BLOCK_PREVIEW_WIDGET;
class QKeyEvent;
class QSplitter;
class QTimer;
class QVBoxLayout;
class TOOL_INTERACTIVE;


class PANEL_DESIGN_BLOCK_CHOOSER : public QWidget
{
public:
    /**
     * Panel for using design blocks.
     *
     * @param aFrame  the parent frame (usually a SCH_EDIT_FRAME or PCB_EDIT_FRAME)
     * @param aParent the parent design block pane
     * @param aSelectHandler a handler to be called on double-click of a design block
     * @param aContextMenuTool the tool that will be used to provide an appropriate context menu
     *                         for the design block actions available in that frame
     */
    PANEL_DESIGN_BLOCK_CHOOSER( EDA_DRAW_FRAME* aFrame, DESIGN_BLOCK_PANE* aParent,
                                std::vector<LIB_ID>&  aHistoryList,
                                std::function<void()> aSelectHandler,
                                TOOL_INTERACTIVE*     aContextMenuTool );

    ~PANEL_DESIGN_BLOCK_CHOOSER() override;

    void SaveSettings();

    /// The Escape key in the search field: true if used.
    bool OnChar( QKeyEvent* aEvent );

    QWidget* GetDetailsPanel() { return m_detailsPanel; }

    void SetPreviewWidget( DESIGN_BLOCK_PREVIEW_WIDGET* aPreview );

    DESIGN_BLOCK_PREVIEW_WIDGET* GetPreviewWidget() const { return m_preview; }

    void FinishSetup();

    void SetPreselect( const LIB_ID& aPreselect );

    void RefreshLibs( bool aProgress = false );

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the design_block that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    void SelectLibId( const LIB_ID& aLibId );

    LIB_TREE* GetLibTree() { return m_tree; }

    void ShowChangedLanguage();

protected:
    static constexpr int DBLCLICK_DELAY = 100; // milliseconds

    void onCloseTimer();
    void onOpenLibsTimer();

    void onDesignBlockSelected();

    /**
     * Handle the selection of an item. This is called when either the search box or the tree
     * receive an Enter, or the tree receives a double click.
     */
    void onDesignBlockChosen();

    void addDesignBlockToHistory( const LIB_ID& aLibId );
    void rebuildHistoryNode();

    /// Show the selected design block in the preview (shared by the GUI toolkits).
    void showSelectedDesignBlock();

    /// Repaint the panel (with the GUI toolkit).
    void refreshView();

protected:
    static wxString       g_designBlockSearchString;

    QTimer*               m_dbl_click_timer;
    QTimer*               m_open_libs_timer;
    QSplitter*            m_vsplitter;

    QWidget*              m_detailsPanel;
    QVBoxLayout*          m_detailsSizer;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    LIB_TREE*                    m_tree;
    DESIGN_BLOCK_PREVIEW_WIDGET* m_preview;

    DESIGN_BLOCK_PANE*    m_parent;
    EDA_DRAW_FRAME*       m_frame;
    std::function<void()> m_selectHandler;

    std::vector<LIB_ID>   m_historyList;

    /// The selection shows its design block (after the libraries are opened)
    bool                  m_showSelection;
};

#endif /* PANEL_DESIGN_BLOCK_CHOOSER_H */
