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
 * @file sch_design_block_pane_qt.cpp
 * SCH_DESIGN_BLOCK_PANE with Qt (eeschema/widgets/sch_design_block_pane_wx.cpp with wxWidgets):
 * the chooser of the design blocks, with the preview of their schematics, and the options of the
 * placement.  What the pane does is in eeschema/widgets/sch_design_block_pane.cpp.
 */

#include <widgets/sch_design_block_pane.h>

#include <QCheckBox>
#include <QVBoxLayout>

#include <kiface_base.h>
#include <sch_actions.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/sch_design_block_control.h>
#include <widgets/panel_design_block_chooser.h>
#include <widgets/sch_design_block_preview_widget.h>

#include <qt_strings.h>

// Do not make these static wxStrings; they need to respond to language changes
#define REPEATED_PLACEMENT _( "Place repeated copies" )
#define PLACE_AS_SHEET     _( "Place as sheet" )
#define PLACE_AS_GROUP     _( "Place as group" )
#define KEEP_ANNOTATIONS   _( "Keep annotations" )


SCH_DESIGN_BLOCK_PANE::SCH_DESIGN_BLOCK_PANE( SCH_EDIT_FRAME* aParent, const LIB_ID* aPreselect,
                                              std::vector<LIB_ID>& aHistoryList ) :
        DESIGN_BLOCK_PANE( aParent, aPreselect, aHistoryList ),
        m_repeatedPlacement( nullptr ),
        m_placeAsGroup( nullptr ),
        m_placeAsSheet( nullptr ),
        m_keepAnnotations( nullptr )
{
    QVBoxLayout* sizer = new QVBoxLayout( this );
    sizer->setContentsMargins( 0, 0, 0, 0 );

    m_chooserPanel = new PANEL_DESIGN_BLOCK_CHOOSER( aParent, this, aHistoryList,
                                                     [aParent]()
                                                     {
                                                         aParent->GetToolManager()->RunAction(
                                                                 SCH_ACTIONS::placeDesignBlock );
                                                     },
                                                     aParent->GetToolManager()->GetTool<SCH_DESIGN_BLOCK_CONTROL>() );

    // Use the same draw engine type as the one used in parent frame m_frame
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = m_frame->GetCanvas()->GetBackend();
    m_chooserPanel->SetPreviewWidget( new SCH_DESIGN_BLOCK_PREVIEW_WIDGET( m_chooserPanel->GetDetailsPanel(),
                                                                           canvasType, true ) );
    sizer->addWidget( m_chooserPanel, 1 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    setObjectName( QStringLiteral( "Design Blocks" ) );

    QVBoxLayout* cbSizer = new QVBoxLayout();
    cbSizer->setContentsMargins( 2, 2, 0, 2 );
    cbSizer->setSpacing( 2 );

    m_repeatedPlacement = new QCheckBox( this );
    m_placeAsGroup = new QCheckBox( this );
    m_placeAsSheet = new QCheckBox( this );
    m_keepAnnotations = new QCheckBox( this );
    setLabelsAndTooltips();
    UpdateCheckboxes();

    // Set all checkbox handlers to the same function
    for( QCheckBox* checkBox : { m_repeatedPlacement, m_placeAsGroup, m_placeAsSheet, m_keepAnnotations } )
    {
        QObject::connect( checkBox, &QCheckBox::toggled, this, [this]() { OnCheckBox(); } );
        cbSizer->addWidget( checkBox );
    }

    sizer->addLayout( cbSizer );

    m_chooserPanel->FinishSetup();
}


void SCH_DESIGN_BLOCK_PANE::setLabelsAndTooltips()
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

    if( m_placeAsSheet )
    {
        m_placeAsSheet->setText( ToQString( PLACE_AS_SHEET ) );
        m_placeAsSheet->setToolTip( ToQString( _( "Place the design block as a new sheet." ) ) );
    }

    if( m_keepAnnotations )
    {
        m_keepAnnotations->setText( ToQString( KEEP_ANNOTATIONS ) );
        m_keepAnnotations->setToolTip( ToQString( _( "Preserve reference designators in the source "
                                                     "schematic. Otherwise, clear then reannotate "
                                                     "according to settings." ) ) );
    }
}


void SCH_DESIGN_BLOCK_PANE::OnCheckBox()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_DesignBlockChooserPanel.repeated_placement = m_repeatedPlacement->isChecked();
        cfg->m_DesignBlockChooserPanel.place_as_group = m_placeAsGroup->isChecked();
        cfg->m_DesignBlockChooserPanel.place_as_sheet = m_placeAsSheet->isChecked();
        cfg->m_DesignBlockChooserPanel.keep_annotations = m_keepAnnotations->isChecked();
    }
}


void SCH_DESIGN_BLOCK_PANE::UpdateCheckboxes()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        QSignalBlocker blockRepeated( m_repeatedPlacement );
        QSignalBlocker blockGroup( m_placeAsGroup );
        QSignalBlocker blockSheet( m_placeAsSheet );
        QSignalBlocker blockAnnotations( m_keepAnnotations );

        m_repeatedPlacement->setChecked( cfg->m_DesignBlockChooserPanel.repeated_placement );
        m_placeAsGroup->setChecked( cfg->m_DesignBlockChooserPanel.place_as_group );
        m_placeAsSheet->setChecked( cfg->m_DesignBlockChooserPanel.place_as_sheet );
        m_keepAnnotations->setChecked( cfg->m_DesignBlockChooserPanel.keep_annotations );
    }
}
