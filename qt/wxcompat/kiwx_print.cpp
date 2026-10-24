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

#include <wx/kiwx_print.h>

#include <algorithm>

#include <QAction>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPrinterInfo>
#include <QToolBar>
#include <QVBoxLayout>

#include <qt_strings.h>
#include <wx/translation.h>


// The settings of a print ---------------------------------------------------------------------

namespace
{
/// The page size of Qt of a paper of wxWidgets (QPageSize::Custom: none).
QPageSize::PageSizeId pageSizeId( wxPaperSize aPaper )
{
    switch( aPaper )
    {
    case wxPAPER_LETTER:    return QPageSize::Letter;
    case wxPAPER_LEGAL:     return QPageSize::Legal;
    case wxPAPER_A0:        return QPageSize::A0;
    case wxPAPER_A1:        return QPageSize::A1;
    case wxPAPER_A2:        return QPageSize::A2;
    case wxPAPER_A3:        return QPageSize::A3;
    case wxPAPER_A4:        return QPageSize::A4;
    case wxPAPER_A5:        return QPageSize::A5;
    case wxPAPER_A6:        return QPageSize::A6;
    case wxPAPER_B4:        return QPageSize::B4;
    case wxPAPER_B5:        return QPageSize::B5;
    case wxPAPER_TABLOID:   return QPageSize::Tabloid;
    case wxPAPER_LEDGER:    return QPageSize::Ledger;
    case wxPAPER_EXECUTIVE: return QPageSize::Executive;
    case wxPAPER_CSHEET:    return QPageSize::ArchC;
    case wxPAPER_DSHEET:    return QPageSize::ArchD;
    case wxPAPER_ESHEET:    return QPageSize::ArchE;
    default:                return QPageSize::Custom;
    }
}


/// The paper of wxWidgets of a page size of Qt (wxPAPER_NONE: a custom size).
wxPaperSize paperId( QPageSize::PageSizeId aPageSize )
{
    for( wxPaperSize paper : { wxPAPER_LETTER, wxPAPER_LEGAL, wxPAPER_A0, wxPAPER_A1, wxPAPER_A2,
                               wxPAPER_A3, wxPAPER_A4, wxPAPER_A5, wxPAPER_A6, wxPAPER_B4,
                               wxPAPER_B5, wxPAPER_TABLOID, wxPAPER_LEDGER, wxPAPER_EXECUTIVE,
                               wxPAPER_CSHEET, wxPAPER_DSHEET, wxPAPER_ESHEET } )
    {
        if( pageSizeId( paper ) == aPageSize )
            return paper;
    }

    return wxPAPER_NONE;
}
} // namespace


void wxPrintData::ApplyCompat( QPrinter& aPrinter ) const
{
    if( !m_printerName.IsEmpty() )
    {
        // A printer that is still there
        if( !QPrinterInfo::printerInfo( ToQString( m_printerName ) ).isNull() )
            aPrinter.setPrinterName( ToQString( m_printerName ) );
    }

    if( !m_filename.IsEmpty() && m_printMode == wxPRINT_MODE_FILE )
        aPrinter.setOutputFileName( ToQString( m_filename ) );

    QPageSize::PageSizeId id = pageSizeId( m_paperId );

    if( id != QPageSize::Custom )
    {
        aPrinter.setPageSize( QPageSize( id ) );
    }
    else if( m_paperSize.x > 0 && m_paperSize.y > 0 )
    {
        aPrinter.setPageSize( QPageSize( QSizeF( m_paperSize.x, m_paperSize.y ),
                                         QPageSize::Millimeter ) );
    }

    aPrinter.setPageOrientation( m_orientation == wxLANDSCAPE ? QPageLayout::Landscape
                                                              : QPageLayout::Portrait );
    aPrinter.setColorMode( m_colour ? QPrinter::Color : QPrinter::GrayScale );
    aPrinter.setCopyCount( std::max( m_copies, 1 ) );
    aPrinter.setCollateCopies( m_collate );

    switch( m_duplex )
    {
    case wxDUPLEX_HORIZONTAL: aPrinter.setDuplex( QPrinter::DuplexShortSide ); break;
    case wxDUPLEX_VERTICAL:   aPrinter.setDuplex( QPrinter::DuplexLongSide );  break;
    default:                  aPrinter.setDuplex( QPrinter::DuplexNone );      break;
    }

    // The pages without the margins of the printer: the printouts place their drawings
    aPrinter.setFullPage( true );
}


void wxPrintData::ReadCompat( const QPrinter& aPrinter )
{
    m_printerName = ToWxString( aPrinter.printerName() );
    m_filename = ToWxString( aPrinter.outputFileName() );
    m_printMode = aPrinter.outputFileName().isEmpty() ? wxPRINT_MODE_PRINTER : wxPRINT_MODE_FILE;

    QPageLayout layout = aPrinter.pageLayout();
    m_orientation = layout.orientation() == QPageLayout::Landscape ? wxLANDSCAPE : wxPORTRAIT;
    m_paperId = paperId( layout.pageSize().id() );

    QSizeF sizeMM = layout.pageSize().size( QPageSize::Millimeter );
    m_paperSize = wxSize( int( sizeMM.width() + 0.5 ), int( sizeMM.height() + 0.5 ) );

    m_colour = aPrinter.colorMode() == QPrinter::Color;
    m_copies = aPrinter.copyCount();
    m_collate = aPrinter.collateCopies();

    switch( aPrinter.duplex() )
    {
    case QPrinter::DuplexShortSide: m_duplex = wxDUPLEX_HORIZONTAL; break;
    case QPrinter::DuplexLongSide:  m_duplex = wxDUPLEX_VERTICAL;   break;
    default:                        m_duplex = wxDUPLEX_SIMPLEX;    break;
    }
}


// The page setup dialog -----------------------------------------------------------------------

wxPageSetupDialog::wxPageSetupDialog( wxWidgetArgCompat aParent, wxPageSetupDialogData* aData ) :
        m_parent( aParent.get() )
{
    if( aData )
        m_data = *aData;
}


int wxPageSetupDialog::ShowModal()
{
    QPrinter printer( QPrinter::HighResolution );
    m_data.GetPrintData().ApplyCompat( printer );

    QPageSetupDialog dialog( &printer, m_parent );

    if( dialog.exec() != QDialog::Accepted )
        return wxID_CANCEL;

    m_data.GetPrintData().ReadCompat( printer );
    return wxID_OK;
}


// The printouts -------------------------------------------------------------------------------

void wxPrintout::GetPPIPrinter( int* aX, int* aY ) const
{
    if( aX )
        *aX = m_ppi;

    if( aY )
        *aY = m_ppi;
}


void wxPrintout::GetPPIScreen( int* aX, int* aY ) const
{
    // The logical pixels of Qt
    if( aX )
        *aX = 96;

    if( aY )
        *aY = 96;
}


void wxPrintout::GetPageSizePixels( int* aWidth, int* aHeight ) const
{
    if( aWidth )
        *aWidth = m_pageSize.x;

    if( aHeight )
        *aHeight = m_pageSize.y;
}


void wxPrintout::GetPageSizeMM( int* aWidth, int* aHeight ) const
{
    if( aWidth )
        *aWidth = m_pageSizeMM.x;

    if( aHeight )
        *aHeight = m_pageSizeMM.y;
}


void wxPrintout::SetupCompat( wxDC* aDC, const wxSize& aPageSizePixels,
                              const wxSize& aPageSizeMM, int aPPI, bool aPreview )
{
    m_dc = aDC;
    m_pageSize = aPageSizePixels;
    m_pageSizeMM = aPageSizeMM;
    m_ppi = aPPI;
    m_preview = aPreview;
}


// The printers --------------------------------------------------------------------------------

wxPrinterError wxPrinter::s_lastError = wxPRINTER_NO_ERROR;


wxPrinter::wxPrinter( wxPrintDialogData* aData )
{
    if( aData )
        m_data = *aData;
}


bool wxPrinter::PrintPagesCompat( QPrinter& aPrinter, wxPrintout* aPrintout, bool aPreview,
                                  int aFromPage, int aToPage )
{
    if( !aPrintout )
        return false;

    aPrintout->OnPreparePrinting();

    int minPage = 1, maxPage = 1, selFrom = 1, selTo = 1;
    aPrintout->GetPageInfo( &minPage, &maxPage, &selFrom, &selTo );

    int fromPage = aFromPage > 0 ? std::max( aFromPage, minPage ) : minPage;
    int toPage = aToPage > 0 ? std::min( aToPage, maxPage ) : maxPage;

    QPainter painter;

    if( !painter.begin( &aPrinter ) )
        return false;

    wxPrinterDC dc( &painter );

    // The pages in the pixels of the printer
    QRect  page = aPrinter.pageLayout().fullRectPixels( aPrinter.resolution() );
    QSizeF pageMM = aPrinter.pageLayout().fullRect( QPageLayout::Millimeter ).size();

    aPrintout->SetupCompat( &dc, wxSize( page.width(), page.height() ),
                            wxSize( int( pageMM.width() + 0.5 ), int( pageMM.height() + 0.5 ) ),
                            aPrinter.resolution(), aPreview );

    aPrintout->OnBeginPrinting();

    bool ok = aPrintout->OnBeginDocument( fromPage, toPage );
    bool first = true;

    for( int pageNum = fromPage; ok && pageNum <= toPage; ++pageNum )
    {
        if( !aPrintout->HasPage( pageNum ) )
            continue;

        if( !first )
            aPrinter.newPage();

        first = false;

        // Each page with the DC as new
        painter.save();
        ok = aPrintout->OnPrintPage( pageNum );
        painter.restore();
    }

    aPrintout->OnEndDocument();
    aPrintout->OnEndPrinting();
    aPrintout->SetupCompat( nullptr, wxSize(), wxSize(), aPrinter.resolution(), aPreview );

    painter.end();
    return ok;
}


bool wxPrinter::Print( wxWidgetArgCompat aParent, wxPrintout* aPrintout, bool aPrompt )
{
    s_lastError = wxPRINTER_NO_ERROR;

    if( !aPrintout )
    {
        s_lastError = wxPRINTER_ERROR;
        return false;
    }

    QPrinter printer( QPrinter::HighResolution );
    m_data.GetPrintData().ApplyCompat( printer );

    int fromPage = 0;
    int toPage = 0;

    if( aPrompt )
    {
        int minPage = 1, maxPage = 1, selFrom = 1, selTo = 1;
        aPrintout->GetPageInfo( &minPage, &maxPage, &selFrom, &selTo );

        printer.setFromTo( 0, 0 );

        QPrintDialog dialog( &printer, aParent.get() );
        dialog.setMinMax( minPage, maxPage );
        dialog.setWindowTitle( ToQString( aPrintout->GetTitle() ) );

        if( dialog.exec() != QDialog::Accepted )
        {
            s_lastError = wxPRINTER_CANCELLED;
            return false;
        }

        m_data.GetPrintData().ReadCompat( printer );

        if( printer.printRange() == QPrinter::PageRange )
        {
            fromPage = printer.fromPage();
            toPage = printer.toPage();
        }
    }

    m_data.SetFromPage( fromPage );
    m_data.SetToPage( toPage );

    if( !PrintPagesCompat( printer, aPrintout, false, fromPage, toPage ) )
    {
        s_lastError = wxPRINTER_ERROR;
        return false;
    }

    return true;
}


// The previews --------------------------------------------------------------------------------

wxPrintPreview::wxPrintPreview( wxPrintout* aPrintout, wxPrintout* aPrintoutForPrinting,
                                wxPrintData* aData ) :
        m_printout( aPrintout ),
        m_printoutForPrinting( aPrintoutForPrinting )
{
    if( aData )
        m_data = wxPrintDialogData( *aData );
}


wxPrintPreview::~wxPrintPreview() = default;


wxPreviewFrame::wxPreviewFrame( wxPrintPreview* aPreview, wxWidgetArgCompat aParent,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize ) :
        wxWindowCompat<QDialog>( aParent.get() ),
        m_preview( aPreview )
{
    setWindowTitle( ToQString( aTitle ) );
    setAttribute( Qt::WA_DeleteOnClose );
    setWindowFlags( windowFlags() | Qt::WindowMinMaxButtonsHint );

    if( aSize.x > 0 && aSize.y > 0 )
        resize( aSize.x, aSize.y );
    else
        resize( 800, 700 );

    if( aPos != wxDefaultPosition )
        move( aPos.x, aPos.y );
}


wxPreviewFrame::~wxPreviewFrame() = default;


void wxPreviewFrame::InitializeWithModality( wxPreviewFrameModalityKind aKind )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );

    // The printer of the preview: the pages and the paper of the print
    m_printer = std::make_unique<QPrinter>( QPrinter::HighResolution );

    if( m_preview )
        m_preview->GetPrintDialogData().GetPrintData().ApplyCompat( *m_printer );

    m_previewWidget = new QPrintPreviewWidget( m_printer.get(), this );

    QToolBar* toolbar = new QToolBar( this );

    QAction* printAction = toolbar->addAction( ToQString( _( "Print..." ) ) );
    QObject::connect( printAction, &QAction::triggered, this, [this]() { print(); } );

    toolbar->addSeparator();

    QAction* zoomIn = toolbar->addAction( ToQString( _( "Zoom In" ) ) );
    QObject::connect( zoomIn, &QAction::triggered, m_previewWidget,
                      [this]() { m_previewWidget->zoomIn(); } );

    QAction* zoomOut = toolbar->addAction( ToQString( _( "Zoom Out" ) ) );
    QObject::connect( zoomOut, &QAction::triggered, m_previewWidget,
                      [this]() { m_previewWidget->zoomOut(); } );

    QAction* fit = toolbar->addAction( ToQString( _( "Fit" ) ) );
    QObject::connect( fit, &QAction::triggered, m_previewWidget,
                      [this]() { m_previewWidget->fitInView(); } );

    toolbar->addSeparator();

    QAction* closeAction = toolbar->addAction( ToQString( _( "Close" ) ) );
    QObject::connect( closeAction, &QAction::triggered, this, [this]() { close(); } );

    layout->addWidget( toolbar );
    layout->addWidget( m_previewWidget, 1 );

    QObject::connect( m_previewWidget, &QPrintPreviewWidget::paintRequested, this,
                      [this]( QPrinter* aPrinter ) { paintPages( aPrinter ); } );

    if( m_preview && m_preview->GetZoom() > 0 )
        m_previewWidget->setZoomFactor( m_preview->GetZoom() / 100.0 );

    switch( aKind )
    {
    case wxPreviewFrame_AppModal:    setWindowModality( Qt::ApplicationModal ); break;
    case wxPreviewFrame_WindowModal: setWindowModality( Qt::WindowModal );      break;
    default:                         setWindowModality( Qt::NonModal );         break;
    }
}


void wxPreviewFrame::paintPages( QPrinter* aPrinter )
{
    if( m_preview && m_preview->GetPrintout() )
        wxPrinter::PrintPagesCompat( *aPrinter, m_preview->GetPrintout(), true );
}


void wxPreviewFrame::print()
{
    if( !m_preview )
        return;

    wxPrintout* printout = m_preview->GetPrintoutForPrinting();

    if( !printout )
        return;

    wxPrinter printer( &m_preview->GetPrintDialogData() );
    printer.Print( this, printout, true );
}
