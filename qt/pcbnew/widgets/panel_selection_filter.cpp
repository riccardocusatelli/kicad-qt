/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <widgets/panel_selection_filter.h>

#include <QMenu>
#include <QPalette>

#include <core/ki_assert.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

#include <qt_strings.h>
#include <qt_ui.h>


PANEL_SELECTION_FILTER::PANEL_SELECTION_FILTER( QWidget* aParent ) :
        PANEL_SELECTION_FILTER_BASE( aParent ),
        m_frame( dynamic_cast<PCB_BASE_EDIT_FRAME*>( aParent ) ),
        m_tool( nullptr ),
        m_flashSteps( 10 )
{
    QFont font = KIQT::InfoFont( this );

    for( QCheckBox* cb : { m_cbLockedItems, m_cbFootprints, m_cbText, m_cbTracks, m_cbVias,
                           m_cbPads, m_cbGraphics, m_cbZones, m_cbKeepouts, m_cbDimensions,
                           m_cbPoints, m_cbOtherItems, m_cbAllItems } )
    {
        cb->setFont( font );
    }

    SetBorders( true, false, false, false );

    KI_ASSERT( m_frame );
    m_tool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    KI_ASSERT( m_tool );

    PCB_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();
    SetCheckboxesFromFilter( opts );

    for( QCheckBox* cb : { m_cbFootprints, m_cbText, m_cbTracks, m_cbVias, m_cbPads, m_cbGraphics,
                           m_cbZones, m_cbKeepouts, m_cbDimensions, m_cbPoints, m_cbOtherItems } )
    {
        cb->setContextMenuPolicy( Qt::CustomContextMenu );
        QObject::connect( cb, &QWidget::customContextMenuRequested, this,
                          [this, cb]( const QPoint& aPos )
                          {
                              onRightClick( cb, aPos );
                          } );
    }

    m_frame->AddLanguageChangedListener( this,
                                         [this]()
                                         {
                                             OnLanguageChanged();
                                         } );

    setMinimumSize( sizeHint() );

    m_flashTimer.setInterval( 50 );
    QObject::connect( &m_flashTimer, &QTimer::timeout, this,
                      [this]()
                      {
                          onFlashTimer();
                      } );
}


PANEL_SELECTION_FILTER::~PANEL_SELECTION_FILTER()
{
    // Stop any active flashing
    m_flashTimer.stop();
    m_flashCounters.clear();
}


void PANEL_SELECTION_FILTER::SetCheckboxesFromFilter( PCB_SELECTION_FILTER_OPTIONS& aOptions )
{
    m_cbLockedItems->setChecked( aOptions.lockedItems );
    m_cbFootprints->setChecked( aOptions.footprints );
    m_cbText->setChecked( aOptions.text );
    m_cbTracks->setChecked( aOptions.tracks );
    m_cbVias->setChecked( aOptions.vias );
    m_cbPads->setChecked( aOptions.pads );
    m_cbGraphics->setChecked( aOptions.graphics );
    m_cbZones->setChecked( aOptions.zones );
    m_cbKeepouts->setChecked( aOptions.keepouts );
    m_cbDimensions->setChecked( aOptions.dimensions );
    m_cbPoints->setChecked( aOptions.points );
    m_cbOtherItems->setChecked( aOptions.otherItems );

    m_cbAllItems->setChecked( aOptions.All() );
}


void PANEL_SELECTION_FILTER::OnFilterChanged( QWidget* aSource )
{
    if( aSource == m_cbAllItems )
    {
        bool newState = m_cbAllItems->isChecked();

        for( QCheckBox* cb : { m_cbFootprints, m_cbText, m_cbTracks, m_cbVias, m_cbPads,
                               m_cbGraphics, m_cbZones, m_cbKeepouts, m_cbDimensions, m_cbPoints,
                               m_cbOtherItems } )
        {
            cb->setChecked( newState );
        }
    }

    PCB_SELECTION_FILTER_OPTIONS& opts = m_tool->GetFilter();

    // If any of the other checkboxes turned off, turn off the All Items checkbox
    bool allChecked = setFilterFromCheckboxes( opts );
    m_cbAllItems->setChecked( allChecked );
}


bool PANEL_SELECTION_FILTER::setFilterFromCheckboxes( PCB_SELECTION_FILTER_OPTIONS& aOptions )
{
    aOptions.lockedItems = m_cbLockedItems->isChecked();
    aOptions.footprints  = m_cbFootprints->isChecked();
    aOptions.text        = m_cbText->isChecked();
    aOptions.tracks      = m_cbTracks->isChecked();
    aOptions.vias        = m_cbVias->isChecked();
    aOptions.pads        = m_cbPads->isChecked();
    aOptions.graphics    = m_cbGraphics->isChecked();
    aOptions.zones       = m_cbZones->isChecked();
    aOptions.keepouts    = m_cbKeepouts->isChecked();
    aOptions.dimensions  = m_cbDimensions->isChecked();
    aOptions.points      = m_cbPoints->isChecked();
    aOptions.otherItems  = m_cbOtherItems->isChecked();

    return aOptions.All();
}


void PANEL_SELECTION_FILTER::onRightClick( QCheckBox* aCheckBox, const QPoint& aPos )
{
    QMenu    menu( this );
    wxString label;

    label.Printf( _( "Only %s" ), ToWxString( aCheckBox->text() ).Lower() );

    QAction* only = menu.addAction( ToQString( label ) );

    if( menu.exec( aCheckBox->mapToGlobal( aPos ) ) != only )
        return;

    for( QCheckBox* cb : { m_cbAllItems, m_cbFootprints, m_cbText, m_cbTracks, m_cbVias, m_cbPads,
                           m_cbGraphics, m_cbZones, m_cbKeepouts, m_cbDimensions, m_cbPoints,
                           m_cbOtherItems } )
    {
        cb->setChecked( false );
    }

    aCheckBox->setChecked( true );

    OnFilterChanged( nullptr );
}


void PANEL_SELECTION_FILTER::OnLanguageChanged()
{
    m_cbAllItems->setText( ToQString( _( "All items" ) ) );
    m_cbLockedItems->setText( ToQString( _( "Locked items" ) ) );
    m_cbLockedItems->setToolTip( ToQString( _( "Allow selection of locked items" ) ) );
    m_cbFootprints->setText( ToQString( _( "Footprints" ) ) );
    m_cbText->setText( ToQString( _( "Text" ) ) );
    m_cbTracks->setText( ToQString( _( "Tracks" ) ) );
    m_cbVias->setText( ToQString( _( "Vias" ) ) );
    m_cbPads->setText( ToQString( _( "Pads" ) ) );
    m_cbGraphics->setText( ToQString( _( "Graphics" ) ) );
    m_cbZones->setText( ToQString( _( "Zones" ) ) );
    m_cbKeepouts->setText( ToQString( _( "Rule Areas" ) ) );
    m_cbDimensions->setText( ToQString( _( "Dimensions" ) ) );
    m_cbPoints->setText( ToQString( _( "Points" ) ) );
    m_cbOtherItems->setText( ToQString( _( "Other items" ) ) );
}


void PANEL_SELECTION_FILTER::flashCheckbox( QCheckBox* aBox )
{
    if( !aBox )
        return;

    // If already flashing, just reset the counter
    m_flashCounters[aBox] = m_flashSteps;
    paintFlash( aBox, m_flashSteps );
}


void PANEL_SELECTION_FILTER::onFlashTimer()
{
    for( auto it = m_flashCounters.begin(); it != m_flashCounters.end(); )
    {
        int step = --( it->second );

        paintFlash( it->first, step );

        if( step <= 0 )
            it = m_flashCounters.erase( it );
        else
            ++it;
    }

    if( m_flashCounters.empty() )
        m_flashTimer.stop();
}


void PANEL_SELECTION_FILTER::paintFlash( QCheckBox* aBox, int aStep )
{
    if( aStep <= 0 )
    {
        // Back to the colors of the panel
        aBox->setAutoFillBackground( false );
        aBox->setPalette( QPalette() );
        return;
    }

    // The highlight color blended with the background by the current flash step
    QColor highlight = palette().color( QPalette::Highlight );
    QColor background = palette().color( QPalette::Window );
    QColor blend( ( highlight.red() * aStep + background.red() * ( m_flashSteps - aStep ) ) / m_flashSteps,
                  ( highlight.green() * aStep + background.green() * ( m_flashSteps - aStep ) ) / m_flashSteps,
                  ( highlight.blue() * aStep + background.blue() * ( m_flashSteps - aStep ) ) / m_flashSteps );

    QPalette flashPalette = aBox->palette();
    flashPalette.setColor( QPalette::Window, blend );
    aBox->setPalette( flashPalette );
    aBox->setAutoFillBackground( true );
}


void PANEL_SELECTION_FILTER::Flash( const PCB_SELECTION_FILTER_OPTIONS& aOptions )
{
    if( aOptions.lockedItems )
        flashCheckbox( m_cbLockedItems );

    if( aOptions.footprints )
        flashCheckbox( m_cbFootprints );

    if( aOptions.text )
        flashCheckbox( m_cbText );

    if( aOptions.tracks )
        flashCheckbox( m_cbTracks );

    if( aOptions.vias )
        flashCheckbox( m_cbVias );

    if( aOptions.pads )
        flashCheckbox( m_cbPads );

    if( aOptions.graphics )
        flashCheckbox( m_cbGraphics );

    if( aOptions.zones )
        flashCheckbox( m_cbZones );

    if( aOptions.keepouts )
        flashCheckbox( m_cbKeepouts );

    if( aOptions.dimensions )
        flashCheckbox( m_cbDimensions );

    if( aOptions.points )
        flashCheckbox( m_cbPoints );

    if( aOptions.otherItems )
        flashCheckbox( m_cbOtherItems );

    if( !m_flashCounters.empty() )
        m_flashTimer.start();
}
