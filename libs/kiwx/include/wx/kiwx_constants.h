/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/defs.h (the constants), wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

/*
 * The constants of the wxWidgets definitions header (window styles, identifiers, key codes,
 * dialog flags...): they are part of the API even without user interface.  Included by defs.h.
 */

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

// With C++20 combining values of different enums is deprecated: allow it explicitly
#define wxALLOW_COMBINING_ENUMS_IMPL( en1, en2 )                                                 \
    inline int operator|( en1 v1, en2 v2 ) { return static_cast<int>( v1 ) | static_cast<int>( v2 ); } \
    inline int operator+( en1 v1, en2 v2 ) { return static_cast<int>( v1 ) + static_cast<int>( v2 ); }

#define wxALLOW_COMBINING_ENUMS( en1, en2 )                                                      \
    wxALLOW_COMBINING_ENUMS_IMPL( en1, en2 )                                                     \
    wxALLOW_COMBINING_ENUMS_IMPL( en2, en1 )

enum wxGeometryCentre
{
    wxCENTRE                  = 0x0001,
    wxCENTER                  = wxCENTRE
};

/*  centering into frame rather than screen (obsolete) */
#define wxCENTER_FRAME          0x0000
/*  centre on screen rather than parent */
#define wxCENTRE_ON_SCREEN      0x0002
#define wxCENTER_ON_SCREEN      wxCENTRE_ON_SCREEN

enum wxOrientation
{
    /* don't change the values of these elements, they are used elsewhere */
    wxHORIZONTAL              = 0x0004,
    wxVERTICAL                = 0x0008,

    wxBOTH                    = wxVERTICAL | wxHORIZONTAL,

    /*  a mask to extract orientation from the combination of flags */
    wxORIENTATION_MASK        = wxBOTH
};

enum wxDirection
{
    wxLEFT                    = 0x0010,
    wxRIGHT                   = 0x0020,
    wxUP                      = 0x0040,
    wxDOWN                    = 0x0080,

    wxTOP                     = wxUP,
    wxBOTTOM                  = wxDOWN,

    wxNORTH                   = wxUP,
    wxSOUTH                   = wxDOWN,
    wxWEST                    = wxLEFT,
    wxEAST                    = wxRIGHT,

    wxALL                     = (wxUP | wxDOWN | wxRIGHT | wxLEFT),

    /*  a mask to extract direction from the combination of flags */
    wxDIRECTION_MASK           = wxALL
};

enum wxAlignment
{
    /*
        0 is a valid wxAlignment value (both wxALIGN_LEFT and wxALIGN_TOP
        use it) so define a symbolic name for an invalid alignment value
        which can be assumed to be different from anything else
     */
    wxALIGN_INVALID           = -1,

    wxALIGN_NOT               = 0x0000,
    wxALIGN_CENTER_HORIZONTAL = 0x0100,
    wxALIGN_CENTRE_HORIZONTAL = wxALIGN_CENTER_HORIZONTAL,
    wxALIGN_LEFT              = wxALIGN_NOT,
    wxALIGN_TOP               = wxALIGN_NOT,
    wxALIGN_RIGHT             = 0x0200,
    wxALIGN_BOTTOM            = 0x0400,
    wxALIGN_CENTER_VERTICAL   = 0x0800,
    wxALIGN_CENTRE_VERTICAL   = wxALIGN_CENTER_VERTICAL,

    wxALIGN_CENTER            = (wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL),
    wxALIGN_CENTRE            = wxALIGN_CENTER,

    /*  a mask to extract alignment from the combination of flags */
    wxALIGN_MASK              = 0x0f00
};

/* misc. flags for wxSizer items */
enum wxSizerFlagBits
{
    /*
        wxADJUST_MINSIZE doesn't do anything any more but we still define
        it for compatibility. Notice that it may be also predefined (as 0,
        hopefully) in the user code in order to use it even in
        !WXWIN_COMPATIBILITY_2_8 builds so don't redefine it in such case.
     */
    wxFIXED_MINSIZE                = 0x8000,
    wxRESERVE_SPACE_EVEN_IF_HIDDEN = 0x0002,

    /*  a mask to extract wxSizerFlagBits from combination of flags */
    wxSIZER_FLAG_BITS_MASK         = 0x8002
};

enum wxStretch
{
    wxSTRETCH_NOT             = 0x0000,
    wxSHRINK                  = 0x1000,
    wxGROW                    = 0x2000,
    wxEXPAND                  = wxGROW,
    wxSHAPED                  = 0x4000,
    wxTILE                    = 0xc000, /* wxSHAPED | wxFIXED_MINSIZE */

    /*  a mask to extract stretch from the combination of flags */
    wxSTRETCH_MASK            = 0x7000 /* sans wxTILE */
};

/*  border flags: the values are chosen for backwards compatibility */
enum wxBorder
{
    /*  this is different from wxBORDER_NONE as by default the controls do have */
    /*  border */
    wxBORDER_DEFAULT = 0,

    wxBORDER_NONE   = 0x00200000,
    wxBORDER_STATIC = 0x01000000,
    wxBORDER_SIMPLE = 0x02000000,
    wxBORDER_RAISED = 0x04000000,
    wxBORDER_SUNKEN = 0x08000000,
    wxBORDER_DOUBLE = 0x10000000, /* deprecated */
    wxBORDER_THEME  = wxBORDER_DOUBLE,

    /*  a mask to extract border style from the combination of flags */
    wxBORDER_MASK   = 0x1f200000
};

/* This makes it easier to specify a 'normal' border for a control */
#define wxDEFAULT_CONTROL_BORDER    wxBORDER_SUNKEN

/*
    Elements of these enums can be combined with each other when using
    wxSizer::Add() overload not using wxSizerFlags.
 */
wxALLOW_COMBINING_ENUMS(wxAlignment, wxBorder)
wxALLOW_COMBINING_ENUMS(wxAlignment, wxDirection)
wxALLOW_COMBINING_ENUMS(wxAlignment, wxGeometryCentre)
wxALLOW_COMBINING_ENUMS(wxAlignment, wxSizerFlagBits)
wxALLOW_COMBINING_ENUMS(wxAlignment, wxStretch)
wxALLOW_COMBINING_ENUMS(wxBorder, wxDirection)
wxALLOW_COMBINING_ENUMS(wxBorder, wxGeometryCentre)
wxALLOW_COMBINING_ENUMS(wxBorder, wxSizerFlagBits)
wxALLOW_COMBINING_ENUMS(wxBorder, wxStretch)
wxALLOW_COMBINING_ENUMS(wxDirection, wxGeometryCentre)
wxALLOW_COMBINING_ENUMS(wxDirection, wxStretch)
wxALLOW_COMBINING_ENUMS(wxDirection, wxSizerFlagBits)
wxALLOW_COMBINING_ENUMS(wxGeometryCentre, wxSizerFlagBits)
wxALLOW_COMBINING_ENUMS(wxGeometryCentre, wxStretch)
wxALLOW_COMBINING_ENUMS(wxSizerFlagBits, wxStretch)

/*  ---------------------------------------------------------------------------- */
/*  Window style flags */
/*  ---------------------------------------------------------------------------- */

/*
 * Values are chosen so they can be |'ed in a bit list.
 * Some styles are used across more than one group,
 * so the values mustn't clash with others in the group.
 * Otherwise, numbers can be reused across groups.
 */

/*
    Summary of the bits used by various styles.

    High word, containing styles which can be used with many windows:

    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
      |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
      |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  \_ wxFULL_REPAINT_ON_RESIZE
      |  |  |  |  |  |  |  |  |  |  |  |  |  |  \____ wxPOPUP_WINDOW
      |  |  |  |  |  |  |  |  |  |  |  |  |  \_______ wxWANTS_CHARS
      |  |  |  |  |  |  |  |  |  |  |  |  \__________ wxTAB_TRAVERSAL
      |  |  |  |  |  |  |  |  |  |  |  \_____________ wxTRANSPARENT_WINDOW
      |  |  |  |  |  |  |  |  |  |  \________________ wxBORDER_NONE
      |  |  |  |  |  |  |  |  |  \___________________ wxCLIP_CHILDREN
      |  |  |  |  |  |  |  |  \______________________ wxALWAYS_SHOW_SB
      |  |  |  |  |  |  |  \_________________________ wxBORDER_STATIC
      |  |  |  |  |  |  \____________________________ wxBORDER_SIMPLE
      |  |  |  |  |  \_______________________________ wxBORDER_RAISED
      |  |  |  |  \__________________________________ wxBORDER_SUNKEN
      |  |  |  \_____________________________________ wxBORDER_{DOUBLE,THEME}
      |  |  \________________________________________ wxCAPTION/wxCLIP_SIBLINGS
      |  \___________________________________________ wxHSCROLL
      \______________________________________________ wxVSCROLL


    Low word style bits is class-specific meaning that the same bit can have
    different meanings for different controls (e.g. 0x10 is wxCB_READONLY
    meaning that the control can't be modified for wxComboBox but wxLB_SORT
    meaning that the control should be kept sorted for wxListBox, while
    wxLB_SORT has a different value -- and this is just fine).
 */

/*
 * Window (Frame/dialog/subwindow/panel item) style flags
 */

/* The cast is needed to avoid g++ -Wnarrowing warnings when initializing
 * values of int type with wxVSCROLL on 32 bit platforms, where its value is
 * greater than INT_MAX.
 */
#define wxVSCROLL               ((int)0x80000000)
#define wxHSCROLL               0x40000000
#define wxCAPTION               0x20000000

/*  New styles (border styles are now in their own enum) */
#define wxDOUBLE_BORDER         wxBORDER_DOUBLE
#define wxSUNKEN_BORDER         wxBORDER_SUNKEN
#define wxRAISED_BORDER         wxBORDER_RAISED
#define wxBORDER                wxBORDER_SIMPLE
#define wxSIMPLE_BORDER         wxBORDER_SIMPLE
#define wxSTATIC_BORDER         wxBORDER_STATIC
#define wxNO_BORDER             wxBORDER_NONE

/*  wxALWAYS_SHOW_SB: instead of hiding the scrollbar when it is not needed, */
/*  disable it - but still show (see also wxLB_ALWAYS_SB style) */
/*  */
/*  NB: as this style is only supported by wxUniversal and wxMSW so far */
#define wxALWAYS_SHOW_SB        0x00800000

/*  Clip children when painting, which reduces flicker in e.g. frames and */
/*  splitter windows, but can't be used in a panel where a static box must be */
/*  'transparent' (panel paints the background for it) */
#define wxCLIP_CHILDREN         0x00400000

/*  Note we're reusing the wxCAPTION style because we won't need captions */
/*  for subwindows/controls */
#define wxCLIP_SIBLINGS         0x20000000

#define wxTRANSPARENT_WINDOW    0x00100000

/*  Add this style to a panel to get tab traversal working outside of dialogs */
/*  (on by default for wxPanel, wxDialog, wxScrolledWindow) */
#define wxTAB_TRAVERSAL         0x00080000

/*  Add this style if the control wants to get all keyboard messages (under */
/*  Windows, it won't normally get the dialog navigation key events) */
#define wxWANTS_CHARS           0x00040000

/*  Make window retained (Motif only, see src/generic/scrolwing.cpp)
 *  This is non-zero only under wxMotif, to avoid a clash with wxPOPUP_WINDOW
 *  on other platforms
 */

#define wxRETAINED              0x00000000
#define wxBACKINGSTORE          wxRETAINED

/*  set this flag to create a special popup window: it will be always shown on */
/*  top of other windows, will capture the mouse and will be dismissed when the */
/*  mouse is clicked outside of it or if it loses focus in any other way */
#define wxPOPUP_WINDOW          0x00020000

/*  force a full repaint when the window is resized (instead of repainting just */
/*  the invalidated area) */
#define wxFULL_REPAINT_ON_RESIZE 0x00010000

/*  obsolete: now this is the default behaviour */
/*  */
/*  don't invalidate the whole window (resulting in a PAINT event) when the */
/*  window is resized (currently, makes sense for wxMSW only) */
#define wxNO_FULL_REPAINT_ON_RESIZE 0

/* A mask which can be used to filter (out) all wxWindow-specific styles.
 */
#define wxWINDOW_STYLE_MASK     \
    (wxVSCROLL|wxHSCROLL|wxBORDER_MASK|wxALWAYS_SHOW_SB|wxCLIP_CHILDREN| \
     wxCLIP_SIBLINGS|wxTRANSPARENT_WINDOW|wxTAB_TRAVERSAL|wxWANTS_CHARS| \
     wxRETAINED|wxPOPUP_WINDOW|wxFULL_REPAINT_ON_RESIZE)

/*
 * Extra window style flags (use wxWS_EX prefix to make it clear that they
 * should be passed to wxWindow::SetExtraStyle(), not SetWindowStyle())
 */

/* This flag is obsolete as recursive validation is now the default (and only
 * possible) behaviour. Simply don't use it any more in the new code. */
#define wxWS_EX_VALIDATE_RECURSIVELY    0x00000000 /* used to be 1 */

/*  wxCommandEvents and the objects of the derived classes are forwarded to the */
/*  parent window and so on recursively by default. Using this flag for the */
/*  given window allows to block this propagation at this window, i.e. prevent */
/*  the events from being propagated further upwards. The dialogs have this */
/*  flag on by default. */
#define wxWS_EX_BLOCK_EVENTS            0x00000002

/*  don't use this window as an implicit parent for the other windows: this must */
/*  be used with transient windows as otherwise there is the risk of creating a */
/*  dialog/frame with this window as a parent which would lead to a crash if the */
/*  parent is destroyed before the child */
#define wxWS_EX_TRANSIENT               0x00000004

/*  don't paint the window background, we'll assume it will */
/*  be done by a theming engine. This is not yet used but could */
/*  possibly be made to work in the future, at least on Windows */
#define wxWS_EX_THEMED_BACKGROUND       0x00000008

/*  this window should always process idle events */
#define wxWS_EX_PROCESS_IDLE            0x00000010

/*  this window should always process UI update events */
#define wxWS_EX_PROCESS_UI_UPDATES      0x00000020

/*  Draw the window in a metal theme on Mac */
#define wxFRAME_EX_METAL                0x00000040
#define wxDIALOG_EX_METAL               0x00000040

/*  Use this style to add a context-sensitive help to the window (currently for */
/*  Win32 only and it doesn't work if wxMINIMIZE_BOX or wxMAXIMIZE_BOX are used) */
#define wxWS_EX_CONTEXTHELP             0x00000080

/* synonyms for wxWS_EX_CONTEXTHELP for compatibility */
#define wxFRAME_EX_CONTEXTHELP          wxWS_EX_CONTEXTHELP
#define wxDIALOG_EX_CONTEXTHELP         wxWS_EX_CONTEXTHELP

/*  Create a window which is attachable to another top level window */
#define wxFRAME_DRAWER          0x0020

/*
 * MDI parent frame style flags
 * Can overlap with some of the above.
 */

#define wxFRAME_NO_WINDOW_MENU  0x0100

/*
 * wxMenuBar style flags
 */
/*  use native docking */
#define wxMB_DOCKABLE       0x0001

/*
 * wxMenu style flags
 */
#define wxMENU_TEAROFF      0x0001

/*
 * Apply to all panel items
 */
#define wxCOLOURED          0x0800
#define wxFIXED_LENGTH      0x0400

/*
 * Styles for wxListBox
 */
#define wxLB_SORT           0x0010
#define wxLB_SINGLE         0x0020
#define wxLB_MULTIPLE       0x0040
#define wxLB_EXTENDED       0x0080
/*  wxLB_OWNERDRAW is Windows-only */
#define wxLB_NEEDED_SB      0x0000
#define wxLB_OWNERDRAW      0x0100
#define wxLB_ALWAYS_SB      0x0200
#define wxLB_NO_SB          0x0400
#define wxLB_HSCROLL        wxHSCROLL
/*  always show an entire number of rows */
#define wxLB_INT_HEIGHT     0x0800

/*
 * wxComboBox style flags
 */
#define wxCB_SIMPLE         0x0004
#define wxCB_SORT           0x0008
#define wxCB_READONLY       0x0010
#define wxCB_DROPDOWN       0x0020

/*
 * wxRadioBox style flags
 * These styles are not used in any port.
 */
#define wxRA_LEFTTORIGHT    0x0001
#define wxRA_TOPTOBOTTOM    0x0002

/*  New, more intuitive names to specify majorDim argument */
#define wxRA_SPECIFY_COLS   wxHORIZONTAL
#define wxRA_SPECIFY_ROWS   wxVERTICAL

/*  Old names for compatibility */
#define wxRA_HORIZONTAL     wxHORIZONTAL
#define wxRA_VERTICAL       wxVERTICAL

/*
 * wxRadioButton style flag
 */
#define wxRB_GROUP          0x0004
#define wxRB_SINGLE         0x0008

/*
 * wxScrollBar flags
 */
#define wxSB_HORIZONTAL      wxHORIZONTAL
#define wxSB_VERTICAL        wxVERTICAL

/*
 * wxSpinButton flags.
 * Note that a wxSpinCtrl is sometimes defined as a wxTextCtrl, and so the
 * flags shouldn't overlap with wxTextCtrl flags that can be used for a single
 * line controls (currently we reuse wxTE_CHARWRAP and wxTE_RICH2 neither of
 * which makes sense for them).
 */
#define wxSP_HORIZONTAL       wxHORIZONTAL /*  4 */
#define wxSP_VERTICAL         wxVERTICAL   /*  8 */
#define wxSP_ARROW_KEYS       0x4000
#define wxSP_WRAP             0x8000

/*
 * wxTabCtrl flags
 */
#define wxTC_RIGHTJUSTIFY     0x0010
#define wxTC_FIXEDWIDTH       0x0020
#define wxTC_TOP              0x0000    /*  default */
#define wxTC_LEFT             0x0020
#define wxTC_RIGHT            0x0040
#define wxTC_BOTTOM           0x0080
#define wxTC_MULTILINE        0x0200    /* == wxNB_MULTILINE */
#define wxTC_OWNERDRAW        0x0400

/*
 * wxStaticBitmap flags
 */
#define wxBI_EXPAND           wxEXPAND

/*
 * wxStaticLine flags
 */
#define wxLI_HORIZONTAL         wxHORIZONTAL
#define wxLI_VERTICAL           wxVERTICAL

/*
    wxTextCtrl flags also used by other controls.

    wxComboBox, wxSpinCtrl and maybe others can also use some of these flags,
    so define them in common header. More wxTE_XXX are in wx/textctrl.h.
 */
#define wxTE_READONLY       0x0010
#define wxTE_MULTILINE      0x0020
#define wxTE_PROCESS_TAB    0x0040

#define wxTE_PROCESS_ENTER  0x0400
#define wxTE_PASSWORD       0x0800

#define wxTE_LEFT           0x0000
#define wxTE_CENTER         wxALIGN_CENTER_HORIZONTAL
#define wxTE_CENTRE         wxTE_CENTER
#define wxTE_RIGHT          wxALIGN_RIGHT

/*
    wxButton flags (wx/button.h)
 */
#define wxBU_LEFT           0x0040
#define wxBU_TOP            0x0080
#define wxBU_RIGHT          0x0100
#define wxBU_BOTTOM         0x0200
#define wxBU_ALIGN_MASK     ( wxBU_LEFT | wxBU_TOP | wxBU_RIGHT | wxBU_BOTTOM )
#define wxBU_EXACTFIT       0x0001
#define wxBU_NOTEXT         0x0002
#define wxBU_AUTODRAW       0x0004

/*
    wxHtmlWindow flags (wx/html/htmlwin.h)
 */
#define wxHW_SCROLLBAR_NEVER    0x0002
#define wxHW_SCROLLBAR_AUTO     0x0004
#define wxHW_NO_SELECTION       0x0008
#define wxHW_DEFAULT_STYLE      wxHW_SCROLLBAR_AUTO

/*
    The visibility of the scrollbars of a window (wx/window.h)
 */
enum wxScrollbarVisibility
{
    wxSHOW_SB_NEVER = -1,
    wxSHOW_SB_DEFAULT,
    wxSHOW_SB_ALWAYS
};


/*
 * extended dialog specifiers. these values are stored in a different
 * flag and thus do not overlap with other style flags. note that these
 * values do not correspond to the return values of the dialogs (for
 * those values, look at the wxID_XXX defines).
 */

/*  wxCENTRE already defined as  0x00000001 */
#define wxYES                   0x00000002
#define wxOK                    0x00000004
#define wxNO                    0x00000008
#define wxYES_NO                (wxYES | wxNO)
#define wxCANCEL                0x00000010
#define wxAPPLY                 0x00000020
#define wxCLOSE                 0x00000040

#define wxOK_DEFAULT            0x00000000  /* has no effect (default) */
#define wxYES_DEFAULT           0x00000000  /* has no effect (default) */
#define wxNO_DEFAULT            0x00000080  /* only valid with wxYES_NO */
#define wxCANCEL_DEFAULT        0x80000000  /* only valid with wxCANCEL */

#define wxICON_WARNING          0x00000100
#define wxICON_ERROR            0x00000200
#define wxICON_QUESTION         0x00000400
#define wxICON_INFORMATION      0x00000800
#define wxICON_EXCLAMATION      wxICON_WARNING
#define wxICON_HAND             wxICON_ERROR
#define wxICON_STOP             wxICON_ERROR
#define wxICON_ASTERISK         wxICON_INFORMATION

#define wxHELP                  0x00001000
#define wxFORWARD               0x00002000
#define wxBACKWARD              0x00004000
#define wxRESET                 0x00008000
#define wxMORE                  0x00010000
#define wxSETUP                 0x00020000
#define wxICON_NONE             0x00040000
#define wxICON_AUTH_NEEDED      0x00080000

#define wxICON_MASK \
    (wxICON_EXCLAMATION|wxICON_HAND|wxICON_QUESTION|wxICON_INFORMATION|wxICON_NONE|wxICON_AUTH_NEEDED)

/*
 * Background styles. See wxWindow::SetBackgroundStyle
 */
enum wxBackgroundStyle
{
    /*
        background is erased in the EVT_ERASE_BACKGROUND handler or using
        the system default background if no such handler is defined (this
        is the default style)
     */
    wxBG_STYLE_ERASE,

    /*
        background is erased by the system, no EVT_ERASE_BACKGROUND event
        is generated at all
     */
    wxBG_STYLE_SYSTEM,

    /*
        background is erased in EVT_PAINT handler and not erased at all
        before it, this should be used if the paint handler paints over
        the entire window to avoid flicker
     */
    wxBG_STYLE_PAINT,

    /*
        Indicates that the window background is not erased, letting the parent
        window show through.
     */
    wxBG_STYLE_TRANSPARENT,

    /* this style is deprecated and doesn't do anything, don't use */
    wxBG_STYLE_COLOUR,

    /*
        this style is deprecated and is synonymous with
        wxBG_STYLE_PAINT, use the new name
     */
    wxBG_STYLE_CUSTOM = wxBG_STYLE_PAINT
};

/*
 * Key types used by (old style) lists and hashes.
 */
enum wxKeyType
{
    wxKEY_NONE,
    wxKEY_INTEGER,
    wxKEY_STRING
};

/*  ---------------------------------------------------------------------------- */
/*  standard IDs */
/*  ---------------------------------------------------------------------------- */

/*  Standard menu IDs */
enum wxStandardID
{
    /*
       These ids delimit the range used by automatically-generated ids
       (i.e. those used when wxID_ANY is specified during construction).
     */
    wxID_AUTO_LOWEST = -1000000,
    wxID_AUTO_HIGHEST = -2000,

    /* no id matches this one when compared to it */
    wxID_NONE = -3,

    /*  id for a separator line in the menu (invalid for normal item) */
    wxID_SEPARATOR = -2,

    /* any id: means that we don't care about the id, whether when installing
     * an event handler or when creating a new window */
    wxID_ANY = -1,


    /* all predefined ids are between wxID_LOWEST and wxID_HIGHEST */
    wxID_LOWEST = 4999,

    wxID_OPEN,
    wxID_CLOSE,
    wxID_NEW,
    wxID_SAVE,
    wxID_SAVEAS,
    wxID_REVERT,
    wxID_EXIT,
    wxID_UNDO,
    wxID_REDO,
    wxID_HELP,
    wxID_PRINT,
    wxID_PRINT_SETUP,
    wxID_PAGE_SETUP,
    wxID_PREVIEW,
    wxID_ABOUT,
    wxID_HELP_CONTENTS,
    wxID_HELP_INDEX,
    wxID_HELP_SEARCH,
    wxID_HELP_COMMANDS,
    wxID_HELP_PROCEDURES,
    wxID_HELP_CONTEXT,
    wxID_CLOSE_ALL,
    wxID_PREFERENCES,

    wxID_EDIT = 5030,
    wxID_CUT,
    wxID_COPY,
    wxID_PASTE,
    wxID_CLEAR,
    wxID_FIND,
    wxID_DUPLICATE,
    wxID_SELECTALL,
    wxID_DELETE,
    wxID_REPLACE,
    wxID_REPLACE_ALL,
    wxID_PROPERTIES,

    wxID_VIEW_DETAILS,
    wxID_VIEW_LARGEICONS,
    wxID_VIEW_SMALLICONS,
    wxID_VIEW_LIST,
    wxID_VIEW_SORTDATE,
    wxID_VIEW_SORTNAME,
    wxID_VIEW_SORTSIZE,
    wxID_VIEW_SORTTYPE,

    wxID_FILE = 5050,
    wxID_FILE1,
    wxID_FILE2,
    wxID_FILE3,
    wxID_FILE4,
    wxID_FILE5,
    wxID_FILE6,
    wxID_FILE7,
    wxID_FILE8,
    wxID_FILE9,

    /*  Standard button and menu IDs */
    wxID_OK = 5100,
    wxID_CANCEL,
    wxID_APPLY,
    wxID_YES,
    wxID_NO,
    wxID_STATIC,
    wxID_FORWARD,
    wxID_BACKWARD,
    wxID_DEFAULT,
    wxID_MORE,
    wxID_SETUP,
    wxID_RESET,
    wxID_CONTEXT_HELP,
    wxID_YESTOALL,
    wxID_NOTOALL,
    wxID_ABORT,
    wxID_RETRY,
    wxID_IGNORE,
    wxID_ADD,
    wxID_REMOVE,

    wxID_UP,
    wxID_DOWN,
    wxID_HOME,
    wxID_REFRESH,
    wxID_STOP,
    wxID_INDEX,

    wxID_BOLD,
    wxID_ITALIC,
    wxID_JUSTIFY_CENTER,
    wxID_JUSTIFY_FILL,
    wxID_JUSTIFY_RIGHT,
    wxID_JUSTIFY_LEFT,
    wxID_UNDERLINE,
    wxID_INDENT,
    wxID_UNINDENT,
    wxID_ZOOM_100,
    wxID_ZOOM_FIT,
    wxID_ZOOM_IN,
    wxID_ZOOM_OUT,
    wxID_UNDELETE,
    wxID_REVERT_TO_SAVED,
    wxID_CDROM,
    wxID_CONVERT,
    wxID_EXECUTE,
    wxID_FLOPPY,
    wxID_HARDDISK,
    wxID_BOTTOM,
    wxID_FIRST,
    wxID_LAST,
    wxID_TOP,
    wxID_INFO,
    wxID_JUMP_TO,
    wxID_NETWORK,
    wxID_SELECT_COLOR,
    wxID_SELECT_FONT,
    wxID_SORT_ASCENDING,
    wxID_SORT_DESCENDING,
    wxID_SPELL_CHECK,
    wxID_STRIKETHROUGH,

    /*  System menu IDs (used by wxUniv): */
    wxID_SYSTEM_MENU = 5200,
    wxID_CLOSE_FRAME,
    wxID_MOVE_FRAME,
    wxID_RESIZE_FRAME,
    wxID_MAXIMIZE_FRAME,
    wxID_ICONIZE_FRAME,
    wxID_RESTORE_FRAME,

    /* MDI window menu ids */
    wxID_MDI_WINDOW_FIRST = 5230,
    wxID_MDI_WINDOW_CASCADE = wxID_MDI_WINDOW_FIRST,
    wxID_MDI_WINDOW_TILE_HORZ,
    wxID_MDI_WINDOW_TILE_VERT,
    wxID_MDI_WINDOW_ARRANGE_ICONS,
    wxID_MDI_WINDOW_PREV,
    wxID_MDI_WINDOW_NEXT,
    wxID_MDI_WINDOW_LAST = wxID_MDI_WINDOW_NEXT,

    /* OS X system menu ids */
    wxID_OSX_MENU_FIRST = 5250,
    wxID_OSX_HIDE = wxID_OSX_MENU_FIRST,
    wxID_OSX_HIDEOTHERS,
    wxID_OSX_SHOWALL,
    wxID_OSX_SERVICES,
    wxID_OSX_MENU_LAST = wxID_OSX_SERVICES,

    /*  IDs used by generic file dialog (13 consecutive starting from this value) */
    wxID_FILEDLGG = 5900,

    /*  IDs used by generic file ctrl (4 consecutive starting from this value) */
    wxID_FILECTRL = 5950,

    wxID_HIGHEST = 5999
};

/*  ---------------------------------------------------------------------------- */
/*  wxWindowID type                                                              */
/*  ---------------------------------------------------------------------------- */

/* Note that this is defined even in non-GUI code as the same type is also used
   for e.g. timer IDs. */
typedef int wxWindowID;

/*  ---------------------------------------------------------------------------- */
/*  other constants */
/*  ---------------------------------------------------------------------------- */

/*  menu and toolbar item kinds */
enum wxItemKind
{
    wxITEM_SEPARATOR = -1,
    wxITEM_NORMAL,
    wxITEM_CHECK,
    wxITEM_RADIO,
    wxITEM_DROPDOWN,
    wxITEM_MAX
};

/*
 * The possible states of a 3-state checkbox (Compatible
 * with the 2-state checkbox).
 */
enum wxCheckBoxState
{
    wxCHK_UNCHECKED,
    wxCHK_CHECKED,
    wxCHK_UNDETERMINED /* 3-state checkbox only */
};


/*  hit test results */
enum wxHitTest
{
    wxHT_NOWHERE,

    /*  scrollbar */
    wxHT_SCROLLBAR_FIRST = wxHT_NOWHERE,
    wxHT_SCROLLBAR_ARROW_LINE_1,    /*  left or upper arrow to scroll by line */
    wxHT_SCROLLBAR_ARROW_LINE_2,    /*  right or down */
    wxHT_SCROLLBAR_ARROW_PAGE_1,    /*  left or upper arrow to scroll by page */
    wxHT_SCROLLBAR_ARROW_PAGE_2,    /*  right or down */
    wxHT_SCROLLBAR_THUMB,           /*  on the thumb */
    wxHT_SCROLLBAR_BAR_1,           /*  bar to the left/above the thumb */
    wxHT_SCROLLBAR_BAR_2,           /*  bar to the right/below the thumb */
    wxHT_SCROLLBAR_LAST,

    /*  window */
    wxHT_WINDOW_OUTSIDE,            /*  not in this window at all */
    wxHT_WINDOW_INSIDE,             /*  in the client area */
    wxHT_WINDOW_VERT_SCROLLBAR,     /*  on the vertical scrollbar */
    wxHT_WINDOW_HORZ_SCROLLBAR,     /*  on the horizontal scrollbar */
    wxHT_WINDOW_CORNER,             /*  on the corner between 2 scrollbars */

    wxHT_MAX
};

/*  ---------------------------------------------------------------------------- */
/*  Possible SetSize flags */
/*  ---------------------------------------------------------------------------- */

/*  Use internally-calculated width if -1 */
#define wxSIZE_AUTO_WIDTH       0x0001
/*  Use internally-calculated height if -1 */
#define wxSIZE_AUTO_HEIGHT      0x0002
/*  Use internally-calculated width and height if each is -1 */
#define wxSIZE_AUTO             (wxSIZE_AUTO_WIDTH|wxSIZE_AUTO_HEIGHT)
/*  Ignore missing (-1) dimensions (use existing). */
/*  For readability only: test for wxSIZE_AUTO_WIDTH/HEIGHT in code. */
#define wxSIZE_USE_EXISTING     0x0000
/*  Allow -1 as a valid position */
#define wxSIZE_ALLOW_MINUS_ONE  0x0004
/*  Don't do parent client adjustments (for implementation only) */
#define wxSIZE_NO_ADJUSTMENTS   0x0008
/*  Change the window position even if it seems to be already correct */
#define wxSIZE_FORCE            0x0010
/*  Emit size event even if size didn't change */
#define wxSIZE_FORCE_EVENT      0x0020

/*  ---------------------------------------------------------------------------- */
/*  GDI descriptions */
/*  ---------------------------------------------------------------------------- */

// Hatch styles used by both pen and brush styles.
//
// NB: Do not use these constants directly, they're for internal use only, use
//     wxBRUSHSTYLE_XXX_HATCH and wxPENSTYLE_XXX_HATCH instead.
enum wxHatchStyle
{
    wxHATCHSTYLE_INVALID = -1,

    /*
        The value of the first style is chosen to fit with
        wxDeprecatedGUIConstants values below, don't change it.
     */
    wxHATCHSTYLE_FIRST = 111,
    wxHATCHSTYLE_BDIAGONAL = wxHATCHSTYLE_FIRST,
    wxHATCHSTYLE_CROSSDIAG,
    wxHATCHSTYLE_FDIAGONAL,
    wxHATCHSTYLE_CROSS,
    wxHATCHSTYLE_HORIZONTAL,
    wxHATCHSTYLE_VERTICAL,
    wxHATCHSTYLE_LAST = wxHATCHSTYLE_VERTICAL
};

/*
    WARNING: the following styles are deprecated; use the
             wxFontFamily, wxFontStyle, wxFontWeight, wxBrushStyle,
             wxPenStyle, wxPenCap, wxPenJoin enum values instead!
*/

/* don't use any elements of this enum in the new code */
enum wxDeprecatedGUIConstants
{
    /*  Text font families */
    wxDEFAULT    = 70,
    wxDECORATIVE,
    wxROMAN,
    wxSCRIPT,
    wxSWISS,
    wxMODERN,
    wxTELETYPE,  /* @@@@ */

    /*  Proportional or Fixed width fonts (not yet used) */
    wxVARIABLE   = 80,
    wxFIXED,

    wxNORMAL     = 90,
    wxLIGHT,
    wxBOLD,
    /*  Also wxNORMAL for normal (non-italic text) */
    wxITALIC,
    wxSLANT,

    /*  Pen styles */
    wxSOLID      =   100,
    wxDOT,
    wxLONG_DASH,
    wxSHORT_DASH,
    wxDOT_DASH,
    wxUSER_DASH,

    wxTRANSPARENT,

    /*  Brush & Pen Stippling. Note that a stippled pen cannot be dashed!! */
    /*  Note also that stippling a Pen IS meaningful, because a Line is */
    wxSTIPPLE_MASK_OPAQUE, /* mask is used for blitting monochrome using text fore and back ground colors */
    wxSTIPPLE_MASK,        /* mask is used for masking areas in the stipple bitmap (TO DO) */
    /*  drawn with a Pen, and without any Brush -- and it can be stippled. */
    wxSTIPPLE =          110,

    wxBDIAGONAL_HATCH = wxHATCHSTYLE_BDIAGONAL,
    wxCROSSDIAG_HATCH = wxHATCHSTYLE_CROSSDIAG,
    wxFDIAGONAL_HATCH = wxHATCHSTYLE_FDIAGONAL,
    wxCROSS_HATCH = wxHATCHSTYLE_CROSS,
    wxHORIZONTAL_HATCH = wxHATCHSTYLE_HORIZONTAL,
    wxVERTICAL_HATCH = wxHATCHSTYLE_VERTICAL,
    wxFIRST_HATCH = wxHATCHSTYLE_FIRST,
    wxLAST_HATCH = wxHATCHSTYLE_LAST
};

/*  ToolPanel in wxFrame (VZ: unused?) */
enum
{
    wxTOOL_TOP = 1,
    wxTOOL_BOTTOM,
    wxTOOL_LEFT,
    wxTOOL_RIGHT
};

/*  the values of the format constants should be the same as corresponding */
/*  CF_XXX constants in Windows API */
enum wxDataFormatId
{
    wxDF_INVALID =          0,
    wxDF_TEXT =             1,  /* CF_TEXT */
    wxDF_BITMAP =           2,  /* CF_BITMAP */
    wxDF_METAFILE =         3,  /* CF_METAFILEPICT */
    wxDF_SYLK =             4,
    wxDF_DIF =              5,
    wxDF_TIFF =             6,
    wxDF_OEMTEXT =          7,  /* CF_OEMTEXT */
    wxDF_DIB =              8,  /* CF_DIB */
    wxDF_PALETTE =          9,
    wxDF_PENDATA =          10,
    wxDF_RIFF =             11,
    wxDF_WAVE =             12,
    wxDF_UNICODETEXT =      13,
    wxDF_ENHMETAFILE =      14,
    wxDF_FILENAME =         15, /* CF_HDROP */
    wxDF_LOCALE =           16,
    wxDF_PRIVATE =          20,
    wxDF_HTML =             30, /* Note: does not correspond to CF_ constant */
    wxDF_PNG =              31, /* Note: does not correspond to CF_ constant */
    wxDF_MAX
};

/* Key codes */
enum wxKeyCode
{
    WXK_NONE    =    0,

    WXK_CONTROL_A = 1,
    WXK_CONTROL_B,
    WXK_CONTROL_C,
    WXK_CONTROL_D,
    WXK_CONTROL_E,
    WXK_CONTROL_F,
    WXK_CONTROL_G,
    WXK_CONTROL_H,
    WXK_CONTROL_I,
    WXK_CONTROL_J,
    WXK_CONTROL_K,
    WXK_CONTROL_L,
    WXK_CONTROL_M,
    WXK_CONTROL_N,
    WXK_CONTROL_O,
    WXK_CONTROL_P,
    WXK_CONTROL_Q,
    WXK_CONTROL_R,
    WXK_CONTROL_S,
    WXK_CONTROL_T,
    WXK_CONTROL_U,
    WXK_CONTROL_V,
    WXK_CONTROL_W,
    WXK_CONTROL_X,
    WXK_CONTROL_Y,
    WXK_CONTROL_Z,

    WXK_BACK    =    8, /* backspace */
    WXK_TAB     =    9,
    WXK_RETURN  =    13,
    WXK_ESCAPE  =    27,

    /* values from 33 to 126 are reserved for the standard ASCII characters */

    WXK_SPACE   =    32,
    WXK_DELETE  =    127,

    /* values from 128 to 255 are reserved for ASCII extended characters
       (note that there isn't a single fixed standard for the meaning
       of these values; avoid them in portable apps!) */

    /* These are not compatible with unicode characters.
       If you want to get a unicode character from a key event, use
       wxKeyEvent::GetUnicodeKey                                    */
    WXK_START   = 300,
    WXK_LBUTTON,
    WXK_RBUTTON,
    WXK_CANCEL,
    WXK_MBUTTON,
    WXK_CLEAR,
    WXK_SHIFT,
    WXK_ALT,
    WXK_CONTROL,
    WXK_MENU,
    WXK_PAUSE,
    WXK_CAPITAL,
    WXK_END,
    WXK_HOME,
    WXK_LEFT,
    WXK_UP,
    WXK_RIGHT,
    WXK_DOWN,
    WXK_SELECT,
    WXK_PRINT,
    WXK_EXECUTE,
    WXK_SNAPSHOT,
    WXK_INSERT,
    WXK_HELP,
    WXK_NUMPAD0,
    WXK_NUMPAD1,
    WXK_NUMPAD2,
    WXK_NUMPAD3,
    WXK_NUMPAD4,
    WXK_NUMPAD5,
    WXK_NUMPAD6,
    WXK_NUMPAD7,
    WXK_NUMPAD8,
    WXK_NUMPAD9,
    WXK_MULTIPLY,
    WXK_ADD,
    WXK_SEPARATOR,
    WXK_SUBTRACT,
    WXK_DECIMAL,
    WXK_DIVIDE,
    WXK_F1,
    WXK_F2,
    WXK_F3,
    WXK_F4,
    WXK_F5,
    WXK_F6,
    WXK_F7,
    WXK_F8,
    WXK_F9,
    WXK_F10,
    WXK_F11,
    WXK_F12,
    WXK_F13,
    WXK_F14,
    WXK_F15,
    WXK_F16,
    WXK_F17,
    WXK_F18,
    WXK_F19,
    WXK_F20,
    WXK_F21,
    WXK_F22,
    WXK_F23,
    WXK_F24,
    WXK_NUMLOCK,
    WXK_SCROLL,
    WXK_PAGEUP,
    WXK_PAGEDOWN,
    WXK_NUMPAD_SPACE,
    WXK_NUMPAD_TAB,
    WXK_NUMPAD_ENTER,
    WXK_NUMPAD_F1,
    WXK_NUMPAD_F2,
    WXK_NUMPAD_F3,
    WXK_NUMPAD_F4,
    WXK_NUMPAD_HOME,
    WXK_NUMPAD_LEFT,
    WXK_NUMPAD_UP,
    WXK_NUMPAD_RIGHT,
    WXK_NUMPAD_DOWN,
    WXK_NUMPAD_PAGEUP,
    WXK_NUMPAD_PAGEDOWN,
    WXK_NUMPAD_END,
    WXK_NUMPAD_BEGIN,
    WXK_NUMPAD_INSERT,
    WXK_NUMPAD_DELETE,
    WXK_NUMPAD_EQUAL,
    WXK_NUMPAD_MULTIPLY,
    WXK_NUMPAD_ADD,
    WXK_NUMPAD_SEPARATOR,
    WXK_NUMPAD_SUBTRACT,
    WXK_NUMPAD_DECIMAL,
    WXK_NUMPAD_DIVIDE,

    WXK_WINDOWS_LEFT,
    WXK_WINDOWS_RIGHT,
    WXK_WINDOWS_MENU ,
    WXK_RAW_CONTROL = WXK_CONTROL,
    WXK_COMMAND = WXK_CONTROL,

    /* Hardware-specific buttons */
    WXK_SPECIAL1 = WXK_WINDOWS_MENU + 2, /* Skip WXK_RAW_CONTROL if necessary */
    WXK_SPECIAL2,
    WXK_SPECIAL3,
    WXK_SPECIAL4,
    WXK_SPECIAL5,
    WXK_SPECIAL6,
    WXK_SPECIAL7,
    WXK_SPECIAL8,
    WXK_SPECIAL9,
    WXK_SPECIAL10,
    WXK_SPECIAL11,
    WXK_SPECIAL12,
    WXK_SPECIAL13,
    WXK_SPECIAL14,
    WXK_SPECIAL15,
    WXK_SPECIAL16,
    WXK_SPECIAL17,
    WXK_SPECIAL18,
    WXK_SPECIAL19,
    WXK_SPECIAL20,

    WXK_BROWSER_BACK,
    WXK_BROWSER_FORWARD,
    WXK_BROWSER_REFRESH,
    WXK_BROWSER_STOP,
    WXK_BROWSER_SEARCH,
    WXK_BROWSER_FAVORITES,
    WXK_BROWSER_HOME,
    WXK_VOLUME_MUTE,
    WXK_VOLUME_DOWN,
    WXK_VOLUME_UP,
    WXK_MEDIA_NEXT_TRACK,
    WXK_MEDIA_PREV_TRACK,
    WXK_MEDIA_STOP,
    WXK_MEDIA_PLAY_PAUSE,
    WXK_LAUNCH_MAIL,

    // Events for these keys are currently only generated by wxGTK, with the
    // exception of WXK_LAUNCH_{A,B}, see WXK_LAUNCH_APP{1,2} below.
    WXK_LAUNCH_0,
    WXK_LAUNCH_1,
    WXK_LAUNCH_2,
    WXK_LAUNCH_3,
    WXK_LAUNCH_4,
    WXK_LAUNCH_5,
    WXK_LAUNCH_6,
    WXK_LAUNCH_7,
    WXK_LAUNCH_8,
    WXK_LAUNCH_9,
    WXK_LAUNCH_A,
    WXK_LAUNCH_B,
    WXK_LAUNCH_C,
    WXK_LAUNCH_D,
    WXK_LAUNCH_E,
    WXK_LAUNCH_F,

    // These constants are the same as the corresponding GTK keys, so give them
    // the same value, but they are also generated by wxMSW.
    WXK_LAUNCH_APP1 = WXK_LAUNCH_A,
    WXK_LAUNCH_APP2 = WXK_LAUNCH_B
};

/* This enum contains bit mask constants used in wxKeyEvent */
enum wxKeyModifier
{
    wxMOD_NONE      = 0x0000,
    wxMOD_ALT       = 0x0001,
    wxMOD_CONTROL   = 0x0002,
    wxMOD_ALTGR     = wxMOD_ALT | wxMOD_CONTROL,
    wxMOD_SHIFT     = 0x0004,
    wxMOD_META      = 0x0008,
    wxMOD_WIN       = wxMOD_META,
    wxMOD_RAW_CONTROL = wxMOD_CONTROL,
    wxMOD_CMD       = wxMOD_CONTROL,
    wxMOD_ALL       = 0xffff
};

/* Shortcut for easier dialog-unit-to-pixel conversion */
#define wxDLG_UNIT(parent, pt) parent->ConvertDialogToPixels(pt)

/* Paper types */

#ifndef _WIN32
#pragma GCC visibility pop
#endif
