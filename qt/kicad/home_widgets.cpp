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

#include "home_widgets.h"

#include <algorithm>

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

#include <frame_type.h>


/// The setting of the theme of the window (see main_window.cpp)
static const char* const DARK_THEME_KEY = "appearance/dark";


const HOME_PALETTE& HomePalette()
{
    // Dark "workbench" and light "airy" variants, as UI_THEME of the wx project manager
    static const HOME_PALETTE dark = { QColor( 0x0F, 0x12, 0x17 ), QColor( 0x16, 0x1A, 0x21 ),
                                       QColor( 0x1D, 0x23, 0x2C ), QColor( 0x27, 0x2E, 0x39 ),
                                       QColor( 0xE6, 0xE8, 0xEC ), QColor( 0x8A, 0x94, 0xA3 ),
                                       QColor( 0x4C, 0x8D, 0xF6 ), QColor( 0xFF, 0xFF, 0xFF ),
                                       QColor( 0x22, 0x29, 0x33 ) };

    static const HOME_PALETTE light = { QColor( 0xF6, 0xF7, 0xF9 ), QColor( 0xFF, 0xFF, 0xFF ),
                                        QColor( 0xF0, 0xF3, 0xF8 ), QColor( 0xE1, 0xE4, 0xEA ),
                                        QColor( 0x1C, 0x20, 0x26 ), QColor( 0x6B, 0x72, 0x80 ),
                                        QColor( 0x2F, 0x6F, 0xE4 ), QColor( 0xFF, 0xFF, 0xFF ),
                                        QColor( 0xEC, 0xEF, 0xF4 ) };

    return QSettings().value( DARK_THEME_KEY, false ).toBool() ? dark : light;
}


QFont HomeMonoFont( int aPointDelta, bool aBold )
{
    QFont font = QApplication::font();

    font.setFamilies( { QStringLiteral( "JetBrains Mono" ), QStringLiteral( "Cascadia Mono" ),
                        QStringLiteral( "Consolas" ), QStringLiteral( "Monospace" ) } );
    font.setPointSizeF( std::max( 6.0, font.pointSizeF() + aPointDelta ) );
    font.setBold( aBold );
    return font;
}


/// The singular or the plural form with the number.
static QString plural( qint64 aCount, const char* aSingular, const char* aPlural )
{
    return QObject::tr( aCount == 1 ? aSingular : aPlural ).arg( aCount );
}


QString FormatRelativeTime( const QDateTime& aTime )
{
    if( !aTime.isValid() )
        return QString();

    const qint64 minutes = aTime.secsTo( QDateTime::currentDateTime() ) / 60;
    const qint64 hours = minutes / 60;
    const qint64 days = aTime.date().daysTo( QDate::currentDate() );

    if( minutes < 1 )
        return QObject::tr( "just now" );

    if( minutes < 60 )
        return plural( minutes, "%1 minute ago", "%1 minutes ago" );

    if( hours < 24 )
        return plural( hours, "%1 hour ago", "%1 hours ago" );

    if( days == 1 )
        return QObject::tr( "yesterday" );

    if( days < 7 )
        return plural( days, "%1 day ago", "%1 days ago" );

    if( days < 35 )
        return plural( days / 7, "%1 week ago", "%1 weeks ago" );

    return aTime.date().toString( Qt::TextDate );
}


// ---------------------------------------------------------------------------------------------
// HOME_PAGE
// ---------------------------------------------------------------------------------------------

HOME_PAGE::HOME_PAGE( QWidget* aParent ) :
        QWidget( aParent )
{
}


QLabel* HOME_PAGE::AddLabel( const QString& aText, TEXT_ROLE aRole )
{
    auto* label = new QLabel( aText, this );

    m_labels.emplace_back( label, aRole );
    applyColours();
    return label;
}


QLabel* HOME_PAGE::AddSectionLabel( const QString& aText )
{
    QLabel* label = AddLabel( aText.toUpper(), TEXT_ROLE::ACCENT );

    label->setFont( HomeMonoFont( -2, true ) );
    return label;
}


QLabel* HOME_PAGE::AddTitle( const QString& aText )
{
    QLabel* label = AddLabel( aText, TEXT_ROLE::TEXT );
    QFont   font = label->font();

    font.setBold( true );
    font.setPointSizeF( font.pointSizeF() * 1.35 );
    label->setFont( font );
    return label;
}


void HOME_PAGE::applyColours()
{
    const HOME_PALETTE& palette = HomePalette();

    for( auto& [label, role] : m_labels )
    {
        QPalette p = label->palette();
        QColor   colour = role == TEXT_ROLE::ACCENT ? palette.accent
                          : role == TEXT_ROLE::TEXT ? palette.text
                                                    : palette.muted;

        p.setColor( QPalette::WindowText, colour );
        label->setPalette( p );
    }
}


void HOME_PAGE::paintEvent( QPaintEvent* aEvent )
{
    QPainter( this ).fillRect( aEvent->rect(), HomePalette().bg );
}


void HOME_PAGE::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::PaletteChange )
        applyColours();

    QWidget::changeEvent( aEvent );
}


// ---------------------------------------------------------------------------------------------
// HOME_BUTTON
// ---------------------------------------------------------------------------------------------

HOME_BUTTON::HOME_BUTTON( const QString& aLabel, bool aPrimary, QWidget* aParent ) :
        QWidget( aParent ),
        m_label( aLabel ),
        m_primary( aPrimary )
{
    setCursor( Qt::PointingHandCursor );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}


QSize HOME_BUTTON::sizeHint() const
{
    const QFontMetrics metrics( font() );

    return QSize( metrics.horizontalAdvance( m_label ) + 24, metrics.height() + 12 );
}


void HOME_BUTTON::paintEvent( QPaintEvent* )
{
    const HOME_PALETTE& palette = HomePalette();
    QPainter            p( this );
    QColor              fill, border, text;

    p.setRenderHint( QPainter::Antialiasing );

    if( m_primary )
    {
        fill = m_hover ? palette.accent.lighter( 110 ) : palette.accent;
        border = fill;
        text = palette.accentText;
    }
    else
    {
        fill = m_hover ? palette.surfaceHover : palette.surface;
        border = m_hover ? palette.muted : palette.border;
        text = palette.text;
    }

    if( m_pressed )
        fill = fill.darker( 108 );

    if( !isEnabled() )
    {
        text = palette.muted;
        fill = palette.surface;
        border = palette.border;
    }

    p.setPen( border );
    p.setBrush( fill );
    p.drawRoundedRect( QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 5, 5 );
    p.setPen( text );
    p.drawText( rect(), Qt::AlignCenter, m_label );
}


void HOME_BUTTON::enterEvent( QEnterEvent* )
{
    m_hover = true;
    update();
}


void HOME_BUTTON::leaveEvent( QEvent* )
{
    m_hover = false;
    m_pressed = false;
    update();
}


void HOME_BUTTON::mousePressEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::LeftButton )
    {
        m_pressed = true;
        update();
    }
}


void HOME_BUTTON::mouseReleaseEvent( QMouseEvent* aEvent )
{
    const bool fire = m_pressed && aEvent->button() == Qt::LeftButton && rect().contains( aEvent->pos() );

    m_pressed = false;
    update();

    if( fire )
        emit clicked();
}


void HOME_BUTTON::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::EnabledChange )
        setCursor( isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor );

    QWidget::changeEvent( aEvent );
}


// ---------------------------------------------------------------------------------------------
// LAUNCHER_CARD
// ---------------------------------------------------------------------------------------------

LAUNCHER_CARD::LAUNCHER_CARD( const QString& aMonogram, const QString& aTitle,
                              const QString& aDescription, bool aCompact, QWidget* aParent ) :
        QWidget( aParent ),
        m_monogram( aMonogram ),
        m_title( aTitle ),
        m_description( aDescription ),
        m_compact( aCompact )
{
    setCursor( Qt::PointingHandCursor );
    setToolTip( aDescription );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}


QSize LAUNCHER_CARD::sizeHint() const
{
    const int lineHeight = QFontMetrics( font() ).height();

    if( m_compact )
        return QSize( 150, 30 + lineHeight * 2 + 26 );

    return QSize( 240, lineHeight * 2 + 26 );
}


void LAUNCHER_CARD::paintEvent( QPaintEvent* )
{
    const HOME_PALETTE& palette = HomePalette();
    QPainter            p( this );
    const bool          enabled = isEnabled();
    const bool          hover = m_hover && enabled;
    const int           pad = 12;
    const int           badgeSize = 30;

    p.setRenderHint( QPainter::Antialiasing );

    // Card
    p.setPen( hover ? palette.accent : palette.border );
    p.setBrush( hover ? palette.surfaceHover : palette.surface );
    p.drawRoundedRect( QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 6, 6 );

    // Monogram badge
    const QRect badge( pad, m_compact ? pad : ( height() - badgeSize ) / 2, badgeSize, badgeSize );

    p.setPen( palette.border );
    p.setBrush( palette.badge );
    p.drawRoundedRect( badge, 5, 5 );

    p.setFont( HomeMonoFont( -1, true ) );
    p.setPen( enabled ? palette.accent : palette.muted );
    p.drawText( badge, Qt::AlignCenter, m_monogram );

    // Text
    const int textX = m_compact ? pad : badge.right() + pad;
    const int textW = width() - textX - pad;

    QFont bold = font();
    bold.setBold( true );

    QFont small = font();
    small.setPointSizeF( std::max( 6.0, small.pointSizeF() - 1 ) );

    const int lineHeight = QFontMetrics( font() ).height();
    int       textY = m_compact ? badge.bottom() + 10 : ( height() - ( lineHeight * 2 + 2 ) ) / 2;

    p.setFont( bold );
    p.setPen( enabled ? palette.text : palette.muted );
    p.drawText( QRect( textX, textY, textW, lineHeight ), Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics( bold ).elidedText( m_title, Qt::ElideRight, textW ) );

    textY += lineHeight + 2;

    p.setFont( small );
    p.setPen( palette.muted );
    p.drawText( QRect( textX, textY, textW, lineHeight ), Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics( small ).elidedText( m_description, Qt::ElideRight, textW ) );
}


void LAUNCHER_CARD::enterEvent( QEnterEvent* )
{
    m_hover = true;
    update();
}


void LAUNCHER_CARD::leaveEvent( QEvent* )
{
    m_hover = false;
    update();
}


void LAUNCHER_CARD::mouseReleaseEvent( QMouseEvent* aEvent )
{
    if( aEvent->button() == Qt::LeftButton && rect().contains( aEvent->pos() ) )
        emit clicked();
}


void LAUNCHER_CARD::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::EnabledChange )
        setCursor( isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor );

    QWidget::changeEvent( aEvent );
}


// ---------------------------------------------------------------------------------------------
// RECENT_PROJECT_ITEM
// ---------------------------------------------------------------------------------------------

RECENT_PROJECT_ITEM::RECENT_PROJECT_ITEM( const QString& aProjectFile, QWidget* aParent ) :
        QWidget( aParent ),
        m_projectFile( aProjectFile )
{
    const QFileInfo info( aProjectFile );

    m_name = info.completeBaseName();
    m_folder = QDir::toNativeSeparators( info.absolutePath() );
    m_exists = info.exists();

    // Only cheap stat() calls: the list must stay instant even with a long history
    if( m_exists )
    {
        m_modified = info.lastModified();

        const QFileInfo schematic( info.absolutePath() + QLatin1Char( '/' ) + m_name
                                   + QStringLiteral( ".kicad_sch" ) );
        const QFileInfo board( info.absolutePath() + QLatin1Char( '/' ) + m_name
                               + QStringLiteral( ".kicad_pcb" ) );

        m_hasSchematic = schematic.exists();
        m_hasBoard = board.exists();

        // The project file is rarely touched; the documents tell when the work happened
        for( const QFileInfo& document : { schematic, board } )
        {
            if( document.exists() && document.lastModified() > m_modified )
                m_modified = document.lastModified();
        }
    }

    setToolTip( m_exists ? aProjectFile : tr( "Project not found: %1" ).arg( aProjectFile ) );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    if( m_exists )
        setCursor( Qt::PointingHandCursor );
}


QSize RECENT_PROJECT_ITEM::sizeHint() const
{
    return QSize( 180, QFontMetrics( font() ).height() * 3 + 34 );
}


void RECENT_PROJECT_ITEM::paintEvent( QPaintEvent* )
{
    const HOME_PALETTE& palette = HomePalette();
    QPainter            p( this );
    const int           pad = 13;

    p.setRenderHint( QPainter::Antialiasing );

    // Card
    const QRect card( 2, 2, width() - 4, height() - 4 );

    p.setPen( m_hover ? palette.accent : palette.border );
    p.setBrush( m_hover ? palette.surfaceHover : palette.surface );
    p.drawRoundedRect( QRectF( card ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 6, 6 );

    const int textLeft = card.x() + pad;
    const int textRight = card.right() - pad;
    int       y = card.y() + pad - 2;

    // Line 1: the name in bold, and the time on the right
    const QFont base = font();
    QFont       bold = base;
    bold.setBold( true );

    const int lineHeight = QFontMetrics( base ).height();
    const QString when = m_exists ? FormatRelativeTime( m_modified ) : tr( "not found" );
    const int     whenWidth = QFontMetrics( base ).horizontalAdvance( when );

    p.setFont( base );
    p.setPen( palette.muted );
    p.drawText( QRect( textRight - whenWidth, y, whenWidth, lineHeight ), Qt::AlignRight, when );

    const int nameWidth = std::max( 10, textRight - textLeft - whenWidth - pad );

    p.setFont( bold );
    p.setPen( m_exists ? palette.text : palette.muted );
    p.drawText( QRect( textLeft, y, nameWidth, lineHeight ), Qt::AlignLeft,
                QFontMetrics( bold ).elidedText( m_name, Qt::ElideRight, nameWidth ) );

    y += lineHeight + 3;

    // Line 2: the folder, shortened in the middle to keep the root and the leaf
    p.setFont( base );
    p.setPen( palette.muted );
    p.drawText( QRect( textLeft, y, textRight - textLeft, lineHeight ), Qt::AlignLeft,
                QFontMetrics( base ).elidedText( m_folder, Qt::ElideMiddle, textRight - textLeft ) );

    y += lineHeight + 5;

    // Line 3: tags of the documents that exist
    if( !m_exists )
        return;

    QFont small = base;
    small.setPointSizeF( std::max( 6.0, small.pointSizeF() - 1 ) );
    p.setFont( small );

    const QFontMetrics metrics( small );
    int                x = textLeft;

    auto drawTag =
            [&]( const QString& aText )
            {
                const QRect tag( x, y, metrics.horizontalAdvance( aText ) + 10, metrics.height() + 4 );

                p.setPen( palette.border );
                p.setBrush( Qt::NoBrush );
                p.drawRoundedRect( QRectF( tag ).adjusted( 0.5, 0.5, -0.5, -0.5 ), 3, 3 );
                p.setPen( palette.muted );
                p.drawText( tag, Qt::AlignCenter, aText );

                x = tag.right() + 6;
            };

    if( m_hasSchematic )
        drawTag( tr( "Schematic" ) );

    if( m_hasBoard )
        drawTag( tr( "PCB" ) );
}


void RECENT_PROJECT_ITEM::enterEvent( QEnterEvent* )
{
    if( m_exists )
    {
        m_hover = true;
        update();
    }
}


void RECENT_PROJECT_ITEM::leaveEvent( QEvent* )
{
    m_hover = false;
    update();
}


void RECENT_PROJECT_ITEM::mouseReleaseEvent( QMouseEvent* aEvent )
{
    if( m_exists && aEvent->button() == Qt::LeftButton && rect().contains( aEvent->pos() ) )
        emit activated( m_projectFile );
}


// ---------------------------------------------------------------------------------------------
// RECENT_PROJECTS_PANEL
// ---------------------------------------------------------------------------------------------

RECENT_PROJECTS_PANEL::RECENT_PROJECTS_PANEL( QWidget* aParent ) :
        HOME_PAGE( aParent )
{
    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 14, 14, 14, 14 );
    layout->setSpacing( 14 );

    layout->addWidget( AddSectionLabel( tr( "Recent Projects" ) ) );
    layout->addWidget( AddTitle( tr( "Pick up where you left off" ) ) );

    // New and Open; the title is on its own line so that a narrow column does not squeeze it
    auto* header = new QHBoxLayout;
    header->setSpacing( 6 );

    auto* newButton = new HOME_BUTTON( tr( "New Project..." ), true );
    auto* openButton = new HOME_BUTTON( tr( "Open..." ), false );

    // A new project needs the templates of the project manager, not in the Qt shell yet
    newButton->setEnabled( false );
    newButton->setToolTip( tr( "Not available in kicad-qt yet" ) );

    connect( openButton, &HOME_BUTTON::clicked, this, &RECENT_PROJECTS_PANEL::openRequested );

    header->addWidget( newButton );
    header->addWidget( openButton );
    header->addStretch();
    layout->addLayout( header );

    // Cards
    auto* scroll = new QScrollArea;
    scroll->setFrameShape( QFrame::NoFrame );
    scroll->setWidgetResizable( true );
    scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scroll->viewport()->setAutoFillBackground( false );

    m_list = new QWidget;
    m_list->setAutoFillBackground( false );
    m_list->setLayout( new QVBoxLayout );
    m_list->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_list->layout()->setSpacing( 6 );
    scroll->setWidget( m_list );
    layout->addWidget( scroll, 1 );

    m_footer = AddLabel( QString(), TEXT_ROLE::MUTED );
    layout->addWidget( m_footer );

    setMinimumWidth( 320 );
}


void RECENT_PROJECTS_PANEL::SetProjects( const QStringList& aProjectFiles )
{
    auto* layout = static_cast<QVBoxLayout*>( m_list->layout() );

    while( QLayoutItem* item = layout->takeAt( 0 ) )
    {
        delete item->widget();
        delete item;
    }

    for( const QString& file : aProjectFiles )
    {
        auto* item = new RECENT_PROJECT_ITEM( file );

        connect( item, &RECENT_PROJECT_ITEM::activated, this,
                 [this]( const QString& aFile )
                 {
                     // Deferred: opening a project rebuilds this list, which deletes the item
                     QMetaObject::invokeMethod( this, [this, aFile]() { emit projectActivated( aFile ); },
                                                Qt::QueuedConnection );
                 } );
        layout->addWidget( item );
    }

    if( aProjectFiles.isEmpty() )
    {
        auto* empty = new QLabel( tr( "No recent projects." ) );

        empty->setForegroundRole( QPalette::PlaceholderText );
        layout->addWidget( empty );
    }

    layout->addStretch();
    m_footer->setText( plural( aProjectFiles.size(), "%1 project", "%1 projects" ) );
}


// ---------------------------------------------------------------------------------------------
// LAUNCHER_PANEL
// ---------------------------------------------------------------------------------------------

LAUNCHER_PANEL::LAUNCHER_PANEL( QWidget* aParent ) :
        HOME_PAGE( aParent )
{
    auto* outer = new QVBoxLayout( this );
    outer->setContentsMargins( 0, 0, 0, 0 );

    auto* scroll = new QScrollArea;
    scroll->setFrameShape( QFrame::NoFrame );
    scroll->setWidgetResizable( true );
    scroll->viewport()->setAutoFillBackground( false );
    outer->addWidget( scroll );

    // The labels of the column are made by the page, in its colours
    auto* column = new QWidget;
    column->setAutoFillBackground( false );
    scroll->setWidget( column );

    auto* layout = new QVBoxLayout( column );
    layout->setContentsMargins( 16, 16, 16, 16 );
    layout->setSpacing( 8 );

    layout->addWidget( AddSectionLabel( tr( "Tools" ) ) );
    layout->addSpacing( 6 );
    layout->addWidget( AddTitle( tr( "Open an Editor" ) ) );

    auto addSection =
            [&]( const QString& aTitle )
            {
                layout->addSpacing( 8 );
                layout->addWidget( AddSectionLabel( aTitle ) );
                layout->addSpacing( 8 );
            };

    auto addRow =
            [&]( const QString& aMonogram, const QString& aTitle, const QString& aHelp,
                 bool aAvailable ) -> LAUNCHER_CARD*
            {
                auto* card = new LAUNCHER_CARD( aMonogram, aTitle, aHelp, false );

                if( !aAvailable )
                {
                    card->setEnabled( false );
                    card->setToolTip( tr( "%1 (not available in kicad-qt yet)" ).arg( aHelp ) );
                }

                layout->addWidget( card );
                return card;
            };

    addSection( tr( "Design" ) );
    m_schematic = addRow( QStringLiteral( "SC" ), tr( "Schematic Editor" ),
                          tr( "Edit the project schematic" ), true );
    m_board = addRow( QStringLiteral( "PC" ), tr( "PCB Editor" ),
                      tr( "Edit the project PCB design" ), true );

    addSection( tr( "Libraries" ) );
    LAUNCHER_CARD* symbols = addRow( QStringLiteral( "SY" ), tr( "Symbol Editor" ),
                                     tr( "Edit global and/or project schematic symbol libraries" ),
                                     true );
    LAUNCHER_CARD* footprints = addRow( QStringLiteral( "FP" ), tr( "Footprint Editor" ),
                                        tr( "Edit global and/or project PCB footprint libraries" ),
                                        true );
    LAUNCHER_CARD* drawingSheets = addRow( QStringLiteral( "DS" ), tr( "Drawing Sheet Editor" ),
                                           tr( "Edit drawing sheet borders and title blocks" ),
                                           true );

    addSection( tr( "Manufacturing and Utilities" ) );

    // Utilities: two columns of compact cards
    auto* grid = new QGridLayout;
    grid->setSpacing( 8 );

    const struct
    {
        const char* monogram;
        const char* title;
        const char* help;
        int         frameType;      ///< The frame of the tool (-1: not available yet)
    } utilities[] = { { "GB", "Gerber Viewer", "Preview Gerber files", FRAME_GERBER },
                      { "CA", "Calculator Tools", "Resistance, current capacity, etc.", -1 },
                      { "IM", "Image Converter", "Bitmaps to symbols or footprints", -1 },
                      { "PM", "Plugin and Content Manager", "KiCad and third-party packages", -1 } };

    int index = 0;

    for( const auto& utility : utilities )
    {
        auto* card = new LAUNCHER_CARD( QString::fromLatin1( utility.monogram ), tr( utility.title ),
                                        tr( utility.help ), true );

        if( utility.frameType >= 0 )
        {
            connect( card, &LAUNCHER_CARD::clicked, this,
                     [this, frameType = utility.frameType]()
                     {
                         emit toolRequested( frameType );
                     } );
        }
        else
        {
            card->setEnabled( false );
            card->setToolTip( tr( "%1 (not available in kicad-qt yet)" ).arg( tr( utility.help ) ) );
        }

        grid->addWidget( card, index / 2, index % 2 );
        ++index;
    }

    layout->addLayout( grid );
    layout->addStretch();

    connect( m_schematic, &LAUNCHER_CARD::clicked, this, &LAUNCHER_PANEL::schematicRequested );
    connect( m_board, &LAUNCHER_CARD::clicked, this, &LAUNCHER_PANEL::boardRequested );
    connect( symbols, &LAUNCHER_CARD::clicked, this, &LAUNCHER_PANEL::symbolEditorRequested );
    connect( footprints, &LAUNCHER_CARD::clicked, this, &LAUNCHER_PANEL::footprintEditorRequested );
    connect( drawingSheets, &LAUNCHER_CARD::clicked, this,
             [this]()
             {
                 emit toolRequested( FRAME_PL_EDITOR );
             } );

    SetDocuments( false, false );
}


void LAUNCHER_PANEL::SetDocuments( bool aSchematic, bool aBoard )
{
    m_schematic->setEnabled( aSchematic );
    m_board->setEnabled( aBoard );
}
