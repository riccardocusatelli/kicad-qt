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
 * @file kiwx_stc.h
 * The styled text control of the wxWidgets compatibility layer of the Qt build (wxcompat):
 * wxStyledTextCtrl, the API of Scintilla that KiCad uses on a QPlainTextEdit.  The positions are
 * the indices of the characters of the text; the lines are the blocks of the document.  The
 * styles of the lexers are not drawn: the default style gives the font and the colours.
 */

#ifndef KIWX_STC_H
#define KIWX_STC_H

#include <kiqt.h>

#include <map>

#include <QPointer>

#include <wx/kiwx_gui.h>

class QListWidget;


#define wxSTC_INVALID_POSITION -1

#define wxSTC_EOL_CRLF 0
#define wxSTC_EOL_CR 1
#define wxSTC_EOL_LF 2

#define wxSTC_STYLE_DEFAULT 32
#define wxSTC_STYLE_LINENUMBER 33
#define wxSTC_STYLE_BRACELIGHT 34
#define wxSTC_STYLE_BRACEBAD 35
#define wxSTC_STYLE_CONTROLCHAR 36
#define wxSTC_STYLE_INDENTGUIDE 37
#define wxSTC_STYLE_MAX 255

#define wxSTC_MARGIN_SYMBOL 0
#define wxSTC_MARGIN_NUMBER 1

#define wxSTC_MARKNUM_FOLDEREND 25
#define wxSTC_MARKNUM_FOLDEROPENMID 26
#define wxSTC_MARKNUM_FOLDERMIDTAIL 27
#define wxSTC_MARKNUM_FOLDERTAIL 28
#define wxSTC_MARKNUM_FOLDERSUB 29
#define wxSTC_MARKNUM_FOLDER 30
#define wxSTC_MARKNUM_FOLDEROPEN 31
#define wxSTC_MASK_FOLDERS 0xFE000000

#define wxSTC_MARK_CIRCLE 0
#define wxSTC_MARK_ARROW 2
#define wxSTC_MARK_EMPTY 5
#define wxSTC_MARK_BOXPLUS 12
#define wxSTC_MARK_BOXMINUS 14

#define wxSTC_INDIC_IME 32
#define wxSTC_INDIC_IME_MAX 35

#define wxSTC_FIND_WHOLEWORD 0x2
#define wxSTC_FIND_MATCHCASE 0x4
#define wxSTC_FIND_WORDSTART 0x00100000
#define wxSTC_FIND_REGEXP 0x00200000

#define wxSTC_WRAP_NONE 0
#define wxSTC_WRAP_WORD 1

#define wxSTC_WS_INVISIBLE 0
#define wxSTC_WS_VISIBLEALWAYS 1

#define wxSTC_MOD_INSERTTEXT 0x1
#define wxSTC_MOD_DELETETEXT 0x2
#define wxSTC_PERFORMED_USER 0x10

#define wxSTC_LEX_CONTAINER 0
#define wxSTC_LEX_NULL 1
#define wxSTC_LEX_SPICE 78

#define wxSTC_TECHNOLOGY_DEFAULT 0
#define wxSTC_TECHNOLOGY_DIRECTWRITE 1

#define wxSTC_POPUP_NEVER 0
#define wxSTC_POPUP_ALL 1

#define wxSTCNameStr wxString( "stcwindow" )


/// An event of a styled text control (wxStyledTextEvent).
class KIQT_API wxStyledTextEvent : public wxCommandEvent
{
public:
    explicit wxStyledTextEvent( wxEventType aType = wxEVT_NULL, int aId = 0 ) :
            wxCommandEvent( aType, aId )
    {
    }

    int GetKey() const { return m_key; }
    void SetKey( int aKey ) { m_key = aKey; }
    int GetModifiers() const { return m_modifiers; }
    void SetModifiers( int aModifiers ) { m_modifiers = aModifiers; }
    int GetPosition() const { return m_position; }
    void SetPosition( int aPosition ) { m_position = aPosition; }
    int GetLength() const { return m_length; }
    void SetLength( int aLength ) { m_length = aLength; }
    int GetModificationType() const { return m_modificationType; }
    void SetModificationType( int aType ) { m_modificationType = aType; }
    int GetLinesAdded() const { return m_linesAdded; }
    int GetLine() const { return m_line; }
    int GetUpdated() const { return 0; }
    wxString GetText() const { return GetString(); }
    void SetText( const wxString& aText ) { SetString( aText ); }

private:
    int m_key = 0;
    int m_modifiers = 0;
    int m_position = 0;
    int m_length = 0;
    int m_modificationType = 0;
    int m_linesAdded = 0;
    int m_line = 0;
};

#define wxStyledTextEventHandler( func ) ( &func )


/// A text editor with the API of Scintilla (wxStyledTextCtrl).
class KIQT_API wxStyledTextCtrl : public wxTextCtrlMultiLine
{
public:
    explicit wxStyledTextCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                               const wxPoint& aPos = wxDefaultPosition,
                               const wxSize& aSize = wxDefaultSize, long aStyle = 0,
                               const wxString& aName = wxSTCNameStr );
    ~wxStyledTextCtrl() override;

    // The text

    wxString GetText() const { return GetValue(); }
    void SetText( const wxString& aText );
    void ClearAll() { SetText( wxEmptyString ); }

    /// Delete the selection (the Clear() of Scintilla).
    void Clear() override;

    int GetTextLength() const { return int( GetLastPosition() ); }
    int GetLength() const { return GetTextLength(); }
    int GetCharAt( int aPos ) const;
    wxString GetTextRange( int aStart, int aEnd ) const { return GetRange( aStart, aEnd ); }
    wxString GetSelectedText() const { return GetStringSelection(); }

    /// Insert a text at the caret (the selection stays).
    void AddText( const wxString& aText );
    void InsertText( int aPos, const wxString& aText );
    void DeleteRange( int aStart, int aLength );
    void ReplaceSelection( const wxString& aText ) { WriteText( aText ); }

    /// Delete the selection, or the character before the caret.
    void DeleteBack();
    void DeleteBackNotLine() { DeleteBack(); }
    void Tab() { WriteText( wxS( "\t" ) ); }
    void NewLine() { WriteText( wxS( "\n" ) ); }

    void SetReadOnly( bool aReadOnly ) { SetEditable( !aReadOnly ); }
    bool GetReadOnly() const { return !IsEditable(); }

    bool GetModify() const { return IsModified(); }
    void SetSavePoint() { SetModified( false ); }

    // The lines

    int GetLineCount() const;
    wxString GetLine( int aLine ) const;

    /// The length of a line, with its end of line.
    int GetLineLength( int aLine ) const;
    int LineFromPosition( int aPos ) const;
    int PositionFromLine( int aLine ) const;
    int GetLineEndPosition( int aLine ) const;
    int GetColumn( int aPos ) const;
    int GetCurrentLine() const { return LineFromPosition( GetCurrentPos() ); }
    int GetFirstVisibleLine() const;
    void ScrollToLine( int aLine );
    void LineScroll( int aColumns, int aLines );
    void EnsureCaretVisible() { ensureCursorVisible(); }
    void ScrollToStart();
    void ScrollToEnd();

    // The caret and the selection

    int GetCurrentPos() const { return int( GetInsertionPoint() ); }
    void SetCurrentPos( int aPos );
    int GetAnchor() const;
    void SetAnchor( int aPos );
    void GotoPos( int aPos );
    void GotoLine( int aLine ) { GotoPos( PositionFromLine( aLine ) ); }
    int GetSelectionStart() const;
    int GetSelectionEnd() const;
    void SetSelectionStart( int aPos );
    void SetSelectionEnd( int aPos );
    void SetEmptySelection( int aPos ) { GotoPos( aPos ); }

    using wxTextCtrlMultiLine::SetSelection;
    using wxTextCtrlMultiLine::GetSelection;

    int WordStartPosition( int aPos, bool aOnlyWordCharacters ) const;
    int WordEndPosition( int aPos, bool aOnlyWordCharacters ) const;

    /// The position of a text in [aMinPos, aMaxPos] (backwards if aMaxPos < aMinPos), or -1.
    int FindText( int aMinPos, int aMaxPos, const wxString& aText, int aFlags = 0,
                  int* aFindEnd = nullptr ) const;
    void SearchAnchor() { m_searchAnchor = GetCurrentPos(); }
    int SearchNext( int aFlags, const wxString& aText );
    int SearchPrev( int aFlags, const wxString& aText );

    // The moves of the caret

    void CharLeft() { moveCaret( QTextCursor::Left, false ); }
    void CharRight() { moveCaret( QTextCursor::Right, false ); }
    void CharLeftExtend() { moveCaret( QTextCursor::Left, true ); }
    void CharRightExtend() { moveCaret( QTextCursor::Right, true ); }
    void WordLeft() { moveCaret( QTextCursor::WordLeft, false ); }
    void WordRight() { moveCaret( QTextCursor::WordRight, false ); }
    void WordLeftExtend() { moveCaret( QTextCursor::WordLeft, true ); }
    void WordRightExtend() { moveCaret( QTextCursor::WordRight, true ); }
    void Home() { moveCaret( QTextCursor::StartOfBlock, false ); }
    void HomeExtend() { moveCaret( QTextCursor::StartOfBlock, true ); }
    void HomeWrap() { moveCaret( QTextCursor::StartOfLine, false ); }
    void LineEnd() { moveCaret( QTextCursor::EndOfBlock, false ); }
    void LineEndExtend() { moveCaret( QTextCursor::EndOfBlock, true ); }
    void LineEndWrap() { moveCaret( QTextCursor::EndOfLine, false ); }
    void DocumentStart() { moveCaret( QTextCursor::Start, false ); }
    void DocumentEnd() { moveCaret( QTextCursor::End, false ); }

    // The undo

    /// The changes until EndUndoAction() are undone together.
    void BeginUndoAction();
    void EndUndoAction();
    void EmptyUndoBuffer() { document()->clearUndoRedoStacks(); }
    void SetUndoCollection( bool aCollect ) { document()->setUndoRedoEnabled( aCollect ); }

    // The braces

    /// The position of the brace matching the one at @a aPos, or -1.
    int BraceMatch( int aPos, int aMaxReStyle = 0 ) const;
    void BraceHighlight( int aPos1, int aPos2 );
    void BraceBadLight( int aPos );
    void SetHighlightGuide( int aColumn ) {}
    int IndicatorValueAt( int aIndicator, int aPos ) const { return 0; }

    // The autocompletion

    /// Show a list of words (separated by the separator) to complete the word at the caret, of
    /// which @a aLenEntered characters are typed.
    void AutoCompShow( int aLenEntered, const wxString& aItemList );
    void AutoCompCancel();
    bool AutoCompActive() const;
    int AutoCompPosStart() const { return m_autoCompStart; }
    void AutoCompComplete();
    void AutoCompSelect( const wxString& aText );
    int AutoCompGetCurrent() const;
    void AutoCompSetSeparator( int aSeparator ) { m_autoCompSeparator = wxChar( aSeparator ); }
    int AutoCompGetSeparator() const { return m_autoCompSeparator; }
    void AutoCompSetIgnoreCase( bool aIgnoreCase ) { m_autoCompIgnoreCase = aIgnoreCase; }
    void AutoCompSetMaxHeight( int aRows ) { m_autoCompMaxRows = aRows; }
    void AutoCompSetMaxWidth( int aCharacters ) { m_autoCompMaxWidth = aCharacters; }
    void AutoCompSetFillUps( const wxString& aCharacters ) { m_autoCompFillUps = aCharacters; }
    void AutoCompStops( const wxString& aCharacters ) { m_autoCompStops = aCharacters; }
    void AutoCompSetAutoHide( bool aAutoHide ) {}
    void AutoCompSetDropRestOfWord( bool aDrop ) {}
    void AutoCompSetChooseSingle( bool aChoose ) {}
    void AutoCompSetCancelAtStart( bool aCancel ) {}

    // The appearance (the default style)

    void StyleSetForeground( int aStyle, const wxColour& aColour );
    void StyleSetBackground( int aStyle, const wxColour& aColour );
    void StyleSetFont( int aStyle, const wxFont& aFont );
    void StyleSetBold( int aStyle, bool aBold ) {}
    void StyleSetItalic( int aStyle, bool aItalic ) {}
    void StyleSetSize( int aStyle, int aSize ) {}
    void StyleSetFaceName( int aStyle, const wxString& aName ) {}
    void StyleClearAll() {}
    void StyleResetDefault() {}
    void SetSelForeground( bool aUse, const wxColour& aColour );
    void SetSelBackground( bool aUse, const wxColour& aColour );
    void SetCaretForeground( const wxColour& aColour ) {}
    void SetCaretLineVisible( bool aShow ) {}

    void SetTabWidth( int aWidth );
    int GetTabWidth() const { return m_tabWidth; }
    void SetUseTabs( bool aUseTabs ) {}
    void SetTabIndents( bool aTabIndents ) {}
    void SetBackSpaceUnIndents( bool aUnIndents ) {}
    void SetIndent( int aIndent ) {}
    void SetIndentationGuides( int aGuides ) {}
    void SetViewWhiteSpace( int aView );
    void SetViewEOL( bool aVisible ) {}
    void SetEOLMode( int aMode ) { m_eolMode = aMode; }
    int GetEOLMode() const { return m_eolMode; }
    void ConvertEOLs( int aMode ) {}
    void SetWrapMode( int aMode );

    void SetScrollWidth( int aWidth ) {}

    /// The extra spaces above and below the lines (the lines of Qt have their own spacing).
    void SetExtraAscent( int aExtra ) {}
    void SetExtraDescent( int aExtra ) {}
    int TextHeight( int aLine ) const { return fontMetrics().lineSpacing(); }
    void SetScrollWidthTracking( bool aTracking ) {}
    void SetUseHorizontalScrollBar( bool aShow );
    void SetUseVerticalScrollBar( bool aShow );

    void SetMarginType( int aMargin, int aType ) {}
    void SetMarginWidth( int aMargin, int aWidth ) { m_marginWidths[aMargin] = aWidth; }
    int GetMarginWidth( int aMargin ) const;
    void SetMarginMask( int aMargin, int aMask ) {}
    void SetMarginSensitive( int aMargin, bool aSensitive ) {}
    void SetMarginLeft( int aWidth );
    void SetMarginRight( int aWidth );
    int GetMarginLeft() const;
    int GetMarginRight() const;
    using wxTextCtrlMultiLine::SetMargins;

    void MarkerDefine( int aMarker, int aSymbol, const wxColour& aForeground = wxNullColour,
                       const wxColour& aBackground = wxNullColour )
    {
    }

    void MarkerSetForeground( int aMarker, const wxColour& aColour ) {}
    void MarkerSetBackground( int aMarker, const wxColour& aColour ) {}
    void SetFoldFlags( int aFlags ) {}
    void SetProperty( const wxString& aKey, const wxString& aValue ) {}
    void SetLexer( int aLexer ) {}
    void SetKeyWords( int aSet, const wxString& aWords ) {}
    void SetTechnology( int aTechnology ) {}
    void UsePopUp( int aPopUpMode );
    void SetCodePage( int aCodePage ) {}

    void SetZoom( int aZoom );
    int GetZoom() const { return m_zoom; }
    void ZoomIn() { SetZoom( m_zoom + 1 ); }
    void ZoomOut() { SetZoom( m_zoom - 1 ); }

    /// The width of a text in the default style.
    int TextWidth( int aStyle, const wxString& aText ) const;

    bool SaveFile( const wxString& aFileName );
    bool LoadFile( const wxString& aFileName );

protected:
    void keyPressEvent( QKeyEvent* aEvent ) override;
    void focusOutEvent( QFocusEvent* aEvent ) override;

private:
    void moveCaret( QTextCursor::MoveOperation aOperation, bool aExtend );

    /// The cursor of the edits: the one of the undo action, or the one of the control.
    QTextCursor editCursor() const;
    void applyEditCursor( const QTextCursor& aCursor );

    void sendEvent( wxEventType aType, int aKey = 0, int aPosition = 0 );
    void updateExtraSelections();
    void updateAutoCompSize();

    int                   m_eolMode;
    int                   m_tabWidth;
    int                   m_zoom;
    int                   m_searchAnchor;
    std::map<int, int>    m_marginWidths;

    QTextCursor           m_undoCursor;
    int                   m_undoDepth;

    int                   m_braceLight1;
    int                   m_braceLight2;
    bool                  m_braceBad;
    wxColour              m_braceLightForeground;
    wxColour              m_braceLightBackground;
    wxColour              m_braceBadForeground;

    QPointer<QListWidget> m_autoComp;
    int                   m_autoCompStart;
    wxChar                m_autoCompSeparator;
    bool                  m_autoCompIgnoreCase;
    int                   m_autoCompMaxRows;
    int                   m_autoCompMaxWidth;
    wxString              m_autoCompFillUps;
    wxString              m_autoCompStops;
};

#endif // KIWX_STC_H
