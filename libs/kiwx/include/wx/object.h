/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/object.h, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>
#include <wx/chartype.h>

/// A reference counted object, deleted when its count drops to zero.
class wxRefCounter
{
public:
    wxRefCounter() = default;

    int  GetRefCount() const { return m_count; }
    void IncRef() { m_count++; }
    void DecRef()
    {
        wxASSERT_MSG( m_count > 0, "invalid ref data count" );

        if( --m_count == 0 )
            delete this;
    }

    wxRefCounter( const wxRefCounter& ) = delete;
    wxRefCounter& operator=( const wxRefCounter& ) = delete;

protected:
    virtual ~wxRefCounter() = default;

private:
    int m_count = 1;
};

typedef wxRefCounter wxObjectRefData;

/// Not used by kiwx: the wxWidgets run time type information is replaced by dynamic_cast.
class wxClassInfo;


/**
 * The root of the wxWidgets classes, with the shared (reference counted) data.  kiwx has no
 * run time type information of its own: dynamic_cast is used instead.
 */
class wxObject
{
public:
    wxObject() = default;
    virtual ~wxObject() { UnRef(); }

    wxObject( const wxObject& aOther ) : m_refData( aOther.m_refData )
    {
        if( m_refData )
            m_refData->IncRef();
    }

    wxObject& operator=( const wxObject& aOther )
    {
        if( this != &aOther )
            Ref( aOther );

        return *this;
    }

    wxObjectRefData* GetRefData() const { return m_refData; }
    void             SetRefData( wxObjectRefData* aData ) { m_refData = aData; }

    void Ref( const wxObject& aClone )
    {
        if( m_refData == aClone.m_refData )
            return;

        UnRef();

        if( aClone.m_refData )
        {
            m_refData = aClone.m_refData;
            m_refData->IncRef();
        }
    }

    void UnRef()
    {
        if( m_refData )
        {
            m_refData->DecRef();
            m_refData = nullptr;
        }
    }

    void UnShare() { AllocExclusive(); }
    bool IsSameAs( const wxObject& aOther ) const { return m_refData == aOther.m_refData; }

protected:
    void AllocExclusive()
    {
        if( !m_refData )
        {
            m_refData = CreateRefData();
        }
        else if( m_refData->GetRefCount() > 1 )
        {
            const wxObjectRefData* ref = m_refData;
            UnRef();
            m_refData = CloneRefData( ref );   // ref is still alive: it had other owners
        }

        wxASSERT_MSG( m_refData && m_refData->GetRefCount() == 1,
                      wxT( "wxObject::AllocExclusive() failed." ) );
    }

    virtual wxObjectRefData* CreateRefData() const
    {
        wxFAIL_MSG( wxT( "CreateRefData() must be overridden if called!" ) );
        return nullptr;
    }

    virtual wxObjectRefData* CloneRefData( const wxObjectRefData* ) const
    {
        wxFAIL_MSG( wxT( "CloneRefData() must be overridden if called!" ) );
        return nullptr;
    }

    wxObjectRefData* m_refData = nullptr;
};


#define wxDECLARE_CLASS( name )
#define wxDECLARE_ABSTRACT_CLASS( name )
#define wxDECLARE_DYNAMIC_CLASS( name )
#define wxDECLARE_DYNAMIC_CLASS_NO_COPY( name )
#define wxIMPLEMENT_CLASS( name, base )
#define wxIMPLEMENT_ABSTRACT_CLASS( name, base )
#define wxIMPLEMENT_DYNAMIC_CLASS( name, base )

/**
 * A smart pointer to a reference counted object (a wxRefCounter, or any class with IncRef() and
 * DecRef()).  As in wxWidgets, the pointer given to the constructor or to reset() is adopted
 * without a new reference: its object was created with a count of one.
 */
template <class T>
class wxObjectDataPtr
{
public:
    typedef T element_type;

    explicit wxObjectDataPtr( T* aPtr = nullptr ) : m_ptr( aPtr ) {}

    wxObjectDataPtr( const wxObjectDataPtr<T>& aOther ) : m_ptr( aOther.get() )
    {
        if( m_ptr )
            m_ptr->IncRef();
    }

    template <typename U>
    wxObjectDataPtr( const wxObjectDataPtr<U>& aOther ) : m_ptr( aOther.get() )
    {
        if( m_ptr )
            m_ptr->IncRef();
    }

    ~wxObjectDataPtr()
    {
        if( m_ptr )
            m_ptr->DecRef();
    }

    T* get() const { return m_ptr; }

    explicit operator bool() const { return m_ptr != nullptr; }

    T& operator*() const { return *m_ptr; }
    T* operator->() const { return m_ptr; }

    void reset( T* aPtr )
    {
        if( m_ptr )
            m_ptr->DecRef();

        m_ptr = aPtr;
    }

    T* release()
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

    wxObjectDataPtr& operator=( const wxObjectDataPtr& aOther )
    {
        if( aOther.m_ptr )
            aOther.m_ptr->IncRef();

        if( m_ptr )
            m_ptr->DecRef();

        m_ptr = aOther.m_ptr;
        return *this;
    }

    template <typename U>
    wxObjectDataPtr& operator=( const wxObjectDataPtr<U>& aOther )
    {
        T* ptr = aOther.get();

        if( ptr )
            ptr->IncRef();

        if( m_ptr )
            m_ptr->DecRef();

        m_ptr = ptr;
        return *this;
    }

    wxObjectDataPtr& operator=( T* aPtr )
    {
        reset( aPtr );
        return *this;
    }

private:
    T* m_ptr;
};


#define wxDynamicCast( obj, className ) dynamic_cast<className*>( obj )
#define wxStaticCast( obj, className ) static_cast<className*>( obj )
#define wxConstCast( obj, className ) const_cast<className*>( obj )

#ifndef _WIN32
#pragma GCC visibility pop
#endif
