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
 * @file pcb_design_block_pane_qt.cpp
 * PCB_DESIGN_BLOCK_PANE with Qt (pcbnew/widgets/pcb_design_block_pane.cpp with wxWidgets): the
 * chooser of the design blocks, with the preview of their boards, and the options of the
 * placement.
 */

#include <widgets/pcb_design_block_pane.h>

#include <QCheckBox>
#include <QVBoxLayout>

#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_design_block_control.h>
#include <widgets/panel_design_block_chooser.h>
#include <widgets/pcb_design_block_preview_widget.h>

#include <qt_strings.h>

// Do not make these static wxStrings; they need to respond to language changes
#define REPEATED_PLACEMENT _( "Place repeated copies" )
#define PLACE_AS_GROUP     _( "Place as group" )
#define KEEP_ANNOTATIONS   _( "Keep annotations" )


PCB_DESIGN_BLOCK_PANE::PCB_DESIGN_BLOCK_PANE( PCB_EDIT_FRAME* aParent, const LIB_ID* aPreselect,
                                              std::vector<LIB_ID>& aHistoryList ) :
        DESIGN_BLOCK_PANE( aParent, aPreselect, aHistoryList ),
        m_repeatedPlacement( nullptr ),
        m_placeAsGroup( nullptr ),
        m_keepAnnotations( nullptr )
{
    QVBoxLayout* sizer = new QVBoxLayout( this );
    sizer->setContentsMargins( 0, 0, 0, 0 );

    m_chooserPanel = new PANEL_DESIGN_BLOCK_CHOOSER( aParent, this, aHistoryList,
                                                     [aParent]()
                                                     {
                                                         aParent->GetToolManager()->RunAction(
                                                                 PCB_ACTIONS::placeDesignBlock );
                                                     },
                                                     aParent->GetToolManager()->GetTool<PCB_DESIGN_BLOCK_CONTROL>() );

    m_chooserPanel->SetPreviewWidget( new PCB_DESIGN_BLOCK_PREVIEW_WIDGET( m_chooserPanel->GetDetailsPanel(),
                                                                           aParent ) );
    sizer->addWidget( m_chooserPanel, 1 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    setObjectName( QStringLiteral( "Design Blocks" ) );

    QVBoxLayout* cbSizer = new QVBoxLayout();
    cbSizer->setContentsMargins( 2, 2, 0, 2 );
    cbSizer->setSpacing( 2 );

    m_repeatedPlacement = new QCheckBox( this );
    m_placeAsGroup = new QCheckBox( this );
    m_keepAnnotations = new QCheckBox( this );
    setLabelsAndTooltips();
    UpdateCheckboxes();

    // Set all checkbox handlers to the same function
    for( QCheckBox* checkBox : { m_repeatedPlacement, m_placeAsGroup, m_keepAnnotations } )
    {
        QObject::connect( checkBox, &QCheckBox::toggled, this, [this]() { OnCheckBox(); } );
        cbSizer->addWidget( checkBox );
    }

    sizer->addLayout( cbSizer );

    m_chooserPanel->FinishSetup();
}


void PCB_DESIGN_BLOCK_PANE::setLabelsAndTooltips()
{
    if( m_repeatedPlacement )
    {
        m_repeatedPlacement->setText( ToQString( REPEATED_PLACEMENT ) );
        m_repeatedPlacement->setToolTip( ToQString( _( "Place copies of the design block on subsequent "
                                                       "clicks." ) ) );
    }

    if( m_placeAsGroup )
    {
        m_placeAsGroup->setText( ToQString( PLACE_AS_GROUP ) );
        m_placeAsGroup->setToolTip( ToQString( _( "Place the design block as a group." ) ) );
    }

    if( m_keepAnnotations )
    {
        m_keepAnnotations->setText( ToQString( KEEP_ANNOTATIONS ) );
        m_keepAnnotations->setToolTip( ToQString( _( "Preserve reference designators in the source "
                                                     "layout. Otherwise, clear them." ) ) );
    }
}


void PCB_DESIGN_BLOCK_PANE::OnCheckBox()
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_DesignBlockChooserPanel.repeated_placement = m_repeatedPlacement->isChecked();
        cfg->m_DesignBlockChooserPanel.place_as_group = m_placeAsGroup->isChecked();
        cfg->m_DesignBlockChooserPanel.keep_annotations = m_keepAnnotations->isChecked();
    }
}


void PCB_DESIGN_BLOCK_PANE::UpdateCheckboxes()
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        QSignalBlocker blockRepeated( m_repeatedPlacement );
        QSignalBlocker blockGroup( m_placeAsGroup );
        QSignalBlocker blockAnnotations( m_keepAnnotations );

        m_repeatedPlacement->setChecked( cfg->m_DesignBlockChooserPanel.repeated_placement );
        m_placeAsGroup->setChecked( cfg->m_DesignBlockChooserPanel.place_as_group );
        m_keepAnnotations->setChecked( cfg->m_DesignBlockChooserPanel.keep_annotations );
    }
}
