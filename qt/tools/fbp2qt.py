#!/usr/bin/env python3
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
fbp2qt: the Qt base classes of the dialogs and panels of KiCad, from their wxFormBuilder files.

The .fbp files stay the description of the layout of the dialogs.  For each one, this script
writes the Qt version of the class wxFormBuilder generates (DIALOG_MOVE_EXACT_BASE...): the same
class name, the same members (with the Qt widgets: QLabel, QLineEdit, QComboBox...), the layouts
of Qt for the sizers, and virtual event handlers connected to the signals of the widgets.  The
derived classes (the logic of the dialogs) are written for Qt by hand.

    fbp2qt.py <file.fbp> <output directory>

writes <output directory>/<file>.h and <file>.cpp (the "file" property of the project).

Event handlers:
    the handlers of the events of a widget get the widget: Handler( QWidget* aSource ); the
    other ones get the data of their event (see EVENTS).
"""

import os
import re
import sys
import xml.etree.ElementTree as ET


class FbpError( Exception ):
    pass


# ---------------------------------------------------------------------------------------------
# Properties

def props( aObj ):
    return { p.get( 'name' ): ( p.text or '' ) for p in aObj.findall( 'property' ) }


def events( aObj ):
    return { e.get( 'name' ): e.text.strip() for e in aObj.findall( 'event' )
             if ( e.text or '' ).strip() }


def flags( aText ):
    return set( f.strip() for f in aText.split( '|' ) if f.strip() )


def cstr( aText ):
    """A C++ string literal of a text of wxFormBuilder (which keeps the escapes as typed)."""
    out = ''
    i = 0

    while i < len( aText ):
        c = aText[i]

        if c == '\\' and i + 1 < len( aText ):
            out += aText[i:i + 2]
            i += 2
            continue

        if c == '"':
            out += '\\"'
        elif c == '\n':
            out += '\\n'
        else:
            out += c

        i += 1

    return '"' + out + '"'


def tr( aText ):
    """A translated text, as a QString."""
    return 'ToQString( _( %s ) )' % cstr( aText )


def lit( aText ):
    """A text that is not translated (a value), as a QString."""
    return 'QString::fromUtf8( %s )' % cstr( aText )


def parse_size( aText ):
    m = re.match( r'\s*(-?\d+)\s*,\s*(-?\d+)', aText or '' )
    return ( int( m.group( 1 ) ), int( m.group( 2 ) ) ) if m else ( -1, -1 )


def parse_choices( aText ):
    """The choices of a wxChoice: "a" "b c" ..."""
    return re.findall( r'"((?:[^"\\]|\\.)*)"', aText or '' )


def parse_int( aText, aDefault = 0 ):
    try:
        return int( aText )
    except ( TypeError, ValueError ):
        return aDefault


def parse_float( aText, aDefault = 0.0 ):
    try:
        return float( aText )
    except ( TypeError, ValueError ):
        return aDefault


def subclass( aProps ):
    """(class, header, forward declared) of the subclass property, or (None, None, False)."""
    parts = [ p.strip() for p in aProps.get( 'subclass', '' ).split( ';' ) ]

    if not parts or not parts[0]:
        return None, None, False

    header = parts[1] if len( parts ) > 1 and parts[1] and parts[1] != 'forward_declare' else None
    return parts[0], header, 'forward_declare' in parts[1:]


# ---------------------------------------------------------------------------------------------
# Events: name -> (parameters of the handler, kind of connection)
#
# The kinds of connection:
#   signal:<Class>::<signal>     a signal of the widget without the data the handler needs
#   filter:<QEvent::Type>        an event of the widget (KIQT::OnEvent)
#   updateui                     an update UI function (KIQT::RegisterUpdateUI)
#   stdbutton:<id>               a button of the standard button box

SOURCE = 'QWidget* aSource'

EVENTS = {
    'OnButtonClick':        ( SOURCE, 'signal:QAbstractButton::clicked' ),
    'OnCheckBox':           ( SOURCE, 'signal:QAbstractButton::clicked' ),
    'OnRadioButton':        ( SOURCE, 'signal:QAbstractButton::clicked' ),
    'OnToggleButton':       ( SOURCE, 'signal:QAbstractButton::clicked' ),
    'OnText':               ( SOURCE, 'text' ),
    'OnTextEnter':          ( SOURCE, 'textenter' ),
    'OnChoice':             ( SOURCE, 'signal:QComboBox::activated' ),
    'OnCombobox':           ( SOURCE, 'signal:QComboBox::activated' ),
    'OnSpinCtrl':           ( SOURCE, 'spin' ),
    'OnSpinCtrlDouble':     ( SOURCE, 'spin' ),
    'OnSpinCtrlText':       ( SOURCE, 'spintext' ),
    'OnSpinUp':             ( SOURCE, 'spinup' ),
    'OnSpinDown':           ( SOURCE, 'spindown' ),
    'OnSlider':             ( SOURCE, 'signal:QAbstractSlider::valueChanged' ),
    'OnScroll':             ( SOURCE, 'signal:QAbstractSlider::valueChanged' ),
    'OnCommandScrollChanged': ( SOURCE, 'signal:QAbstractSlider::valueChanged' ),
    'OnCommandScrollThumbTrack': ( SOURCE, 'signal:QAbstractSlider::sliderMoved' ),
    'OnListBox':            ( SOURCE, 'signal:QListWidget::currentRowChanged' ),
    'OnListBoxDClick':      ( SOURCE, 'signal:QListWidget::itemDoubleClicked' ),
    'OnCheckListBoxToggled': ( SOURCE, 'signal:QListWidget::itemChanged' ),
    'OnListItemActivated':  ( SOURCE, 'signal:QTreeWidget::itemActivated' ),
    'OnListItemSelected':   ( SOURCE, 'signal:QTreeWidget::itemSelectionChanged' ),
    'OnListItemDeselected': ( SOURCE, 'signal:QTreeWidget::itemSelectionChanged' ),
    'OnKillFocus':          ( SOURCE, 'filter:QEvent::FocusOut' ),
    'OnSetFocus':           ( SOURCE, 'filter:QEvent::FocusIn' ),
    'OnSize':               ( SOURCE, 'filter:QEvent::Resize' ),
    'OnLeftDown':           ( SOURCE, 'filter:QEvent::MouseButtonPress' ),
    'OnLeftUp':             ( SOURCE, 'filter:QEvent::MouseButtonRelease' ),
    'OnLeftDClick':         ( SOURCE, 'filter:QEvent::MouseButtonDblClick' ),
    'OnRightDown':          ( SOURCE, 'filter:QEvent::MouseButtonPress' ),
    'OnRightUp':            ( SOURCE, 'filter:QEvent::MouseButtonRelease' ),
    'OnMotion':             ( SOURCE, 'filter:QEvent::MouseMove' ),
    'OnMouseWheel':         ( SOURCE, 'filter:QEvent::Wheel' ),
    'OnUpdateUI':           ( SOURCE, 'updateui' ),
    'OnHyperlink':          ( 'const QString& aUrl', 'link' ),
    'OnHtmlLinkClicked':    ( 'const QString& aUrl', 'htmllink' ),
    'OnNotebookPageChanged': ( 'int aPage', 'signal:QTabWidget::currentChanged' ),
    'OnGridCellLeftClick':  ( 'int aRow, int aCol', 'signal:QTableWidget::cellClicked' ),
    'OnGridCellLeftDClick': ( 'int aRow, int aCol', 'signal:QTableWidget::cellDoubleClicked' ),
    'OnGridCellRightClick': ( 'int aRow, int aCol', 'gridrightclick' ),
    'OnAuiNotebookPageChanged': ( 'int aPage', 'signal:QTabWidget::currentChanged' ),
    'OnSearchButton':       ( SOURCE, 'textenter' ),
    'OnCancelButton':       ( SOURCE, 'searchcancel' ),
    'OnOKButtonClick':      ( SOURCE, 'stdbutton:Ok' ),
    'OnCancelButtonClick':  ( SOURCE, 'stdbutton:Cancel' ),
    'OnApplyButtonClick':   ( SOURCE, 'stdbutton:Apply' ),
    'OnHelpButtonClick':    ( SOURCE, 'stdbutton:Help' ),
    'OnSaveButtonClick':    ( SOURCE, 'stdbutton:Save' ),
    'OnYesButtonClick':     ( SOURCE, 'stdbutton:Yes' ),
    'OnNoButtonClick':      ( SOURCE, 'stdbutton:No' ),
}

# ---------------------------------------------------------------------------------------------
# The wxWidgets compatibility mode (--wx-compat): the controls are the classes of the
# compatibility layer (qt/wxcompat: wxCheckBox is a QCheckBox with the API of wxWidgets), and
# the handlers get the events of wxWidgets (wxCommandEvent...), as the bases wxFormBuilder
# writes: the code of the dialogs of KiCad builds on them unchanged.

# The class of the compatibility layer of a class of wxWidgets
COMPAT_CLASSES = {
    'wxStaticText': 'wxStaticText',
    'wxTextCtrl': 'wxTextCtrl',
    'wxSearchCtrl': 'wxSearchCtrl',
    'wxButton': 'wxButton',
    'wxToggleButton': 'wxToggleButton',
    'wxBitmapButton': 'wxBitmapButton',
    'wxCheckBox': 'wxCheckBox',
    'wxRadioButton': 'wxRadioButton',
    'wxChoice': 'wxChoice',
    'wxComboBox': 'wxComboBox',
    'wxBitmapComboBox': 'wxBitmapComboBox',
    'wxSpinCtrl': 'wxSpinCtrl',
    'wxSpinCtrlDouble': 'wxSpinCtrlDouble',
    'wxSlider': 'wxSlider',
    'wxGauge': 'wxGauge',
    'wxStaticLine': 'wxStaticLine',
    'wxStaticBitmap': 'wxStaticBitmap',
    'wxListBox': 'wxListBox',
    'wxCheckListBox': 'wxCheckListBox',
    'wxHyperlinkCtrl': 'wxHyperlinkCtrl',
    'wxFilePickerCtrl': 'wxFilePickerCtrl',
    'wxDirPickerCtrl': 'wxDirPickerCtrl',
    'wxListCtrl': 'wxListCtrl',
    'wxTreeCtrl': 'wxTreeCtrl',
    'wxDataViewCtrl': 'wxDataViewCtrl',
    'wxDataViewListCtrl': 'wxDataViewListCtrl',
    'wxDataViewTreeCtrl': 'wxDataViewTreeCtrl',
    'wxGrid': 'wxGrid',
    'wxStyledTextCtrl': 'wxStyledTextCtrl',
    'wxGenericDirCtrl': 'wxGenericDirCtrl',
    'wxDatePickerCtrl': 'wxDatePickerCtrl',
}

# The events of the compatibility mode: name -> (class of the event, type of the event, how it
# is filled).  The ways:
#   clicked, checked     the clicked signal of a button (checked: SetInt( isChecked ))
#   text, textenter      the text changes, the Enter key
#   choice               the activated signal of a combo box (SetInt( index ), SetString( text ))
#   spin, spindouble     the value of a spin control
#   spintext             the end of the edition of a spin control
#   slider, sliderthumb  the value of a slider
#   listbox, listdclick, checklist   the lists
#   focusin, focusout, size, mouse:<QEvent::Type>, key:<QEvent::Type>   the events of a widget
#   updateui             an update UI function
#   link                 a hyperlink
#   page                 the page of a notebook
#   bind                 the events the window sends itself (the grids): Bind() on it
#   stdbutton:<id>       a button of the standard button box
COMPAT_EVENTS = {
    'OnButtonClick':        ( 'wxCommandEvent', 'wxEVT_BUTTON', 'clicked' ),
    'OnCheckBox':           ( 'wxCommandEvent', 'wxEVT_CHECKBOX', 'checked' ),
    'OnRadioButton':        ( 'wxCommandEvent', 'wxEVT_RADIOBUTTON', 'checked' ),
    'OnToggleButton':       ( 'wxCommandEvent', 'wxEVT_TOGGLEBUTTON', 'checked' ),
    'OnText':               ( 'wxCommandEvent', 'wxEVT_TEXT', 'text' ),
    'OnTextEnter':          ( 'wxCommandEvent', 'wxEVT_TEXT_ENTER', 'textenter' ),
    'OnChoice':             ( 'wxCommandEvent', 'wxEVT_CHOICE', 'choice' ),
    'OnCombobox':           ( 'wxCommandEvent', 'wxEVT_COMBOBOX', 'choice' ),
    'OnSpinCtrl':           ( 'wxSpinEvent', 'wxEVT_SPINCTRL', 'spin' ),
    'OnSpinCtrlDouble':     ( 'wxSpinDoubleEvent', 'wxEVT_SPINCTRLDOUBLE', 'spindouble' ),
    'OnSpinCtrlText':       ( 'wxCommandEvent', 'wxEVT_TEXT', 'spintext' ),
    'OnSlider':             ( 'wxCommandEvent', 'wxEVT_SLIDER', 'slider' ),
    'OnScroll':             ( 'wxScrollEvent', 'wxEVT_SCROLL_CHANGED', 'slider' ),
    'OnCommandScrollChanged': ( 'wxScrollEvent', 'wxEVT_SCROLL_CHANGED', 'slider' ),
    'OnCommandScrollThumbTrack': ( 'wxScrollEvent', 'wxEVT_SCROLL_THUMBTRACK', 'sliderthumb' ),
    'OnListBox':            ( 'wxCommandEvent', 'wxEVT_LISTBOX', 'listbox' ),
    'OnListBoxDClick':      ( 'wxCommandEvent', 'wxEVT_LISTBOX_DCLICK', 'listdclick' ),
    'OnCheckListBoxToggled': ( 'wxCommandEvent', 'wxEVT_CHECKLISTBOX', 'checklist' ),
    'OnKillFocus':          ( 'wxFocusEvent', 'wxEVT_KILL_FOCUS', 'focusout' ),
    'OnSetFocus':           ( 'wxFocusEvent', 'wxEVT_SET_FOCUS', 'focusin' ),
    'OnSize':               ( 'wxSizeEvent', 'wxEVT_SIZE', 'size' ),
    'OnLeftDown':           ( 'wxMouseEvent', 'wxEVT_LEFT_DOWN', 'mouse:QEvent::MouseButtonPress' ),
    'OnLeftUp':             ( 'wxMouseEvent', 'wxEVT_LEFT_UP', 'mouse:QEvent::MouseButtonRelease' ),
    'OnLeftDClick':         ( 'wxMouseEvent', 'wxEVT_LEFT_DCLICK', 'mouse:QEvent::MouseButtonDblClick' ),
    'OnRightDown':          ( 'wxMouseEvent', 'wxEVT_RIGHT_DOWN', 'mouse:QEvent::MouseButtonPress' ),
    'OnRightUp':            ( 'wxMouseEvent', 'wxEVT_RIGHT_UP', 'mouse:QEvent::MouseButtonRelease' ),
    'OnMotion':             ( 'wxMouseEvent', 'wxEVT_MOTION', 'mouse:QEvent::MouseMove' ),
    'OnMouseWheel':         ( 'wxMouseEvent', 'wxEVT_MOUSEWHEEL', 'mouse:QEvent::Wheel' ),
    'OnCharHook':           ( 'wxKeyEvent', 'wxEVT_CHAR_HOOK', 'bind' ),
    'OnPaint':              ( 'wxPaintEvent', 'wxEVT_PAINT', 'bind' ),
    'OnKeyDown':            ( 'wxKeyEvent', 'wxEVT_KEY_DOWN', 'key:QEvent::KeyPress' ),
    'OnUpdateUI':           ( 'wxUpdateUIEvent', 'wxEVT_UPDATE_UI', 'updateui' ),
    'OnHyperlink':          ( 'wxHyperlinkEvent', 'wxEVT_HYPERLINK', 'bind' ),
    'OnListItemSelected':   ( 'wxListEvent', 'wxEVT_LIST_ITEM_SELECTED', 'bind' ),
    'OnListItemDeselected': ( 'wxListEvent', 'wxEVT_LIST_ITEM_DESELECTED', 'bind' ),
    'OnListItemActivated':  ( 'wxListEvent', 'wxEVT_LIST_ITEM_ACTIVATED', 'bind' ),
    'OnListItemFocused':    ( 'wxListEvent', 'wxEVT_LIST_ITEM_FOCUSED', 'bind' ),
    'OnListItemRightClick': ( 'wxListEvent', 'wxEVT_LIST_ITEM_RIGHT_CLICK', 'bind' ),
    'OnListItemChecked':    ( 'wxListEvent', 'wxEVT_LIST_ITEM_CHECKED', 'bind' ),
    'OnListItemUnchecked':  ( 'wxListEvent', 'wxEVT_LIST_ITEM_UNCHECKED', 'bind' ),
    'OnTreeSelChanged':     ( 'wxTreeEvent', 'wxEVT_TREE_SEL_CHANGED', 'bind' ),
    'OnTreeSelChanging':    ( 'wxTreeEvent', 'wxEVT_TREE_SEL_CHANGING', 'bind' ),
    'OnTreeItemActivated':  ( 'wxTreeEvent', 'wxEVT_TREE_ITEM_ACTIVATED', 'bind' ),
    'OnTreeItemRightClick':( 'wxTreeEvent', 'wxEVT_TREE_ITEM_RIGHT_CLICK', 'bind' ),
    'OnTreeItemMenu':       ( 'wxTreeEvent', 'wxEVT_TREE_ITEM_MENU', 'bind' ),
    'OnTreeItemExpanded':   ( 'wxTreeEvent', 'wxEVT_TREE_ITEM_EXPANDED', 'bind' ),
    'OnTreeItemExpanding':( 'wxTreeEvent', 'wxEVT_TREE_ITEM_EXPANDING', 'bind' ),
    'OnTreeItemCollapsed':( 'wxTreeEvent', 'wxEVT_TREE_ITEM_COLLAPSED', 'bind' ),
    'OnTreeItemCollapsing':( 'wxTreeEvent', 'wxEVT_TREE_ITEM_COLLAPSING', 'bind' ),
    'OnTreeBeginDrag':      ( 'wxTreeEvent', 'wxEVT_TREE_BEGIN_DRAG', 'bind' ),
    'OnTreeEndDrag':        ( 'wxTreeEvent', 'wxEVT_TREE_END_DRAG', 'bind' ),
    'OnTreeBeginLabelEdit':( 'wxTreeEvent', 'wxEVT_TREE_BEGIN_LABEL_EDIT', 'bind' ),
    'OnTreeEndLabelEdit':   ( 'wxTreeEvent', 'wxEVT_TREE_END_LABEL_EDIT', 'bind' ),
    'OnTreeKeyDown':        ( 'wxTreeEvent', 'wxEVT_TREE_KEY_DOWN', 'bind' ),
    'OnListColClick':       ( 'wxListEvent', 'wxEVT_LIST_COL_CLICK', 'bind' ),
    'OnListColRightClick':  ( 'wxListEvent', 'wxEVT_LIST_COL_RIGHT_CLICK', 'bind' ),
    'OnListKeyDown':        ( 'wxListEvent', 'wxEVT_LIST_KEY_DOWN', 'bind' ),
    'OnListBeginDrag':      ( 'wxListEvent', 'wxEVT_LIST_BEGIN_DRAG', 'bind' ),
    'OnFileChanged':        ( 'wxFileDirPickerEvent', 'wxEVT_FILEPICKER_CHANGED', 'bind' ),
    'OnDirctrlSelectionChanged': ( 'wxCommandEvent', 'wxEVT_DIRCTRL_SELECTIONCHANGED', 'bind' ),
    'OnDirctrlFileActivated':    ( 'wxCommandEvent', 'wxEVT_DIRCTRL_FILEACTIVATED', 'bind' ),
    'OnDataViewCtrlSelectionChanged':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_SELECTION_CHANGED', 'bind' ),
    'OnDataViewCtrlItemActivated':       ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_ACTIVATED', 'bind' ),
    'OnDataViewCtrlItemContextMenu':     ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_CONTEXT_MENU', 'bind' ),
    'OnDataViewCtrlItemValueChanged':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_VALUE_CHANGED', 'bind' ),
    'OnDataViewCtrlItemExpanded':        ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDED', 'bind' ),
    'OnDataViewCtrlItemCollapsed':       ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSED', 'bind' ),
    'OnDataViewCtrlItemExpanding':       ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDING', 'bind' ),
    'OnDataViewCtrlItemCollapsing':      ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSING', 'bind' ),
    'OnDataViewCtrlItemStartEditing':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_START_EDITING', 'bind' ),
    'OnDataViewCtrlItemEditingStarted':  ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_STARTED', 'bind' ),
    'OnDataViewCtrlItemEditingDone':     ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_DONE', 'bind' ),
    'OnDataViewCtrlColumnHeaderClick':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_CLICK', 'bind' ),
    'OnDataViewCtrlColumnHeaderRightClick': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK', 'bind' ),
    'OnDataViewCtrlColumnSorted':        ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_SORTED', 'bind' ),
    'OnDataViewCtrlColumnReordered':     ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_REORDERED', 'bind' ),
    'OnDataViewCtrlItemBeginDrag':       ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_BEGIN_DRAG', 'bind' ),
    'OnDataViewCtrlItemDropPossible':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE', 'bind' ),
    'OnDataViewCtrlItemDrop':            ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP', 'bind' ),
    'OnDataViewListCtrlSelectionChanged': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_SELECTION_CHANGED', 'bind' ),
    'OnDataViewListCtrlItemActivated':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_ACTIVATED', 'bind' ),
    'OnDataViewListCtrlItemContextMenu': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_CONTEXT_MENU', 'bind' ),
    'OnDataViewListCtrlItemValueChanged': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_VALUE_CHANGED', 'bind' ),
    'OnDataViewListCtrlItemExpanded':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDED', 'bind' ),
    'OnDataViewListCtrlItemCollapsed':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSED', 'bind' ),
    'OnDataViewListCtrlItemExpanding':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDING', 'bind' ),
    'OnDataViewListCtrlItemCollapsing':  ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSING', 'bind' ),
    'OnDataViewListCtrlItemStartEditing': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_START_EDITING', 'bind' ),
    'OnDataViewListCtrlItemEditingStarted': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_STARTED', 'bind' ),
    'OnDataViewListCtrlItemEditingDone': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_DONE', 'bind' ),
    'OnDataViewListCtrlColumnHeaderClick': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_CLICK', 'bind' ),
    'OnDataViewListCtrlColumnHeaderRightClick': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK', 'bind' ),
    'OnDataViewListCtrlColumnSorted':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_SORTED', 'bind' ),
    'OnDataViewListCtrlColumnReordered': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_REORDERED', 'bind' ),
    'OnDataViewListCtrlItemBeginDrag':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_BEGIN_DRAG', 'bind' ),
    'OnDataViewListCtrlItemDropPossible': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE', 'bind' ),
    'OnDataViewListCtrlItemDrop':        ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP', 'bind' ),
    'OnDataViewTreeCtrlSelectionChanged': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_SELECTION_CHANGED', 'bind' ),
    'OnDataViewTreeCtrlItemActivated':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_ACTIVATED', 'bind' ),
    'OnDataViewTreeCtrlItemContextMenu': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_CONTEXT_MENU', 'bind' ),
    'OnDataViewTreeCtrlItemValueChanged': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_VALUE_CHANGED', 'bind' ),
    'OnDataViewTreeCtrlItemExpanded':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDED', 'bind' ),
    'OnDataViewTreeCtrlItemCollapsed':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSED', 'bind' ),
    'OnDataViewTreeCtrlItemExpanding':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EXPANDING', 'bind' ),
    'OnDataViewTreeCtrlItemCollapsing':  ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_COLLAPSING', 'bind' ),
    'OnDataViewTreeCtrlItemStartEditing': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_START_EDITING', 'bind' ),
    'OnDataViewTreeCtrlItemEditingStarted': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_STARTED', 'bind' ),
    'OnDataViewTreeCtrlItemEditingDone': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_EDITING_DONE', 'bind' ),
    'OnDataViewTreeCtrlColumnHeaderClick': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_CLICK', 'bind' ),
    'OnDataViewTreeCtrlColumnHeaderRightClick': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK', 'bind' ),
    'OnDataViewTreeCtrlColumnSorted':    ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_SORTED', 'bind' ),
    'OnDataViewTreeCtrlColumnReordered': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_COLUMN_REORDERED', 'bind' ),
    'OnDataViewTreeCtrlItemBeginDrag':   ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_BEGIN_DRAG', 'bind' ),
    'OnDataViewTreeCtrlItemDropPossible': ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE', 'bind' ),
    'OnDataViewTreeCtrlItemDrop':        ( 'wxDataViewEvent', 'wxEVT_DATAVIEW_ITEM_DROP', 'bind' ),
    'OnDirChanged':         ( 'wxFileDirPickerEvent', 'wxEVT_DIRPICKER_CHANGED', 'bind' ),
    'OnNotebookPageChanged': ( 'wxNotebookEvent', 'wxEVT_NOTEBOOK_PAGE_CHANGED', 'page' ),
    'OnAuiNotebookPageChanged': ( 'wxAuiNotebookEvent', 'wxEVT_NOTEBOOK_PAGE_CHANGED', 'page' ),
    'OnGridCellLeftClick':  ( 'wxGridEvent', 'wxEVT_GRID_CELL_LEFT_CLICK', 'bind' ),
    'OnGridCellLeftDClick': ( 'wxGridEvent', 'wxEVT_GRID_CELL_LEFT_DCLICK', 'bind' ),
    'OnGridCellRightClick': ( 'wxGridEvent', 'wxEVT_GRID_CELL_RIGHT_CLICK', 'bind' ),
    'OnGridCellRightDClick': ( 'wxGridEvent', 'wxEVT_GRID_CELL_RIGHT_DCLICK', 'bind' ),
    'OnGridCellChange':     ( 'wxGridEvent', 'wxEVT_GRID_CELL_CHANGED', 'bind' ),
    'OnGridCellChanging':   ( 'wxGridEvent', 'wxEVT_GRID_CELL_CHANGING', 'bind' ),
    'OnGridSelectCell':     ( 'wxGridEvent', 'wxEVT_GRID_SELECT_CELL', 'bind' ),
    'OnGridEditorShown':    ( 'wxGridEvent', 'wxEVT_GRID_EDITOR_SHOWN', 'bind' ),
    'OnGridEditorHidden':   ( 'wxGridEvent', 'wxEVT_GRID_EDITOR_HIDDEN', 'bind' ),
    'OnGridEditorCreated':  ( 'wxGridEditorCreatedEvent', 'wxEVT_GRID_EDITOR_CREATED', 'bind' ),
    'OnGridLabelLeftClick': ( 'wxGridEvent', 'wxEVT_GRID_LABEL_LEFT_CLICK', 'bind' ),
    'OnGridLabelLeftDClick': ( 'wxGridEvent', 'wxEVT_GRID_LABEL_LEFT_DCLICK', 'bind' ),
    'OnGridLabelRightClick': ( 'wxGridEvent', 'wxEVT_GRID_LABEL_RIGHT_CLICK', 'bind' ),
    'OnGridColSize':        ( 'wxGridSizeEvent', 'wxEVT_GRID_COL_SIZE', 'bind' ),
    'OnGridRowSize':        ( 'wxGridSizeEvent', 'wxEVT_GRID_ROW_SIZE', 'bind' ),
    'OnGridRangeSelect':    ( 'wxGridRangeSelectEvent', 'wxEVT_GRID_RANGE_SELECTED', 'bind' ),
    'OnGridCmdCellChange':  ( 'wxGridEvent', 'wxEVT_GRID_CELL_CHANGED', 'bind' ),
    'OnGridCmdSelectCell':  ( 'wxGridEvent', 'wxEVT_GRID_SELECT_CELL', 'bind' ),
    'OnRadioBox':           ( 'wxCommandEvent', 'wxEVT_RADIOBOX', 'radiobox' ),
    'OnClose':              ( 'wxCloseEvent', 'wxEVT_CLOSE_WINDOW', 'close' ),
    'OnInitDialog':         ( 'wxInitDialogEvent', 'wxEVT_INIT_DIALOG', 'initdialog' ),
    'OnActivate':           ( 'wxActivateEvent', 'wxEVT_ACTIVATE', 'activate' ),
    'OnIdle':               ( 'wxIdleEvent', 'wxEVT_IDLE', 'idle' ),
    'OnSpinUp':             ( 'wxSpinEvent', 'wxEVT_SPIN_UP', 'spinup' ),
    'OnSpinDown':           ( 'wxSpinEvent', 'wxEVT_SPIN_DOWN', 'spindown' ),
    'OnSearchButton':       ( 'wxCommandEvent', 'wxEVT_TEXT_ENTER', 'textenter' ),
    'OnCancelButton':       ( 'wxCommandEvent', 'wxEVT_BUTTON', 'searchcancel' ),
    'OnKeyUp':              ( 'wxKeyEvent', 'wxEVT_KEY_DOWN', 'key:QEvent::KeyRelease' ),
    'OnChar':               ( 'wxKeyEvent', 'wxEVT_KEY_DOWN', 'key:QEvent::KeyPress' ),
    'OnNotebookPageChanging': ( 'wxNotebookEvent', 'wxEVT_NOTEBOOK_PAGE_CHANGING', 'page' ),
    'OnHtmlLinkClicked':    ( 'wxHtmlLinkEvent', 'wxEVT_HTML_LINK_CLICKED', 'htmllink' ),
    'OnOKButtonClick':      ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Ok' ),
    'OnCancelButtonClick':  ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Cancel' ),
    'OnApplyButtonClick':   ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Apply' ),
    'OnHelpButtonClick':    ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Help' ),
    'OnSaveButtonClick':    ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Save' ),
    'OnYesButtonClick':     ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:Yes' ),
    'OnNoButtonClick':      ( 'wxCommandEvent', 'wxEVT_BUTTON', 'stdbutton:No' ),
}


# The standard ids of wxWidgets (wxID_OK...): the other ids of the windows are defined by the
# generated headers, as wxFormBuilder does
def standard_ids():
    path = os.path.join( os.path.dirname( os.path.abspath( __file__ ) ), '..', '..', 'libs', 'kiwx',
                         'include', 'wx', 'kiwx_constants.h' )

    with open( path, encoding = 'utf-8' ) as f:
        return set( re.findall( r'\b(wxID_\w+)\s*[=,]', f.read() ) )


STANDARD_IDS = standard_ids()


# The arguments of the constructors of wxFormBuilder after the parent and the id, by class: the
# subclasses and the custom controls of KiCad (compatibility mode) are created with them
def wx_ctor_args( aClass, aProps, aStyle ):
    if aClass == 'wxBitmapButton':
        return 'wxNullBitmap, wxDefaultPosition, wxDefaultSize, %s' % aStyle
    if aClass == 'wxButton':
        return '_( %s ), wxDefaultPosition, wxDefaultSize, %s' % ( cstr( aProps.get( 'label', '' ) ), aStyle )
    if aClass == 'wxTextCtrl':
        return 'wxString::FromUTF8( %s ), wxDefaultPosition, wxDefaultSize, %s' % ( cstr( aProps.get( 'value', '' ) ), aStyle )
    if aClass in ( 'wxBitmapComboBox', 'wxComboBox' ):
        return 'wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, %s' % aStyle
    if aClass == 'wxChoice':
        return 'wxDefaultPosition, wxDefaultSize, 0, nullptr, %s' % aStyle
    if aClass in ( 'wxGrid', 'wxHtmlWindow', 'wxTreeCtrl', 'CustomControl' ):
        return 'wxDefaultPosition, wxDefaultSize, %s' % aStyle

    return None


# The classes of wxWidgets used as custom controls, and their Qt counterparts
WX_CUSTOM_CLASSES = {
    'wxScrolledCanvas': 'QScrollArea',
}

# The buttons of wxStdDialogButtonSizer: property, member suffix, Qt button, KiCad id
STD_BUTTONS = [
    ( 'OK',          'OK',          'Ok',     'wxID_OK' ),
    ( 'Yes',         'Yes',         'Yes',    'wxID_YES' ),
    ( 'Save',        'Save',        'Save',   'wxID_SAVE' ),
    ( 'Apply',       'Apply',       'Apply',  'wxID_APPLY' ),
    ( 'No',          'No',          'No',     'wxID_NO' ),
    ( 'Cancel',      'Cancel',      'Cancel', 'wxID_CANCEL' ),
    ( 'Help',        'Help',        'Help',   'wxID_HELP' ),
    ( 'ContextHelp', 'ContextHelp', 'Help',   'wxID_CONTEXT_HELP' ),
]


# ---------------------------------------------------------------------------------------------
# Generator

class Generator:
    def __init__( self, aFbp, aCompat = False ):
        self.fbp = aFbp
        self.compat = aCompat       # the wxWidgets compatibility mode
        root = ET.parse( aFbp ).getroot()
        project = root.find( 'object' )

        if project is None or project.get( 'class' ) != 'Project':
            raise FbpError( 'no project' )

        self.project = props( project )
        self.file = self.project.get( 'file' ) or os.path.splitext( os.path.basename( aFbp ) )[0]
        self.forms = [ o for o in project.findall( 'object' ) ]

        self.ids = []               # the ids of the windows (ID_...), in their order
        self.includes = set()       # headers of the .h
        self.cpp_includes = set()   # headers of the .cpp
        self.forward = set()        # forward declared classes

    # The classes of the file ------------------------------------------------------------------

    def generate( self ):
        classes = []

        for form in self.forms:
            classes.append( FormClass( self, form ).generate() )

        guard = 'FBP2QT_' + re.sub( r'\W', '_', self.file ).upper() + '_H'

        h = [ '// Generated by qt/tools/fbp2qt.py from %s: do not edit.' % os.path.basename( self.fbp ),
              '', '#pragma once', '', '#include <kiqt.h>     // Qt, without its METHOD macro', '' ]

        for inc in sorted( self.includes ):
            h.append( '#include %s' % inc )

        h.append( '' )
        h.append( '#include <wx/intl.h>' )
        h.append( '#include <qt_strings.h>' )
        h.append( '' )

        # The ids of the windows, as wxFormBuilder defines them
        first = parse_int( self.project.get( 'first_id' ), 1000 )

        for n, wid in enumerate( dict.fromkeys( self.ids ) ):
            h.append( '#ifndef %s' % wid )
            h.append( '#define %s %d' % ( wid, first + n ) )
            h.append( '#endif' )

        if self.ids:
            h.append( '' )

        for fwd in sorted( self.forward ):
            h.append( 'class %s;' % fwd )

        if self.forward:
            h.append( '' )

        cpp = [ '// Generated by qt/tools/fbp2qt.py from %s: do not edit.' % os.path.basename( self.fbp ),
                '', '#include "%s.h"' % self.file, '' ]

        for inc in sorted( self.cpp_includes - self.includes ):
            cpp.append( '#include %s' % inc )

        cpp.append( '' )
        cpp.append( '#include <ui_hooks.h>' )
        cpp.append( '' )

        for decl, impl in classes:
            h += [ '' ] + decl
            cpp += [ '' ] + impl

        return '\n'.join( h ) + '\n', '\n'.join( cpp ) + '\n'


class FormClass:
    """A dialog, a panel or a frame of the file."""

    def __init__( self, aGen, aForm ):
        self.gen = aGen
        self.form = aForm
        self.p = props( aForm )
        self.kind = aForm.get( 'class' )
        self.name = self.p['name']
        self.members = []           # (permission, type, name)
        self.handlers = {}          # name -> parameters
        self.stdHandlers = set()    # the handlers of standard buttons (OK, Cancel...)
        self.code = []              # lines of the constructor
        self.counter = 0
        self.overrides = []         # extra declarations (overrides of events of the form)

    def tmp( self, aPrefix ):
        self.counter += 1
        return '%s%d' % ( aPrefix, self.counter )

    def include( self, aHeader, aInHeader = True ):
        inc = aHeader if aHeader.startswith( '<' ) or aHeader.startswith( '"' ) else '<%s>' % aHeader

        if aInHeader:
            self.gen.includes.add( inc )
        else:
            self.gen.cpp_includes.add( inc )

    def qt( self, aClass ):
        """Include a class of Qt (in the header, where the members are declared)."""
        self.include( '<%s>' % aClass )
        return aClass

    def member( self, aObj, aType, aName = None ):
        """Declare the member of an object if it has a permission; return its name."""
        p = props( aObj )
        name = aName or p.get( 'name' )
        perm = p.get( 'permission', 'protected' )

        if perm in ( 'protected', 'public', 'private' ):
            self.members.append( ( perm, aType, name ) )
            return name

        # A local variable
        return None

    def var( self, aObj, aType, aExpr ):
        """The variable of an object: its member, or a local variable."""
        p = props( aObj )
        name = self.member( aObj, aType )

        if name:
            self.code.append( '    %s = %s;' % ( name, aExpr ) )
            return name

        local = p.get( 'name' ) or self.tmp( 'item' )
        local = re.sub( r'\W', '_', local )
        self.code.append( '    %s* %s = %s;' % ( aType, local, aExpr ) )
        return local

    def handler( self, aName, aParams ):
        known = self.handlers.setdefault( aName, [] )

        if aParams in known:
            return

        # As wxFormBuilder, a handler of events of different classes is overloaded (compatibility
        # mode); the handlers of Qt have one signature
        if known and not self.gen.compat:
            raise FbpError( 'handler %s with parameters (%s) and (%s)'
                            % ( aName, known[0], aParams ) )

        known.append( aParams )

    # The events of a widget ---------------------------------------------------------------

    def connect_events( self, aObj, aVar, aQtClass ):
        if self.gen.compat:
            return self.connect_events_compat( aObj, aVar, aQtClass )

        for event, handler in events( aObj ).items():
            if event not in EVENTS:
                raise FbpError( 'event %s of %s (%s) not supported'
                                % ( event, aObj.get( 'class' ), props( aObj ).get( 'name' ) ) )

            params, kind = EVENTS[event]
            self.handler( handler, params )
            src = 'static_cast<QWidget*>( %s )' % aVar if aVar != 'this' else 'this'

            if kind.startswith( 'signal:' ):
                signal = kind[len( 'signal:' ):]

                if params == SOURCE:
                    self.code.append( '    QObject::connect( %s, &%s, this, [this, src = %s]() { %s( src ); } );'
                                      % ( aVar, signal, src, handler ) )
                elif params == 'int aPage':
                    self.code.append( '    QObject::connect( %s, &%s, this, [this]( int aPage ) { %s( aPage ); } );'
                                      % ( aVar, signal, handler ) )
                elif params == 'int aRow, int aCol':
                    self.code.append( '    QObject::connect( %s, &%s, this, [this]( int aRow, int aCol ) { %s( aRow, aCol ); } );'
                                      % ( aVar, signal, handler ) )
                else:
                    raise FbpError( 'event %s: parameters' % event )

            elif kind == 'text':
                if aQtClass in ( 'QLineEdit', ):
                    signal = 'QLineEdit::textChanged'
                elif aQtClass in ( 'QPlainTextEdit', ):
                    signal = 'QPlainTextEdit::textChanged'
                elif aQtClass in ( 'QComboBox', ):
                    signal = 'QComboBox::editTextChanged'
                else:
                    raise FbpError( 'OnText of %s' % aQtClass )

                self.code.append( '    QObject::connect( %s, &%s, this, [this, src = %s]() { %s( src ); } );'
                                  % ( aVar, signal, src, handler ) )

            elif kind == 'textenter':
                if aQtClass == 'QLineEdit':
                    target = aVar
                elif aQtClass == 'QComboBox':
                    target = '%s->lineEdit()' % aVar
                else:
                    raise FbpError( 'OnTextEnter of %s' % aQtClass )

                self.code.append( '    QObject::connect( %s, &QLineEdit::returnPressed, this, [this, src = %s]() { %s( src ); } );'
                                  % ( target, src, handler ) )

            elif kind == 'searchcancel':
                self.code.append( '    QObject::connect( %s, &QLineEdit::textChanged, this, [this, src = %s]( const QString& aText ) { if( aText.isEmpty() ) %s( src ); } );'
                                  % ( aVar, src, handler ) )

            elif kind == 'spin':
                signal = 'QSpinBox::valueChanged' if aQtClass == 'QSpinBox' else 'QDoubleSpinBox::valueChanged'
                self.code.append( '    QObject::connect( %s, &%s, this, [this, src = %s]() { %s( src ); } );'
                                  % ( aVar, signal, src, handler ) )

            elif kind in ( 'spinup', 'spindown' ):
                setter = 'SetUpCallback' if kind == 'spinup' else 'SetDownCallback'
                self.code.append( '    %s->%s( [this, src = %s]() { %s( src ); } );'
                                  % ( aVar, setter, src, handler ) )

            elif kind == 'spintext':
                self.code.append( '    QObject::connect( %s, &QAbstractSpinBox::editingFinished, this, [this, src = %s]() { %s( src ); } );'
                                  % ( aVar, src, handler ) )

            elif kind.startswith( 'filter:' ):
                self.code.append( '    KIQT::OnEvent( %s, %s, [this, src = %s]( QEvent* ) { %s( src ); } );'
                                  % ( aVar, kind[len( 'filter:' ):], src, handler ) )

            elif kind == 'gridrightclick':
                # The context menu of a cell of a grid
                self.code.append( '    %s->setContextMenuPolicy( Qt::CustomContextMenu );' % aVar )
                self.code.append( '    QObject::connect( %s, &QWidget::customContextMenuRequested, this, [this, grid = %s]( const QPoint& aPos ) { '
                                  'if( QTableWidgetItem* item = grid->itemAt( aPos ) ) %s( item->row(), item->column() ); } );'
                                  % ( aVar, aVar, handler ) )

            elif kind == 'updateui':
                self.code.append( '    KIQT::RegisterUpdateUI( this, [this, src = %s]() { %s( src ); } );'
                                  % ( src, handler ) )

            elif kind == 'link':
                self.code.append( '    QObject::connect( %s, &QLabel::linkActivated, this, [this]( const QString& aUrl ) { %s( aUrl ); } );'
                                  % ( aVar, handler ) )

            elif kind == 'htmllink':
                self.code.append( '    QObject::connect( %s, &QTextBrowser::anchorClicked, this, [this]( const QUrl& aUrl ) { %s( aUrl.toString() ); } );'
                                  % ( aVar, handler ) )

            elif kind.startswith( 'stdbutton:' ):
                # Connected by the button box (see the wxStdDialogButtonSizer)
                pass

            else:
                raise FbpError( 'event kind %s' % kind )

    # The events of the compatibility mode ------------------------------------------------

    def compat_call( self, aEvClass, aType, aVar, aHandler, aFill = '', aArgs = '' ):
        """A lambda making the event of wxWidgets and calling the handler."""
        widget = 'this' if aVar == 'this' else aVar
        capture = '[this]' if aVar == 'this' else '[this, w = %s]' % aVar
        prefix = 'auto* w = this; ' if aVar == 'this' else ''
        return ( '%s( %s ) { %s%s evt( %s ); evt.SetId( wxGetWindowIdCompat( w ) ); evt.SetEventObject( w ); %s%s( evt ); }'
                 % ( capture, aArgs, prefix, aEvClass, aType, aFill, aHandler ) )

    def connect_events_compat( self, aObj, aVar, aQtClass ):
        for event, handler in events( aObj ).items():
            if event not in COMPAT_EVENTS:
                raise FbpError( 'event %s of %s (%s) not supported'
                                % ( event, aObj.get( 'class' ), props( aObj ).get( 'name' ) ) )

            evClass, evType, way = COMPAT_EVENTS[event]
            self.handler( handler, '%s& event' % evClass )
            call = lambda fill = '', args = '': self.compat_call( evClass, evType, aVar, handler, fill, args )

            if way == 'bind':
                self.code.append( '    %s->Bind( %s, [this]( %s& aEvent ) { %s( aEvent ); } );'
                                  % ( aVar, evType, evClass, handler ) )

            elif way in ( 'clicked', 'checked' ):
                fill = 'evt.SetInt( w->isChecked() ? 1 : 0 ); ' if way == 'checked' else ''
                self.code.append( '    QObject::connect( %s, &QAbstractButton::clicked, this, %s );'
                                  % ( aVar, call( fill ) ) )

            elif way == 'text':
                if aQtClass in ( 'QLineEdit', 'wxTextCtrl' ):
                    signal, getter = 'QLineEdit::textChanged', 'w->text()'
                elif aQtClass in ( 'QPlainTextEdit', 'wxTextCtrlMultiLine' ):
                    signal, getter = 'QPlainTextEdit::textChanged', 'w->toPlainText()'
                elif aQtClass in ( 'QComboBox', 'wxComboBox', 'wxChoice' ):
                    signal, getter = 'QComboBox::editTextChanged', 'w->currentText()'
                else:
                    raise FbpError( 'OnText of %s' % aQtClass )

                self.code.append( '    QObject::connect( %s, &%s, this, %s );'
                                  % ( aVar, signal, call( 'evt.SetString( ToWxString( %s ) ); ' % getter ) ) )

            elif way == 'textenter':
                if aQtClass in ( 'QLineEdit', 'wxTextCtrl' ):
                    target = aVar
                elif aQtClass in ( 'QComboBox', 'wxComboBox' ):
                    target = '%s->lineEdit()' % aVar
                else:
                    raise FbpError( 'OnTextEnter of %s' % aQtClass )

                self.code.append( '    QObject::connect( %s, &QLineEdit::returnPressed, this, %s );'
                                  % ( target, call() ) )

            elif way == 'choice':
                self.code.append( '    QObject::connect( %s, &QComboBox::activated, this, %s );'
                                  % ( aVar, call( 'evt.SetInt( aIndex ); evt.SetString( ToWxString( w->itemText( aIndex ) ) ); ', 'int aIndex' ) ) )

            elif way == 'spin':
                self.code.append( '    QObject::connect( %s, &QSpinBox::valueChanged, this, %s );'
                                  % ( aVar, call( 'evt.SetInt( aValue ); ', 'int aValue' ) ) )

            elif way == 'spindouble':
                self.code.append( '    QObject::connect( %s, &QDoubleSpinBox::valueChanged, this, %s );'
                                  % ( aVar, call( 'evt.SetValue( aValue ); ', 'double aValue' ) ) )

            elif way == 'spintext':
                self.code.append( '    QObject::connect( %s, &QAbstractSpinBox::editingFinished, this, %s );'
                                  % ( aVar, call() ) )

            elif way in ( 'slider', 'sliderthumb' ):
                signal = 'QAbstractSlider::valueChanged' if way == 'slider' else 'QAbstractSlider::sliderMoved'
                self.code.append( '    QObject::connect( %s, &%s, this, %s );'
                                  % ( aVar, signal, call( 'evt.SetInt( aValue ); ', 'int aValue' ) ) )

            elif way == 'listbox':
                self.code.append( '    QObject::connect( %s, &QListWidget::currentRowChanged, this, %s );'
                                  % ( aVar, call( 'evt.SetInt( aRow ); evt.SetString( ToWxString( w->item( aRow ) ? w->item( aRow )->text() : QString() ) ); ', 'int aRow' ) ) )

            elif way == 'listdclick':
                self.code.append( '    QObject::connect( %s, &QListWidget::itemDoubleClicked, this, %s );'
                                  % ( aVar, call( 'evt.SetInt( w->row( aItem ) ); ', 'QListWidgetItem* aItem' ) ) )

            elif way == 'checklist':
                self.code.append( '    QObject::connect( %s, &QListWidget::itemChanged, this, %s );'
                                  % ( aVar, call( 'evt.SetInt( w->row( aItem ) ); ', 'QListWidgetItem* aItem' ) ) )

            elif way in ( 'focusin', 'focusout' ):
                qtType = 'QEvent::FocusIn' if way == 'focusin' else 'QEvent::FocusOut'
                self.code.append( '    KIQT::OnEvent( %s, %s, %s );' % ( aVar, qtType, call( '', 'QEvent*' ) ) )

            elif way == 'size':
                self.code.append( '    KIQT::OnEvent( %s, QEvent::Resize, %s );'
                                  % ( aVar, call( '', 'QEvent* aQtEvent' ).replace( 'wxSizeEvent evt( wxEVT_SIZE )', 'wxSizeEvent evt( wxSizeOfEventCompat( aQtEvent ) )' ) ) )

            elif way.startswith( 'mouse:' ):
                fill = 'wxFillMouseEventCompat( evt, aQtEvent ); '
                self.code.append( '    KIQT::OnEvent( %s, %s, %s );'
                                  % ( aVar, way[len( 'mouse:' ):], call( fill, 'QEvent* aQtEvent' ) ) )

            elif way.startswith( 'key:' ):
                # A key not skipped by the handler is not processed by the widget
                src = 'static_cast<QObject*>( %s )' % aVar if aVar != 'this' else 'static_cast<QObject*>( this )'
                self.code.append( '    KIQT::FilterEvents( %s, [this, src = %s]( QEvent* aQtEvent ) { '
                                  'if( aQtEvent->type() != %s ) return false; '
                                  'wxKeyEvent evt( %s ); evt.SetEventObject( src ); wxFillKeyEventCompat( evt, aQtEvent ); '
                                  '%s( evt ); return !evt.GetSkipped(); } );'
                                  % ( aVar, src, way[len( 'key:' ):], evType, handler ) )

            elif way == 'updateui':
                src = 'static_cast<QObject*>( %s )' % aVar if aVar != 'this' else 'static_cast<QObject*>( this )'
                self.code.append( '    KIQT::RegisterUpdateUI( this, [this, src = %s]() { wxUpdateUIEvent evt; evt.SetEventObject( src ); %s( evt ); } );'
                                  % ( src, handler ) )

            elif way == 'link':
                self.code.append( '    QObject::connect( %s, &QLabel::linkActivated, this, %s );'
                                  % ( aVar, call( 'evt.SetURL( ToWxString( aUrl ) ); ', 'const QString& aUrl' ) ) )

            elif way == 'page':
                self.code.append( '    QObject::connect( %s, &QTabWidget::currentChanged, this, %s );'
                                  % ( aVar, call( 'evt.SetSelection( aPage ); ', 'int aPage' ) ) )

            elif way == 'radiobox':
                self.code.append( '    %s->OnSelected( %s );'
                                  % ( aVar, call( 'evt.SetInt( aIndex ); evt.SetString( w->GetString( aIndex ) ); ', 'int aIndex' ) ) )

            elif way == 'close':
                # The window stays open if the handler doesn't skip the event (as wxWidgets)
                self.code.append( '    KIQT::FilterEvents( %s, [this]( QEvent* aQtEvent ) { '
                                  'if( aQtEvent->type() != QEvent::Close ) return false; '
                                  'wxCloseEvent evt( wxEVT_CLOSE_WINDOW, GetId() ); evt.SetEventObject( this ); '
                                  '%s( evt ); return !evt.GetSkipped(); } );' % ( aVar, handler ) )

            elif way == 'initdialog':
                # Before the dialog is shown the first time
                self.code.append( '    KIQT::OnEvent( %s, QEvent::Show, [this, done = false]( QEvent* ) mutable { '
                                  'if( done ) return; done = true; wxInitDialogEvent evt( GetId() ); '
                                  'evt.SetEventObject( this ); %s( evt ); } );' % ( aVar, handler ) )

            elif way == 'activate':
                for qtType, active in ( ( 'QEvent::WindowActivate', 'true' ), ( 'QEvent::WindowDeactivate', 'false' ) ):
                    self.code.append( '    KIQT::OnEvent( %s, %s, [this]( QEvent* ) { '
                                      'wxActivateEvent evt( wxEVT_ACTIVATE, %s, GetId() ); evt.SetEventObject( this ); %s( evt ); } );'
                                      % ( aVar, qtType, active, handler ) )

            elif way == 'idle':
                self.code.append( '    KIQT::RegisterUpdateUI( this, [this]() { wxIdleEvent evt; evt.SetEventObject( this ); %s( evt ); } );'
                                  % handler )

            elif way in ( 'spinup', 'spindown' ):
                setter = 'SetUpCallback' if way == 'spinup' else 'SetDownCallback'
                self.code.append( '    %s->%s( %s );' % ( aVar, setter, call() ) )

            elif way == 'searchcancel':
                self.code.append( '    QObject::connect( %s, &QLineEdit::textChanged, this, [this, w = %s]( const QString& aText ) { '
                                  'if( !aText.isEmpty() ) return; %s evt( %s ); evt.SetEventObject( w ); %s( evt ); } );'
                                  % ( aVar, aVar, evClass, evType, handler ) )

            elif way == 'htmllink':
                self.code.append( '    QObject::connect( %s, &QTextBrowser::anchorClicked, this, [this, w = %s]( const QUrl& aUrl ) { '
                                  'wxHtmlLinkEvent evt( 0, wxHtmlLinkInfo( ToWxString( aUrl.toString() ) ) ); evt.SetEventObject( w ); %s( evt ); } );'
                                  % ( aVar, aVar, handler ) )

            elif way.startswith( 'stdbutton:' ):
                # Connected by the button box (see the wxStdDialogButtonSizer)
                pass

            else:
                raise FbpError( 'event way %s' % way )

    # The common properties of the widgets ---------------------------------------------------

    def common( self, aObj, aVar ):
        p = props( aObj )

        if p.get( 'tooltip' ):
            self.code.append( '    %s->setToolTip( %s );' % ( aVar, tr( p['tooltip'] ) ) )

        if p.get( 'enabled', '1' ) == '0':
            self.code.append( '    %s->setEnabled( false );' % aVar )

        if p.get( 'hidden', '0' ) == '1':
            self.code.append( '    %s->hide();' % aVar )

        w, h = parse_size( p.get( 'minimum_size' ) )
        sw, sh = parse_size( p.get( 'size' ) )
        w, h = max( w, sw ), max( h, sh )

        if w > 0:
            self.code.append( '    %s->setMinimumWidth( %d );' % ( aVar, w ) )

        if h > 0:
            self.code.append( '    %s->setMinimumHeight( %d );' % ( aVar, h ) )

        mw, mh = parse_size( p.get( 'maximum_size' ) )

        if mw > 0:
            self.code.append( '    %s->setMaximumWidth( %d );' % ( aVar, mw ) )

        if mh > 0:
            self.code.append( '    %s->setMaximumHeight( %d );' % ( aVar, mh ) )

        font = p.get( 'font', '' )

        if font:
            # face,style,weight,size,family,underlined
            parts = [ x.strip() for x in font.split( ',' ) ]

            if len( parts ) >= 4:
                style = parse_int( parts[1], 90 )
                weight = parse_int( parts[2], 90 )
                size = parse_int( parts[3], -1 )
                lines = []

                if weight == 92:
                    lines.append( 'f.setBold( true );' )

                if style == 93:
                    lines.append( 'f.setItalic( true );' )

                if size > 0:
                    lines.append( 'f.setPointSize( %d );' % size )

                if lines:
                    self.code.append( '    { QFont f = %s->font(); %s %s->setFont( f ); }'
                                      % ( aVar, ' '.join( lines ), aVar ) )

    # Widgets -----------------------------------------------------------------------------

    def widget( self, aObj, aParent ):
        """Create a widget; return its variable."""
        cls = aObj.get( 'class' )
        p = props( aObj )
        sub, subHeader, fwd = subclass( p )
        style = flags( p.get( 'style', '' ) ) | flags( p.get( 'window_style', '' ) )

        def make( aQtClass, aArgs = None ):
            """The variable of a widget of class aQtClass (or its subclass)."""
            args = aArgs if aArgs is not None else aParent
            qtType = aQtClass

            # A class of KiCad: created with the arguments of wxFormBuilder
            if sub and self.gen.compat and aArgs is None:
                ctorArgs = wx_ctor_args( cls, p, self.wx_style( p ) )

                if ctorArgs is not None:
                    args = '%s, %s, %s' % ( aParent, self.wx_id( p ), ctorArgs )

            # Without the compatibility layer, a grid is a QTableWidget (WX_GRID is the grid of
            # the layer)
            if sub == 'WX_GRID' and not self.gen.compat:
                self.qt( aQtClass )
                return self.var( aObj, qtType, 'new %s( %s )' % ( qtType, args ) ), qtType

            if sub:
                qtType = sub

                if subHeader:
                    if fwd:
                        self.gen.forward.add( sub )
                        self.include( subHeader, aInHeader = False )
                    else:
                        self.include( subHeader )
                else:
                    raise FbpError( 'subclass %s without header' % sub )
            elif self.gen.compat and cls in COMPAT_CLASSES:
                # The control of the compatibility layer (derived from aQtClass)
                qtType = COMPAT_CLASSES[cls]

                if cls == 'wxTextCtrl' and 'wxTE_MULTILINE' in style:
                    qtType = 'wxTextCtrlMultiLine'

                self.include( '<wx/kiwx_gui.h>' )
                self.qt( aQtClass )

                if cls == 'wxStyledTextCtrl':
                    self.include( '<wx/stc/stc.h>' )

                # The arguments of the constructors of wxWidgets, for the controls made with them
                if cls not in ( 'wxHyperlinkCtrl', 'wxFilePickerCtrl', 'wxDirPickerCtrl', 'wxListCtrl',
                                'wxTreeCtrl', 'wxDataViewCtrl', 'wxDataViewListCtrl', 'wxDataViewTreeCtrl',
                                'wxGrid', 'wxGenericDirCtrl' ):
                    args = aParent
            else:
                self.qt( aQtClass )

            return self.var( aObj, qtType, 'new %s( %s )' % ( qtType, args ) ), qtType

        if cls == 'wxStaticText':
            var, t = make( 'QLabel' )
            self.code.append( '    %s->setText( %s );' % ( var, tr( p.get( 'label', '' ) ) ) )

            if parse_int( p.get( 'wrap' ), -1 ) > 0:
                if self.gen.compat:
                    # The line breaks of wxWidgets, at the width of the wrap
                    self.code.append( '    %s->Wrap( %d );' % ( var, parse_int( p.get( 'wrap' ), -1 ) ) )
                else:
                    self.code.append( '    %s->setWordWrap( true );' % var )

            if 'wxALIGN_RIGHT' in style:
                self.code.append( '    %s->setAlignment( Qt::AlignRight | Qt::AlignVCenter );' % var )
            elif 'wxALIGN_CENTER_HORIZONTAL' in style or 'wxALIGN_CENTRE_HORIZONTAL' in style:
                self.code.append( '    %s->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );' % var )

            qt = 'QLabel'

        elif cls == 'wxTextCtrl':
            multiline = 'wxTE_MULTILINE' in style
            qt = 'QPlainTextEdit' if multiline else 'QLineEdit'
            var, t = make( qt )

            if p.get( 'value' ):
                self.code.append( '    %s->%s( %s );'
                                  % ( var, 'setPlainText' if multiline else 'setText', lit( p['value'] ) ) )

            if 'wxTE_READONLY' in style:
                self.code.append( '    %s->setReadOnly( true );' % var )

            if not multiline and parse_int( p.get( 'maxlength' ), 0 ) > 0:
                self.code.append( '    %s->setMaxLength( %s );' % ( var, p['maxlength'] ) )

            if not multiline and 'wxTE_PASSWORD' in style:
                self.code.append( '    %s->setEchoMode( QLineEdit::Password );' % var )

        elif cls == 'wxSearchCtrl':
            qt = 'QLineEdit'
            var, t = make( qt )
            self.code.append( '    %s->setClearButtonEnabled( true );' % var )

            if p.get( 'value' ):
                self.code.append( '    %s->setText( %s );' % ( var, lit( p['value'] ) ) )

        elif cls in ( 'wxButton', 'wxToggleButton' ):
            qt = 'QPushButton'
            var, t = make( qt )
            self.code.append( '    %s->setText( %s );' % ( var, tr( p.get( 'label', '' ) ) ) )

            if cls == 'wxToggleButton':
                self.code.append( '    %s->setCheckable( true );' % var )

            if p.get( 'default', '0' ) == '1':
                self.code.append( '    %s->setDefault( true );' % var )

        elif cls == 'wxBitmapButton':
            qt = 'QToolButton'
            var, t = make( qt )

            if p.get( 'label' ) and p.get( 'label' ) != 'MyButton':
                self.code.append( '    %s->setToolTip( %s );' % ( var, tr( p['label'] ) ) )

        elif cls == 'wxCheckBox':
            qt = 'QCheckBox'
            var, t = make( qt )
            self.code.append( '    %s->setText( %s );' % ( var, tr( p.get( 'label', '' ) ) ) )

            if p.get( 'checked', '0' ) == '1':
                self.code.append( '    %s->setChecked( true );' % var )

        elif cls == 'wxRadioButton':
            qt = 'QRadioButton'
            var, t = make( qt )
            self.code.append( '    %s->setText( %s );' % ( var, tr( p.get( 'label', '' ) ) ) )

            # The groups of wxWidgets: a button with wxRB_GROUP starts a group
            if 'wxRB_GROUP' in style or self.radioGroup is None:
                self.radioGroup = self.tmp( 'radioGroup' )
                self.qt( 'QButtonGroup' )
                self.code.append( '    QButtonGroup* %s = new QButtonGroup( this );' % self.radioGroup )

            self.code.append( '    %s->addButton( %s );' % ( self.radioGroup, var ) )

            if p.get( 'value', '0' ) == '1':
                self.code.append( '    %s->setChecked( true );' % var )

        elif cls in ( 'wxChoice', 'wxComboBox', 'wxBitmapComboBox' ):
            qt = 'QComboBox'
            var, t = make( qt )
            editable = cls == 'wxComboBox' and 'wxCB_READONLY' not in style

            if editable or t != qt:
                self.code.append( '    %s->setEditable( %s );' % ( var, 'true' if editable else 'false' ) )

            choices = parse_choices( p.get( 'choices' ) )

            for choice in choices:
                self.code.append( '    %s->addItem( %s );' % ( var, tr( choice ) ) )

            sel = parse_int( p.get( 'selection' ), -1 )

            if choices and sel >= 0:
                self.code.append( '    %s->setCurrentIndex( %d );' % ( var, sel ) )

            if editable and p.get( 'value' ):
                self.code.append( '    %s->setEditText( %s );' % ( var, lit( p['value'] ) ) )

            if not choices and not editable:
                self.code.append( '    %s->setSizeAdjustPolicy( QComboBox::AdjustToContents );' % var )

        elif cls == 'wxSpinCtrl':
            qt = 'QSpinBox'
            var, t = make( qt )
            self.code.append( '    %s->setRange( %d, %d );'
                              % ( var, parse_int( p.get( 'min' ), 0 ), parse_int( p.get( 'max' ), 100 ) ) )
            self.code.append( '    %s->setValue( %d );' % ( var, parse_int( p.get( 'initial' ), 0 ) ) )

        elif cls == 'wxSpinCtrlDouble':
            qt = 'QDoubleSpinBox'
            var, t = make( qt )
            self.code.append( '    %s->setDecimals( %d );' % ( var, parse_int( p.get( 'digits' ), 2 ) ) )
            self.code.append( '    %s->setRange( %s, %s );'
                              % ( var, parse_float( p.get( 'min' ), 0 ), parse_float( p.get( 'max' ), 100 ) ) )
            self.code.append( '    %s->setSingleStep( %s );' % ( var, parse_float( p.get( 'inc' ), 1 ) ) )
            self.code.append( '    %s->setValue( %s );' % ( var, parse_float( p.get( 'initial' ), 0 ) ) )

        elif cls == 'wxRadioBox' and self.gen.compat:
            # The box of radio buttons of the compatibility layer
            qt = 'wxRadioBox'
            self.include( '<wx/kiwx_gui.h>' )
            choices = parse_choices( p.get( 'choices' ) )
            spec = 'wxRA_SPECIFY_ROWS' if 'wxRA_SPECIFY_ROWS' in style else 'wxRA_SPECIFY_COLS'
            items = self.tmp( 'radioChoices' )
            self.code.append( '    wxArrayString %s;' % items )

            for c in choices:
                self.code.append( '    %s.Add( _( %s ) );' % ( items, cstr( c ) ) )

            var = self.var( aObj, qt, 'new wxRadioBox( %s, wxID_ANY, _( %s ), wxDefaultPosition, wxDefaultSize, %s, %d, %s )'
                            % ( aParent, cstr( p.get( 'label', '' ) ), items,
                                parse_int( p.get( 'majorDimension' ), 1 ), spec ) )
            sel = parse_int( p.get( 'selection' ), 0 )

            if choices and sel > 0:
                self.code.append( '    %s->SetSelection( %d );' % ( var, sel ) )

        elif cls == 'wxSpinButton':
            # The arrows of a spin control, without text
            qt = 'SPIN_BUTTON'
            self.include( '<widgets/spin_button.h>' )
            var = self.var( aObj, qt, 'new %s( %s )' % ( qt, aParent ) )

        elif cls == 'wxSlider':
            qt = 'QSlider'
            orient = 'Qt::Vertical' if 'wxSL_VERTICAL' in style else 'Qt::Horizontal'
            var, t = make( qt, '%s, %s' % ( orient, aParent ) )

            if t != qt:
                self.code.append( '    %s->setOrientation( %s );' % ( var, orient ) )
            self.code.append( '    %s->setRange( %d, %d );'
                              % ( var, parse_int( p.get( 'minValue' ), 0 ), parse_int( p.get( 'maxValue' ), 100 ) ) )
            self.code.append( '    %s->setValue( %d );' % ( var, parse_int( p.get( 'value' ), 0 ) ) )

        elif cls == 'wxGauge':
            qt = 'QProgressBar'
            var, t = make( qt )
            self.code.append( '    %s->setRange( 0, %d );' % ( var, parse_int( p.get( 'range' ), 100 ) ) )
            self.code.append( '    %s->setValue( %d );' % ( var, parse_int( p.get( 'value' ), 0 ) ) )

        elif cls == 'wxStaticLine':
            qt = 'QFrame'
            var, t = make( qt )
            shape = 'QFrame::VLine' if 'wxLI_VERTICAL' in style else 'QFrame::HLine'
            self.code.append( '    %s->setFrameShape( %s );' % ( var, shape ) )
            self.code.append( '    %s->setFrameShadow( QFrame::Sunken );' % var )

        elif cls == 'wxStaticBitmap':
            qt = 'QLabel'
            var, t = make( qt )

        elif cls == 'wxHyperlinkCtrl' and self.gen.compat:
            qt = 'QLabel'
            self.include( '<wx/kiwx_pickers.h>' )
            var, t = make( qt, '%s, wxID_ANY, _( %s ), wxString::FromUTF8( %s ), wxDefaultPosition, '
                               'wxDefaultSize, %s' % ( aParent, cstr( p.get( 'label', '' ) ),
                                                       cstr( p.get( 'url', '' ) ),
                                                       p.get( 'style' ) or 'wxHL_DEFAULT_STYLE' ) )

        elif cls in ( 'wxFilePickerCtrl', 'wxDirPickerCtrl' ) and self.gen.compat:
            qt = 'QWidget'
            self.include( '<wx/kiwx_pickers.h>' )

            if cls == 'wxFilePickerCtrl':
                args = '%s, wxID_ANY, wxString::FromUTF8( %s ), _( %s ), wxString::FromUTF8( %s ), ' \
                       'wxDefaultPosition, wxDefaultSize, %s' \
                       % ( aParent, cstr( p.get( 'value', '' ) ), cstr( p.get( 'message', '' ) ),
                           cstr( p.get( 'wildcard', '' ) or '*' ),
                           p.get( 'style' ) or 'wxFLP_DEFAULT_STYLE' )
            else:
                args = '%s, wxID_ANY, wxString::FromUTF8( %s ), _( %s ), wxDefaultPosition, ' \
                       'wxDefaultSize, %s' \
                       % ( aParent, cstr( p.get( 'value', '' ) ), cstr( p.get( 'message', '' ) ),
                           p.get( 'style' ) or 'wxDIRP_DEFAULT_STYLE' )

            var, t = make( qt, args )

        elif cls == 'wxHyperlinkCtrl':
            qt = 'QLabel'
            var, t = make( qt )
            url = p.get( 'url', '' )
            label = p.get( 'label', '' )
            self.code.append( '    %s->setText( QStringLiteral( "<a href=\\"%%1\\">%%2</a>" ).arg( QString::fromUtf8( %s ), %s ) );'
                              % ( var, cstr( url ), tr( label ) ) )
            self.code.append( '    %s->setTextInteractionFlags( Qt::TextBrowserInteraction );' % var )

            if not events( aObj ).get( 'OnHyperlink' ):
                self.code.append( '    %s->setOpenExternalLinks( true );' % var )

        elif cls == 'wxListBox':
            qt = 'QListWidget'
            var, t = make( qt )

            for choice in parse_choices( p.get( 'choices' ) ):
                self.code.append( '    %s->addItem( %s );' % ( var, tr( choice ) ) )

            if 'wxLB_MULTIPLE' in style or 'wxLB_EXTENDED' in style:
                self.code.append( '    %s->setSelectionMode( QAbstractItemView::ExtendedSelection );' % var )

        elif cls == 'wxListCtrl' and self.gen.compat:
            qt = 'QTreeView'
            self.include( '<wx/kiwx_listctrl.h>' )
            var, t = make( qt, '%s, wxID_ANY, wxDefaultPosition, wxDefaultSize, %s'
                               % ( aParent, p.get( 'style' ) or 'wxLC_REPORT' ) )

        elif cls == 'wxTreeCtrl' and self.gen.compat:
            # The tree of the compatibility layer (its items are added by the code of the dialog)
            qt = 'QTreeWidget'
            self.include( '<wx/treectrl.h>' )
            var, t = make( qt, '%s, wxID_ANY, wxDefaultPosition, wxDefaultSize, %s'
                               % ( aParent, p.get( 'style' ) or 'wxTR_DEFAULT_STYLE' ) )

        elif cls in ( 'wxDataViewCtrl', 'wxDataViewListCtrl', 'wxDataViewTreeCtrl' ) and self.gen.compat:
            # The data view of the compatibility layer (its model is given by the code of the dialog)
            qt = 'QTreeView'
            self.include( '<wx/dataview.h>' )
            var, t = make( qt, '%s, wxID_ANY, wxDefaultPosition, wxDefaultSize, %s'
                               % ( aParent, p.get( 'style' ) or '0' ) )

            if aObj.findall( 'object' ):
                raise FbpError( 'columns of %s in the fbp: not supported' % cls )

        elif cls == 'wxListCtrl':
            # The report view of a list (the columns are added by the code of the dialog)
            qt = 'QTreeWidget'
            var, t = make( qt )
            self.code.append( '    %s->setRootIsDecorated( false );' % var )
            self.code.append( '    %s->setUniformRowHeights( true );' % var )
            self.code.append( '    %s->setColumnCount( 0 );' % var )

            if 'wxLC_NO_HEADER' in style:
                self.code.append( '    %s->setHeaderHidden( true );' % var )

            if 'wxLC_SINGLE_SEL' not in style:
                self.code.append( '    %s->setSelectionMode( QAbstractItemView::ExtendedSelection );' % var )

        elif cls == 'wxGrid' and self.gen.compat:
            # The grid of the compatibility layer (or WX_GRID), set up as wxFormBuilder does
            qt = 'QTableView'
            self.include( '<wx/grid.h>' )
            var, t = make( qt, '%s, wxID_ANY' % aParent )

            def bool_prop( aName, aDefault ):
                return 'true' if parse_int( p.get( aName ), aDefault ) else 'false'

            def align( aName, aDefault ):
                value = ( p.get( aName ) or aDefault ).strip()
                return value if value.startswith( 'wxALIGN_' ) else aDefault

            self.code.append( '    %s->CreateGrid( %d, %d );'
                              % ( var, parse_int( p.get( 'rows' ), 0 ), parse_int( p.get( 'cols' ), 0 ) ) )
            self.code.append( '    %s->EnableEditing( %s );' % ( var, bool_prop( 'editing', 1 ) ) )
            self.code.append( '    %s->EnableGridLines( %s );' % ( var, bool_prop( 'grid_lines', 1 ) ) )
            self.code.append( '    %s->EnableDragGridSize( %s );' % ( var, bool_prop( 'drag_grid_size', 0 ) ) )
            self.code.append( '    %s->SetMargins( %d, %d );'
                              % ( var, parse_int( p.get( 'margin_width' ), 0 ),
                                  parse_int( p.get( 'margin_height' ), 0 ) ) )

            # The columns
            for col, size in enumerate( ( p.get( 'column_sizes' ) or '' ).split( ',' ) ):
                if size.strip():
                    self.code.append( '    %s->SetColSize( %d, %d );' % ( var, col, parse_int( size, 80 ) ) )

            if parse_int( p.get( 'autosize_cols' ), 0 ):
                self.code.append( '    %s->AutoSizeColumns();' % var )

            self.code.append( '    %s->EnableDragColMove( %s );' % ( var, bool_prop( 'drag_col_move', 0 ) ) )
            self.code.append( '    %s->EnableDragColSize( %s );' % ( var, bool_prop( 'drag_col_size', 1 ) ) )

            for col, label in enumerate( parse_choices( p.get( 'col_label_values' ) ) ):
                self.code.append( '    %s->SetColLabelValue( %d, _( %s ) );' % ( var, col, cstr( label ) ) )

            if p.get( 'col_label_size' ) not in ( None, '', 'wxGRID_AUTOSIZE' ):
                self.code.append( '    %s->SetColLabelSize( %d );' % ( var, parse_int( p.get( 'col_label_size' ), 30 ) ) )

            self.code.append( '    %s->SetColLabelAlignment( %s, %s );'
                              % ( var, align( 'col_label_horiz_alignment', 'wxALIGN_CENTER' ),
                                  align( 'col_label_vert_alignment', 'wxALIGN_CENTER' ) ) )

            # The rows
            for row, size in enumerate( ( p.get( 'row_sizes' ) or '' ).split( ',' ) ):
                if size.strip():
                    self.code.append( '    %s->SetRowSize( %d, %d );' % ( var, row, parse_int( size, 20 ) ) )

            self.code.append( '    %s->EnableDragRowSize( %s );' % ( var, bool_prop( 'drag_row_size', 1 ) ) )

            if p.get( 'row_label_size' ) not in ( None, '', 'wxGRID_AUTOSIZE' ):
                self.code.append( '    %s->SetRowLabelSize( %d );' % ( var, parse_int( p.get( 'row_label_size' ), 80 ) ) )

            self.code.append( '    %s->SetRowLabelAlignment( %s, %s );'
                              % ( var, align( 'row_label_horiz_alignment', 'wxALIGN_CENTER' ),
                                  align( 'row_label_vert_alignment', 'wxALIGN_CENTER' ) ) )

            # The cells
            self.code.append( '    %s->SetDefaultCellAlignment( %s, %s );'
                              % ( var, align( 'cell_horiz_alignment', 'wxALIGN_LEFT' ),
                                  align( 'cell_vert_alignment', 'wxALIGN_TOP' ) ) )

        elif cls == 'wxGrid':
            # A table (WX_GRID of the Qt build), its rows and columns set by the code
            qt = 'QTableWidget'
            var, t = make( qt )
            self.code.append( '    %s->setRowCount( %d );' % ( var, parse_int( p.get( 'rows' ), 0 ) ) )
            self.code.append( '    %s->setColumnCount( %d );' % ( var, parse_int( p.get( 'cols' ), 0 ) ) )

            if parse_int( p.get( 'col_label_size' ), 1 ) == 0:
                self.code.append( '    %s->horizontalHeader()->hide();' % var )

            if parse_int( p.get( 'row_label_size' ), 1 ) == 0:
                self.code.append( '    %s->verticalHeader()->hide();' % var )

            # The labels of the columns ("Name" "Value"), translated
            labels = parse_choices( p.get( 'col_label_values' ) )

            if labels:
                self.code.append( '    %s->setHorizontalHeaderLabels( { %s } );'
                                  % ( var, ', '.join( tr( label ) for label in labels ) ) )

            self.qt( 'QHeaderView' )

        elif cls == 'wxCheckListBox':
            qt = 'QListWidget'
            var, t = make( qt )

            for choice in parse_choices( p.get( 'choices' ) ):
                item = self.tmp( 'checkItem' )
                self.code.append( '    QListWidgetItem* %s = new QListWidgetItem( %s, %s );' % ( item, tr( choice ), var ) )
                self.code.append( '    %s->setFlags( %s->flags() | Qt::ItemIsUserCheckable );' % ( item, item ) )
                self.code.append( '    %s->setCheckState( Qt::Unchecked );' % item )

        elif cls == 'wxGenericDirCtrl' and self.gen.compat:
            qt = 'QWidget'
            var, t = make( qt, '%s, %s, wxEmptyString, wxDefaultPosition, wxDefaultSize, %s, '
                                      'wxEmptyString, %s' % ( aParent, self.wx_id( p ), self.wx_style( p ),
                                                              parse_int( p.get( 'defaultfilter' ), 0 ) ) )
            self.include( '<wx/dirctrl.h>' )
            self.code.append( '    %s->ShowHidden( %s );' % ( var, 'true' if p.get( 'show_hidden' ) == '1' else 'false' ) )

        elif cls == 'wxHtmlWindow':
            qt = 'QTextBrowser'
            var, t = make( qt )
            self.code.append( '    %s->setOpenLinks( false );' % var )

        elif cls == 'wxStyledTextCtrl':
            qt = 'QPlainTextEdit'
            var, t = make( qt )
            self.code.append( '    %s->setLineWrapMode( QPlainTextEdit::NoWrap );' % var )

        elif cls == 'wxDatePickerCtrl':
            qt = 'QDateEdit'
            var, t = make( qt )
            self.code.append( '    %s->setCalendarPopup( true );' % var )

            if self.gen.compat:
                self.include( '<wx/datectrl.h>' )

        elif cls == 'wxInfoBar':
            qt = 'QFrame'
            var, t = make( qt )

        elif cls == 'CustomControl':
            klass = p.get( 'class', '' ).strip()

            if not klass:
                raise FbpError( 'custom control %s without class' % p.get( 'name' ) )

            for line in p.get( 'include', '' ).splitlines():
                m = re.match( r'\s*#include\s*([<"][^>"]+[>"])', line )

                # The headers of wxWidgets have no use in Qt
                if m and not m.group( 1 )[1:].startswith( 'wx/' ):
                    self.include( m.group( 1 ) )

            if klass in WX_CUSTOM_CLASSES:
                # A class of wxWidgets: its Qt counterpart
                qt = WX_CUSTOM_CLASSES[klass]
                var, t = make( qt )

                if qt == 'QScrollArea':
                    self.code.append( '    %s->setWidgetResizable( true );' % var )
                    self.code.append( '    %s->setFrameShape( QFrame::NoFrame );' % var )
            else:
                # The header of the class can be in its subclass property only
                if sub == klass and subHeader:
                    if fwd:
                        self.gen.forward.add( klass )
                        self.include( subHeader, aInHeader = False )
                    else:
                        self.include( subHeader )

                args = aParent

                if self.gen.compat:
                    # The construction code of wxFormBuilder, in the parent of the generated code
                    construction = p.get( 'construction', '' ).strip()
                    m = re.match( r'^\s*\w+\s*=\s*new\s+%s\s*\(\s*[^,()]+(?:\([^()]*\))?\s*(,.*)\)\s*;\s*$'
                                  % re.escape( klass ), construction, re.S )

                    if m:
                        args = aParent + m.group( 1 ).rstrip()
                    else:
                        args = '%s, %s, %s' % ( aParent, self.wx_id( p ),
                                                wx_ctor_args( 'CustomControl', p, self.wx_style( p ) ) )

                declaration = p.get( 'declaration', '' ).strip() if self.gen.compat else ''
                perm = p.get( 'permission', 'protected' )

                if declaration and perm in ( 'protected', 'public', 'private' ):
                    # The members of the declaration of wxFormBuilder (the control and its data)
                    for line in declaration.splitlines():
                        if line.strip():
                            self.members.append( ( perm, None, line.strip() ) )

                    var = p.get( 'name' )
                    self.code.append( '    %s = new %s( %s );' % ( var, klass, args ) )
                else:
                    var = self.var( aObj, klass, 'new %s( %s )' % ( klass, args ) )

                qt = klass

        elif cls in ( 'wxPanel', 'wxScrolledWindow', 'wxNotebook', 'wxAuiNotebook', 'wxSimplebook',
                      'wxSplitterWindow' ):
            return self.container( aObj, aParent )

        else:
            raise FbpError( 'class %s (%s) not supported' % ( cls, p.get( 'name' ) ) )

        self.common( aObj, var )
        self.window_id( aObj, var )
        self.connect_events( aObj, var, qt if not sub else self.base_qt( cls, style ) )
        return var

    def wx_id( self, aProps ):
        """The id of a window for its constructor (compatibility mode)."""
        wid = aProps.get( 'id', 'wxID_ANY' ).strip() or 'wxID_ANY'

        if wid not in STANDARD_IDS:
            self.gen.ids.append( wid )

        return wid

    def wx_style( self, aProps ):
        """The style of a window for its constructor, as wxFormBuilder writes it."""
        parts = [ aProps.get( k, '' ).strip() for k in ( 'style', 'window_style' ) ]
        parts = [ x for x in parts if x ]
        return '|'.join( parts ) if parts else '0'

    def window_id( self, aObj, aVar ):
        """The id of a window (compatibility mode): the code of the dialogs compares them."""
        if not self.gen.compat:
            return

        wid = props( aObj ).get( 'id', 'wxID_ANY' ).strip()

        if wid and wid != 'wxID_ANY':
            if wid not in STANDARD_IDS:
                self.gen.ids.append( wid )

            self.code.append( '    wxSetWindowIdCompat( %s, %s );' % ( aVar, wid ) )

    def base_qt( self, aClass, aStyle ):
        """The Qt class of a wx class (for the signals of a subclass)."""
        return { 'wxTextCtrl': 'QPlainTextEdit' if 'wxTE_MULTILINE' in aStyle else 'QLineEdit',
                 'wxChoice': 'QComboBox', 'wxComboBox': 'QComboBox', 'wxBitmapComboBox': 'QComboBox',
                 'wxSpinCtrl': 'QSpinBox', 'wxSpinCtrlDouble': 'QDoubleSpinBox' }.get( aClass, '' )

    # Containers ----------------------------------------------------------------------------

    def container( self, aObj, aParent ):
        cls = aObj.get( 'class' )
        p = props( aObj )
        sub, subHeader, fwd = subclass( p )
        style = flags( p.get( 'style', '' ) ) | flags( p.get( 'window_style', '' ) )

        def klass( aQt ):
            if sub:
                if not subHeader:
                    raise FbpError( 'subclass %s without header' % sub )

                if fwd:
                    self.gen.forward.add( sub )
                    self.include( subHeader, aInHeader = False )
                else:
                    self.include( subHeader )

                return sub

            return self.qt( aQt )

        if self.gen.compat and not sub and cls in ( 'wxPanel', 'wxNotebook', 'wxAuiNotebook' ):
            self.include( '<wx/kiwx_gui.h>' )

        def compat_klass( aQt, aCompat ):
            return aCompat if self.gen.compat and not sub else klass( aQt )

        if cls == 'wxPanel':
            t = compat_klass( 'QWidget', 'wxPanel' )
            var = self.var( aObj, t, 'new %s( %s )' % ( t, aParent ) )
            self.common( aObj, var )
            self.window_id( aObj, var )
            self.children_layout( aObj, var )
            self.connect_events( aObj, var, 'QWidget' )
            return var

        if cls == 'wxScrolledWindow' and self.gen.compat and not sub:
            # The sizer is in the content of the scroll area, which holds the children; they are
            # created in the window (their parent with wxWidgets), the layout moves them
            # The style (the directions of scrolling) and the scroll rates, as wxFormBuilder
            # writes them: they make the best size of the window (wxScrolled::DoGetBestSize)
            style = self.wx_style( p )

            if style == '0':
                style = 'wxHSCROLL|wxVSCROLL'

            var = self.var( aObj, 'wxScrolledWindow',
                            'new wxScrolledWindow( %s, wxID_ANY, wxDefaultPosition, wxDefaultSize, '
                            '%s )' % ( aParent, style ) )
            self.code.append( '    %s->SetScrollRate( %s, %s );'
                              % ( var, p.get( 'scroll_rate_x', '5' ) or '5',
                                  p.get( 'scroll_rate_y', '5' ) or '5' ) )
            self.qt( 'QWidget' )
            content = self.tmp( 'scrolled' )
            self.code.append( '    QWidget* %s = %s->GetClientWidgetCompat();' % ( content, var ) )
            self.common( aObj, var )
            self.window_id( aObj, var )
            self.children_layout( aObj, var, content )
            self.connect_events( aObj, var, 'QWidget' )
            return var

        if cls == 'wxScrolledWindow':
            t = klass( 'QScrollArea' )
            var = self.var( aObj, t, 'new %s( %s )' % ( t, aParent ) )
            self.qt( 'QWidget' )
            content = self.tmp( 'scrolled' )
            self.code.append( '    QWidget* %s = new QWidget( %s );' % ( content, var ) )
            self.code.append( '    %s->setWidgetResizable( true );' % var )
            self.code.append( '    %s->setFrameShape( QFrame::NoFrame );' % var )
            self.common( aObj, var )
            self.children_layout( aObj, content )
            self.code.append( '    %s->setWidget( %s );' % ( var, content ) )
            self.connect_events( aObj, var, 'QWidget' )
            return var

        if cls in ( 'wxNotebook', 'wxAuiNotebook' ):
            t = compat_klass( 'QTabWidget', cls )
            var = self.var( aObj, t, 'new %s( %s )' % ( t, aParent ) )
            self.common( aObj, var )
            selected = None

            for page in aObj.findall( 'object' ):
                if page.get( 'class' ) not in ( 'notebookpage', 'auinotebookpage' ):
                    raise FbpError( 'notebook item %s' % page.get( 'class' ) )

                pp = props( page )

                for child in page.findall( 'object' ):
                    w = self.widget( child, var )
                    self.code.append( '    %s->addTab( %s, %s );' % ( var, w, tr( pp.get( 'label', '' ) ) ) )

                    if pp.get( 'select', '0' ) == '1':
                        selected = w

            if selected:
                self.code.append( '    %s->setCurrentWidget( %s );' % ( var, selected ) )

            self.connect_events( aObj, var, 'QTabWidget' )
            return var

        if cls == 'wxSimplebook':
            t = compat_klass( 'QStackedWidget', 'wxSimplebook' )
            var = self.var( aObj, t, 'new %s( %s )' % ( t, aParent ) )
            self.common( aObj, var )
            selected = None

            for page in aObj.findall( 'object' ):
                pp = props( page )

                for child in page.findall( 'object' ):
                    w = self.widget( child, var )
                    self.code.append( '    %s->addWidget( %s );' % ( var, w ) )

                    if pp.get( 'select', '0' ) == '1':
                        selected = w

            if selected:
                self.code.append( '    %s->setCurrentWidget( %s );' % ( var, selected ) )

            self.connect_events( aObj, var, 'QStackedWidget' )
            return var

        if cls == 'wxSplitterWindow' and self.gen.compat and not sub:
            var = self.var( aObj, 'wxSplitterWindow', 'new wxSplitterWindow( %s )' % aParent )
            self.common( aObj, var )
            self.window_id( aObj, var )
            windows = []

            for item in aObj.findall( 'object' ):
                for child in item.findall( 'object' ):
                    windows.append( self.widget( child, var ) )

            pos = parse_int( p.get( 'sashpos' ), 0 )
            split = 'SplitVertically' if p.get( 'splitmode', 'wxSPLIT_VERTICAL' ) == 'wxSPLIT_VERTICAL' \
                    else 'SplitHorizontally'

            if len( windows ) == 2:
                self.code.append( '    %s->%s( %s, %s, %d );' % ( var, split, windows[0], windows[1], pos ) )
            elif len( windows ) == 1:
                self.code.append( '    %s->Initialize( %s );' % ( var, windows[0] ) )

            gravity = p.get( 'sashgravity' )

            if gravity and float( gravity ) > 0:
                self.code.append( '    %s->SetSashGravity( %s );' % ( var, gravity ) )

            minPane = parse_int( p.get( 'min_pane_size' ), 0 )

            if minPane > 0:
                self.code.append( '    %s->SetMinimumPaneSize( %d );' % ( var, minPane ) )

            self.connect_events( aObj, var, 'QSplitter' )
            return var

        if cls == 'wxSplitterWindow':
            t = klass( 'QSplitter' )
            orient = 'Qt::Horizontal' if p.get( 'splitmode', 'wxSPLIT_VERTICAL' ) == 'wxSPLIT_VERTICAL' else 'Qt::Vertical'
            var = self.var( aObj, t, 'new %s( %s, %s )' % ( t, orient, aParent ) )
            self.common( aObj, var )

            for item in aObj.findall( 'object' ):
                for child in item.findall( 'object' ):
                    w = self.widget( child, var )
                    self.code.append( '    %s->addWidget( %s );' % ( var, w ) )

            pos = parse_int( p.get( 'sashpos' ), 0 )

            if pos > 0:
                self.code.append( '    %s->setSizes( { %d, 1000 } );' % ( var, pos ) )

            self.connect_events( aObj, var, 'QSplitter' )
            return var

        raise FbpError( 'container %s' % cls )

    def children_layout( self, aObj, aWidget, aLayoutWidget = None ):
        """The layout of a window: its sizer.  The children are created in aWidget, the layout is
        set on aLayoutWidget (the content of a scroll area), aWidget by default."""
        for child in aObj.findall( 'object' ):
            if child.get( 'class' ) in SIZERS:
                layout = self.sizer( child, aWidget, aTop = True )
                self.code.append( '    %s->setLayout( %s );' % ( aLayoutWidget or aWidget, layout ) )
            else:
                raise FbpError( '%s in a window' % child.get( 'class' ) )

    # Sizers ---------------------------------------------------------------------------

    def sizer( self, aObj, aParent, aTop = False ):
        """Create the layout of a sizer, with the widgets of its items (children of aParent)."""
        cls = aObj.get( 'class' )
        p = props( aObj )
        saveGroup = self.radioGroup

        if self.gen.compat and cls in ( 'wxBoxSizer', 'wxStaticBoxSizer', 'wxFlexGridSizer', 'wxGridSizer',
                                         'wxGridBagSizer', 'wxWrapSizer' ):
            # The sizers of the compatibility layer
            self.include( '<wx/kiwx_gui.h>' )
            vertical = p.get( 'orient' ) == 'wxVERTICAL'
            orient = 'wxVERTICAL' if vertical else 'wxHORIZONTAL'

            if cls in ( 'wxBoxSizer', 'wxWrapSizer' ):
                var = self.var( aObj, 'wxBoxSizer', 'new wxBoxSizer( %s )' % orient )
                self.box_items( aObj, var, aParent, vertical )
            elif cls == 'wxStaticBoxSizer':
                box = self.compat_static_box( aObj, aParent )
                self.radioGroup = saveGroup
                return self.wrap_widget( box )
            elif cls in ( 'wxFlexGridSizer', 'wxGridSizer' ):
                var = self.var( aObj, 'wxFlexGridSizer', 'new wxFlexGridSizer( %d )' % parse_int( p.get( 'cols' ), 0 ) )
                self.grid_items( aObj, var, aParent )
            else:
                var = self.var( aObj, 'wxGridBagSizer', 'new wxGridBagSizer()' )
                self.gridbag_items( aObj, var, aParent )

        elif cls == 'wxBoxSizer':
            t = self.qt( 'QVBoxLayout' if p.get( 'orient' ) == 'wxVERTICAL' else 'QHBoxLayout' )
            var = self.var( aObj, t, 'new %s()' % t )
            self.box_items( aObj, var, aParent, p.get( 'orient' ) == 'wxVERTICAL' )

        elif cls == 'wxStaticBoxSizer':
            # The member is the box (QGroupBox), the items are in its layout
            self.qt( 'QGroupBox' )
            box = self.var( aObj, 'QGroupBox', 'new QGroupBox( %s, %s )' % ( tr( p.get( 'label', '' ) ), aParent ) )
            vertical = p.get( 'orient' ) == 'wxVERTICAL'
            t = self.qt( 'QVBoxLayout' if vertical else 'QHBoxLayout' )
            inner = self.tmp( 'boxLayout' )
            self.code.append( '    %s* %s = new %s( %s );' % ( t, inner, t, box ) )
            self.box_items( aObj, inner, box, vertical )
            self.common( aObj, box )
            self.radioGroup = saveGroup
            return self.wrap_widget( box )

        elif cls in ( 'wxFlexGridSizer', 'wxGridSizer' ):
            t = self.qt( 'QGridLayout' )
            var = self.var( aObj, t, 'new QGridLayout()' )
            self.grid_items( aObj, var, aParent )

        elif cls == 'wxGridBagSizer':
            t = self.qt( 'QGridLayout' )
            var = self.var( aObj, t, 'new QGridLayout()' )
            self.gridbag_items( aObj, var, aParent )

        elif cls == 'wxStdDialogButtonSizer':
            var = self.button_box( aObj, aParent )
            self.radioGroup = saveGroup
            return self.wrap_widget( var )

        else:
            raise FbpError( 'sizer %s' % cls )

        self.radioGroup = saveGroup
        return var

    def compat_static_box( self, aObj, aParent ):
        """A wxStaticBoxSizer of the compatibility mode: its box (returned) and its sizer."""
        p = props( aObj )
        vertical = p.get( 'orient' ) == 'wxVERTICAL'
        box = self.tmp( 'staticBox' )
        self.code.append( '    wxStaticBox* %s = new wxStaticBox( %s, wxID_ANY, %s );'
                          % ( box, aParent, tr( p.get( 'label', '' ) ) ) )
        var = self.var( aObj, 'wxStaticBoxSizer', 'new wxStaticBoxSizer( %s, %s )'
                        % ( box, 'wxVERTICAL' if vertical else 'wxHORIZONTAL' ) )
        saveGroup = self.radioGroup
        self.box_items( aObj, var, box, vertical )
        self.radioGroup = saveGroup
        self.common( aObj, box )
        return box

    def wrap_widget( self, aWidget ):
        """A layout holding only a widget (for the sizers that are widgets in Qt)."""
        self.qt( 'QVBoxLayout' )
        lay = self.tmp( 'widgetLayout' )
        self.code.append( '    QVBoxLayout* %s = new QVBoxLayout();' % lay )
        self.code.append( '    %s->setContentsMargins( 0, 0, 0, 0 );' % lay )
        self.code.append( '    %s->addWidget( %s );' % ( lay, aWidget ) )
        return lay

    def item( self, aItem, aParent ):
        """The widget or layout of a sizer item: ('widget'|'layout'|'spacer', var or size)."""
        children = aItem.findall( 'object' )

        if len( children ) != 1:
            raise FbpError( 'sizer item with %d objects' % len( children ) )

        child = children[0]
        cls = child.get( 'class' )

        if cls == 'spacer':
            cp = props( child )
            return 'spacer', ( parse_int( cp.get( 'width' ), 0 ), parse_int( cp.get( 'height' ), 0 ) )

        if cls in SIZERS:
            if cls in ( 'wxStdDialogButtonSizer', ):
                return 'widget', self.button_box( child, aParent )

            if cls == 'wxStaticBoxSizer' and self.gen.compat:
                self.include( '<wx/kiwx_gui.h>' )
                return 'widget', self.compat_static_box( child, aParent )

            if cls == 'wxStaticBoxSizer':
                cp = props( child )
                self.qt( 'QGroupBox' )
                box = self.var( child, 'QGroupBox',
                                'new QGroupBox( %s, %s )' % ( tr( cp.get( 'label', '' ) ), aParent ) )
                vertical = cp.get( 'orient' ) == 'wxVERTICAL'
                t = self.qt( 'QVBoxLayout' if vertical else 'QHBoxLayout' )
                inner = self.tmp( 'boxLayout' )
                self.code.append( '    %s* %s = new %s( %s );' % ( t, inner, t, box ) )
                saveGroup = self.radioGroup
                self.box_items( child, inner, box, vertical )
                self.radioGroup = saveGroup
                self.common( child, box )
                return 'widget', box

            return 'layout', self.sizer( child, aParent )

        return 'widget', self.widget( child, aParent )

    def alignment( self, aFlags, aVertical, aExpand ):
        """The alignment of an item in a box layout (0 to fill the cell)."""
        parts = []

        if aVertical:
            # The cross axis is horizontal
            if aExpand:
                pass
            elif 'wxALIGN_CENTER_HORIZONTAL' in aFlags or 'wxALIGN_CENTER' in aFlags:
                parts.append( 'Qt::AlignHCenter' )
            elif 'wxALIGN_RIGHT' in aFlags:
                parts.append( 'Qt::AlignRight' )
            else:
                parts.append( 'Qt::AlignLeft' )
        else:
            if aExpand:
                pass
            elif 'wxALIGN_CENTER_VERTICAL' in aFlags or 'wxALIGN_CENTER' in aFlags:
                parts.append( 'Qt::AlignVCenter' )
            elif 'wxALIGN_BOTTOM' in aFlags:
                parts.append( 'Qt::AlignBottom' )
            else:
                parts.append( 'Qt::AlignTop' )

        return ' | '.join( parts ) if parts else 'Qt::Alignment()'

    def box_items( self, aSizer, aLayout, aParent, aVertical ):
        for item in aSizer.findall( 'object' ):
            if item.get( 'class' ) != 'sizeritem':
                raise FbpError( 'box sizer item %s' % item.get( 'class' ) )

            ip = props( item )
            f = flags( ip.get( 'flag', '' ) )
            stretch = parse_int( ip.get( 'proportion' ), 0 )
            kind, value = self.item( item, aParent )

            if kind == 'spacer':
                if stretch > 0:
                    self.code.append( '    %s->addStretch( %d );' % ( aLayout, stretch ) )
                else:
                    size = value[1] if aVertical else value[0]
                    self.code.append( '    %s->addSpacing( %d );' % ( aLayout, max( size, 0 ) ) )
            elif kind == 'layout':
                self.code.append( '    %s->addLayout( %s, %d );' % ( aLayout, value, stretch ) )
            else:
                expand = 'wxEXPAND' in f or 'wxGROW' in f
                self.code.append( '    %s->addWidget( %s, %d, %s );'
                                  % ( aLayout, value, stretch, self.alignment( f, aVertical, expand ) ) )

    def cell_alignment( self, aFlags ):
        if 'wxEXPAND' in aFlags or 'wxGROW' in aFlags:
            return 'Qt::Alignment()'

        parts = []

        if 'wxALIGN_RIGHT' in aFlags:
            parts.append( 'Qt::AlignRight' )
        elif 'wxALIGN_CENTER_HORIZONTAL' in aFlags or 'wxALIGN_CENTER' in aFlags:
            parts.append( 'Qt::AlignHCenter' )
        else:
            parts.append( 'Qt::AlignLeft' )

        if 'wxALIGN_BOTTOM' in aFlags:
            parts.append( 'Qt::AlignBottom' )
        elif 'wxALIGN_CENTER_VERTICAL' in aFlags or 'wxALIGN_CENTER' in aFlags:
            parts.append( 'Qt::AlignVCenter' )
        else:
            parts.append( 'Qt::AlignTop' )

        return ' | '.join( parts )

    def place( self, aLayout, aKind, aValue, aRow, aCol, aRowSpan, aColSpan, aFlags ):
        if aKind == 'spacer':
            self.qt( 'QSpacerItem' )
            self.code.append( '    %s->addItem( new QSpacerItem( %d, %d ), %d, %d, %d, %d );'
                              % ( aLayout, max( aValue[0], 0 ), max( aValue[1], 0 ), aRow, aCol, aRowSpan, aColSpan ) )
        elif aKind == 'layout':
            self.code.append( '    %s->addLayout( %s, %d, %d, %d, %d );'
                              % ( aLayout, aValue, aRow, aCol, aRowSpan, aColSpan ) )
        else:
            self.code.append( '    %s->addWidget( %s, %d, %d, %d, %d, %s );'
                              % ( aLayout, aValue, aRow, aCol, aRowSpan, aColSpan, self.cell_alignment( aFlags ) ) )

    def grid_gaps( self, aSizer, aLayout ):
        p = props( aSizer )
        self.code.append( '    %s->setHorizontalSpacing( %d );' % ( aLayout, max( parse_int( p.get( 'hgap' ), 0 ), 4 ) ) )
        self.code.append( '    %s->setVerticalSpacing( %d );' % ( aLayout, max( parse_int( p.get( 'vgap' ), 0 ), 4 ) ) )

        for key, fn in ( ( 'growablecols', 'setColumnStretch' ), ( 'growablerows', 'setRowStretch' ) ):
            for g in ( p.get( key ) or '' ).split( ',' ):
                g = g.strip()

                if not g:
                    continue

                index, _, prop = g.partition( ':' )
                self.code.append( '    %s->%s( %d, %d );' % ( aLayout, fn, int( index ), parse_int( prop, 1 ) or 1 ) )

    def grid_items( self, aSizer, aLayout, aParent ):
        p = props( aSizer )
        cols = parse_int( p.get( 'cols' ), 0 )
        rows = parse_int( p.get( 'rows' ), 0 )
        n = 0

        for item in aSizer.findall( 'object' ):
            if item.get( 'class' ) != 'sizeritem':
                raise FbpError( 'grid sizer item %s' % item.get( 'class' ) )

            ip = props( item )
            kind, value = self.item( item, aParent )

            if cols > 0:
                r, c = divmod( n, cols )
            elif rows > 0:
                c, r = divmod( n, rows )
            else:
                r, c = n, 0

            self.place( aLayout, kind, value, r, c, 1, 1, flags( ip.get( 'flag', '' ) ) )
            n += 1

        self.grid_gaps( aSizer, aLayout )

    def gridbag_items( self, aSizer, aLayout, aParent ):
        for item in aSizer.findall( 'object' ):
            if item.get( 'class' ) != 'gbsizeritem':
                raise FbpError( 'grid bag sizer item %s' % item.get( 'class' ) )

            ip = props( item )
            kind, value = self.item( item, aParent )
            self.place( aLayout, kind, value, parse_int( ip.get( 'row' ), 0 ), parse_int( ip.get( 'column' ), 0 ),
                        max( parse_int( ip.get( 'rowspan' ), 1 ), 1 ), max( parse_int( ip.get( 'colspan' ), 1 ), 1 ),
                        flags( ip.get( 'flag', '' ) ) )

        self.grid_gaps( aSizer, aLayout )

    def button_box( self, aObj, aParent ):
        if self.gen.compat:
            return self.button_box_compat( aObj, aParent )

        p = props( aObj )
        ev = events( aObj )
        self.qt( 'QDialogButtonBox' )
        self.qt( 'QPushButton' )
        box = self.var( aObj, 'QDialogButtonBox', 'new QDialogButtonBox( %s )' % aParent )
        name = p.get( 'name' )

        for prop, suffix, qtButton, wxId in STD_BUTTONS:
            if p.get( prop, '0' ) != '1':
                continue

            button = name + suffix if name else self.tmp( 'button' )
            perm = p.get( 'permission', 'protected' )

            if name and perm in ( 'protected', 'public', 'private' ):
                self.members.append( ( perm, 'QPushButton', button ) )
                self.code.append( '    %s = %s->addButton( QDialogButtonBox::%s );' % ( button, box, qtButton ) )
            else:
                self.code.append( '    QPushButton* %s = %s->addButton( QDialogButtonBox::%s );'
                                  % ( button, box, qtButton ) )

            # The handler of the button, or the standard action of KiCad
            handler = None

            for evName, ( params, kind ) in EVENTS.items():
                if kind == 'stdbutton:%s' % qtButton and evName in ev and \
                        ( prop != 'ContextHelp' or evName == 'OnContextHelpButtonClick' ):
                    handler = ev[evName]

            if handler:
                # As the handlers of wxFormBuilder (that skip the event), the default handler
                # does what the button does without handler: standardButtonClicked()
                self.handler( handler, SOURCE )
                self.stdHandlers.add( handler )
                self.code.append( '    %s->setProperty( "kicadStandardButtonId", %s );' % ( button, wxId ) )
                self.code.append( '    QObject::connect( %s, &QPushButton::clicked, this, [this, src = static_cast<QWidget*>( %s )]() { %s( src ); } );'
                                  % ( button, button, handler ) )
            else:
                self.code.append( '    connectStandardButton( %s, %s );' % ( button, wxId ) )

            if prop == 'OK':
                self.code.append( '    %s->setDefault( true );' % button )

        return box

    def button_box_compat( self, aObj, aParent ):
        """The wxStdDialogButtonSizer of the compatibility mode: its buttons are wxButtons."""
        p = props( aObj )
        ev = events( aObj )
        self.include( '<wx/kiwx_gui.h>' )
        box = self.var( aObj, 'wxStdDialogButtonSizer', 'new wxStdDialogButtonSizer( %s )' % aParent )
        name = p.get( 'name' )

        for prop, suffix, qtButton, wxId in STD_BUTTONS:
            if p.get( prop, '0' ) != '1':
                continue

            button = name + suffix if name else self.tmp( 'button' )
            perm = p.get( 'permission', 'protected' )
            expr = '%s->AddStandardButton( QDialogButtonBox::%s, %s )' % ( box, qtButton, wxId )

            if name and perm in ( 'protected', 'public', 'private' ):
                self.members.append( ( perm, 'wxButton', button ) )
                self.code.append( '    %s = %s;' % ( button, expr ) )
            else:
                self.code.append( '    wxButton* %s = %s;' % ( button, expr ) )

            # The handler of the button, or the standard action of KiCad
            handler = None

            for evName, ( evClass, evType, way ) in COMPAT_EVENTS.items():
                if way == 'stdbutton:%s' % qtButton and evName in ev and \
                        ( prop != 'ContextHelp' or evName == 'OnContextHelpButtonClick' ):
                    handler = ev[evName]

            if handler:
                # As with wxWidgets, the button does its standard action if the handler skips
                # the event (the default handler)
                self.handler( handler, 'wxCommandEvent& event' )
                self.code.append( '    %s->setProperty( "kicadStandardButtonId", %s );' % ( button, wxId ) )
                self.code.append( '    QObject::connect( %s, &QPushButton::clicked, this, [this, w = %s]() { '
                                  'wxCommandEvent evt( wxEVT_BUTTON, %s ); evt.SetEventObject( w ); %s( evt ); '
                                  'if( evt.GetSkipped() ) standardButtonClicked( w ); } );'
                                  % ( button, button, wxId, handler ) )
            else:
                self.code.append( '    connectStandardButton( %s, %s );' % ( button, wxId ) )

            if prop == 'OK':
                self.code.append( '    %s->setDefault( true );' % button )

        return box

    # The class -----------------------------------------------------------------------------

    def generate( self ):
        self.radioGroup = None
        p = self.p
        sub, subHeader, fwd = subclass( p )

        if self.kind == 'Dialog':
            base = sub or 'DIALOG_SHIM'

            if sub and subHeader:
                self.include( subHeader )
            else:
                self.include( '<dialog_shim.h>' )

            ctorDecl = 'QWidget* aParent, const wxString& aTitle = _( %s )' % cstr( p.get( 'title', '' ) )
            ctorImpl = 'QWidget* aParent, const wxString& aTitle'
            init = '%s( aParent, aTitle )' % base

            if self.gen.compat:
                # The parameters of the bases of wxFormBuilder
                ctorDecl = ( 'wxWidgetArgCompat aParent, int aId = wxID_ANY, const wxString& aTitle = _( %s ), '
                             'const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize, '
                             'long aStyle = 0' % cstr( p.get( 'title', '' ) ) )
                ctorImpl = ( 'wxWidgetArgCompat aParent, int aId, const wxString& aTitle, const wxPoint& aPos, '
                             'const wxSize& aSize, long aStyle' )
        elif self.kind == 'Panel':
            if sub:
                base = sub

                if subHeader:
                    self.include( subHeader )
            elif self.gen.compat:
                base = 'wxPanel'
                self.include( '<wx/kiwx_gui.h>' )
            else:
                base = self.qt( 'QWidget' )

            ctorDecl = ctorImpl = 'QWidget* aParent'
            init = '%s( aParent )' % base

            if self.gen.compat:
                # The parameters of the bases of wxFormBuilder
                ctorDecl = ( 'wxWidgetArgCompat aParent, int aId = wxID_ANY, const wxPoint& aPos = wxDefaultPosition, '
                             'const wxSize& aSize = wxDefaultSize, long aStyle = 0, '
                             'const wxString& aName = wxEmptyString' )
                ctorImpl = ( 'wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize, '
                             'long aStyle, const wxString& aName' )
        else:
            raise FbpError( 'form %s not supported' % self.kind )

        # The contents
        topLayout = None

        for child in self.form.findall( 'object' ):
            if child.get( 'class' ) in SIZERS:
                topLayout = self.sizer( child, 'this', aTop = True )
                self.code.append( '    setLayout( %s );' % topLayout )
            else:
                raise FbpError( '%s in the form' % child.get( 'class' ) )

        if self.gen.compat:
            self.code.append( '    SetId( aId );' )

        self.connect_events( self.form, 'this', 'QWidget' )

        # The size
        w, h = parse_size( p.get( 'size' ) )

        if self.gen.compat and topLayout:
            # As wxFormBuilder: laid out, and fitted to the contents without a size of the form
            # (the code of the derived class measures its windows)
            self.code.append( '    Layout();' )

            if w <= 0 or h <= 0:
                self.code.append( '    if( wxSizer* sizer = GetSizer() )' )
                self.code.append( '        sizer->Fit( this );' )

        if w > 0 and h > 0:
            self.code.append( '    resize( %d, %d );' % ( w, h ) )

        mw, mh = parse_size( p.get( 'minimum_size' ) )

        if mw > 0 or mh > 0:
            self.code.append( '    setMinimumSize( %d, %d );' % ( max( mw, 0 ), max( mh, 0 ) ) )

        # The declaration
        decl = [ '/**',
                 ' * %s: generated from %s.' % ( self.name, os.path.basename( self.gen.fbp ) ),
                 ' */',
                 'class %s : public %s' % ( self.name, base ),
                 '{' ]

        for perm in ( 'private', 'protected', 'public' ):
            members = [ m for m in self.members if m[0] == perm ]

            if not members and perm != 'protected':
                continue

            decl.append( '%s:' % perm )

            for _, t, n in members:
                if t is None:
                    decl.append( '    %s' % n )     # a declaration of the fbp
                else:
                    decl.append( '    %s* %s = nullptr;' % ( t, n ) )

            if perm == 'protected' and self.handlers:
                decl.append( '' )
                decl.append( '    // The event handlers: override them in the derived class' )

                for name in sorted( self.handlers ):
                    for params in self.handlers[name]:
                        if self.gen.compat:
                            # As the handlers of wxFormBuilder: the event is skipped
                            decl.append( '    virtual void %s( %s ) { event.Skip(); }'
                                         % ( name, params ) )
                        elif name in self.stdHandlers:
                            decl.append( '    virtual void %s( %s ) { standardButtonClicked( aSource ); }'
                                         % ( name, params ) )
                        else:
                            decl.append( '    virtual void %s( %s ) {}' % ( name, params ) )

            decl.append( '' )

        decl.append( 'public:' )
        decl.append( '    %s( %s );' % ( self.name, ctorDecl ) )
        decl.append( '    ~%s() override = default;' % self.name )
        decl.append( '};' )

        impl = [ '%s::%s( %s ) :' % ( self.name, self.name, ctorImpl ),
                 '        %s' % init,
                 '{' ]
        impl += self.code
        impl.append( '}' )

        return decl, impl


SIZERS = ( 'wxBoxSizer', 'wxStaticBoxSizer', 'wxFlexGridSizer', 'wxGridSizer', 'wxGridBagSizer',
           'wxStdDialogButtonSizer', 'wxWrapSizer' )


def main():
    args = [ a for a in sys.argv[1:] if a != '--wx-compat' ]
    compat = '--wx-compat' in sys.argv[1:]

    if len( args ) != 2:
        sys.stderr.write( 'usage: fbp2qt.py [--wx-compat] <file.fbp> <output directory>\n' )
        return 2

    sys.argv = [ sys.argv[0] ] + args

    try:
        gen = Generator( sys.argv[1], compat )
        h, cpp = gen.generate()
    except FbpError as e:
        sys.stderr.write( '%s: %s\n' % ( sys.argv[1], e ) )
        return 1

    os.makedirs( sys.argv[2], exist_ok = True )

    for ext, text in ( ( '.h', h ), ( '.cpp', cpp ) ):
        path = os.path.join( sys.argv[2], gen.file + ext )

        # Unchanged files keep their date (no rebuild)
        if os.path.exists( path ) and open( path ).read() == text:
            continue

        with open( path, 'w' ) as f:
            f.write( text )

    return 0


if __name__ == '__main__':
    sys.exit( main() )
