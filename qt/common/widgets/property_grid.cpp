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
 * @file property_grid.cpp
 * The property grid of the properties panels with Qt (see widgets/property_grid.h).
 */

#include <widgets/property_grid.h>

#include <dialogs/dialog_color_picker.h>
#include <eda_draw_frame.h>
#include <kiway.h>
#include <kiway_player.h>
#include <properties/pg_value_text.h>
#include <widgets/unit_binder.h>

#include <qt_strings.h>
#include <qt_ui.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QUrl>

#include <wx/intl.h>


namespace
{

/// The role of the items holding their PG_PROPERTY.
constexpr int PROPERTY_ROLE = Qt::UserRole;

/// The text of the values that differ between the selected items.
const QString UNSPECIFIED_TEXT = QStringLiteral( "<...>" );


/// A swatch of a color, for the color properties and the layers.
QIcon colorIcon( const KIGFX::COLOR4D& aColor, int aSize )
{
    QPixmap pixmap( aSize, aSize );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setPen( Qt::darkGray );
    painter.setBrush( ToQColor( aColor ) );
    painter.drawRect( 0, 0, aSize - 1, aSize - 1 );

    return QIcon( pixmap );
}


/// The editor of the texts with a button (the footprints, the links and files).
class BUTTON_EDITOR : public QWidget
{
public:
    BUTTON_EDITOR( QWidget* aParent ) :
            QWidget( aParent )
    {
        auto* layout = new QHBoxLayout( this );
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        m_text = new QLineEdit( this );
        m_button = new QToolButton( this );
        m_button->setText( QStringLiteral( "..." ) );

        layout->addWidget( m_text, 1 );
        layout->addWidget( m_button );

        setFocusProxy( m_text );
    }

    QLineEdit*   m_text;
    QToolButton* m_button;
};


/// The editors of the values of the grid.
class PROPERTY_DELEGATE : public QStyledItemDelegate
{
public:
    PROPERTY_DELEGATE( PROPERTY_GRID* aGrid ) :
            QStyledItemDelegate( aGrid ),
            m_grid( aGrid )
    {
    }

    QWidget* createEditor( QWidget* aParent, const QStyleOptionViewItem& aOption,
                           const QModelIndex& aIndex ) const override
    {
        PG_PROPERTY* prop = m_grid->PropertyOf( aIndex );

        if( !prop || aIndex.column() != 1 || prop->IsCategory() || prop->IsReadOnly() )
            return nullptr;

        QWidget* editor = nullptr;

        switch( prop->GetEditor() )
        {
        case PG_PROPERTY::EDITOR::TEXT:
        case PG_PROPERTY::EDITOR::RATIO:
            editor = new QLineEdit( aParent );
            break;

        case PG_PROPERTY::EDITOR::UNIT:
        {
            auto* text = new QLineEdit( aParent );

            if( EDA_DRAW_FRAME* frame = m_grid->GetEditorFrame() )
            {
                auto binder = std::make_shared<UNIT_BINDER>( frame, nullptr, text, nullptr, true,
                                                             false );
                binder->RequireEval();
                binder->SetUnits( frame->GetUserUnits() );

                switch( prop->GetUnitKind() )
                {
                case PG_UNIT_KIND::DISTANCE:
                    binder->SetCoordType( prop->GetCoordType() );
                    break;

                case PG_UNIT_KIND::AREA:
                    binder->SetDataType( EDA_DATA_TYPE::AREA );
                    break;

                case PG_UNIT_KIND::ANGLE:
                    binder->SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );
                    binder->SetUnits( EDA_UNITS::DEGREES );
                    break;

                case PG_UNIT_KIND::TIME:
                    binder->SetUnits( EDA_UNITS::PS );
                    break;

                default:
                    break;
                }

                m_binders[text] = binder;
                QObject::connect( text, &QObject::destroyed,
                                  [this, text]()
                                  {
                                      m_binders.erase( text );
                                  } );
            }

            editor = text;
            break;
        }

        case PG_PROPERTY::EDITOR::CHOICE:
        {
            auto*                   combo = new QComboBox( aParent );
            const PROPERTY_CHOICES& choices = prop->GetChoices();
            int                     size = combo->style()->pixelMetric( QStyle::PM_SmallIconSize );

            for( unsigned ii = 0; ii < choices.GetCount(); ++ii )
            {
                KIGFX::COLOR4D color = prop->GetChoiceColor( choices.GetValue( ii ) );

                if( color != KIGFX::COLOR4D::UNSPECIFIED )
                    combo->addItem( colorIcon( color, size ), ToQString( choices.GetLabel( ii ) ) );
                else
                    combo->addItem( ToQString( choices.GetLabel( ii ) ) );
            }

            // the choice is applied as soon as it is made
            QObject::connect( combo, QOverload<int>::of( &QComboBox::activated ),
                              [this, combo]()
                              {
                                  const_cast<PROPERTY_DELEGATE*>( this )->commitData( combo );
                              } );

            editor = combo;
            break;
        }

        case PG_PROPERTY::EDITOR::FPID:
        case PG_PROPERTY::EDITOR::URL:
        {
            auto* buttonEditor = new BUTTON_EDITOR( aParent );
            PROPERTY_GRID* grid = m_grid;
            QPointer<BUTTON_EDITOR> guard( buttonEditor );

            QObject::connect( buttonEditor->m_button, &QToolButton::clicked,
                              [grid, prop, guard]()
                              {
                                  wxString text = guard ? ToWxString( guard->m_text->text() )
                                                        : prop->GetValueAsString();
                                  grid->RunDialogEditor( prop, text );
                              } );

            editor = buttonEditor;
            break;
        }

        default:
            if( PG_CUSTOM_EDITOR* custom = m_grid->GetCustomEditor( prop->GetEditor() ) )
                editor = custom->CreateEditor( aParent, m_grid, prop );

            break;
        }

        if( editor )
        {
            m_grid->SetActiveEditor( editor );
            QObject::connect( editor, &QObject::destroyed,
                              [grid = m_grid]()
                              {
                                  grid->SetActiveEditor( nullptr );
                              } );
        }

        return editor;
    }

    void setEditorData( QWidget* aEditor, const QModelIndex& aIndex ) const override
    {
        PG_PROPERTY* prop = m_grid->PropertyOf( aIndex );

        if( !prop )
            return;

        if( auto* combo = dynamic_cast<QComboBox*>( aEditor ) )
        {
            int index = prop->IsValueUnspecified()
                                ? -1
                                : prop->GetChoices().Index( static_cast<int>( prop->GetValue().GetLong() ) );
            combo->setCurrentIndex( index );
        }
        else if( auto* buttonEditor = dynamic_cast<BUTTON_EDITOR*>( aEditor ) )
        {
            buttonEditor->m_text->setText( ToQString( prop->GetValueAsString() ) );
        }
        else if( auto* text = dynamic_cast<QLineEdit*>( aEditor ) )
        {
            auto it = m_binders.find( text );

            if( it != m_binders.end() )
            {
                PGUnitBinderShowValue( *it->second, prop->GetValue(), prop->GetUnitKind(),
                                       prop->GetAngleScale(), prop->IsValueUnspecified() );
            }
            else
            {
                text->setText( ToQString( prop->GetValueAsString() ) );
            }

            text->selectAll();
        }
    }

    void setModelData( QWidget* aEditor, QAbstractItemModel*, const QModelIndex& aIndex ) const override
    {
        PG_PROPERTY* prop = m_grid->PropertyOf( aIndex );

        if( !prop )
            return;

        wxVariant value = prop->GetValue();
        bool      changed = false;

        if( auto* combo = dynamic_cast<QComboBox*>( aEditor ) )
        {
            int index = combo->currentIndex();

            if( index >= 0 && static_cast<unsigned>( index ) < prop->GetChoices().GetCount() )
            {
                long newValue = prop->GetChoices().GetValue( index );
                changed = prop->IsValueUnspecified() || value.GetLong() != newValue;
                value = newValue;
            }
        }
        else if( auto* buttonEditor = dynamic_cast<BUTTON_EDITOR*>( aEditor ) )
        {
            wxString text = ToWxString( buttonEditor->m_text->text() );

            if( text != prop->GetValueAsString() )
                changed = prop->StringToValue( value, text );
        }
        else if( auto* text = dynamic_cast<QLineEdit*>( aEditor ) )
        {
            wxString newText = ToWxString( text->text() );
            auto     it = m_binders.find( text );

            if( it != m_binders.end() )
            {
                if( !( prop->IsValueUnspecified() && newText.IsEmpty() ) )
                {
                    changed = PGUnitBinderGetValue( *it->second, newText, value,
                                                    prop->GetUnitKind(), prop->GetAngleScale() );
                }
            }
            else if( prop->GetEditor() == PG_PROPERTY::EDITOR::RATIO )
            {
                wxString normalizedText;
                changed = PGRatioFromText( newText, value, normalizedText );
            }
            else if( newText != prop->GetValueAsString()
                     && !( prop->IsValueUnspecified() && newText.IsEmpty() ) )
            {
                changed = prop->StringToValue( value, newText );
            }
        }
        else if( PG_CUSTOM_EDITOR* custom = m_grid->GetCustomEditor( prop->GetEditor() ) )
        {
            changed = custom->GetValue( aEditor, prop, value );
        }

        if( changed )
            m_grid->ChangePropertyValue( prop, value );
    }

    void updateEditorGeometry( QWidget* aEditor, const QStyleOptionViewItem& aOption,
                               const QModelIndex& ) const override
    {
        aEditor->setGeometry( aOption.rect );
    }

private:
    PROPERTY_GRID*                                                 m_grid;
    mutable std::map<QLineEdit*, std::shared_ptr<UNIT_BINDER>>     m_binders;
};

} // namespace


PROPERTY_GRID::PROPERTY_GRID( QWidget* aParent ) :
        QTreeWidget( aParent ),
        m_category( nullptr ),
        m_frame( nullptr ),
        m_activeEditor( nullptr ),
        m_filling( false )
{
    setColumnCount( 2 );
    setHeaderHidden( true );
    setRootIsDecorated( false );
    setIndentation( 0 );
    setUniformRowHeights( true );
    setAlternatingRowColors( false );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked
                     | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed );
    setItemDelegate( new PROPERTY_DELEGATE( this ) );

    header()->setStretchLastSection( true );
    header()->setSectionResizeMode( 0, QHeaderView::Interactive );

    connect( header(), &QHeaderView::sectionResized,
             [this]( int aSection, int, int )
             {
                 if( aSection == 0 && !m_filling && OnSplitterMoved )
                     OnSplitterMoved();
             } );

    connect( this, &QTreeWidget::itemClicked, this, &PROPERTY_GRID::onItemClicked );

    // The check boxes of the boolean properties
    connect( this, &QTreeWidget::itemChanged,
             [this]( QTreeWidgetItem* aItem, int aColumn )
             {
                 if( m_filling || aColumn != 1 )
                     return;

                 PG_PROPERTY* prop = PropertyOf( indexFromItem( aItem ) );

                 if( !prop || prop->GetEditor() != PG_PROPERTY::EDITOR::CHECKBOX )
                     return;

                 bool checked = aItem->checkState( 1 ) == Qt::Checked;

                 if( prop->IsReadOnly() )
                     RefreshProperty( prop );
                 else
                     ChangePropertyValue( prop, wxVariant( checked ) );
             } );
}


PROPERTY_GRID::~PROPERTY_GRID()
{
    // the properties don't refresh a grid being destroyed
    for( std::unique_ptr<PG_PROPERTY>& prop : m_properties )
        prop->SetGrid( nullptr );
}


void PROPERTY_GRID::Clear()
{
    CommitChangesFromEditor();

    m_filling = true;
    clear();
    m_filling = false;

    m_items.clear();
    m_properties.clear();
    m_category = nullptr;
}


void PROPERTY_GRID::Append( PG_PROPERTY* aProperty )
{
    m_properties.emplace_back( aProperty );
    aProperty->SetGrid( this );

    QTreeWidgetItem* item;

    if( aProperty->IsCategory() )
    {
        item = new QTreeWidgetItem( this );
        item->setFirstColumnSpanned( true );
        item->setFlags( Qt::ItemIsEnabled );

        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        item->setBackground( 0, palette().alternateBase() );

        m_category = item;
    }
    else
    {
        item = m_category ? new QTreeWidgetItem( m_category ) : new QTreeWidgetItem( this );
    }

    item->setData( 0, PROPERTY_ROLE, QVariant::fromValue( static_cast<void*>( aProperty ) ) );
    m_items[aProperty] = item;

    RefreshProperty( aProperty );

    if( m_category )
        m_category->setExpanded( true );
}


std::vector<PG_PROPERTY*> PROPERTY_GRID::Properties() const
{
    std::vector<PG_PROPERTY*> properties;

    for( const std::unique_ptr<PG_PROPERTY>& prop : m_properties )
        properties.push_back( prop.get() );

    return properties;
}


PG_PROPERTY* PROPERTY_GRID::PropertyOf( const QModelIndex& aIndex ) const
{
    if( !aIndex.isValid() )
        return nullptr;

    QModelIndex first = aIndex.sibling( aIndex.row(), 0 );
    return static_cast<PG_PROPERTY*>( first.data( PROPERTY_ROLE ).value<void*>() );
}


QTreeWidgetItem* PROPERTY_GRID::itemOf( PG_PROPERTY* aProperty ) const
{
    auto it = m_items.find( aProperty );
    return it == m_items.end() ? nullptr : it->second;
}


PG_PROPERTY* PROPERTY_GRID::GetSelectedProperty() const
{
    return PropertyOf( currentIndex() );
}


void PROPERTY_GRID::RefreshProperty( PG_PROPERTY* aProperty )
{
    QTreeWidgetItem* item = itemOf( aProperty );

    if( !item )
        return;

    bool wasFilling = m_filling;
    m_filling = true;

    item->setText( 0, ToQString( aProperty->GetLabel() ) );

    if( !aProperty->IsCategory() )
    {
        QString help = ToQString( aProperty->GetHelpString() );
        item->setToolTip( 0, help.isEmpty() ? ToQString( aProperty->GetLabel() ) : help );

        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        int           iconSize = style()->pixelMetric( QStyle::PM_SmallIconSize );

        item->setIcon( 1, QIcon() );

        if( aProperty->GetEditor() == PG_PROPERTY::EDITOR::CHECKBOX )
        {
            flags |= Qt::ItemIsUserCheckable;
            item->setText( 1, QString() );

            if( aProperty->IsValueUnspecified() )
                item->setCheckState( 1, Qt::PartiallyChecked );
            else
                item->setCheckState( 1, aProperty->GetValue().GetBool() ? Qt::Checked : Qt::Unchecked );
        }
        else
        {
            if( !aProperty->IsReadOnly() && aProperty->GetEditor() != PG_PROPERTY::EDITOR::COLOR )
                flags |= Qt::ItemIsEditable;

            item->setText( 1, aProperty->IsValueUnspecified()
                                      ? UNSPECIFIED_TEXT
                                      : ToQString( aProperty->GetValueAsString() ) );

            if( auto* colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( aProperty ) )
            {
                if( !aProperty->IsValueUnspecified() )
                    item->setIcon( 1, colorIcon( colorProp->GetColor(), iconSize ) );
            }
            else if( aProperty->GetEditor() == PG_PROPERTY::EDITOR::CHOICE
                     && !aProperty->IsValueUnspecified() )
            {
                KIGFX::COLOR4D color =
                        aProperty->GetChoiceColor( static_cast<int>( aProperty->GetValue().GetLong() ) );

                if( color != KIGFX::COLOR4D::UNSPECIFIED )
                    item->setIcon( 1, colorIcon( color, iconSize ) );
            }
        }

        item->setFlags( flags );

        QBrush textBrush = aProperty->IsReadOnly() ? palette().brush( QPalette::Disabled,
                                                                       QPalette::Text )
                                                   : palette().brush( QPalette::Text );
        item->setForeground( 0, textBrush );
        item->setForeground( 1, textBrush );
    }

    m_filling = wasFilling;
}


bool PROPERTY_GRID::ChangePropertyValue( PG_PROPERTY* aProperty, const wxVariant& aValue )
{
    wxString name = aProperty->GetName();

    if( OnChanging && !OnChanging( name, aValue ) )
    {
        RefreshProperty( aProperty );
        return false;
    }

    aProperty->SetValue( aValue );

    // The change may rebuild the grid (and delete its editors): after the edit is done
    if( OnChanged )
    {
        QMetaObject::invokeMethod(
                this,
                [this, name, aValue]()
                {
                    if( OnChanged )
                        OnChanged( name, aValue );
                },
                Qt::QueuedConnection );
    }

    return true;
}


bool PROPERTY_GRID::IsEditorFocused() const
{
    return m_activeEditor != nullptr;
}


void PROPERTY_GRID::CommitChangesFromEditor()
{
    if( QWidget* editor = m_activeEditor )
    {
        commitData( editor );
        closeEditor( editor, QAbstractItemDelegate::NoHint );
        m_activeEditor = nullptr;
    }
}


void PROPERTY_GRID::SetSplitterPosition( int aPosition )
{
    m_filling = true;
    header()->resizeSection( 0, std::max( aPosition, 20 ) );
    m_filling = false;
}


int PROPERTY_GRID::GetSplitterPosition() const
{
    return header()->sectionSize( 0 );
}


void PROPERTY_GRID::CenterSplitter()
{
    SetSplitterPosition( viewport()->width() / 2 );
}


void PROPERTY_GRID::SetCustomEditor( PG_PROPERTY::EDITOR aKind,
                                     std::unique_ptr<PG_CUSTOM_EDITOR> aEditor )
{
    m_customEditors[aKind] = std::move( aEditor );
}


PG_CUSTOM_EDITOR* PROPERTY_GRID::GetCustomEditor( PG_PROPERTY::EDITOR aKind ) const
{
    auto it = m_customEditors.find( aKind );
    return it == m_customEditors.end() ? nullptr : it->second.get();
}


void PROPERTY_GRID::RunDialogEditor( PG_PROPERTY* aProperty, const wxString& aCurrentText )
{
    if( !aProperty || aProperty->IsReadOnly() )
        return;

    switch( aProperty->GetEditor() )
    {
    case PG_PROPERTY::EDITOR::COLOR:
    {
        auto* colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( aProperty );

        if( !colorProp )
            return;

        KIGFX::COLOR4D current = colorProp->GetColor();

        if( current == KIGFX::COLOR4D::UNSPECIFIED )
            current = KIGFX::COLOR4D::WHITE;

        DIALOG_COLOR_PICKER dialog( this, current, true );

        if( dialog.ShowModal() == wxID_OK )
        {
            wxVariant value = PGColorFromString( dialog.GetColor().ToCSSString() );
            ChangePropertyValue( aProperty, value );
        }

        break;
    }

    case PG_PROPERTY::EDITOR::FPID:
    {
        EDA_DRAW_FRAME* frame = aProperty->GetEditorFrame() ? aProperty->GetEditorFrame() : m_frame;

        if( !frame )
            return;

        wxString fpid = aCurrentText;

        if( KIWAY_PLAYER* chooser = frame->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, frame ) )
        {
            if( chooser->ShowModal( &fpid, frame ) )
            {
                CommitChangesFromEditor();
                ChangePropertyValue( aProperty, wxVariant( fpid ) );
            }

            chooser->deleteLater();
        }

        break;
    }

    case PG_PROPERTY::EDITOR::URL:
    {
        wxString url = aCurrentText;

        if( url.IsEmpty() || url == wxS( "~" ) )
        {
            QString file = QFileDialog::getOpenFileName( this, ToQString( _( "Open file" ) ) );

            if( !file.isEmpty() )
            {
                CommitChangesFromEditor();
                ChangePropertyValue( aProperty, wxVariant( wxS( "file://" ) + ToWxString( file ) ) );
            }
        }
        else
        {
            QUrl link = QUrl::fromUserInput( ToQString( url ) );
            QDesktopServices::openUrl( link );
        }

        break;
    }

    default:
        break;
    }
}


void PROPERTY_GRID::onItemClicked( QTreeWidgetItem* aItem, int aColumn )
{
    if( aColumn != 1 )
        return;

    PG_PROPERTY* prop = PropertyOf( indexFromItem( aItem ) );

    if( prop && prop->GetEditor() == PG_PROPERTY::EDITOR::COLOR )
        RunDialogEditor( prop );
}


void PROPERTY_GRID::selectNextEditable( bool aForward )
{
    CommitChangesFromEditor();

    std::vector<QTreeWidgetItem*> rows;

    for( QTreeWidgetItemIterator it( this ); *it; ++it )
    {
        PG_PROPERTY* prop = PropertyOf( indexFromItem( *it ) );

        if( prop && !prop->IsCategory() && !prop->IsReadOnly() && !( *it )->isHidden() )
            rows.push_back( *it );
    }

    if( rows.empty() )
        return;

    QTreeWidgetItem* current = currentItem();
    auto             pos = std::find( rows.begin(), rows.end(), current );
    QTreeWidgetItem* target;

    if( pos == rows.end() )
        target = aForward ? rows.front() : rows.back();
    else if( aForward )
        target = ( pos + 1 == rows.end() ) ? rows.front() : *( pos + 1 );
    else
        target = ( pos == rows.begin() ) ? rows.back() : *( pos - 1 );

    setCurrentItem( target, 1 );

    if( target->flags() & Qt::ItemIsEditable )
        editItem( target, 1 );
}


void PROPERTY_GRID::keyPressEvent( QKeyEvent* aEvent )
{
    if( ( aEvent->key() == Qt::Key_Tab || aEvent->key() == Qt::Key_Backtab )
        && !( aEvent->modifiers() & ( Qt::ControlModifier | Qt::AltModifier ) ) )
    {
        // Tab steps between the properties that can be edited (wrapping around)
        selectNextEditable( aEvent->key() == Qt::Key_Tab );
        aEvent->accept();
        return;
    }

    PG_PROPERTY* prop = GetSelectedProperty();

    if( aEvent->matches( QKeySequence::Copy ) && prop && !prop->IsCategory() )
    {
        QApplication::clipboard()->setText( ToQString( prop->GetValueAsString() ) );
        aEvent->accept();
        return;
    }

    if( aEvent->key() == Qt::Key_Space && prop
        && prop->GetEditor() == PG_PROPERTY::EDITOR::CHECKBOX && !prop->IsReadOnly() )
    {
        bool checked = !prop->IsValueUnspecified() && prop->GetValue().GetBool();
        ChangePropertyValue( prop, wxVariant( !checked ) );
        aEvent->accept();
        return;
    }

    if( ( aEvent->key() == Qt::Key_Return || aEvent->key() == Qt::Key_Enter ) && prop
        && prop->GetEditor() == PG_PROPERTY::EDITOR::COLOR )
    {
        RunDialogEditor( prop );
        aEvent->accept();
        return;
    }

    QTreeWidget::keyPressEvent( aEvent );
}
