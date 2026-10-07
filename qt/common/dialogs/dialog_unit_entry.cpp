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

/**
 * @file dialog_unit_entry.cpp
 * The dialogs to enter a value (or a point) with units, of the Qt build.
 */

#include <dialogs/dialog_unit_entry.h>

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <eda_draw_frame.h>
#include <string_utils.h>
#include <widgets/unit_binder.h>

#include <qt_strings.h>


/// The buttons OK and Cancel at the bottom right of a dialog
static QDialogButtonBox* standardButtons( QWidget* aParent )
{
    return new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, aParent );
}


WX_UNIT_ENTRY_DIALOG::WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                            const wxString& aLabel, long long int aDefaultValue ) :
        DIALOG_SHIM( aParent, aCaption )
{
    // The layout of dialog_unit_entry_base.fbp
    QVBoxLayout* bSizerMain = new QVBoxLayout( this );
    QHBoxLayout* bSizerContent = new QHBoxLayout();

    m_label = new QLabel( ToQString( aLabel ), this );
    m_textCtrl = new QLineEdit( this );
    m_unit_label = new QLabel( ToQString( _( "unit" ) ), this );

    bSizerContent->addWidget( m_label );
    bSizerContent->addWidget( m_textCtrl, 1 );
    bSizerContent->addWidget( m_unit_label );
    bSizerMain->addLayout( bSizerContent, 1 );

    QHBoxLayout* bSizerButtons = new QHBoxLayout();
    bSizerButtons->addStretch( 1 );
    bSizerButtons->addWidget( standardButtons( this ) );
    bSizerMain->addLayout( bSizerButtons );

    m_unit_binder = std::make_unique<UNIT_BINDER>( aParent, m_label, m_textCtrl, m_unit_label,
                                                   true );
    m_unit_binder->SetValue( aDefaultValue );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( aCaption + aLabel );

    SetInitialFocus( m_textCtrl );
    SetupStandardButtons();

    finishDialogSettings();
}


WX_UNIT_ENTRY_DIALOG::~WX_UNIT_ENTRY_DIALOG() = default;


int WX_UNIT_ENTRY_DIALOG::GetValue()
{
    return m_unit_binder->GetIntValue();
}


WX_PT_ENTRY_DIALOG::WX_PT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                        const wxString& aLabelX, const wxString& aLabelY,
                                        const VECTOR2I& aDefaultValue, bool aShowResetButt ) :
        DIALOG_SHIM( aParent, aCaption )
{
    // The layout of dialog_unit_entry_base.fbp
    QVBoxLayout* bSizerMain = new QVBoxLayout( this );
    QGridLayout* fgSizer = new QGridLayout();

    m_labelX = new QLabel( ToQString( aLabelX ), this );
    m_textCtrlX = new QLineEdit( this );
    m_unitsX = new QLabel( ToQString( _( "unit" ) ), this );
    m_labelY = new QLabel( ToQString( aLabelY ), this );
    m_textCtrlY = new QLineEdit( this );
    m_unitsY = new QLabel( ToQString( _( "unit" ) ), this );

    fgSizer->addWidget( m_labelX,    0, 0 );
    fgSizer->addWidget( m_textCtrlX, 0, 1 );
    fgSizer->addWidget( m_unitsX,    0, 2 );
    fgSizer->addWidget( m_labelY,    1, 0 );
    fgSizer->addWidget( m_textCtrlY, 1, 1 );
    fgSizer->addWidget( m_unitsY,    1, 2 );
    fgSizer->setColumnStretch( 1, 1 );
    bSizerMain->addLayout( fgSizer, 1 );

    QHBoxLayout* bSizerButtons = new QHBoxLayout();

    m_ButtonReset = new QPushButton( ToQString( _( "Reset" ) ), this );

    // The space of the button is kept when it is hidden
    QSizePolicy policy = m_ButtonReset->sizePolicy();
    policy.setRetainSizeWhenHidden( true );
    m_ButtonReset->setSizePolicy( policy );

    bSizerButtons->addWidget( m_ButtonReset );
    bSizerButtons->addStretch( 1 );
    bSizerButtons->addWidget( standardButtons( this ) );
    bSizerMain->addLayout( bSizerButtons );

    QObject::connect( m_ButtonReset, &QPushButton::clicked, this,
                      [this]()
                      {
                          ResetValues();
                      } );

    m_unit_binder_x = std::make_unique<UNIT_BINDER>( aParent, m_labelX, m_textCtrlX, m_unitsX, true );
    m_unit_binder_y = std::make_unique<UNIT_BINDER>( aParent, m_labelY, m_textCtrlY, m_unitsY, true );

    m_ButtonReset->setVisible( aShowResetButt );

    m_unit_binder_x->SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_unit_binder_y->SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    OptOut( m_textCtrlX );
    OptOut( m_textCtrlY );

    m_unit_binder_x->SetValue( aDefaultValue.x );
    m_unit_binder_y->SetValue( aDefaultValue.y );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( aCaption + aLabelX + aLabelY );

    SetInitialFocus( m_textCtrlX );
    SetupStandardButtons();

    finishDialogSettings();
}


WX_PT_ENTRY_DIALOG::~WX_PT_ENTRY_DIALOG() = default;


VECTOR2I WX_PT_ENTRY_DIALOG::GetValue()
{
    return VECTOR2I( m_unit_binder_x->GetIntValue(), m_unit_binder_y->GetIntValue() );
}


void WX_PT_ENTRY_DIALOG::ResetValues()
{
    m_unit_binder_x->SetValue( 0 );
    m_unit_binder_y->SetValue( 0 );
}
