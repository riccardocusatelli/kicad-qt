/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/any.cpp, wxWindows Library Licence 3.1),
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

// For compilers that support precompilation, includes "wx/wx.h".

#include <wx/any.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace wxPrivate;

//-------------------------------------------------------------------------
// wxAnyValueTypeGlobals
//-------------------------------------------------------------------------

typedef std::unordered_map<wxAnyValueType*, wxVariantDataFactory> wxAnyTypeToVariantDataFactoryMap;

//
// Helper class to manage global variables related to type conversion
// between wxAny and wxVariant.
//
class wxAnyValueTypeGlobals
{
public:
    wxAnyValueTypeGlobals()
    {
    }
    ~wxAnyValueTypeGlobals()
    {
        m_anyToVariant.clear();
    }

    void PreRegisterAnyToVariant(wxAnyToVariantRegistration* reg)
    {
        m_anyToVariantRegs.push_back(reg);
    }

    // Find wxVariantData factory function for given value type,
    // (or compatible, if possible)
    wxVariantDataFactory FindVariantDataFactory(const wxAnyValueType* type_)
    {
        // Ideally we'd have the hash map of type 'const wxAnyValueType*',
        // but WX_DECLARE_HASH_MAP() has some trouble with it.
        wxAnyValueType* type = const_cast<wxAnyValueType*>(type_);

        wxAnyTypeToVariantDataFactoryMap& anyToVariant = m_anyToVariant;
        wxAnyTypeToVariantDataFactoryMap::const_iterator it;
        it = anyToVariant.find(type);
        if ( it != anyToVariant.end() )
            return it->second;

        // Not found, handle pre-registrations
        size_t i = m_anyToVariantRegs.size();
        while ( i > 0 )
        {
            i--;
            wxAnyToVariantRegistration* reg = m_anyToVariantRegs[i];
            wxAnyValueType* assocType = reg->GetAssociatedType();
            if ( assocType )
            {
                // Both variant data and wxAnyValueType have been
                // now been properly initialized, so remove the
                // pre-registration entry and move data to anyToVarian
                // map.
                anyToVariant[assocType] = reg->GetFactory();
                m_anyToVariantRegs.erase( m_anyToVariantRegs.begin() + i );
            }
        }

        // Then try again
        it = anyToVariant.find(type);
        if ( it != anyToVariant.end() )
            return it->second;

        // Finally, attempt to find a compatible type
        for ( it = anyToVariant.begin(); it != anyToVariant.end(); ++it )
        {
            if ( type->IsSameType(it->first) )
            {
                wxVariantDataFactory f = it->second;
                anyToVariant[type] = f;
                return f;
            }
        }

        // Nothing found
        return NULL;
    }

private:
    wxAnyTypeToVariantDataFactoryMap        m_anyToVariant;
    std::vector<wxAnyToVariantRegistration*>   m_anyToVariantRegs;
};

static std::unique_ptr<wxAnyValueTypeGlobals>& GetAnyValueTypeGlobals()
{
    static std::unique_ptr<wxAnyValueTypeGlobals> s_wxAnyValueTypeGlobals;
    if ( !s_wxAnyValueTypeGlobals )
    {
        // Notice that it is _not_ sufficient to just initialize the static
        // object like this because it can be used after it was reset by
        // wxAnyValueTypeGlobalsManager if the library is shut down and then
        // initialized again.
        s_wxAnyValueTypeGlobals.reset(new wxAnyValueTypeGlobals());
    }

    return s_wxAnyValueTypeGlobals;
}

WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplVariantData)

void wxPreRegisterAnyToVariant(wxAnyToVariantRegistration* reg)
{
    GetAnyValueTypeGlobals()->PreRegisterAnyToVariant(reg);
}

bool wxConvertAnyToVariant(const wxAny& any, wxVariant* variant)
{
    if ( any.IsNull() )
    {
        variant->MakeNull();
        return true;
    }

    // (signed) integer is a special case, because there is only one type
    // in wxAny, and two ("long" and "longlong") in wxVariant. For better
    // backwards compatibility, convert all values that fit in "long",
    // and others to "longlong".
    if ( wxANY_CHECK_TYPE(any, signed int) )
    {
#if defined(wxLongLong_t) && wxUSE_LONGLONG
        wxLongLong_t ll = 0;
        if ( any.GetAs(&ll) )
        {
            // NB: Do not use LONG_MAX here. Explicitly using 32-bit
            //     integer constraint yields more consistent behaviour across
            //     builds.
            if ( ll > wxINT32_MAX || ll < wxINT32_MIN )
                *variant = wxLongLong(ll);
            else
                *variant = static_cast<long>(ll);
        }
        else
        {
            return false;
        }
#else
        long l;
        if ( any.GetAs(&l) )
            *variant = l;
        else
            return false;
#endif
        return true;
    }

    // Find matching factory function
    wxVariantDataFactory f =
        GetAnyValueTypeGlobals()->FindVariantDataFactory(any.GetType());

    wxVariantData* data = NULL;

    if ( f )
    {
        data = f(any);
    }
    else
    {
        // Check if wxAny wrapped wxVariantData*
        if ( !any.GetAs(&data) )
        {
            // Ok, one last chance: while unlikely, it is possible that the
            // wxAny actually contains wxVariant.
            if ( wxANY_CHECK_TYPE(any, wxVariant) )
                *variant = any.As<wxVariant>();
            return false;
        }

        // Wrapper's GetValue() does not increase reference
        // count, se have to do it before the data gets passed
        // to a new variant.
        data->IncRef();
    }

    variant->SetData(data);
    return true;
}

//-------------------------------------------------------------------------
// Dynamic conversion member functions
//-------------------------------------------------------------------------

//
// Define integer minimum and maximum as helpers
#ifdef wxLongLong_t
    #define UseIntMin  (wxINT64_MIN)
    #define UseIntMax  (wxINT64_MAX)
    #define UseUintMax (wxUINT64_MAX)
#else
    #define UseIntMin  (LONG_MIN)
    #define UseIntMax  (LONG_MAX)
    #define UseUintMax (ULONG_MAX)
#endif

namespace
{

const double UseIntMinF = static_cast<double>(UseIntMin);
const double UseIntMaxF = static_cast<double>(UseIntMax);
const double UseUintMaxF = static_cast<double>(UseUintMax);

} // anonymous namespace

bool wxAnyValueTypeImplInt::ConvertValue(const wxAnyValueBuffer& src,
                                         wxAnyValueType* dstType,
                                         wxAnyValueBuffer& dst) const
{
    wxAnyBaseIntType value = GetValue(src);
    if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxString) )
    {
#if defined(wxLongLong_t) && wxUSE_LONGLONG
        wxLongLong ll(value);
        wxString s = ll.ToString();
#else
        wxString s = wxString::Format(wxS("%ld"), (long)value);
#endif
        wxAnyValueTypeImpl<wxString>::SetValue(s, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseUintType) )
    {
        if ( value < 0 )
            return false;
        wxAnyBaseUintType ul = (wxAnyBaseUintType) value;
        wxAnyValueTypeImplUint::SetValue(ul, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, double) )
    {
        double value2 = static_cast<double>(value);
        wxAnyValueTypeImplDouble::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, bool) )
    {
        bool value2 = value ? true : false;
        wxAnyValueTypeImpl<bool>::SetValue(value2, dst);
    }
    else
        return false;

    return true;
}

bool wxAnyValueTypeImplUint::ConvertValue(const wxAnyValueBuffer& src,
                                          wxAnyValueType* dstType,
                                          wxAnyValueBuffer& dst) const
{
    wxAnyBaseUintType value = GetValue(src);
    if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxString) )
    {
#if defined(wxLongLong_t) && wxUSE_LONGLONG
        wxULongLong ull(value);
        wxString s = ull.ToString();
#else
        wxString s = wxString::Format(wxS("%lu"), (long)value);
#endif
        wxAnyValueTypeImpl<wxString>::SetValue(s, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseIntType) )
    {
        if ( value > UseIntMax )
            return false;
        wxAnyBaseIntType l = (wxAnyBaseIntType) value;
        wxAnyValueTypeImplInt::SetValue(l, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, double) )
    {
        double value2 = static_cast<double>(value);
        wxAnyValueTypeImplDouble::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, bool) )
    {
        bool value2 = value ? true : false;
        wxAnyValueTypeImpl<bool>::SetValue(value2, dst);
    }
    else
        return false;

    return true;
}

// Convert wxString to destination wxAny value type
bool wxAnyConvertString(const wxString& value,
                        wxAnyValueType* dstType,
                        wxAnyValueBuffer& dst)
{
    if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxString) )
    {
        wxAnyValueTypeImpl<wxString>::SetValue(value, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseIntType) )
    {
        wxAnyBaseIntType value2;
#ifdef wxLongLong_t
        if ( !value.ToLongLong(&value2) )
#else
        if ( !value.ToLong(&value2) )
#endif
            return false;
        wxAnyValueTypeImplInt::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseUintType) )
    {
        wxAnyBaseUintType value2;
#ifdef wxLongLong_t
        if ( !value.ToULongLong(&value2) )
#else
        if ( !value.ToULong(&value2) )
#endif
            return false;
        wxAnyValueTypeImplUint::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, double) )
    {
        double value2;
        if ( !value.ToCDouble(&value2) )
            return false;
        wxAnyValueTypeImplDouble::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, bool) )
    {
        bool value2;
        wxString s(value);
        s.MakeLower();
        if ( s == wxS("true") ||
             s == wxS("yes") ||
             s == wxS('1') )
            value2 = true;
        else if ( s == wxS("false") ||
                  s == wxS("no") ||
                  s == wxS('0') )
            value2 = false;
        else
            return false;

        wxAnyValueTypeImpl<bool>::SetValue(value2, dst);
    }
    else
        return false;

    return true;
}

bool wxAnyValueTypeImpl<bool>::ConvertValue(const wxAnyValueBuffer& src,
                                            wxAnyValueType* dstType,
                                            wxAnyValueBuffer& dst) const
{
    bool value = GetValue(src);
    if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseIntType) )
    {
        wxAnyBaseIntType value2 = static_cast<wxAnyBaseIntType>(value);
        wxAnyValueTypeImplInt::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseUintType) )
    {
        wxAnyBaseIntType value2 = static_cast<wxAnyBaseUintType>(value);
        wxAnyValueTypeImplUint::SetValue(value2, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxString) )
    {
        wxString s;
        if ( value )
            s = wxS("true");
        else
            s = wxS("false");
        wxAnyValueTypeImpl<wxString>::SetValue(s, dst);
    }
    else
        return false;

    return true;
}

bool wxAnyValueTypeImplDouble::ConvertValue(const wxAnyValueBuffer& src,
                                            wxAnyValueType* dstType,
                                            wxAnyValueBuffer& dst) const
{
    double value = GetValue(src);
    if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseIntType) )
    {
        if ( value < UseIntMinF || value > UseIntMaxF )
            return false;
        wxAnyBaseUintType ul = static_cast<wxAnyBaseUintType>(value);
        wxAnyValueTypeImplUint::SetValue(ul, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxAnyBaseUintType) )
    {
        if ( value < 0.0 || value > UseUintMaxF )
            return false;
        wxAnyBaseUintType ul = static_cast<wxAnyBaseUintType>(value);
        wxAnyValueTypeImplUint::SetValue(ul, dst);
    }
    else if ( wxANY_VALUE_TYPE_CHECK_TYPE(dstType, wxString) )
    {
        wxString s = wxString::FromCDouble(value, 14);
        wxAnyValueTypeImpl<wxString>::SetValue(s, dst);
    }
    else
        return false;

    return true;
}

WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplInt)
WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplUint)
WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImpl<bool>)
WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplDouble)

WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplwxString)
WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplConstCharPtr)
WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImplConstWchar_tPtr)

WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImpl<wxDateTime>)

//WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImpl<wxObject*>)
//WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImpl<wxArrayString>)

//-------------------------------------------------------------------------
// wxAnyNullValueType implementation
//-------------------------------------------------------------------------

class wxAnyNullValue
{
protected:
    // this field is unused, but can't be private to avoid Clang's
    // "Private field 'm_dummy' is not used" warning
    void*   m_dummy;
};

template <>
class wxAnyValueTypeImpl<wxAnyNullValue> : public wxAnyValueType
{
    WX_DECLARE_ANY_VALUE_TYPE(wxAnyValueTypeImpl<wxAnyNullValue>)
public:
    // Dummy implementations
    virtual void DeleteValue(wxAnyValueBuffer& buf) const override
    {
        wxUnusedVar(buf);
    }

    virtual void CopyBuffer(const wxAnyValueBuffer& src,
                            wxAnyValueBuffer& dst) const override
    {
        wxUnusedVar(src);
        wxUnusedVar(dst);
    }

    virtual bool ConvertValue(const wxAnyValueBuffer& src,
                              wxAnyValueType* dstType,
                              wxAnyValueBuffer& dst) const override
    {
        wxUnusedVar(src);
        wxUnusedVar(dstType);
        wxUnusedVar(dst);
        return false;
    }

private:
};

WX_IMPLEMENT_ANY_VALUE_TYPE(wxAnyValueTypeImpl<wxAnyNullValue>)

wxAnyValueType* wxAnyNullValueType =
    wxAnyValueTypeImpl<wxAnyNullValue>::GetInstance();

WX_DEFINE_LIST(wxAnyList)
