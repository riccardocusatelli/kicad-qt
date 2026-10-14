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
 * @file kiwx_validators.h
 * The validators of the wxWidgets compatibility layer of the Qt build (wxcompat): wxValidator,
 * wxTextValidator, wxGenericValidator.  A window keeps its validator (SetValidator()); the data
 * transfers and the validation of the dialogs call the validators of their controls, as with
 * wxWidgets; the text validators filter the characters typed in their text controls.
 */

#ifndef KIWX_VALIDATORS_H
#define KIWX_VALIDATORS_H

#include <kiqt.h>

#include <memory>

#include <wx/arrstr.h>
#include <wx/object.h>
#include <wx/string.h>
#include <wx/kiwx_events.h>

class QWidget;
class wxTextCtrl;


/**
 * A reference to a window: a QWidget with the part of the API of wxWindow the code of KiCad uses
 * on the windows it keeps as wxWindow* (m_validatorWindow->IsEnabled(),
 * m_colLabelWin->GetBackgroundColour()...).  Converts to QWidget*.
 */
class KIQT_API wxWindowRefCompat
{
public:
    wxWindowRefCompat( QWidget* aWindow = nullptr ) : m_window( aWindow ) {}

    operator QWidget*() const { return m_window; }
    explicit operator bool() const { return m_window != nullptr; }
    const wxWindowRefCompat* operator->() const { return this; }

    bool IsEnabled() const;
    bool IsShown() const;
    void SetFocus() const;
    void Refresh() const;
    wxString GetName() const;
    class wxColour GetBackgroundColour() const;
    class wxColour GetForegroundColour() const;
    int GetId() const;
    void SetToolTip( const wxString& aTip ) const;
    void UnsetToolTip() const { SetToolTip( wxEmptyString ); }

    // The handlers of the events of the window (wxEvtHandler::Bind(), Connect())
    template <typename EVENT, typename CLASS, typename OBJECT>
    void Bind( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ), OBJECT* aObject,
               int aId = wxID_ANY, int aLastId = wxID_ANY ) const
    {
        bindHandler( aType,
                     [aMethod, aObject]( wxEvent& aEvent )
                     {
                         ( static_cast<CLASS*>( aObject )->*aMethod )( static_cast<EVENT&>( aEvent ) );
                     },
                     aObject, aId, aLastId );
    }

    template <typename FUNCTOR>
    void Bind( wxEventType aType, FUNCTOR aFunctor, int aId = wxID_ANY,
               int aLastId = wxID_ANY ) const
    {
        using EVENT = KIWX_DETAIL::EVENT_OF<FUNCTOR>;

        bindHandler( aType,
                     [aFunctor]( wxEvent& aEvent ) mutable
                     {
                         aFunctor( static_cast<EVENT&>( aEvent ) );
                     },
                     nullptr, aId, aLastId );
    }

    template <typename EVENT, typename CLASS, typename OBJECT>
    void Connect( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ), void* aUserData = nullptr,
                  OBJECT* aSink = nullptr ) const
    {
        if( aSink )
            Bind( aType, aMethod, aSink );
    }

    template <typename EVENT, typename CLASS, typename OBJECT>
    bool Unbind( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ), OBJECT* aObject,
                 int aId = wxID_ANY, int aLastId = wxID_ANY ) const
    {
        return m_window && wxEventSinkCompat( m_window )->Remove( aType, aObject );
    }

    template <typename EVENT, typename CLASS, typename OBJECT>
    bool Disconnect( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ),
                     void* aUserData = nullptr, OBJECT* aSink = nullptr ) const
    {
        return m_window && wxEventSinkCompat( m_window )->Remove( aType, aSink );
    }

    bool ProcessEvent( wxEvent& aEvent ) const;

private:
    void bindHandler( wxEventType aType, wxCompatEventSink::HANDLER aHandler, const void* aKey,
                      int aId, int aLastId ) const;

    QWidget* m_window;
};

using wxValidatorWindowCompat = wxWindowRefCompat;


class KIQT_API wxValidator : public wxObject
{
public:
    wxValidator() = default;
    wxValidator( const wxValidator& aOther ) : wxObject( aOther ), m_validatorWindow( aOther.m_validatorWindow ) {}
    ~wxValidator() override = default;

    /// A copy of the validator (each window has its own).
    virtual wxObject* Clone() const { return nullptr; }

    /**
     * The handlers of the events of the window of the validator (wxEvtHandler::Connect() of a
     * validator): the key events of a text control filtered by the validator.
     */
    template <typename EVENT, typename CLASS>
    void Connect( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ) )
    {
        CLASS* self = static_cast<CLASS*>( this );
        m_events->Add( aType,
                       [self, aMethod]( wxEvent& aEvent ) { ( self->*aMethod )( static_cast<EVENT&>( aEvent ) ); },
                       this );
        m_eventTypes.push_back( aType );
    }

    template <typename EVENT, typename CLASS>
    void Disconnect( wxEventType aType, void ( CLASS::*aMethod )( EVENT& ) )
    {
        m_events->Remove( aType, this );
    }

    /// The types of the events of the window the validator handles (connected by SetWindow()).
    const std::vector<wxEventType>& GetEventTypesCompat() const { return m_eventTypes; }
    wxCompatEventSink* GetEventSinkCompat() const { return m_events.get(); }

    /// Check the value of the window (and show an error to the user).
    virtual bool Validate( QWidget* aParent ) { return true; }

    virtual bool TransferToWindow() { return true; }
    virtual bool TransferFromWindow() { return true; }

    void SetWindow( QWidget* aWindow ) { m_validatorWindow = aWindow; }
    QWidget* GetWindow() const { return m_validatorWindow; }

    static void SuppressBellOnError( bool = true ) {}
    static bool IsSilent() { return true; }

    /// The characters typed in the window that the validator accepts (all by default).
    virtual bool IsValidCharCompat( wxUniChar aChar ) const { return true; }

protected:
    void Copy( const wxValidator& aOther ) { m_validatorWindow = aOther.m_validatorWindow; }

    wxValidatorWindowCompat m_validatorWindow;

private:
    // Not copied: the handlers of a copy are connected by its own constructor
    std::shared_ptr<wxCompatEventSink> m_events = std::make_shared<wxCompatEventSink>();
    std::vector<wxEventType>           m_eventTypes;
};

KIQT_API extern const wxValidator wxDefaultValidator;


enum wxTextValidatorStyle
{
    wxFILTER_NONE = 0x0,
    wxFILTER_EMPTY = 0x1,
    wxFILTER_ASCII = 0x2,
    wxFILTER_ALPHA = 0x4,
    wxFILTER_ALPHANUMERIC = 0x8,
    wxFILTER_DIGITS = 0x10,
    wxFILTER_NUMERIC = 0x20,
    wxFILTER_INCLUDE_LIST = 0x40,
    wxFILTER_INCLUDE_CHAR_LIST = 0x80,
    wxFILTER_EXCLUDE_LIST = 0x100,
    wxFILTER_EXCLUDE_CHAR_LIST = 0x200,
    wxFILTER_XDIGITS = 0x400,
    wxFILTER_SPACE = 0x800
};


class KIQT_API wxTextValidator : public wxValidator
{
public:
    wxTextValidator( long aStyle = wxFILTER_NONE, wxString* aValue = nullptr );
    wxTextValidator( const wxTextValidator& aOther ) = default;

    wxObject* Clone() const override { return new wxTextValidator( *this ); }

    bool Validate( QWidget* aParent ) override;
    bool TransferToWindow() override;
    bool TransferFromWindow() override;

    long GetStyle() const { return m_validatorStyle; }
    void SetStyle( long aStyle ) { m_validatorStyle = aStyle; }
    bool HasFlag( wxTextValidatorStyle aStyle ) const { return m_validatorStyle & aStyle; }

    void SetCharIncludes( const wxString& aChars ) { m_charIncludes = aChars; }
    void SetCharExcludes( const wxString& aChars ) { m_charExcludes = aChars; }
    void AddCharIncludes( const wxString& aChars ) { m_charIncludes += aChars; }
    void AddCharExcludes( const wxString& aChars ) { m_charExcludes += aChars; }
    const wxString& GetCharIncludes() const { return m_charIncludes; }
    const wxString& GetCharExcludes() const { return m_charExcludes; }

    void SetIncludes( const wxArrayString& aIncludes ) { m_includes = aIncludes; }
    void SetExcludes( const wxArrayString& aExcludes ) { m_excludes = aExcludes; }
    const wxArrayString& GetIncludes() const { return m_includes; }
    const wxArrayString& GetExcludes() const { return m_excludes; }

    /// The error message if @a aValue is not valid, or an empty string.
    virtual wxString IsValid( const wxString& aValue ) const;

    bool IsValidCharCompat( wxUniChar aChar ) const override;

    /// The text control of the validator (nullptr if its window isn't one).
    wxTextCtrl* GetTextEntry();

protected:
    bool IsCharIncluded( wxUniChar aChar ) const;
    bool IsCharExcluded( wxUniChar aChar ) const;

    wxString*     m_stringValue;
    long          m_validatorStyle;
    wxString      m_charIncludes;
    wxString      m_charExcludes;
    wxArrayString m_includes;
    wxArrayString m_excludes;
};


/// The validator of the value of a control (a bool, a string, an int).
class KIQT_API wxGenericValidator : public wxValidator
{
public:
    explicit wxGenericValidator( bool* aValue );
    explicit wxGenericValidator( wxString* aValue );
    explicit wxGenericValidator( int* aValue );

    wxObject* Clone() const override { return new wxGenericValidator( *this ); }

    bool TransferToWindow() override;
    bool TransferFromWindow() override;

private:
    bool*     m_bool = nullptr;
    wxString* m_string = nullptr;
    int*      m_int = nullptr;
};


/// The validators of the windows (SetValidator(), GetValidator()).
KIQT_API void wxSetValidatorCompat( QWidget* aWindow, const wxValidator& aValidator );
KIQT_API wxValidator* wxGetValidatorCompat( const QWidget* aWindow );

#endif // KIWX_VALIDATORS_H
