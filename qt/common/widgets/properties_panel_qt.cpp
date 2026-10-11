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
 * @file properties_panel_qt.cpp
 * The parts of PROPERTIES_PANEL with Qt (common/widgets/properties_panel_wx.cpp with
 * wxWidgets): the panel, its caption and its PROPERTY_GRID.
 */

#include <widgets/properties_panel.h>

#include <eda_base_frame.h>
#include <widgets/property_grid.h>

#include <qt_strings.h>

#include <QLabel>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <wx/intl.h>


PROPERTIES_PANEL::PROPERTIES_PANEL( QWidget* aParent, EDA_BASE_FRAME* aFrame ) :
        QWidget( aParent ),
        m_SuppressGridChangeEvents( 0 ),
        m_frame( aFrame ),
        m_splitter_key_proportion( -1 )
{
    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    m_caption = new QLabel( ToQString( _( "No objects selected" ) ), this );
    m_caption->setContentsMargins( 5, 5, 5, 5 );
    layout->addWidget( m_caption );

    m_grid = new PROPERTY_GRID( this );
    layout->addWidget( m_grid, 1 );

    m_grid->OnChanging =
            [this]( const wxString& aName, const wxVariant& aValue )
            {
                return valueChanging( aName, aValue );
            };

    m_grid->OnChanged =
            [this]( const wxString& aName, const wxVariant& aValue )
            {
                valueChanged( aName, aValue );
            };

    m_grid->OnSplitterMoved =
            [this]()
            {
                if( m_grid->width() > 0 )
                {
                    m_splitter_key_proportion = static_cast<float>( m_grid->GetSplitterPosition() )
                                                / m_grid->width();
                }
            };

    if( m_frame )
        m_frame->AddLanguageChangedListener( this, [this]() { OnLanguageChanged(); } );
}


PROPERTIES_PANEL::~PROPERTIES_PANEL()
{
    // the grid must not call the panel back while being destroyed
    m_grid->OnChanging = nullptr;
    m_grid->OnChanged = nullptr;
    m_grid->OnSplitterMoved = nullptr;
}


void PROPERTIES_PANEL::OnLanguageChanged()
{
    m_grid->Clear();
    m_displayed.clear();    // no ownership of pointers

    UpdateData();
}


void PROPERTIES_PANEL::showEvent( QShowEvent* aEvent )
{
    QWidget::showEvent( aEvent );

    if( !aEvent->spontaneous() )
        UpdateData();
}


void PROPERTIES_PANEL::resizeEvent( QResizeEvent* aEvent )
{
    QWidget::resizeEvent( aEvent );

    // once the grid has its new size
    QTimer::singleShot( 0, this, [this]() { RecalculateSplitterPos(); } );
}


void PROPERTIES_PANEL::RecalculateSplitterPos()
{
    if( m_splitter_key_proportion < 0 )
        m_grid->CenterSplitter();
    else
        m_grid->SetSplitterPosition( static_cast<int>( m_splitter_key_proportion * m_grid->width() ) );
}


void PROPERTIES_PANEL::SetSplitterProportion( float aProportion )
{
    m_splitter_key_proportion = aProportion;
    RecalculateSplitterPos();
}


void PROPERTIES_PANEL::setCaption( const wxString& aCaption )
{
    m_caption->setText( ToQString( aCaption ) );
}


void PROPERTIES_PANEL::gridReset()
{
    m_grid->Clear();
    m_displayed.clear();
}


std::vector<TOOLKIT_PG_PROPERTY*> PROPERTIES_PANEL::gridProperties() const
{
    // the properties, not the categories (as wxPropertyGrid iterates them)
    std::vector<TOOLKIT_PG_PROPERTY*> properties;

    for( PG_PROPERTY* prop : m_grid->Properties() )
    {
        if( !prop->IsCategory() )
            properties.push_back( prop );
    }

    return properties;
}


void PROPERTIES_PANEL::gridAppendCategory( const wxString& aCaption )
{
    m_grid->Append( new PGPROPERTY_CATEGORY( aCaption ) );
}


void PROPERTIES_PANEL::gridAppend( TOOLKIT_PG_PROPERTY* aProperty )
{
    m_grid->Append( aProperty );
}
