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

#include <wx/kiwx_stc.h>

#include <algorithm>

#include <QFile>
#include <QKeyEvent>
#include <QListWidget>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextStream>

#include <qt_strings.h>


wxStyledTextCtrl::wxStyledTextCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                                    const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxTextCtrlMultiLine( aParent ),
        m_eolMode( wxSTC_EOL_LF ),
        m_tabWidth( 8 ),
        m_zoom( 0 ),
        m_searchAnchor( 0 ),
        m_undoDepth( 0 ),
        m_braceLight1( -1 ),
        m_braceLight2( -1 ),
        m_braceBad( false ),
        m_autoCompStart( 0 ),
        m_autoCompSeparator( ' ' ),
        m_autoCompIgnoreCase( false ),
        m_autoCompMaxRows( 5 ),
        m_autoCompMaxWidth( 0 ),
        m_autoCompFillUps(),
        m_autoCompStops()
{
    SetId( aId );

    if( !aName.IsEmpty() )
        SetName( aName );

    // Scintilla doesn't wrap the lines by default
    setLineWrapMode( QPlainTextEdit::NoWrap );
    SetTabWidth( m_tabWidth );

    if( aStyle & wxBORDER_NONE )
        setFrameShape( QFrame::NoFrame );

    // The events of Scintilla
    QObject::connect( this, &QPlainTextEdit::textChanged, this,
                      [this]()
                      {
                          sendEvent( wxEVT_STC_MODIFIED, 0, GetCurrentPos() );
                          sendEvent( wxEVT_STC_CHANGE, 0, GetCurrentPos() );
                          sendEvent( wxEVT_STC_UPDATEUI );
                      } );

    QObject::connect( this, &QPlainTextEdit::cursorPositionChanged, this,
                      [this]() { sendEvent( wxEVT_STC_UPDATEUI ); } );

    QObject::connect( this, &QPlainTextEdit::selectionChanged, this,
                      [this]() { sendEvent( wxEVT_STC_UPDATEUI ); } );
}


wxStyledTextCtrl::~wxStyledTextCtrl()
{
    delete m_autoComp;
}


void wxStyledTextCtrl::sendEvent( wxEventType aType, int aKey, int aPosition )
{
    wxStyledTextEvent event( aType, GetId() );
    event.SetEventObject( this );
    event.SetKey( aKey );
    event.SetPosition( aPosition );

    if( aType == wxEVT_STC_MODIFIED )
        event.SetModificationType( wxSTC_MOD_INSERTTEXT | wxSTC_PERFORMED_USER );

    ProcessEvent( event );
}


// The text ---------------------------------------------------------------------------------

void wxStyledTextCtrl::SetText( const wxString& aText )
{
    // Undoable, as the SetText() of Scintilla
    QTextCursor cursor = editCursor();
    cursor.select( QTextCursor::Document );
    cursor.insertText( ToQString( aText ) );
    applyEditCursor( cursor );
}


void wxStyledTextCtrl::Clear()
{
    QTextCursor cursor = editCursor();
    cursor.removeSelectedText();
    applyEditCursor( cursor );
}


int wxStyledTextCtrl::GetCharAt( int aPos ) const
{
    if( aPos < 0 )
        return 0;

    QChar c = document()->characterAt( aPos );

    // The paragraph separators of the document are the ends of the lines
    if( c == QChar::ParagraphSeparator || c == QChar::LineSeparator )
        return '\n';

    return c.isNull() ? 0 : c.unicode();
}


void wxStyledTextCtrl::AddText( const wxString& aText )
{
    QTextCursor cursor = editCursor();
    cursor.clearSelection();
    cursor.insertText( ToQString( aText ) );
    applyEditCursor( cursor );
}


void wxStyledTextCtrl::InsertText( int aPos, const wxString& aText )
{
    // The caret and the selection stay on their text
    QTextCursor cursor( document() );
    cursor.setPosition( std::clamp( aPos, 0, GetTextLength() ) );

    if( m_undoDepth > 0 )
        cursor.joinPreviousEditBlock();

    cursor.insertText( ToQString( aText ) );

    if( m_undoDepth > 0 )
        cursor.endEditBlock();
}


void wxStyledTextCtrl::DeleteRange( int aStart, int aLength )
{
    QTextCursor cursor( document() );
    int         length = GetTextLength();

    cursor.setPosition( std::clamp( aStart, 0, length ) );
    cursor.setPosition( std::clamp( aStart + aLength, 0, length ), QTextCursor::KeepAnchor );

    if( m_undoDepth > 0 )
        cursor.joinPreviousEditBlock();

    cursor.removeSelectedText();

    if( m_undoDepth > 0 )
        cursor.endEditBlock();
}


void wxStyledTextCtrl::DeleteBack()
{
    QTextCursor cursor = editCursor();

    if( cursor.hasSelection() )
        cursor.removeSelectedText();
    else
        cursor.deletePreviousChar();

    applyEditCursor( cursor );

    // The autocompletion follows the deletions, until its start
    if( AutoCompActive() )
    {
        if( GetCurrentPos() < m_autoCompStart )
            AutoCompCancel();
        else
            sendEvent( wxEVT_STC_AUTOCOMP_CHAR_DELETED, 0, GetCurrentPos() );
    }
}


// The lines --------------------------------------------------------------------------------

int wxStyledTextCtrl::GetLineCount() const
{
    return document()->blockCount();
}


wxString wxStyledTextCtrl::GetLine( int aLine ) const
{
    QTextBlock block = document()->findBlockByNumber( aLine );

    if( !block.isValid() )
        return wxString();

    QString text = block.text();

    if( block.next().isValid() )
        text += '\n';

    return ToWxString( text );
}


int wxStyledTextCtrl::GetLineLength( int aLine ) const
{
    QTextBlock block = document()->findBlockByNumber( aLine );

    if( !block.isValid() )
        return 0;

    return block.length() - ( block.next().isValid() ? 0 : 1 );
}


int wxStyledTextCtrl::LineFromPosition( int aPos ) const
{
    QTextBlock block = document()->findBlock( std::clamp( aPos, 0, GetTextLength() ) );
    return block.isValid() ? block.blockNumber() : 0;
}


int wxStyledTextCtrl::PositionFromLine( int aLine ) const
{
    if( aLine < 0 )
        return 0;

    QTextBlock block = document()->findBlockByNumber( aLine );
    return block.isValid() ? block.position() : GetTextLength();
}


int wxStyledTextCtrl::GetLineEndPosition( int aLine ) const
{
    QTextBlock block = document()->findBlockByNumber( aLine );
    return block.isValid() ? block.position() + block.length() - 1 : GetTextLength();
}


int wxStyledTextCtrl::GetColumn( int aPos ) const
{
    QTextBlock block = document()->findBlock( std::clamp( aPos, 0, GetTextLength() ) );

    if( !block.isValid() )
        return 0;

    // The tabs go to the next tab stop
    QString text = block.text();
    int     column = 0;

    for( int i = 0; i < aPos - block.position() && i < text.size(); ++i )
        column = text[i] == '\t' ? ( column / m_tabWidth + 1 ) * m_tabWidth : column + 1;

    return column;
}


int wxStyledTextCtrl::GetFirstVisibleLine() const
{
    return firstVisibleBlock().blockNumber();
}


void wxStyledTextCtrl::ScrollToLine( int aLine )
{
    verticalScrollBar()->setValue( aLine );
}


void wxStyledTextCtrl::LineScroll( int aColumns, int aLines )
{
    verticalScrollBar()->setValue( verticalScrollBar()->value() + aLines );
    horizontalScrollBar()->setValue( horizontalScrollBar()->value()
                                     + aColumns * fontMetrics().averageCharWidth() );
}


void wxStyledTextCtrl::ScrollToStart()
{
    horizontalScrollBar()->setValue( 0 );
    verticalScrollBar()->setValue( 0 );
}


void wxStyledTextCtrl::ScrollToEnd()
{
    verticalScrollBar()->setValue( verticalScrollBar()->maximum() );
}


// The caret and the selection --------------------------------------------------------------

void wxStyledTextCtrl::SetCurrentPos( int aPos )
{
    // The anchor stays: the selection goes from the anchor to the caret
    QTextCursor cursor = textCursor();
    cursor.setPosition( std::clamp( aPos, 0, GetTextLength() ), QTextCursor::KeepAnchor );
    setTextCursor( cursor );
}


int wxStyledTextCtrl::GetAnchor() const
{
    return textCursor().anchor();
}


void wxStyledTextCtrl::SetAnchor( int aPos )
{
    QTextCursor cursor = textCursor();
    int         position = cursor.position();

    cursor.setPosition( std::clamp( aPos, 0, GetTextLength() ) );
    cursor.setPosition( position, QTextCursor::KeepAnchor );
    setTextCursor( cursor );
}


void wxStyledTextCtrl::GotoPos( int aPos )
{
    QTextCursor cursor = textCursor();
    cursor.setPosition( std::clamp( aPos, 0, GetTextLength() ) );
    setTextCursor( cursor );
    ensureCursorVisible();
}


int wxStyledTextCtrl::GetSelectionStart() const
{
    return textCursor().selectionStart();
}


int wxStyledTextCtrl::GetSelectionEnd() const
{
    return textCursor().selectionEnd();
}


void wxStyledTextCtrl::SetSelectionStart( int aPos )
{
    SetSelection( aPos, std::max( aPos, GetSelectionEnd() ) );
}


void wxStyledTextCtrl::SetSelectionEnd( int aPos )
{
    SetSelection( std::min( aPos, GetSelectionStart() ), aPos );
}


static bool isWordChar( QChar aChar )
{
    return aChar.isLetterOrNumber() || aChar == '_';
}


int wxStyledTextCtrl::WordStartPosition( int aPos, bool aOnlyWordCharacters ) const
{
    QString text = toPlainText();
    int     pos = std::clamp( aPos, 0, int( text.size() ) );

    if( aOnlyWordCharacters )
    {
        while( pos > 0 && isWordChar( text[pos - 1] ) )
            --pos;
    }
    else
    {
        bool word = pos > 0 && isWordChar( text[pos - 1] );

        while( pos > 0 && !text[pos - 1].isSpace() && isWordChar( text[pos - 1] ) == word )
            --pos;
    }

    return pos;
}


int wxStyledTextCtrl::WordEndPosition( int aPos, bool aOnlyWordCharacters ) const
{
    QString text = toPlainText();
    int     pos = std::clamp( aPos, 0, int( text.size() ) );

    if( aOnlyWordCharacters )
    {
        while( pos < text.size() && isWordChar( text[pos] ) )
            ++pos;
    }
    else
    {
        bool word = pos < text.size() && isWordChar( text[pos] );

        while( pos < text.size() && !text[pos].isSpace() && isWordChar( text[pos] ) == word )
            ++pos;
    }

    return pos;
}


int wxStyledTextCtrl::FindText( int aMinPos, int aMaxPos, const wxString& aText, int aFlags,
                                int* aFindEnd ) const
{
    QString             text = toPlainText();
    QString             target = ToQString( aText );
    Qt::CaseSensitivity sensitivity = ( aFlags & wxSTC_FIND_MATCHCASE ) ? Qt::CaseSensitive
                                                                       : Qt::CaseInsensitive;
    bool                backwards = aMaxPos < aMinPos;
    int                 low = std::clamp( std::min( aMinPos, aMaxPos ), 0, int( text.size() ) );
    int                 high = std::clamp( std::max( aMinPos, aMaxPos ), 0, int( text.size() ) );
    QString             range = text.mid( low, high - low );
    qsizetype           found = backwards ? range.lastIndexOf( target, -1, sensitivity )
                                          : range.indexOf( target, 0, sensitivity );

    if( found < 0 || target.isEmpty() )
        return -1;

    if( aFindEnd )
        *aFindEnd = int( low + found + target.size() );

    return int( low + found );
}


int wxStyledTextCtrl::SearchNext( int aFlags, const wxString& aText )
{
    int end = 0;
    int pos = FindText( m_searchAnchor, GetTextLength(), aText, aFlags, &end );

    if( pos >= 0 )
        SetSelection( pos, end );

    return pos;
}


int wxStyledTextCtrl::SearchPrev( int aFlags, const wxString& aText )
{
    int end = 0;
    int pos = FindText( m_searchAnchor, 0, aText, aFlags, &end );

    if( pos >= 0 )
        SetSelection( pos, end );

    return pos;
}


void wxStyledTextCtrl::moveCaret( QTextCursor::MoveOperation aOperation, bool aExtend )
{
    QTextCursor cursor = textCursor();
    cursor.movePosition( aOperation, aExtend ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor );
    setTextCursor( cursor );
}


// The undo ---------------------------------------------------------------------------------

QTextCursor wxStyledTextCtrl::editCursor() const
{
    QTextCursor cursor = textCursor();

    // The edits of an undo action join its edit block
    if( m_undoDepth > 0 )
    {
        QTextCursor block = m_undoCursor;
        block.setPosition( cursor.anchor() );
        block.setPosition( cursor.position(), QTextCursor::KeepAnchor );
        return block;
    }

    return cursor;
}


void wxStyledTextCtrl::applyEditCursor( const QTextCursor& aCursor )
{
    QTextCursor cursor = textCursor();
    cursor.setPosition( aCursor.anchor() );
    cursor.setPosition( aCursor.position(), QTextCursor::KeepAnchor );
    setTextCursor( cursor );
}


void wxStyledTextCtrl::BeginUndoAction()
{
    if( m_undoDepth++ == 0 )
    {
        m_undoCursor = textCursor();
        m_undoCursor.beginEditBlock();
    }
}


void wxStyledTextCtrl::EndUndoAction()
{
    if( m_undoDepth > 0 && --m_undoDepth == 0 )
        m_undoCursor.endEditBlock();
}


// The braces -------------------------------------------------------------------------------

int wxStyledTextCtrl::BraceMatch( int aPos, int aMaxReStyle ) const
{
    static const QString s_open = QStringLiteral( "([{<" );
    static const QString s_close = QStringLiteral( ")]}>" );

    QString text = toPlainText();

    if( aPos < 0 || aPos >= text.size() )
        return -1;

    QChar     c = text[aPos];
    qsizetype open = s_open.indexOf( c );
    qsizetype close = s_close.indexOf( c );

    if( open < 0 && close < 0 )
        return -1;

    QChar match = open >= 0 ? s_close[open] : s_open[close];
    int   step = open >= 0 ? 1 : -1;
    int   depth = 0;

    for( int i = aPos; i >= 0 && i < text.size(); i += step )
    {
        if( text[i] == c )
            ++depth;
        else if( text[i] == match && --depth == 0 )
            return i;
    }

    return -1;
}


void wxStyledTextCtrl::BraceHighlight( int aPos1, int aPos2 )
{
    m_braceLight1 = aPos1;
    m_braceLight2 = aPos2;
    m_braceBad = false;
    updateExtraSelections();
}


void wxStyledTextCtrl::BraceBadLight( int aPos )
{
    m_braceLight1 = aPos;
    m_braceLight2 = -1;
    m_braceBad = true;
    updateExtraSelections();
}


void wxStyledTextCtrl::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> selections;

    auto addBrace =
            [&]( int aPos, bool aBad )
            {
                if( aPos < 0 || aPos >= GetTextLength() )
                    return;

                QTextEdit::ExtraSelection selection;
                selection.cursor = QTextCursor( document() );
                selection.cursor.setPosition( aPos );
                selection.cursor.setPosition( aPos + 1, QTextCursor::KeepAnchor );

                if( aBad )
                {
                    selection.format.setForeground( m_braceBadForeground.IsOk()
                                                            ? QColor( m_braceBadForeground )
                                                            : QColor( Qt::red ) );
                }
                else
                {
                    if( m_braceLightForeground.IsOk() )
                        selection.format.setForeground( QColor( m_braceLightForeground ) );

                    selection.format.setBackground( m_braceLightBackground.IsOk()
                                                            ? QColor( m_braceLightBackground )
                                                            : palette().color( QPalette::Midlight ) );
                }

                selections.push_back( selection );
            };

    addBrace( m_braceLight1, m_braceBad );

    if( !m_braceBad )
        addBrace( m_braceLight2, false );

    setExtraSelections( selections );
}


// The autocompletion -----------------------------------------------------------------------

void wxStyledTextCtrl::AutoCompShow( int aLenEntered, const wxString& aItemList )
{
    if( !m_autoComp )
    {
        // A list that doesn't take the focus: the keys stay in the editor
        m_autoComp = new QListWidget( this );
        m_autoComp->setWindowFlags( Qt::ToolTip | Qt::FramelessWindowHint );
        m_autoComp->setAttribute( Qt::WA_ShowWithoutActivating );
        m_autoComp->setFocusPolicy( Qt::NoFocus );
        m_autoComp->setSelectionMode( QAbstractItemView::SingleSelection );

        QObject::connect( m_autoComp, &QListWidget::itemClicked, this,
                          [this]( QListWidgetItem* aItem )
                          {
                              m_autoComp->setCurrentItem( aItem );
                              AutoCompComplete();
                          } );
    }

    m_autoCompStart = std::max( GetCurrentPos() - aLenEntered, 0 );

    m_autoComp->clear();

    for( const QString& item : ToQString( aItemList ).split( QChar( m_autoCompSeparator ),
                                                             Qt::SkipEmptyParts ) )
    {
        m_autoComp->addItem( item );
    }

    if( m_autoComp->count() == 0 )
    {
        AutoCompCancel();
        return;
    }

    // The first item starting with the typed text, or the first one
    AutoCompSelect( GetTextRange( m_autoCompStart, GetCurrentPos() ) );

    if( !m_autoComp->currentItem() )
        m_autoComp->setCurrentRow( 0 );

    updateAutoCompSize();

    QRect  caret = cursorRect();
    QPoint pos = viewport()->mapToGlobal( caret.bottomLeft() );
    m_autoComp->move( pos );
    m_autoComp->show();
}


void wxStyledTextCtrl::updateAutoCompSize()
{
    QFontMetrics metrics = m_autoComp->fontMetrics();
    int          width = 0;

    for( int i = 0; i < m_autoComp->count(); ++i )
        width = std::max( width, metrics.horizontalAdvance( m_autoComp->item( i )->text() ) );

    if( m_autoCompMaxWidth > 0 )
        width = std::min( width, m_autoCompMaxWidth * metrics.averageCharWidth() );

    int rows = std::min( m_autoComp->count(), std::max( m_autoCompMaxRows, 1 ) );
    int rowHeight = m_autoComp->sizeHintForRow( 0 );
    int frame = 2 * m_autoComp->frameWidth();

    m_autoComp->resize( width + frame + m_autoComp->verticalScrollBar()->sizeHint().width() + 8,
                        rows * rowHeight + frame );
}


void wxStyledTextCtrl::AutoCompCancel()
{
    if( m_autoComp && m_autoComp->isVisible() )
    {
        m_autoComp->hide();
        sendEvent( wxEVT_STC_AUTOCOMP_CANCELLED );
    }
}


bool wxStyledTextCtrl::AutoCompActive() const
{
    return m_autoComp && m_autoComp->isVisible();
}


void wxStyledTextCtrl::AutoCompComplete()
{
    if( !AutoCompActive() )
        return;

    QListWidgetItem* item = m_autoComp->currentItem();
    m_autoComp->hide();

    if( !item )
        return;

    wxString text = ToWxString( item->text() );

    sendEvent( wxEVT_STC_AUTOCOMP_SELECTION, 0, m_autoCompStart );

    // The typed text is replaced by the item
    QTextCursor cursor = editCursor();
    cursor.setPosition( std::min( m_autoCompStart, GetTextLength() ) );
    cursor.setPosition( GetCurrentPos(), QTextCursor::KeepAnchor );
    cursor.insertText( ToQString( text ) );
    applyEditCursor( cursor );

    sendEvent( wxEVT_STC_AUTOCOMP_COMPLETED, 0, m_autoCompStart );
}


void wxStyledTextCtrl::AutoCompSelect( const wxString& aText )
{
    if( !m_autoComp )
        return;

    QString             prefix = ToQString( aText );
    Qt::CaseSensitivity sensitivity = m_autoCompIgnoreCase ? Qt::CaseInsensitive
                                                           : Qt::CaseSensitive;

    for( int i = 0; i < m_autoComp->count(); ++i )
    {
        if( m_autoComp->item( i )->text().startsWith( prefix, sensitivity ) )
        {
            m_autoComp->setCurrentRow( i );
            return;
        }
    }

    m_autoComp->setCurrentItem( nullptr );
}


int wxStyledTextCtrl::AutoCompGetCurrent() const
{
    return m_autoComp ? m_autoComp->currentRow() : -1;
}


void wxStyledTextCtrl::keyPressEvent( QKeyEvent* aEvent )
{
    if( AutoCompActive() )
    {
        int row = m_autoComp->currentRow();
        int page = std::max( m_autoCompMaxRows - 1, 1 );

        switch( aEvent->key() )
        {
        case Qt::Key_Up:
            m_autoComp->setCurrentRow( std::max( row - 1, 0 ) );
            return;

        case Qt::Key_Down:
            m_autoComp->setCurrentRow( std::min( row + 1, m_autoComp->count() - 1 ) );
            return;

        case Qt::Key_PageUp:
            m_autoComp->setCurrentRow( std::max( row - page, 0 ) );
            return;

        case Qt::Key_PageDown:
            m_autoComp->setCurrentRow( std::min( row + page, m_autoComp->count() - 1 ) );
            return;

        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            AutoCompComplete();
            return;

        case Qt::Key_Escape:
            AutoCompCancel();
            return;

        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Home:
        case Qt::Key_End:
            AutoCompCancel();
            break;

        default:
            if( !aEvent->text().isEmpty() )
            {
                QChar c = aEvent->text().front();

                if( m_autoCompFillUps.Find( wxChar( c.unicode() ) ) != wxNOT_FOUND )
                    AutoCompComplete();
                else if( m_autoCompStops.Find( wxChar( c.unicode() ) ) != wxNOT_FOUND )
                    AutoCompCancel();
            }

            break;
        }
    }

    int     before = GetTextLength();
    QString text = aEvent->text();

    wxTextCtrlMultiLine::keyPressEvent( aEvent );

    // A character typed in the text (the Return of the line ends included)
    if( !text.isEmpty() && GetTextLength() != before && text.front().isPrint() )
        sendEvent( wxEVT_STC_CHARADDED, text.front().unicode(), GetCurrentPos() );
    else if( ( aEvent->key() == Qt::Key_Return || aEvent->key() == Qt::Key_Enter )
             && GetTextLength() != before )
        sendEvent( wxEVT_STC_CHARADDED, '\n', GetCurrentPos() );

    // The list follows the typed text
    if( AutoCompActive() )
        AutoCompSelect( GetTextRange( m_autoCompStart, GetCurrentPos() ) );
}


void wxStyledTextCtrl::focusOutEvent( QFocusEvent* aEvent )
{
    AutoCompCancel();
    wxTextCtrlMultiLine::focusOutEvent( aEvent );
}


// The appearance ---------------------------------------------------------------------------

void wxStyledTextCtrl::StyleSetForeground( int aStyle, const wxColour& aColour )
{
    if( aStyle == wxSTC_STYLE_DEFAULT )
    {
        QPalette pal = palette();
        pal.setColor( QPalette::Text, aColour );
        setPalette( pal );
    }
    else if( aStyle == wxSTC_STYLE_BRACELIGHT )
    {
        m_braceLightForeground = aColour;
    }
    else if( aStyle == wxSTC_STYLE_BRACEBAD )
    {
        m_braceBadForeground = aColour;
    }
}


void wxStyledTextCtrl::StyleSetBackground( int aStyle, const wxColour& aColour )
{
    if( aStyle == wxSTC_STYLE_DEFAULT )
    {
        QPalette pal = palette();
        pal.setColor( QPalette::Base, aColour );
        setPalette( pal );
    }
    else if( aStyle == wxSTC_STYLE_BRACELIGHT )
    {
        m_braceLightBackground = aColour;
    }
}


void wxStyledTextCtrl::StyleSetFont( int aStyle, const wxFont& aFont )
{
    if( aStyle == wxSTC_STYLE_DEFAULT || aStyle == 0 )
    {
        setFont( aFont );
        SetTabWidth( m_tabWidth );
    }
}


void wxStyledTextCtrl::SetSelForeground( bool aUse, const wxColour& aColour )
{
    if( aUse )
    {
        QPalette pal = palette();
        pal.setColor( QPalette::HighlightedText, aColour );
        setPalette( pal );
    }
}


void wxStyledTextCtrl::SetSelBackground( bool aUse, const wxColour& aColour )
{
    if( aUse )
    {
        QPalette pal = palette();
        pal.setColor( QPalette::Highlight, aColour );
        setPalette( pal );
    }
}


void wxStyledTextCtrl::SetTabWidth( int aWidth )
{
    m_tabWidth = std::max( aWidth, 1 );
    setTabStopDistance( m_tabWidth * fontMetrics().horizontalAdvance( ' ' ) );
}


void wxStyledTextCtrl::SetViewWhiteSpace( int aView )
{
    QTextOption option = document()->defaultTextOption();
    option.setFlags( aView == wxSTC_WS_INVISIBLE
                             ? option.flags() & ~QTextOption::ShowTabsAndSpaces
                             : option.flags() | QTextOption::ShowTabsAndSpaces );
    document()->setDefaultTextOption( option );
}


void wxStyledTextCtrl::SetWrapMode( int aMode )
{
    setLineWrapMode( aMode == wxSTC_WRAP_NONE ? QPlainTextEdit::NoWrap
                                              : QPlainTextEdit::WidgetWidth );
}


void wxStyledTextCtrl::SetUseHorizontalScrollBar( bool aShow )
{
    setHorizontalScrollBarPolicy( aShow ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );
}


void wxStyledTextCtrl::SetUseVerticalScrollBar( bool aShow )
{
    setVerticalScrollBarPolicy( aShow ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );
}


int wxStyledTextCtrl::GetMarginWidth( int aMargin ) const
{
    auto it = m_marginWidths.find( aMargin );
    return it != m_marginWidths.end() ? it->second : 0;
}


void wxStyledTextCtrl::SetMarginLeft( int aWidth )
{
    QMargins margins = viewportMargins();
    margins.setLeft( aWidth );
    setViewportMargins( margins );
}


void wxStyledTextCtrl::SetMarginRight( int aWidth )
{
    QMargins margins = viewportMargins();
    margins.setRight( aWidth );
    setViewportMargins( margins );
}


int wxStyledTextCtrl::GetMarginLeft() const
{
    return viewportMargins().left();
}


int wxStyledTextCtrl::GetMarginRight() const
{
    return viewportMargins().right();
}


void wxStyledTextCtrl::UsePopUp( int aPopUpMode )
{
    setContextMenuPolicy( aPopUpMode == wxSTC_POPUP_NEVER ? Qt::NoContextMenu
                                                          : Qt::DefaultContextMenu );
}


void wxStyledTextCtrl::SetZoom( int aZoom )
{
    if( aZoom > m_zoom )
        zoomIn( aZoom - m_zoom );
    else if( aZoom < m_zoom )
        zoomOut( m_zoom - aZoom );

    m_zoom = aZoom;
    SetTabWidth( m_tabWidth );
    sendEvent( wxEVT_STC_ZOOM );
}


int wxStyledTextCtrl::TextWidth( int aStyle, const wxString& aText ) const
{
    return fontMetrics().horizontalAdvance( ToQString( aText ) );
}


bool wxStyledTextCtrl::SaveFile( const wxString& aFileName )
{
    QFile file( ToQString( aFileName ) );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
        return false;

    file.write( toPlainText().toUtf8() );
    SetSavePoint();
    return true;
}


bool wxStyledTextCtrl::LoadFile( const wxString& aFileName )
{
    QFile file( ToQString( aFileName ) );

    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        return false;

    SetValue( ToWxString( QString::fromUtf8( file.readAll() ) ) );
    EmptyUndoBuffer();
    return true;
}
