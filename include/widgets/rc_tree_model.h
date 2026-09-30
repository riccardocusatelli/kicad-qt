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
 * @file rc_tree_model.h
 * wxDataViewCtrl model listing RC_ITEMs (DRC/ERC dialogs).  Split from rc_item.h so the
 * violation data (RC_ITEM) does not depend on wx GUI headers.
 */

#ifndef RC_TREE_MODEL_H
#define RC_TREE_MODEL_H

#include <deque>
#include <memory>
#include <vector>

#include <wx/dataview.h>

#include <rc_item.h>

class EDA_DRAW_FRAME;
class MARKER_BASE;


class RC_TREE_NODE
{
public:
    enum NODE_TYPE
    {
        MARKER,
        MAIN_ITEM,
        AUX_ITEM,
        AUX_ITEM2,
        AUX_ITEM3,
        COMMENT
    };

    RC_TREE_NODE( RC_TREE_NODE* aParent, const std::shared_ptr<RC_ITEM>& aRcItem,
                  NODE_TYPE aType ) :
            m_Type( aType ),
            m_RcItem( aRcItem ),
            m_Parent( aParent )
    {}

    ~RC_TREE_NODE()
    {
        for( RC_TREE_NODE* child : m_Children )
            delete child;
    }

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    RC_TREE_NODE( const RC_TREE_NODE& ) = delete;
    RC_TREE_NODE& operator=( const RC_TREE_NODE& ) = delete;

    NODE_TYPE                  m_Type;
    std::shared_ptr<RC_ITEM>   m_RcItem;

    struct HANDLE
    {
        RC_TREE_NODE* m_Node = nullptr;
    };

    HANDLE*                    m_Handle = nullptr;
    RC_TREE_NODE*              m_Parent;
    std::vector<RC_TREE_NODE*> m_Children;
};


class RC_TREE_MODEL : public wxDataViewModel, public wxEvtHandler
{
public:
    static wxDataViewItem ToItem( RC_TREE_NODE const* aNode )
    {
        return aNode && aNode->m_Handle ? wxDataViewItem( aNode->m_Handle ) : wxDataViewItem();
    }

    static RC_TREE_NODE* ToNode( wxDataViewItem aItem )
    {
        auto* handle = static_cast<RC_TREE_NODE::HANDLE*>( aItem.GetID() );
        return handle ? handle->m_Node : nullptr;
    }

    const wxDataViewCtrl* GetView() const { return m_view; }

    static KIID ToUUID( wxDataViewItem aItem );

    RC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView );

    ~RC_TREE_MODEL();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    RC_TREE_MODEL( const RC_TREE_MODEL& ) = delete;
    RC_TREE_MODEL& operator=( const RC_TREE_MODEL& ) = delete;

    void Update( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities );

    void ExpandAll();

    void PrevMarker();
    void NextMarker();
    void SelectMarker( const MARKER_BASE* aMarker );
    void CenterMarker( const MARKER_BASE* aMarker );

    bool IsContainer( wxDataViewItem const& aItem ) const override;

    wxDataViewItem GetParent( wxDataViewItem const& aItem ) const override;

    unsigned int GetChildren( wxDataViewItem const& aItem,
                              wxDataViewItemArray&  aChildren ) const override;

    bool HasContainerColumns( wxDataViewItem const& aItem ) const override { return true; }

    bool HasValue( const wxDataViewItem& item, unsigned col ) const override
    {
        if( m_tree.empty() )
            return false;
        else
            return wxDataViewModel::HasValue( item, col );
    }

    /**
     * Called by the wxDataView to fetch an item's value.
     */
    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) const override;

    /**
     * Called by the wxDataView to edit an item's content.
     */
    bool SetValue( wxVariant const& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) override
    {
        // Editing not supported
        return false;
    }

    /**
     * Called by the wxDataView to fetch an item's formatting.  Return true if the
     * item has non-default attributes.
     */
    bool GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                  wxDataViewItemAttr& aAttr ) const override;

    void ValueChanged( RC_TREE_NODE* aNode );

    void DeleteCurrentItem( bool aDeep );

    /**
     * Delete the current item or all items.
     *
     * If all, \a aIncludeExclusions determines whether or not exclusions are also deleted.
     */
    void DeleteItems( bool aCurrentOnly, bool aIncludeExclusions, bool aDeep );

protected:
    RC_TREE_NODE* createNode( RC_TREE_NODE* aParent, const std::shared_ptr<RC_ITEM>& aRcItem,
                              RC_TREE_NODE::NODE_TYPE aType );
    void          retireNodeTree( RC_TREE_NODE* aNode );
    void          deleteNodeTree( RC_TREE_NODE* aNode );

    /**
     * Retire and destroy every node in the tree.
     *
     * Retired handles are kept alive so that stale wxDataViewItems resolve to nullptr.
     */
    void          clearTree();

    /**
     * Repopulate the node tree from \a aProvider.  Touches no wxWidgets state.
     */
    void          rebuildTree( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities );

    void     rebuildModel( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities );

    EDA_DRAW_FRAME*                    m_editFrame;
    wxDataViewCtrl*                    m_view;
    int                                m_severities;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_rcItemsProvider;

    /// Stable wx item IDs.  Handles are retired but never freed before the model dies because
    /// wxDataViewCtrl hands back item IDs long after the rows they named were destroyed; the
    /// deque keeps existing handle addresses valid as more are appended.
    std::deque<RC_TREE_NODE::HANDLE>   m_handles;
    std::vector<RC_TREE_NODE*>         m_tree;              // I own this
};

#endif // RC_TREE_MODEL_H
