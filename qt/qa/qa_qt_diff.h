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
 * @file qa_qt_diff.h
 * The diff viewer and the interactive 3-way merge of KiCad 11 in the Qt QA of the editors:
 * the KIFACE opens them on edited copies of the document of the test.
 */

#pragma once

#include "qa_qt_frames.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPointer>
#include <QScreen>
#include <QHash>
#include <QTemporaryDir>
#include <QTreeWidget>

#include <cli/exit_codes.h>
#include <diff_merge/diff_doc_kind.h>
#include <kiface_base.h>
#include <kiface_services.h>
#include <reporter.h>
#include <gal/graphics_abstraction_layer.h>
#include <widgets/widget_diff_canvas.h>


namespace QA_QT_DIFF
{

using QA_QT_FRAMES::check;
using QA_QT_FRAMES::settle;


/// Run a function on the next modal dialog (the active modal widget: not another dialog that
/// is shown).
inline QTimer* onNextModal( std::function<void( QDialog* )> aFunction )
{
    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [poll, aFunction]()
                      {
                          QDialog* dialog = qobject_cast<QDialog*>( QApplication::activeModalWidget() );

                          if( dialog && dialog->isVisible() )
                          {
                              poll->stop();
                              poll->deleteLater();
                              std::printf( "    dialog \"%s\" (%s)\n",
                                           dialog->windowTitle().toUtf8().constData(),
                                           dialog->metaObject()->className() );
                              aFunction( dialog );
                          }
                      } );

    poll->start( 20 );
    return poll;
}


/// The text of a file (empty if it cannot be read).
inline QString readText( const QString& aPath )
{
    QFile file( aPath );

    if( !file.open( QIODevice::ReadOnly ) )
        return QString();

    return QString::fromUtf8( file.readAll() );
}


inline bool writeText( const QString& aPath, const QString& aText )
{
    QFile file( aPath );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        return false;

    return file.write( aText.toUtf8() ) >= 0;
}


/**
 * Replace the first s-expression ( @a aHead ... ) of a document by what @a aEdit makes of it
 * (the whole expression, with its parentheses).  @return false if there is none.
 */
inline bool editFirstExpression( QString& aText, const QString& aHead,
                                 const std::function<QString( const QString& )>& aEdit )
{
    int start = aText.indexOf( QStringLiteral( "(" ) + aHead + QLatin1Char( '\n' ) );

    if( start < 0 )
        start = aText.indexOf( QStringLiteral( "(" ) + aHead + QLatin1Char( ' ' ) );

    if( start < 0 )
        return false;

    int  depth = 0;
    bool quoted = false;

    for( int i = start; i < aText.size(); ++i )
    {
        QChar c = aText[i];

        if( c == QLatin1Char( '"' ) && ( i == 0 || aText[i - 1] != QLatin1Char( '\\' ) ) )
            quoted = !quoted;

        if( quoted )
            continue;

        if( c == QLatin1Char( '(' ) )
        {
            ++depth;
        }
        else if( c == QLatin1Char( ')' ) && --depth == 0 )
        {
            QString expression = aText.mid( start, i + 1 - start );
            aText.replace( start, i + 1 - start, aEdit( expression ) );
            return true;
        }
    }

    return false;
}


/// The (width ...) of an expression set to @a aWidth.
inline QString withWidth( const QString& aExpression, const QString& aWidth )
{
    QString result = aExpression;
    int     at = result.indexOf( QStringLiteral( "(width " ) );

    if( at >= 0 )
    {
        int end = result.indexOf( QLatin1Char( ')' ), at );
        result.replace( at, end + 1 - at, QStringLiteral( "(width %1)" ).arg( aWidth ) );
    }

    return result;
}


/// A copy of @a aFile in @a aDir, with the (width ...) of its first ( @a aHead ...) changed.
inline QString editedCopy( const QString& aFile, const QDir& aDir, const QString& aName,
                           const QString& aHead, const QString& aWidth )
{
    QString text = readText( aFile );
    QString path = aDir.filePath( aName );

    bool edited = editFirstExpression( text, aHead,
                                       [&]( const QString& aExpression )
                                       {
                                           return withWidth( aExpression, aWidth );
                                       } );

    if( !edited || !writeText( path, text ) )
        return QString();

    return path;
}


/// The canvases of the comparison in a dialog.
inline std::vector<WIDGET_DIFF_CANVAS*> diffCanvases( QWidget* aDialog )
{
    std::vector<WIDGET_DIFF_CANVAS*> canvases;

    for( QWidget* widget : aDialog->findChildren<QWidget*>() )
    {
        if( WIDGET_DIFF_CANVAS* canvas = dynamic_cast<WIDGET_DIFF_CANVAS*>( widget ) )
            canvases.push_back( canvas );
    }

    return canvases;
}


/**
 * The image of a widget as it is on the display: the canvas of the editor's OpenGL backend is a
 * native window over the widget, which QWidget::grab() does not see.
 */
inline QPixmap grabFromScreen( QWidget* aWidget )
{
    QScreen* screen = aWidget->screen();

    if( !screen )
        return aWidget->grab();

    QPoint origin = aWidget->mapToGlobal( QPoint( 0, 0 ) ) - screen->geometry().topLeft();
    return screen->grabWindow( 0, origin.x(), origin.y(), aWidget->width(), aWidget->height() );
}


/// A canvas draws something: its image has more than the colour of its background.
inline bool drawsSomething( QWidget* aCanvas, const QColor& aBackground = QColor() )
{
    QImage image = grabFromScreen( aCanvas ).toImage();

    if( image.isNull() )
        return false;

    // Not one colour (the background) on nearly all the canvas
    QHash<QRgb, int> counts;
    int              samples = 0;

    for( int y = 0; y < image.height(); y += 4 )
    {
        for( int x = 0; x < image.width(); x += 4 )
        {
            counts[image.pixel( x, y )]++;
            samples++;
        }
    }

    int  most = 0;
    QRgb main = 0;

    for( auto it = counts.cbegin(); it != counts.cend(); ++it )
    {
        if( it.value() > most )
        {
            most = it.value();
            main = it.key();
        }
    }

    std::printf( "    canvas %dx%d: %d colours, the main one (#%06x) on %.1f%%\n", image.width(),
                 image.height(), int( counts.size() ), main & 0xffffff,
                 samples ? 100.0 * most / samples : 0.0 );

    // The main colour is the background of the canvas, not the windows behind it
    if( aBackground.isValid() )
    {
        auto isNear = []( int a, int b ) { return std::abs( a - b ) <= 24; };

        if( !isNear( qRed( main ), aBackground.red() ) || !isNear( qGreen( main ), aBackground.green() )
            || !isNear( qBlue( main ), aBackground.blue() ) )
        {
            std::printf( "    the main colour is not the background #%06x of the canvas\n",
                         aBackground.rgb() & 0xffffff );
            return false;
        }
    }

    return samples > 0 && most < samples * 0.98;
}


/// The colour the GAL of a diff canvas clears its background with.
inline QColor background( WIDGET_DIFF_CANVAS* aCanvas )
{
    const KIGFX::COLOR4D& c = aCanvas->GetGAL()->GetClearColor();
    return QColor::fromRgbF( float( c.r ), float( c.g ), float( c.b ) );
}


/// The number of rows of the trees of a dialog (the changes of the comparison).
inline int treeRows( QWidget* aDialog )
{
    int rows = 0;

    for( QTreeWidget* tree : aDialog->findChildren<QTreeWidget*>() )
    {
        QTreeWidgetItemIterator it( tree );

        while( *it )
        {
            ++rows;
            ++it;
        }
    }

    return rows;
}


/// Close a dialog with Escape (reject); @return false if it stays open (it is then closed by
/// force, so that the test goes on).
inline bool rejectDialog( QDialog* aDialog )
{
    aDialog->reject();
    settle( 200 );

    if( !aDialog->isVisible() )
        return true;

    std::printf( "    the dialog stays open after Escape\n" );
    aDialog->QDialog::done( QDialog::Rejected );
    return false;
}


/// The first item without children of the trees of a dialog (a change).
inline QTreeWidgetItem* firstLeaf( QWidget* aDialog )
{
    for( QTreeWidget* tree : aDialog->findChildren<QTreeWidget*>() )
    {
        QTreeWidgetItemIterator it( tree );

        for( ; *it; ++it )
        {
            if( ( *it )->childCount() == 0 )
                return *it;
        }
    }

    return nullptr;
}


inline void saveShot( QWidget* aDialog, const QString& aScreenshot, const QString& aSuffix )
{
    if( aScreenshot.isEmpty() )
        return;

    QString path = aScreenshot;
    path.replace( QStringLiteral( ".png" ), QStringLiteral( "-%1.png" ).arg( aSuffix ) );
    grabFromScreen( aDialog ).save( path );
}


/**
 * The diff viewer (DIALOG_KICAD_DIFF) of the KIFACE on @a aFile and a copy with the width of its
 * first ( @a aHead ...) changed, then the 3-way merge dialog (DIALOG_KICAD_MERGE_3WAY) on two
 * copies which change that width differently (a conflict), cancelled.
 */
inline void testDiffAndMerge( KIFACE& aKiface, KICAD_DIFF::DOC_KIND aKind, wxWindow* aParent,
                              const QString& aFile, const QString& aHead, const QString& aName,
                              const QString& aScreenshot )
{
    std::printf( "%s: diff and merge\n", aName.toUtf8().constData() );

    QTemporaryDir dir;
    QDir          tmp( dir.path() );
    QString       ext = QFileInfo( aFile ).suffix();
    QString       ancestor = tmp.filePath( QStringLiteral( "ancestor." ) + ext );

    check( QFile::copy( aFile, ancestor ), "copy the document for the comparison" );

    QString ours = editedCopy( aFile, tmp, QStringLiteral( "ours." ) + ext, aHead,
                               QStringLiteral( "0.3" ) );
    QString theirs = editedCopy( aFile, tmp, QStringLiteral( "theirs." ) + ext, aHead,
                                 QStringLiteral( "0.4" ) );

    check( !ours.isEmpty() && !theirs.isEmpty(), "edited copies of the document" );

    if( ours.isEmpty() || theirs.isEmpty() )
        return;

    auto wx = []( const QString& aPath ) { return wxString::FromUTF8( aPath.toUtf8().constData() ); };

    // The diff viewer
    auto openDiff = GetKifaceService<KIFACE_OPEN_DIFF_DIALOG_SERVICE>( &aKiface );
    check( openDiff != nullptr, "the KIFACE opens the diff viewer" );

    if( openDiff )
    {
        bool shown = false;
        int  canvases = 0;
        bool drawn = false;
        bool pickedDrawn = false;
        bool closed = false;
        int  rows = 0;

        QPointer<QTimer> poll = onNextModal( [&]( QDialog* aDialog )
                    {
                        shown = true;
                        settle( 1000 );

                        std::vector<WIDGET_DIFF_CANVAS*> list = diffCanvases( aDialog );
                        canvases = int( list.size() );

                        for( WIDGET_DIFF_CANVAS* canvas : list )
                            drawn = drawn || drawsSomething( canvas, background( canvas ) );

                        rows = treeRows( aDialog );
                        saveShot( aDialog, aScreenshot, aName + QStringLiteral( "-diff" ) );

                        // A change picked in the tree: the canvas shows it
                        if( QTreeWidgetItem* leaf = firstLeaf( aDialog ) )
                        {
                            leaf->treeWidget()->setCurrentItem( leaf );
                            settle( 500 );

                            for( WIDGET_DIFF_CANVAS* canvas : list )
                                pickedDrawn = pickedDrawn || drawsSomething( canvas );

                            saveShot( aDialog, aScreenshot, aName + QStringLiteral( "-diff-change" ) );
                        }

                        closed = rejectDialog( aDialog );
                    } );

        WX_STRING_REPORTER reporter;
        openDiff( static_cast<int>( aKind ), wx( ancestor ), wx( ours ), wxS( "ancestor" ),
                  wxS( "ours" ), aParent, &reporter );
        settle( 200 );

        if( poll && !shown )
            delete poll.data();

        if( reporter.HasMessage() )
            std::printf( "    %s\n", reporter.GetMessages().utf8_str().data() );

        std::printf( "    %d canvas(es), %d row(s) of changes\n", canvases, rows );
        check( shown, "the diff viewer is shown" );
        check( canvases > 0, "the diff viewer has its canvas" );
        check( drawn, "the canvas of the diff viewer draws the documents" );
        check( rows > 0, "the diff viewer lists the change" );
        check( pickedDrawn, "the canvas shows the change picked in the list" );
        check( closed, "Escape closes the diff viewer" );
    }

    // The 3-way merge with a conflict, cancelled
    auto merge = GetKifaceService<KIFACE_MERGE_DOCUMENT_SERVICE>( &aKiface );
    check( merge != nullptr, "the KIFACE merges documents" );

    if( merge )
    {
        bool shown = false;
        int  canvases = 0;
        bool drawn = false;
        bool closed = false;

        QPointer<QTimer> poll = onNextModal( [&]( QDialog* aDialog )
                    {
                        shown = true;
                        settle( 500 );

                        std::vector<WIDGET_DIFF_CANVAS*> list = diffCanvases( aDialog );
                        canvases = int( list.size() );

                        for( WIDGET_DIFF_CANVAS* canvas : list )
                            drawn = drawn || drawsSomething( canvas );

                        saveShot( aDialog, aScreenshot, aName + QStringLiteral( "-merge" ) );
                        closed = rejectDialog( aDialog );
                    } );

        QString            merged = tmp.filePath( QStringLiteral( "merged." ) + ext );
        WX_STRING_REPORTER reporter;
        int result = merge( static_cast<int>( aKind ), wx( ancestor ), wx( ours ), wx( theirs ),
                            wx( merged ), true, false, &reporter );
        settle( 200 );

        if( poll && !shown )
            delete poll.data();

        if( reporter.HasMessage() )
            std::printf( "    %s\n", reporter.GetMessages().utf8_str().data() );

        std::printf( "    merge: exit code %d, %d canvas(es)\n", result, canvases );
        check( shown, "the 3-way merge dialog is shown for the conflict" );
        check( canvases > 0, "the 3-way merge dialog has its canvas" );
        check( drawn, "the canvas of the 3-way merge dialog draws the document" );
        check( closed, "Escape closes the 3-way merge dialog" );
        check( result == CLI::EXIT_CODES::ERR_CANCELLED, "the cancelled merge is cancelled" );
    }
}

} // namespace QA_QT_DIFF
