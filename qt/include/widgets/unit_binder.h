/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file unit_binder.h
 * UNIT_BINDER of the Qt build: the interface of include/widgets/unit_binder.h for the widgets
 * of Qt (a QLabel for the label and the units, a QLineEdit, an editable QComboBox or a QLabel
 * for the value).
 */

#ifndef __UNIT_BINDER_H_
#define __UNIT_BINDER_H_

#include <span>

#include <base_units.h>
#include <units_provider.h>
#include <wx/kiwx_window.h>
#include <libeval/numeric_evaluator.h>

class QLabel;
class QObject;
class QWidget;
class DIALOG_SHIM;
class EDA_BASE_FRAME;
class EDA_DRAW_FRAME;


class UNIT_BINDER
{
public:

    /**
     * @param aParent is the parent EDA_DRAW_FRAME (the units of the values).
     * @param aLabel is the static text used to label the text input widget (note: the label
     *               text, trimmed of its colon, will also be used in error messages)
     * @param aValueCtrl is the control used to edit or display the given value (QLineEdit,
     *                   QComboBox or QLabel).
     * @param aUnitLabel is the units label displayed after the text input widget
     * @param aAllowEval indicates \a aTextInput will perform math evaluation
     * @param aBindFocusEvent indicates the error message of a validation is shown when the
     *                        value control gets the focus back
     */
    UNIT_BINDER( EDA_DRAW_FRAME* aParent,
                 QLabel* aLabel, QWidget* aValueCtrl, QLabel* aUnitLabel,
                 bool aAllowEval = true, bool aBindFocusEvent = true );

    UNIT_BINDER( UNITS_PROVIDER* aUnitsProvider, wxWidgetArgCompat aEventSource,
                 QLabel* aLabel, QWidget* aValueCtrl, QLabel* aUnitLabel,
                 bool aAllowEval = true, bool aBindFocusEvent = true );

    virtual ~UNIT_BINDER();

    UNIT_BINDER( const UNIT_BINDER& ) = delete;
    UNIT_BINDER& operator=( const UNIT_BINDER& ) = delete;

    /**
     * Normally not needed (as the UNIT_BINDER inherits from the parent frame), but can be
     * used to set to DEGREES for angular controls.
     */
    virtual void SetUnits( EDA_UNITS aUnits );

    bool UnitsInvariant() const
    {
        return m_units == EDA_UNITS::UNSCALED || m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT;
    }

    virtual void SetNegativeZero() { m_negativeZero = true; }

    /**
     * Normally not needed, but can be used to set the precision when using
     * internal units that are floats (not integers) like DEGREES or PERCENT.
     * Not used for integer values in IU
     * @param aLength is the number of digits for mantissa (0 = no truncation)
     * must be <= 6
     */
    virtual void SetPrecision( int aLength );

    /**
     * Used to override the datatype of the displayed property (default is DISTANCE)
     * @param aDataType is the datatype to use for the units text display
     */
    void SetDataType( EDA_DATA_TYPE aDataType );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion.
     */
    virtual void SetValue( long long int aValue );

    void SetValue( const wxString& aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion.
     *
     * The value will be truncated according to the precision set by SetPrecision() (if not <= 0).
     */
    virtual void SetDoubleValue( double aValue );

    virtual void SetAngleValue( const EDA_ANGLE& aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion
     * WITHOUT trigger an update event.
     */
    virtual void ChangeValue( int aValue );

    void ChangeValue( const wxString& aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion
     * WITHOUT triggering an update event.
     *
     * The value will be truncated according to the precision set by SetPrecision() (if not <= 0).
     */
    virtual void ChangeDoubleValue( double aValue );

    virtual void ChangeAngleValue( const EDA_ANGLE& aValue );

    /**
     * Return the current value in Internal Units.
     */
    virtual long long int GetValue() const;

    int GetIntValue() const
    {
        const long long int value = GetValue();
        const long long int clamped =
                std::clamp<long long>( value, static_cast<long long>( std::numeric_limits<int>::lowest() ),
                                       static_cast<long long>( std::numeric_limits<int>::max() ) );

        return static_cast<int>( clamped );
    }

    /**
     * Return the current value in Internal Units.
     *
     * The returned value will be truncated according to the precision set by
     * SetPrecision() (if not <= 0).
     */
    virtual double GetDoubleValue() const;

    virtual EDA_ANGLE GetAngleValue();

    /**
     * Set the list of options for a combobox control.  Doesn't change the value of the control.
     */
    virtual void SetOptionsList( std::span<const long long int> aOptions );

    virtual void SetDoubleOptionsList( std::span<const double> aOptions );

    /**
     * Return true if the control holds the indeterminate value (for instance, if it
     * represents a multiple selection of differing values).
     */
    bool IsIndeterminate() const;

    /**
     * Return true if the control holds no value (ie: empty string, **not** 0).
     */
    bool IsNull() const;

    void SetNull();

    /**
     * Validate the control against the given range, informing the user of any errors found.
     *
     * @param aMin a minimum value for validation
     * @param aMax a maximum value for validation
     * @param aUnits the units of the min/max parameters (use UNSCALED for internal units)
     * @return false on error.
     */
    virtual bool Validate( double aMin, double aMax, EDA_UNITS aUnits = EDA_UNITS::UNSCALED );

    void SetLabel( const wxString& aLabel );

    /**
     * Enable/disable the label, widget and units label.
     */
    void Enable( bool aEnable );

    /**
     * Show/hide the label, widget and units label.
     *
     * @param aShow called for the Show() routine in wx
     * @param aResize if true, the element will be sized to 0 on hide and -1 on show
     */
    void Show( bool aShow, bool aResize = false );

    ORIGIN_TRANSFORMS::COORD_TYPES_T GetCoordType() const
    {
        return m_coordType;
    }

    /**
     * Set the current origin transform mode.
     */
    void SetCoordType( ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType )
    {
        m_coordType = aCoordType;
    }

    /**
     * Force the binder to evaluate the text.
     */
    void RequireEval()
    {
        m_needsEval = true;
    }

protected:
    void onClick();
    void onComboBox();
    void onSetFocus();
    void onKillFocus();
    void delayedFocusHandler();
    void onUnitsChanged( EDA_BASE_FRAME* aProvider );

    /**
     * When m_precision > 0 truncate the value aValue to show only m_precision digits in
     * mantissa.  Used in GetDoubleValue to return a rounded value.  Mainly for units set
     * to DEGREES.
     *
     * @param aValue is the value to modify.
     * @param aValueUsesUserUnits must be set to true if aValue is a user value,
     *                            and set to false if aValue is a internal unit value.
     * @return the "rounded" value.
     */
    double setPrecision( double aValue, bool aValueUsesUserUnits ) const;

    wxString getTextForValue( long long int aValue ) const;
    wxString getTextForDoubleValue( double aValue ) const;

    /// The text of the value control.
    wxString getText() const;

    /// Set the text of the value control; @a aNotify sends the change signals.
    void setText( const wxString& aText, bool aNotify );

protected:
    bool                m_bindFocusEvent;

    /// The bound widgets.
    QLabel*             m_label;
    QWidget*            m_valueCtrl;
    QWidget*            m_eventSource;
    QLabel*             m_unitLabel;      ///< Can be nullptr.

    /// Currently used units.
    const EDA_IU_SCALE* m_iuScale;
    EDA_UNITS           m_units;
    bool                m_negativeZero;   ///< Indicates "-0" should be displayed for 0.
    EDA_DATA_TYPE       m_dataType;
    int                 m_precision;      ///< 0 to 6.

    wxString            m_errorMessage;

    mutable NUMERIC_EVALUATOR   m_eval;
    bool                m_allowEval;
    bool                m_needsEval;

    long                m_selStart;       ///< Selection start and end of the original text.
    long                m_selEnd;

    bool                m_unitsInValue;   ///< Units label should be included in value text.

    /// A reference to an #ORIGIN_TRANSFORMS object.
    ORIGIN_TRANSFORMS&  m_originTransforms;

    /// Type of coordinate for display origin transforms.
    ORIGIN_TRANSFORMS::COORD_TYPES_T m_coordType;

    /// The dialog this binder registered itself with, or nullptr if it isn't owned by a dialog.
    DIALOG_SHIM*        m_dialogShim;

    /// The frame whose units are followed, and the identifier of the listener there.
    EDA_BASE_FRAME*     m_unitsFrame;
    int                 m_unitsListener;

    /// Receives the events of the value control (focus, clicks) for the binder.
    QObject*            m_eventFilter;

public:
    void DetachFromDialogShim() { m_dialogShim = nullptr; }
};


/**
 * Specialization for wxPropertyGrid, where we have no labels and units are displayed in the editor
 */
class PROPERTY_EDITOR_UNIT_BINDER : public UNIT_BINDER
{
public:
    PROPERTY_EDITOR_UNIT_BINDER( EDA_DRAW_FRAME* aParent );

    virtual ~PROPERTY_EDITOR_UNIT_BINDER();

    void SetControl( QWidget* aControl );
};

#endif /* __UNIT_BINDER_H_ */
