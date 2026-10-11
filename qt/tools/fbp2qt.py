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
    def __init__( self, aFbp ):
        self.fbp = aFbp
        root = ET.parse( aFbp ).getroot()
        project = root.find( 'object' )

        if project is None or project.get( 'class' ) != 'Project':
            raise FbpError( 'no project' )

        self.project = props( project )
        self.file = self.project.get( 'file' ) or os.path.splitext( os.path.basename( aFbp ) )[0]
        self.forms = [ o for o in project.findall( 'object' ) ]

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
        if aName in self.handlers and self.handlers[aName] != aParams:
            raise FbpError( 'handler %s with parameters (%s) and (%s)'
                            % ( aName, self.handlers[aName], aParams ) )

        self.handlers[aName] = aParams

    # The events of a widget ---------------------------------------------------------------

    def connect_events( self, aObj, aVar, aQtClass ):
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
            else:
                self.qt( aQtClass )

            return self.var( aObj, qtType, 'new %s( %s )' % ( qtType, args ) ), qtType

        if cls == 'wxStaticText':
            var, t = make( 'QLabel' )
            self.code.append( '    %s->setText( %s );' % ( var, tr( p.get( 'label', '' ) ) ) )

            if parse_int( p.get( 'wrap' ), -1 ) > 0:
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

            if editable:
                self.code.append( '    %s->setEditable( true );' % var )

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

        elif cls == 'wxSpinButton':
            # The arrows of a spin control, without text
            qt = 'SPIN_BUTTON'
            self.include( '<widgets/spin_button.h>' )
            var = self.var( aObj, qt, 'new %s( %s )' % ( qt, aParent ) )

        elif cls == 'wxSlider':
            qt = 'QSlider'
            orient = 'Qt::Vertical' if 'wxSL_VERTICAL' in style else 'Qt::Horizontal'
            var, t = make( qt, '%s, %s' % ( orient, aParent ) )
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

        elif cls == 'wxGrid':
            # A table (WX_GRID of the Qt build), its rows and columns set by the code
            var, t = make( 'QTableWidget' )
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

                var = self.var( aObj, klass, 'new %s( %s )' % ( klass, aParent ) )
                qt = klass

        elif cls in ( 'wxPanel', 'wxScrolledWindow', 'wxNotebook', 'wxAuiNotebook', 'wxSimplebook',
                      'wxSplitterWindow' ):
            return self.container( aObj, aParent )

        else:
            raise FbpError( 'class %s (%s) not supported' % ( cls, p.get( 'name' ) ) )

        self.common( aObj, var )
        self.connect_events( aObj, var, qt if not sub else self.base_qt( cls, style ) )
        return var

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

        if cls == 'wxPanel':
            t = klass( 'QWidget' )
            var = self.var( aObj, t, 'new %s( %s )' % ( t, aParent ) )
            self.common( aObj, var )
            self.children_layout( aObj, var )
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
            t = klass( 'QTabWidget' )
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
            t = klass( 'QStackedWidget' )
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

    def children_layout( self, aObj, aWidget ):
        """The layout of a window: its sizer."""
        for child in aObj.findall( 'object' ):
            if child.get( 'class' ) in SIZERS:
                layout = self.sizer( child, aWidget, aTop = True )
                self.code.append( '    %s->setLayout( %s );' % ( aWidget, layout ) )
            else:
                raise FbpError( '%s in a window' % child.get( 'class' ) )

    # Sizers ---------------------------------------------------------------------------

    def sizer( self, aObj, aParent, aTop = False ):
        """Create the layout of a sizer, with the widgets of its items (children of aParent)."""
        cls = aObj.get( 'class' )
        p = props( aObj )
        saveGroup = self.radioGroup

        if cls == 'wxBoxSizer':
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
                self.handler( handler, SOURCE )
                self.code.append( '    QObject::connect( %s, &QPushButton::clicked, this, [this, src = static_cast<QWidget*>( %s )]() { %s( src ); } );'
                                  % ( button, button, handler ) )
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
        elif self.kind == 'Panel':
            if sub:
                base = sub

                if subHeader:
                    self.include( subHeader )
            else:
                base = self.qt( 'QWidget' )

            ctorDecl = ctorImpl = 'QWidget* aParent'
            init = '%s( aParent )' % base
        else:
            raise FbpError( 'form %s not supported' % self.kind )

        # The contents
        for child in self.form.findall( 'object' ):
            if child.get( 'class' ) in SIZERS:
                layout = self.sizer( child, 'this', aTop = True )
                self.code.append( '    setLayout( %s );' % layout )
            else:
                raise FbpError( '%s in the form' % child.get( 'class' ) )

        self.connect_events( self.form, 'this', 'QWidget' )

        # The size
        w, h = parse_size( p.get( 'size' ) )

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
                decl.append( '    %s* %s = nullptr;' % ( t, n ) )

            if perm == 'protected' and self.handlers:
                decl.append( '' )
                decl.append( '    // The event handlers: override them in the derived class' )

                for name in sorted( self.handlers ):
                    decl.append( '    virtual void %s( %s ) {}' % ( name, self.handlers[name] ) )

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
    if len( sys.argv ) != 3:
        sys.stderr.write( 'usage: fbp2qt.py <file.fbp> <output directory>\n' )
        return 2

    try:
        gen = Generator( sys.argv[1] )
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
