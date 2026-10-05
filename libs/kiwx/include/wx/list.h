/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/defs.h>

#include <cstddef>
#include <iterator>

/**
 * The doubly linked list of pointers of WX_DECLARE_LIST(), with its node API
 * (compatibility_iterator) and a minimal STL interface.  With DeleteContents( true ) the
 * list deletes the objects it holds.
 */
template <typename T>
class kiwxList
{
public:
    class Node
    {
    public:
        T*    GetData() const { return m_data; }
        void  SetData( T* aData ) { m_data = aData; }
        Node* GetNext() const { return m_next; }
        Node* GetPrevious() const { return m_prev; }

        int IndexOf() const
        {
            int i = 0;

            for( const Node* n = m_prev; n; n = n->m_prev )
                ++i;

            return i;
        }

    private:
        friend class kiwxList;

        T*    m_data = nullptr;
        Node* m_next = nullptr;
        Node* m_prev = nullptr;
    };

    typedef Node* compatibility_iterator;
    typedef T*    value_type;

    class const_iterator
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T*;
        using difference_type = std::ptrdiff_t;
        using pointer = T**;
        using reference = T*;

        const_iterator( Node* aNode = nullptr, const kiwxList* aList = nullptr ) :
                m_node( aNode ), m_list( aList )
        {
        }

        T* operator*() const { return m_node->GetData(); }

        const_iterator& operator++()
        {
            m_node = m_node->GetNext();
            return *this;
        }

        const_iterator operator++( int )
        {
            const_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator& operator--()
        {
            m_node = m_node ? m_node->GetPrevious() : m_list->GetLast();
            return *this;
        }

        bool operator==( const const_iterator& aOther ) const { return m_node == aOther.m_node; }
        bool operator!=( const const_iterator& aOther ) const { return m_node != aOther.m_node; }

    private:
        Node*           m_node;
        const kiwxList* m_list;
    };

    typedef const_iterator iterator;

    kiwxList() = default;
    kiwxList( const kiwxList& aOther ) { *this = aOther; }

    /// Copies the pointers (and the DeleteContents() setting, as wxWidgets).
    kiwxList& operator=( const kiwxList& aOther )
    {
        if( this != &aOther )
        {
            Clear();
            m_deleteContents = aOther.m_deleteContents;

            for( Node* n = aOther.m_first; n; n = n->m_next )
                Append( n->m_data );
        }

        return *this;
    }

    ~kiwxList() { Clear(); }

    size_t GetCount() const { return m_count; }
    size_t size() const { return m_count; }
    bool   IsEmpty() const { return m_count == 0; }
    bool   empty() const { return m_count == 0; }

    Node* GetFirst() const { return m_first; }
    Node* GetLast() const { return m_last; }

    Node* Item( size_t aIndex ) const
    {
        Node* n = m_first;

        while( n && aIndex-- )
            n = n->m_next;

        return n;
    }

    T* operator[]( size_t aIndex ) const
    {
        Node* n = Item( aIndex );
        return n ? n->m_data : nullptr;
    }

    Node* Append( T* aObject )
    {
        Node* n = new Node;
        n->m_data = aObject;
        n->m_prev = m_last;

        if( m_last )
            m_last->m_next = n;
        else
            m_first = n;

        m_last = n;
        m_count++;
        return n;
    }

    /// Inserts at the start.
    Node* Insert( T* aObject ) { return Insert( m_first, aObject ); }

    /// Inserts at the given position.
    Node* Insert( size_t aPos, T* aObject )
    {
        return aPos >= m_count ? Append( aObject ) : Insert( Item( aPos ), aObject );
    }

    /// Inserts before the given node (at the end if null).
    Node* Insert( Node* aBefore, T* aObject )
    {
        if( !aBefore )
            return Append( aObject );

        Node* n = new Node;
        n->m_data = aObject;
        n->m_next = aBefore;
        n->m_prev = aBefore->m_prev;

        if( aBefore->m_prev )
            aBefore->m_prev->m_next = n;
        else
            m_first = n;

        aBefore->m_prev = n;
        m_count++;
        return n;
    }

    void push_back( T* aObject ) { Append( aObject ); }
    void push_front( T* aObject ) { Insert( aObject ); }
    T*   front() const { return m_first->m_data; }
    T*   back() const { return m_last->m_data; }

    Node* Find( const T* aObject ) const
    {
        for( Node* n = m_first; n; n = n->m_next )
        {
            if( n->m_data == aObject )
                return n;
        }

        return nullptr;
    }

    bool Member( const T* aObject ) const { return Find( aObject ) != nullptr; }

    int IndexOf( const T* aObject ) const
    {
        Node* n = Find( aObject );
        return n ? n->IndexOf() : wxNOT_FOUND;
    }

    /// Removes the node, deleting the object if DeleteContents( true ).
    bool DeleteNode( Node* aNode )
    {
        if( !aNode )
            return false;

        unlink( aNode );

        if( m_deleteContents )
            delete aNode->m_data;

        delete aNode;
        return true;
    }

    /// Removes the node without deleting the object.
    void Erase( Node* aNode )
    {
        if( aNode )
        {
            unlink( aNode );
            delete aNode;
        }
    }

    bool DeleteObject( T* aObject ) { return DeleteNode( Find( aObject ) ); }

    void Clear()
    {
        Node* n = m_first;

        while( n )
        {
            Node* next = n->m_next;

            if( m_deleteContents )
                delete n->m_data;

            delete n;
            n = next;
        }

        m_first = m_last = nullptr;
        m_count = 0;
    }

    void clear() { Clear(); }

    void DeleteContents( bool aDestroy ) { m_deleteContents = aDestroy; }
    bool GetDeleteContents() const { return m_deleteContents; }

    const_iterator begin() const { return const_iterator( m_first, this ); }
    const_iterator end() const { return const_iterator( nullptr, this ); }

private:
    void unlink( Node* aNode )
    {
        if( aNode->m_prev )
            aNode->m_prev->m_next = aNode->m_next;
        else
            m_first = aNode->m_next;

        if( aNode->m_next )
            aNode->m_next->m_prev = aNode->m_prev;
        else
            m_last = aNode->m_prev;

        m_count--;
    }

    Node*  m_first = nullptr;
    Node*  m_last = nullptr;
    size_t m_count = 0;
    bool   m_deleteContents = false;
};


#define WX_DECLARE_LIST_WITH_DECL( T, name, decl )                                               \
    decl name : public kiwxList<T>                                                               \
    {                                                                                            \
    public:                                                                                      \
        using kiwxList<T>::kiwxList;                                                             \
    }
#define WX_DECLARE_LIST( T, name ) WX_DECLARE_LIST_WITH_DECL( T, name, class )
#define WX_DECLARE_EXPORTED_LIST( T, name ) WX_DECLARE_LIST( T, name )
#define WX_DECLARE_USER_EXPORTED_LIST( T, name, expmode ) WX_DECLARE_LIST( T, name )
#define WX_DEFINE_LIST( name )
#define WX_DEFINE_EXPORTED_LIST( name )

#pragma GCC visibility pop
