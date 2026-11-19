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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// The Qt build of include/widgets/editor_tabs_panel.h: the same model and API, with a QTabBar
// for the strip. The shared canvas stays in the central area of the frame, under the strip.

#ifndef EDITOR_TABS_PANEL_H
#define EDITOR_TABS_PANEL_H

#include <functional>
#include <vector>

#include <QWidget>

#include <wx/string.h>

#include <widgets/kicad_tab_art.h>

class EDA_DRAW_PANEL_GAL;
class QTabBar;


/// GUI-free preview/dirty/close state machine. One unedited preview tab is reused for the next
/// preview; editing it or double-clicking its tab promotes it to permanent.
class EDITOR_TABS_MODEL
{
public:
    struct ENTRY
    {
        wxString key;
        bool     preview = false;
        bool     modified = false;
    };

    /**
     * Return the index to display the document at, reusing a preview slot or appending a new tab.
     */
    int OpenDocument( const wxString& aKey, bool aAsPreview );

    /**
     * Drop the document with @p aKey, freeing the preview slot if it held it.
     */
    void CloseDocument( const wxString& aKey );

    /**
     * Update the modified flag. Turning it on promotes a preview tab to permanent.
     */
    void MarkModified( const wxString& aKey, bool aModified );

    /**
     * Clear the preview flag so the tab becomes permanent and is no longer reused.
     */
    void Promote( const wxString& aKey );

    /**
     * Re-key the entry for @p aOldKey to @p aNewKey, keeping its position and state.
     */
    void Rename( const wxString& aOldKey, const wxString& aNewKey );

    /**
     * True when the document has no unsaved edits and can be closed silently.
     */
    bool CanCloseWithoutPrompt( const wxString& aKey ) const;

    int FindIndex( const wxString& aKey ) const;

    /**
     * Index of the current reusable preview tab, or -1 if none.
     */
    int PreviewIndex() const;

    const std::vector<ENTRY>& Entries() const { return m_entries; }

private:
    std::vector<ENTRY> m_entries;
};


/// The tab strip of the documents open on the single shared GAL canvas. Each open document is one
/// tab; the host owns the documents.
class EDITOR_TABS_PANEL : public QWidget
{
public:
    EDITOR_TABS_PANEL( QWidget* aParent, EDA_DRAW_PANEL_GAL* aSharedCanvas );
    ~EDITOR_TABS_PANEL() override;

    /// Host swaps the active document context to the tab at the given index.
    std::function<void( int )> onActivateTab;

    /// Host prompts as needed; return false to veto the close.
    std::function<bool( int )> onCloseTabRequested;

    /// Host reports the visual state (modified/preview) for the tab at the given index.
    std::function<TAB_VISUAL_STATE( int )> onQueryVisualState;

    /// The canvas is never reparented in the Qt build: nothing to hand back.
    void ReleaseSharedCanvas() {}

    void SetSuppressActivateOnClose( bool aSuppress ) { m_suppressActivateOnClose = aSuppress; }

    int  ActivateOrCreateTab( const wxString& aKey, const wxString& aLabel, bool aAsPreview );
    bool CloseTab( int aIdx );
    void CloseOthers( int aKeepIdx );
    void CloseToRight( int aIdx );
    void CloseAll();
    void MarkModified( int aIdx, bool aModified );
    void PromoteTab( int aIdx );
    void SelectTab( int aIdx );
    void AdvanceTab( bool aForward );
    int  GetActiveTab() const;
    int  FindTab( const wxString& aKey ) const;
    void RefreshTabLabels();
    void RenameTab( const wxString& aOldKey, const wxString& aNewKey, const wxString& aNewLabel );

    const EDITOR_TABS_MODEL& Model() const { return m_model; }
    EDITOR_TABS_MODEL&       MutableModel() { return m_model; }

private:
    bool closeTabInternal( int aIdx );
    void activateTab( int aIdx );
    void touchMru( const wxString& aKey );
    void forgetMru( const wxString& aKey );
    void showContextMenu( const QPoint& aPos );
    void applyLabel( int aIdx );

    TAB_VISUAL_STATE visualStateForIndex( int aIdx ) const;

private:
    QTabBar*              m_tabs = nullptr;
    EDA_DRAW_PANEL_GAL*   m_sharedCanvas = nullptr;
    EDITOR_TABS_MODEL     m_model;

    /// The labels without decorations, index-aligned with the model entries.
    std::vector<wxString> m_labels;

    /// Guards activateTab() against re-entrancy from programmatic selections.
    bool                  m_activating = false;
    bool                  m_suppressActivateOnClose = false;

    /// Most-recently-used key order for Ctrl+Tab cycling; front is most recent.
    std::vector<wxString> m_mru;
};

#endif // EDITOR_TABS_PANEL_H
