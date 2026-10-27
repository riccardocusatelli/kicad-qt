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

#pragma once

/**
 * @file kiwx_print.h
 * The printing of wxWidgets on the printers of Qt: the settings of a print (wxPrintData) are the
 * ones of a QPrinter, a printout (wxPrintout) draws its pages with a wxDC on the painter of a
 * QPrinter, wxPrinter prints with the print dialog of Qt and wxPreviewFrame shows the pages with
 * QPrintPreviewWidget.
 */

#include <kiqt.h>

#include <memory>

#include <QDialog>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_gdi.h>
#include <wx/kiwx_window.h>

class QPrinter;
class QPrintPreviewWidget;


#define wxPRINT_QUALITY_HIGH    -1
#define wxPRINT_QUALITY_MEDIUM  -2
#define wxPRINT_QUALITY_LOW     -3
#define wxPRINT_QUALITY_DRAFT   -4

typedef int wxPrintQuality;

enum wxPrintMode
{
    wxPRINT_MODE_NONE = 0,
    wxPRINT_MODE_PREVIEW = 1,
    wxPRINT_MODE_FILE = 2,
    wxPRINT_MODE_PRINTER = 3,
    wxPRINT_MODE_STREAM = 4
};

enum wxPrinterError
{
    wxPRINTER_NO_ERROR = 0,
    wxPRINTER_CANCELLED,
    wxPRINTER_ERROR
};

/// A top level window that ends like a dialog (an extra style of the windows).
#define wxTOPLEVEL_EX_DIALOG 0x00000008

enum wxPreviewFrameModalityKind
{
    wxPreviewFrame_AppModal,
    wxPreviewFrame_WindowModal,
    wxPreviewFrame_NonModal
};


/// The settings of a print: the printer, the paper, the orientation, the copies...
/// The paper sources of the printers (wxPrintBin, in the order of wxWidgets).
enum wxPrintBin
{
    wxPRINTBIN_DEFAULT,
    wxPRINTBIN_ONLYONE,
    wxPRINTBIN_LOWER,
    wxPRINTBIN_MIDDLE,
    wxPRINTBIN_MANUAL,
    wxPRINTBIN_ENVELOPE,
    wxPRINTBIN_ENVMANUAL,
    wxPRINTBIN_AUTO,
    wxPRINTBIN_TRACTOR,
    wxPRINTBIN_SMALLFMT,
    wxPRINTBIN_LARGEFMT,
    wxPRINTBIN_LARGECAPACITY,
    wxPRINTBIN_CASSETTE,
    wxPRINTBIN_FORMSOURCE,
    wxPRINTBIN_USER
};


class KIQT_API wxPrintData
{
public:
    wxPrintData() = default;

    bool IsOk() const { return true; }
    bool Ok() const { return IsOk(); }

    void SetOrientation( wxPrintOrientation aOrientation ) { m_orientation = aOrientation; }
    wxPrintOrientation GetOrientation() const { return m_orientation; }
    bool IsOrientationReversed() const { return false; }

    /// The paper: a standard size, or wxPAPER_NONE and a size in millimeters.
    void SetPaperId( wxPaperSize aPaperId ) { m_paperId = aPaperId; }
    wxPaperSize GetPaperId() const { return m_paperId; }
    void SetPaperSize( const wxSize& aSizeMM ) { m_paperSize = aSizeMM; }
    const wxSize& GetPaperSize() const { return m_paperSize; }

    void SetQuality( wxPrintQuality aQuality ) { m_quality = aQuality; }
    wxPrintQuality GetQuality() const { return m_quality; }
    void SetColour( bool aColour ) { m_colour = aColour; }
    bool GetColour() const { return m_colour; }
    void SetNoCopies( int aCopies ) { m_copies = aCopies; }
    int GetNoCopies() const { return m_copies; }
    void SetCollate( bool aCollate ) { m_collate = aCollate; }
    bool GetCollate() const { return m_collate; }
    void SetDuplex( wxDuplexMode aDuplex ) { m_duplex = aDuplex; }
    wxDuplexMode GetDuplex() const { return m_duplex; }

    /// The paper source (kept: the printers of Qt choose their own source).
    void SetBin( wxPrintBin aBin ) { m_bin = aBin; }
    wxPrintBin GetBin() const { return m_bin; }

    /// The printer (empty: the default printer).
    void SetPrinterName( const wxString& aName ) { m_printerName = aName; }
    const wxString& GetPrinterName() const { return m_printerName; }

    /// The file of a print to a file.
    void SetFilename( const wxString& aFilename ) { m_filename = aFilename; }
    const wxString& GetFilename() const { return m_filename; }
    void SetPrintMode( wxPrintMode aMode ) { m_printMode = aMode; }
    wxPrintMode GetPrintMode() const { return m_printMode; }

    /// Set up a printer with the settings.
    void ApplyCompat( QPrinter& aPrinter ) const;

    /// Read the settings of a printer (after its dialogs).
    void ReadCompat( const QPrinter& aPrinter );

private:
    wxPrintOrientation m_orientation = wxPORTRAIT;
    wxPaperSize        m_paperId = wxPAPER_A4;
    wxSize             m_paperSize = wxSize( 210, 297 );
    wxPrintQuality     m_quality = wxPRINT_QUALITY_HIGH;
    bool               m_colour = true;
    int                m_copies = 1;
    bool               m_collate = false;
    wxDuplexMode       m_duplex = wxDUPLEX_SIMPLEX;
    wxPrintBin         m_bin = wxPRINTBIN_DEFAULT;
    wxString           m_printerName;
    wxString           m_filename;
    wxPrintMode        m_printMode = wxPRINT_MODE_PRINTER;
};


/// The settings of the print dialog: the print settings and the pages to print.
class KIQT_API wxPrintDialogData
{
public:
    wxPrintDialogData() = default;
    explicit wxPrintDialogData( const wxPrintData& aPrintData ) : m_printData( aPrintData ) {}

    wxPrintData& GetPrintData() { return m_printData; }
    const wxPrintData& GetPrintData() const { return m_printData; }
    void SetPrintData( const wxPrintData& aPrintData ) { m_printData = aPrintData; }

    int GetFromPage() const { return m_fromPage; }
    int GetToPage() const { return m_toPage; }
    int GetMinPage() const { return m_minPage; }
    int GetMaxPage() const { return m_maxPage; }
    void SetFromPage( int aPage ) { m_fromPage = aPage; }
    void SetToPage( int aPage ) { m_toPage = aPage; }
    void SetMinPage( int aPage ) { m_minPage = aPage; }
    void SetMaxPage( int aPage ) { m_maxPage = aPage; }
    bool GetAllPages() const { return m_allPages; }
    void SetAllPages( bool aAllPages ) { m_allPages = aAllPages; }
    int GetNoCopies() const { return m_printData.GetNoCopies(); }
    void SetNoCopies( int aCopies ) { m_printData.SetNoCopies( aCopies ); }
    void EnablePageNumbers( bool aEnable ) {}
    void EnableSelection( bool aEnable ) {}
    void EnablePrintToFile( bool aEnable ) {}

private:
    wxPrintData m_printData;
    int         m_fromPage = 0;
    int         m_toPage = 0;
    int         m_minPage = 0;
    int         m_maxPage = 0;
    bool        m_allPages = true;
};


/// The settings of the page setup dialog: the print settings and the paper.
class KIQT_API wxPageSetupDialogData
{
public:
    wxPageSetupDialogData() = default;
    explicit wxPageSetupDialogData( const wxPrintData& aPrintData ) : m_printData( aPrintData ) {}

    wxPrintData& GetPrintData() { return m_printData; }
    const wxPrintData& GetPrintData() const { return m_printData; }
    void SetPrintData( const wxPrintData& aPrintData ) { m_printData = aPrintData; }

    wxPageSetupDialogData& operator=( const wxPrintData& aPrintData )
    {
        m_printData = aPrintData;
        return *this;
    }

    /// A paper of a size in millimeters (not a standard paper).
    void SetPaperSize( const wxSize& aSizeMM )
    {
        m_printData.SetPaperId( wxPAPER_NONE );
        m_printData.SetPaperSize( aSizeMM );
    }

    wxSize GetPaperSize() const { return m_printData.GetPaperSize(); }
    void SetPaperId( wxPaperSize aPaperId ) { m_printData.SetPaperId( aPaperId ); }
    wxPaperSize GetPaperId() const { return m_printData.GetPaperId(); }

    void SetMarginTopLeft( const wxPoint& aMargin ) { m_marginTopLeft = aMargin; }
    void SetMarginBottomRight( const wxPoint& aMargin ) { m_marginBottomRight = aMargin; }
    wxPoint GetMarginTopLeft() const { return m_marginTopLeft; }
    wxPoint GetMarginBottomRight() const { return m_marginBottomRight; }
    void EnableMargins( bool aEnable ) {}
    void EnableOrientation( bool aEnable ) {}
    void EnablePaper( bool aEnable ) {}
    void EnablePrinter( bool aEnable ) {}

private:
    wxPrintData m_printData;
    wxPoint     m_marginTopLeft;
    wxPoint     m_marginBottomRight;
};


/// The page setup dialog of Qt with the settings of wxWidgets.
class KIQT_API wxPageSetupDialog
{
public:
    explicit wxPageSetupDialog( wxWidgetArgCompat aParent, wxPageSetupDialogData* aData = nullptr );

    int ShowModal();

    wxPageSetupDialogData& GetPageSetupDialogData() { return m_data; }
    wxPageSetupDialogData& GetPageSetupData() { return m_data; }

private:
    QWidget*              m_parent;
    wxPageSetupDialogData m_data;
};


/**
 * A document to print: OnPrintPage() draws a page with the DC of GetDC().  The DC is the one of
 * a printer or of a preview, its logical coordinates are the pixels of the page.
 */
class KIQT_API wxPrintout
{
public:
    explicit wxPrintout( const wxString& aTitle = wxS( "Printout" ) ) : m_title( aTitle ) {}
    virtual ~wxPrintout() = default;

    wxPrintout( const wxPrintout& ) = delete;
    wxPrintout& operator=( const wxPrintout& ) = delete;

    virtual bool OnPrintPage( int aPage ) = 0;
    virtual bool HasPage( int aPage ) { return aPage == 1; }

    virtual void GetPageInfo( int* aMinPage, int* aMaxPage, int* aSelPageFrom, int* aSelPageTo )
    {
        *aMinPage = 1;
        *aMaxPage = 32000;
        *aSelPageFrom = 1;
        *aSelPageTo = 1;
    }

    virtual void OnPreparePrinting() {}
    virtual void OnBeginPrinting() {}
    virtual void OnEndPrinting() {}
    virtual bool OnBeginDocument( int aStartPage, int aEndPage ) { return true; }
    virtual void OnEndDocument() {}

    const wxString& GetTitle() const { return m_title; }
    wxDC* GetDC() const { return m_dc; }
    bool IsPreview() const { return m_preview; }

    void GetPPIPrinter( int* aX, int* aY ) const;
    void GetPPIScreen( int* aX, int* aY ) const;
    void GetPageSizePixels( int* aWidth, int* aHeight ) const;
    void GetPageSizeMM( int* aWidth, int* aHeight ) const;

    /// The page and the paper, in logical coordinates (the pixels of the device).
    wxRect GetLogicalPageRect() const { return wxRect( 0, 0, m_pageSize.x, m_pageSize.y ); }
    wxRect GetLogicalPaperRect() const { return GetLogicalPageRect(); }
    wxRect GetPaperRectPixels() const { return GetLogicalPageRect(); }

    /// Print with the DC on a device: called by the printers and the previews.
    void SetupCompat( wxDC* aDC, const wxSize& aPageSizePixels, const wxSize& aPageSizeMM,
                      int aPPI, bool aPreview );

private:
    wxString m_title;
    wxDC*    m_dc = nullptr;
    wxSize   m_pageSize;
    wxSize   m_pageSizeMM;
    int      m_ppi = 72;
    bool     m_preview = false;
};


/// Print the pages of a printout on a printer (wxPrinter).
class KIQT_API wxPrinter
{
public:
    explicit wxPrinter( wxPrintDialogData* aData = nullptr );

    /// Print, after the print dialog if @a aPrompt.
    bool Print( wxWidgetArgCompat aParent, wxPrintout* aPrintout, bool aPrompt = true );

    wxPrintDialogData& GetPrintDialogData() { return m_data; }

    static wxPrinterError GetLastError() { return s_lastError; }

    /**
     * Print the pages of a printout on a printer ready to print (or a preview): the pages from
     * @a aFromPage to @a aToPage (0: the pages of the printout).
     *
     * @return false if the printout stops the print.
     */
    static bool PrintPagesCompat( QPrinter& aPrinter, wxPrintout* aPrintout, bool aPreview,
                                  int aFromPage = 0, int aToPage = 0 );

private:
    wxPrintDialogData     m_data;
    static wxPrinterError s_lastError;
};


/// The two printouts of a print preview: the one shown, and the one printed from the preview.
class KIQT_API wxPrintPreview
{
public:
    wxPrintPreview( wxPrintout* aPrintout, wxPrintout* aPrintoutForPrinting = nullptr,
                    wxPrintData* aData = nullptr );
    ~wxPrintPreview();

    // Owns objects through unique_ptr; MSVC emits the copy of a dllexport class eagerly.
    wxPrintPreview( const wxPrintPreview& ) = delete;
    wxPrintPreview& operator=( const wxPrintPreview& ) = delete;

    bool IsOk() const { return m_printout != nullptr; }

    void SetZoom( int aPercent ) { m_zoom = aPercent; }
    int GetZoom() const { return m_zoom; }

    wxPrintout* GetPrintout() const { return m_printout.get(); }
    wxPrintout* GetPrintoutForPrinting() const { return m_printoutForPrinting.get(); }
    wxPrintDialogData& GetPrintDialogData() { return m_data; }

private:
    std::unique_ptr<wxPrintout> m_printout;
    std::unique_ptr<wxPrintout> m_printoutForPrinting;
    wxPrintDialogData           m_data;
    int                         m_zoom = 100;
};


/**
 * The window of a print preview (wxPreviewFrame): the pages of the preview, a button to print
 * them and the zoom.  The frame takes the preview.
 */
class KIQT_API wxPreviewFrame : public wxWindowCompat<QDialog>
{
public:
    wxPreviewFrame( wxPrintPreview* aPreview, wxWidgetArgCompat aParent, const wxString& aTitle,
                    const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize );
    ~wxPreviewFrame() override;

    void Initialize() { InitializeWithModality( wxPreviewFrame_AppModal ); }
    void InitializeWithModality( wxPreviewFrameModalityKind aKind );

private:
    void paintPages( QPrinter* aPrinter );
    void print();

    std::unique_ptr<wxPrintPreview> m_preview;
    std::unique_ptr<QPrinter>       m_printer;
    QPrintPreviewWidget*            m_previewWidget = nullptr;
};


/// A DC drawing on a printer (wxPrinterDC): the DCs of the printouts.
class KIQT_API wxPrinterDC : public wxDC
{
public:
    using wxDC::wxDC;
};
