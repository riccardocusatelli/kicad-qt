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
 * @file kiwx_combo.h
 * The combo controls of the wxWidgets compatibility layer of the Qt build (wxcompat): a text
 * control with a button (wxComboCtrl) that shows a popup (wxComboPopup) or does an action of its
 * own (OnButtonClick()).
 */

#ifndef KIWX_COMBO_H
#define KIWX_COMBO_H

#include <wx/kiwx_gui.h>
#include <wx/kiwx_textentry.h>

class QFrame;
class QToolButton;
class wxComboCtrl;

// The flags of Customize()
#define wxCC_BUTTON_OUTSIDE_BORDER      0x0001
#define wxCC_POPUP_ON_MOUSE_UP          0x0002
#define wxCC_NO_TEXT_AUTO_SELECT        0x0004
#define wxCC_BUTTON_STAYS_DOWN          0x0008
#define wxCC_FULL_BUTTON                0x0010
#define wxCC_BUTTON_COVERS_BORDER       0x0020
#define wxCC_IFLAG_CREATED              0x0100
#define wxCC_IFLAG_BUTTON_OUTSIDE       0x0200
#define wxCC_IFLAG_LEFT_MARGIN_SET      0x0400
#define wxCC_IFLAG_PARENT_TAB_TRAVERSAL 0x0800
#define wxCC_IFLAG_USE_ALT_POPUP        0x1000
#define wxCC_IFLAG_DISABLE_POPUP_ANIM   0x2000
#define wxCC_IFLAG_HAS_NONSTANDARD_BUTTON 0x4000


/**
 * The popup of a wxComboCtrl: its control (GetControl()), created by Create() in the popup
 * window, gives and shows the value of the combo control as a string.
 */
class KIQT_API wxComboPopup
{
public:
    wxComboPopup() : m_combo( nullptr ), m_created( false ) {}
    virtual ~wxComboPopup() = default;

    virtual void Init() {}

    /// Create the control of the popup, child of @a aParent.
    virtual bool Create( wxWindow* aParent ) = 0;
    virtual void DestroyPopup() {}

    virtual wxWindow* GetControl() = 0;

    virtual void SetStringValue( const wxString& aValue ) {}
    virtual wxString GetStringValue() const = 0;

    virtual void OnPopup() {}
    virtual void OnDismiss() {}
    virtual void OnComboDoubleClick() {}
    virtual void OnComboKeyEvent( wxKeyEvent& aEvent ) { aEvent.Skip(); }
    virtual void OnComboCharEvent( wxKeyEvent& aEvent ) { aEvent.Skip(); }

    /// The size of the popup, from the width of the combo control and the maximum height.
    virtual wxSize GetAdjustedSize( int aMinWidth, int aPrefHeight, int aMaxHeight );

    virtual bool LazyCreate() { return false; }
    virtual bool FindItem( const wxString& aItem, wxString* aTrueItem = nullptr ) { return true; }

    void Dismiss();
    bool IsCreated() const { return m_created; }

    wxComboCtrl* GetComboCtrl() const { return m_combo; }
    void InitBase( wxComboCtrl* aCombo ) { m_combo = aCombo; }

protected:
    friend class wxComboCtrl;

    wxComboCtrl* m_combo;
    bool         m_created;
};


class KIQT_API wxComboCtrl : public wxWindowCompat<QWidget>, public wxLineEditTextEntryCompat
{
public:
    wxComboCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                 const wxString& aValue = wxEmptyString, const wxPoint& aPos = wxDefaultPosition,
                 const wxSize& aSize = wxDefaultSize, long aStyle = 0,
                 const wxValidator& aValidator = wxDefaultValidator,
                 const wxString& aName = wxEmptyString );
    ~wxComboCtrl() override;

    /// The popup of the control (the control takes it).
    void SetPopupControl( wxComboPopup* aPopup );
    wxComboPopup* GetPopupControl() const { return m_popup; }

    void Popup();
    void Dismiss();
    void ShowPopup() { Popup(); }
    void HidePopup( bool aGenerateEvent = false ) { Dismiss(); }
    bool IsPopupShown() const;
    wxWindow* GetPopupWindow() const;

    void SetPopupMinWidth( int aWidth ) { m_popupMinWidth = aWidth; }
    void SetPopupMaxHeight( int aHeight ) { m_popupMaxHeight = aHeight; }
    void SetPopupExtents( int, int ) {}
    void UseAltPopupWindow( bool = true ) {}
    void Customize( int aFlags ) {}
    void SetButtonPosition( int aWidth = -1, int aHeight = -1, int aSide = wxRIGHT, int aSpacingX = 0 );
    void SetButtonBitmaps( const wxBitmapBundle& aNormal, bool aPushButtonBg = false,
                           const wxBitmapBundle& aPressed = wxBitmapBundle(),
                           const wxBitmapBundle& aHover = wxBitmapBundle(),
                           const wxBitmapBundle& aDisabled = wxBitmapBundle() );
    QToolButton* GetButton() const { return m_button; }

    wxTextCtrl* GetTextCtrl() const { return m_text; }

    // The text: the API of wxTextEntry on the text control of the combo control
    void SetText( const wxString& aValue ) { ChangeValue( aValue ); }
    void SetValueByUser( const wxString& aValue ) { SetValue( aValue ); }

    /// The button was clicked: shows the popup (or an action of the derived class).
    virtual void OnButtonClick();

protected:
    virtual void DoSetPopupControl( wxComboPopup* aPopup );

    QLineEdit* lineEditCompat() const override;

    wxComboPopup* m_popup;

private:
    wxTextCtrl*  m_text;
    QToolButton* m_button;
    QFrame*      m_popupWindow;
    int          m_popupMinWidth;
    int          m_popupMaxHeight;
};

// The flags of the drawing of the items of a wxOwnerDrawnComboBox
#define wxODCB_DCLICK_CYCLES        0x0400
#define wxODCB_STD_CONTROL_PAINT    0x1000
#define wxODCB_PAINTING_CONTROL     0x0001
#define wxODCB_PAINTING_SELECTED    0x0002


/**
 * A combo box drawing its items (wxOwnerDrawnComboBox): the items of its list are drawn by
 * OnDrawItem(), with the heights of OnMeasureItem().  The control shows the text of the item.
 */
class KIQT_API wxOwnerDrawnComboBox : public wxComboBox
{
public:
    wxOwnerDrawnComboBox( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                          const wxString& aValue = wxEmptyString,
                          const wxPoint& aPos = wxDefaultPosition,
                          const wxSize& aSize = wxDefaultSize,
                          const wxArrayString& aChoices = wxArrayString(), long aStyle = 0 );
    wxOwnerDrawnComboBox( wxWidgetArgCompat aParent, int aId, const wxString& aValue,
                          const wxPoint& aPos, const wxSize& aSize, int aCount,
                          const wxString aChoices[], long aStyle = 0 );

    bool IsPopupShown() const;

    /// Draw an item (wxODCB_PAINTING_SELECTED: a selected item of the list).
    virtual void OnDrawItem( wxDC& aDc, const wxRect& aRect, int aItem, int aFlags ) const {}

    /// The height of an item (-1: the default one).
    virtual wxCoord OnMeasureItem( size_t aItem ) const { return -1; }
    virtual wxCoord OnMeasureItemWidth( size_t aItem ) const { return -1; }

    virtual void OnDrawBackground( wxDC& aDc, const wxRect& aRect, int aItem, int aFlags ) const {}

    void showPopup() override;
    void hidePopup() override;

private:
    void initCompat();

    bool m_popupShown = false;
};

#endif // KIWX_COMBO_H
