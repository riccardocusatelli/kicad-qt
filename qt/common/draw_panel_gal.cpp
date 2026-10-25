/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file draw_panel_gal.cpp
 * EDA_DRAW_PANEL_GAL of the Qt build: the port of common/draw_panel_gal.cpp to a QWidget,
 * drawn by the Cairo GAL of the Qt shell (QT_CAIRO_GAL).
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)

#include <class_draw_panel_gal.h>
#include <ui_hooks.h>

#include <QApplication>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QWheelEvent>

#include <eda_draw_frame.h>
#include <macros.h>
#include <scoped_set_reset.h>
#include <trace_helpers.h>

#include <view/view.h>
#include <view/view_overlay.h>
#include <view/qt_view_controls.h>
#include <gal/painter.h>
#include <base_screen.h>
#include <gal/cursors.h>
#include <gal/graphics_abstraction_layer.h>

#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>

#include <kiplatform/ui.h>

#include <core/profile.h>

#include <pgm_base.h>
#include <settings/common_settings.h>
#include <confirm.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <qt_cairo_gal.h>
#include <gal/opengl/opengl_gal.h>


/**
 * Flag to enable drawing panel debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceDrawPanel[] = wxT( "KICAD_DRAW_PANEL" );


/// The event that asks the tools to see the position of the cursor in the world again.
class REFRESH_MOUSE_EVENT : public QEvent
{
public:
    static QEvent::Type EventType()
    {
        static const QEvent::Type type = static_cast<QEvent::Type>( QEvent::registerEventType() );
        return type;
    }

    REFRESH_MOUSE_EVENT( int aModifiers ) :
            QEvent( EventType() ),
            m_modifiers( aModifiers )
    {
    }

    int m_modifiers;
};


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWidgetArgCompat aParentWindow,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        wxWindowCompat<QWidget>( aParentWindow ),
        m_parent( aParentWindow ),
        m_edaFrame( nullptr ),
        m_refreshTimer( [this]() { onRefreshTimer(); } ),
        m_drawing( false ),
        m_drawingEnabled( false ),
        m_needIdleRefresh( false ),
        m_gal( nullptr ),
        m_view( nullptr ),
        m_painter( nullptr ),
        m_viewControls( nullptr ),
        m_backend( GAL_TYPE_NONE ),
        m_options( aOptions ),
        m_eventDispatcher( nullptr ),
        m_lostFocus( false ),
        m_stealsFocus( true ),
        m_statusPopup( nullptr )
{
    m_PaintEventCounter = std::make_unique<PROF_COUNTER>( "Draw panel paint events" );

    setLayoutDirection( Qt::LeftToRight );
    setMouseTracking( true );
    setFocusPolicy( Qt::StrongFocus );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setAttribute( Qt::WA_AcceptTouchEvents );
    setMinimumSize( 64, 64 );

    // If we're in a dialog, we have to go looking for our parent frame
    for( QWidget* ancestor = aParentWindow; ancestor && !m_edaFrame;
         ancestor = ancestor->parentWidget() )
    {
        m_edaFrame = dynamic_cast<EDA_DRAW_FRAME*>( ancestor );
    }

    SwitchBackend( aGalType );
}


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWidgetArgCompat aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        EDA_DRAW_PANEL_GAL( aParentWindow, aOptions, aGalType )
{
    if( aWindowId != wxID_ANY )
        KIQT::SetWindowId( this, aWindowId );

    if( aSize.x > 0 && aSize.y > 0 )
        resize( aSize.x, aSize.y );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    StopDrawing();

    KI_ASSERT( !m_drawing );

    delete m_viewControls;
    delete m_view;
    delete m_gal;
    m_gal = nullptr;    // Ensure OnShow is not called
}


KIGFX::VIEW_CONTROLS* EDA_DRAW_PANEL_GAL::GetViewControls() const
{
    return m_viewControls;
}


void EDA_DRAW_PANEL_GAL::SetFocus()
{
    setFocus( Qt::OtherFocusReason );
    m_lostFocus = false;
}


bool EDA_DRAW_PANEL_GAL::StatusPopupHasFocus()
{
    return m_statusPopup && m_statusPopup->hasFocus();
}


bool EDA_DRAW_PANEL_GAL::IsDialogPreview() const
{
    // The canvas of a frame is shown in the frame; the others are previews (in dialogs)
    return !m_edaFrame || m_edaFrame->GetCanvas() != this;
}


void EDA_DRAW_PANEL_GAL::createViewControls()
{
    m_viewControls = new KIGFX::QT_VIEW_CONTROLS( m_view, this );
}


VECTOR2D EDA_DRAW_PANEL_GAL::ToDevice( const QPointF& aPoint ) const
{
    const double ratio = devicePixelRatioF();
    return VECTOR2D( aPoint.x() * ratio, aPoint.y() * ratio );
}


VECTOR2I EDA_DRAW_PANEL_GAL::GetClientSizeDevice() const
{
    const double ratio = devicePixelRatioF();
    return VECTOR2I( KiROUND( width() * ratio ), KiROUND( height() * ratio ) );
}


bool EDA_DRAW_PANEL_GAL::WarpPointer( const VECTOR2D& aDevicePoint )
{
    const double ratio = devicePixelRatioF();

    return KIPLATFORM::UI::WarpPointer( this, KiROUND( aDevicePoint.x / ratio ),
                                        KiROUND( aDevicePoint.y / ratio ) );
}


void EDA_DRAW_PANEL_GAL::PostRefreshMouse( int aModifiers )
{
    QCoreApplication::postEvent( this, new REFRESH_MOUSE_EVENT( aModifiers ) );
}


void EDA_DRAW_PANEL_GAL::paintEvent( QPaintEvent* aEvent )
{
    wxPaintEvent event;
    onPaint( event );
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& aEvent )
{
    DoRePaint( false );

    // The OpenGL GAL draws in its own window, over the panel
    if( m_backend == GAL_TYPE_OPENGL )
        return;

    QPainter painter( this );

    if( m_image.isNull() )
        painter.fillRect( rect(), palette().window() );
    else
        painter.drawImage( QPointF( 0, 0 ), m_image );

    if( m_gal && m_backend == GAL_TYPE_CAIRO )
        static_cast<KIGFX::QT_CAIRO_GAL*>( m_gal )->PaintCursor( painter, devicePixelRatioF() );
}


void EDA_DRAW_PANEL_GAL::present()
{
    KIGFX::QT_CAIRO_GAL* gal = static_cast<KIGFX::QT_CAIRO_GAL*>( m_gal );
    const VECTOR2I       size = gal->GetScreenPixelSize();

    // The raster is ARGB32 with premultiplied alpha in native byte order, as QImage's
    // Format_ARGB32_Premultiplied
    QImage raster( gal->Raster(), size.x, size.y, gal->RasterStride(),
                   QImage::Format_ARGB32_Premultiplied );

    m_image = raster.copy();
    m_image.setDevicePixelRatio( devicePixelRatioF() );
}


bool EDA_DRAW_PANEL_GAL::recoverFromGalError( const std::exception& aError )
{
    try
    {
        DisplayErrorMessage( m_parent, _( "Graphics error" ), wxString( aError.what() ) );
    }
    catch( ... )
    {
    }

    return false;
}


bool EDA_DRAW_PANEL_GAL::DoRePaint( bool aAllowSkip )
{
    if( !m_refreshMutex.try_lock() )
        return false;

    std::lock_guard<std::mutex> lock( m_refreshMutex, std::adopt_lock );

    if( !m_drawingEnabled || !m_view || !m_painter )
        return false;

    if( !m_gal->IsInitialized() || !m_gal->IsVisible() || m_gal->IsContextLocked() )
        return false;

    if( m_drawing )
        return false;

    m_lastRepaintStart = std::chrono::steady_clock::now();

    // Update current zoom settings if the canvas is managed by a EDA frame
    // (i.e. not by a preview panel in a dialog)
    if( !IsDialogPreview() && GetParentEDAFrame() && GetParentEDAFrame()->GetScreen() )
        GetParentEDAFrame()->GetScreen()->m_ScrollCenter = GetView()->GetCenter();

    SCOPED_SET_RESET<bool> drawing( m_drawing, true );

    ( *m_PaintEventCounter )++;

    KIGFX::RENDER_SETTINGS* settings =
            static_cast<KIGFX::RENDER_SETTINGS*>( m_painter->GetSettings() );

    PROF_TIMER cntUpd("view-upd-items");
    PROF_TIMER cntTotal("view-total");
    PROF_TIMER cntCtx("view-context-create");
    PROF_TIMER cntCtxDestroy("view-context-destroy");
    PROF_TIMER cntRedraw("view-redraw-rects");

    bool isDirty = false;

    cntTotal.Start();

    try
    {
        VECTOR2D cursorPos = m_viewControls ? m_viewControls->GetCursorPosition()
                                            : VECTOR2D();
        bool viewDirty = m_view->IsDirty();
        bool cursorMoved = ( cursorPos != m_lastCursorPosition );
        bool hasPendingItemUpdates = m_view->HasPendingItemUpdates();

        // Skip all update work when nothing has changed since the previous frame.
        // Never skip when responding to a native paint event or explicit ForceRefresh
        // because the window content may have been invalidated by the OS.
        if( aAllowSkip && !viewDirty && !cursorMoved && !hasPendingItemUpdates )
        {
            m_lastRepaintEnd = std::chrono::steady_clock::now();
            return true;
        }

        if( hasPendingItemUpdates )
        {
            cntUpd.Start();

            try
            {
                m_view->UpdateItems();
            }
            catch( std::out_of_range& err )
            {
                // Don't do anything here but don't fail
                // This can happen when we don't catch `at()` calls
                KI_LOG_TRACE( traceDrawPanel, wxS( "Out of Range error: %s" ), err.what() );
            }

            cntUpd.Stop();
            viewDirty = m_view->IsDirty();
        }

        // After processing item updates, skip the drawing when neither the view targets nor
        // the cursor position have changed.
        if( aAllowSkip && !viewDirty && !cursorMoved )
        {
            m_lastRepaintEnd = std::chrono::steady_clock::now();
            return true;
        }

        m_lastCursorPosition = cursorPos;

        // GAL_DRAWING_CONTEXT can throw in the dtor, so we need to scope
        // the full lifetime inside the try block
        {
            cntCtx.Start();
            KIGFX::GAL_DRAWING_CONTEXT ctx( m_gal );
            cntCtx.Stop();

            if( m_view->IsTargetDirty( KIGFX::TARGET_OVERLAY )
                && !m_gal->HasTarget( KIGFX::TARGET_OVERLAY ) )
            {
                m_view->MarkDirty();
            }

            m_gal->SetClearColor( settings->GetBackgroundColor() );
            m_gal->SetGridColor( settings->GetGridColor() );
            m_gal->SetCursorColor( settings->GetCursorColor() );

            if( m_view->IsDirty() )
            {
                // Cairo only needs to clear when NONCACHED content changed.
                if( m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                    m_gal->ClearScreen();

                m_view->ClearTargets();

                // Grid has to be redrawn only when the NONCACHED target is redrawn
                if( m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                    m_gal->DrawGrid();

                cntRedraw.Start();
                m_view->Redraw();
                cntRedraw.Stop();
                isDirty = true;
            }

            m_gal->DrawCursor( cursorPos );

            cntCtxDestroy.Start();
        }

        // ctx goes out of scope here so destructor would be called
        cntCtxDestroy.Stop();
    }
    catch( std::exception& err )
    {
        KI_LOG_TRACE( traceDrawPanel, wxS( "DoRePaint exception: %s" ), err.what() );

        if( recoverFromGalError( err ) )
            return true;

        StopDrawing();
    }
    catch( ... )
    {
        DisplayErrorMessage( m_parent, _( "Graphics error" ), _( "Unknown exception" ) );
        StopDrawing();
    }

    if( isDirty )
    {
        KI_TRACE( traceGalProfile, "View timing: %s %s %s %s %s\n",
            cntTotal.to_string(),
            cntUpd.to_string(),
            cntRedraw.to_string(),
            cntCtx.to_string(),
            cntCtxDestroy.to_string()
        );
    }

    m_lastRepaintEnd = std::chrono::steady_clock::now();

    return true;
}


void EDA_DRAW_PANEL_GAL::resizeEvent( QResizeEvent* aEvent )
{
    ResizeGal();
    QWidget::resizeEvent( aEvent );
}


void EDA_DRAW_PANEL_GAL::ResizeGal( bool aForce )
{
    // If we get a second update call before the first finishes, don't crash
    if( m_gal->IsContextLocked() )
        return;

    KIGFX::GAL_CONTEXT_LOCKER locker( m_gal );

    // The OpenGL GAL has the logical size (it scales its buffers), the Cairo GAL the pixels
    VECTOR2I clientSize = m_backend == GAL_TYPE_OPENGL ? VECTOR2I( width(), height() )
                                                       : GetClientSizeDevice();

    if( !aForce && clientSize == m_gal->GetScreenPixelSize() )
        return;

    clientSize.x = std::max( 10, clientSize.x );
    clientSize.y = std::max( 10, clientSize.y );

    m_gal->ResizeScreen( clientSize.x, clientSize.y );

    // ResizeScreen reallocates every compositor buffer, so nothing survives the resize
    if( m_view )
        m_view->MarkDirty();
}


void EDA_DRAW_PANEL_GAL::RequestRefresh()
{
    m_needIdleRefresh = true;
    update();
}


void EDA_DRAW_PANEL_GAL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    // The drawing is done by the paint event, at most once by frame of the screen
    update();
}


void EDA_DRAW_PANEL_GAL::ForceRefresh()
{
    if( !m_drawingEnabled )
    {
        if( m_gal && m_gal->IsInitialized() )
        {
            m_drawingEnabled = true;
        }
        else
        {
            // Try again soon
            m_refreshTimer.StartOnce( 100 );
            return;
        }
    }

    repaint();
}


bool EDA_DRAW_PANEL_GAL::GetScreenshot( QImage& aDstImage )
{
    if( !m_gal )
        return false;

    DoRePaint( false );

    if( m_backend == GAL_TYPE_OPENGL )
    {
        std::vector<uint8_t> rgba;
        VECTOR2I             size;

        if( !static_cast<KIGFX::OPENGL_GAL*>( m_gal )->GetScreenshot( rgba, size ) )
            return false;

        aDstImage = QImage( rgba.data(), size.x, size.y, QImage::Format_RGBA8888 ).copy();
    }
    else
    {
        aDstImage = m_image;
    }

    return !aDstImage.isNull();
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;
}


void EDA_DRAW_PANEL_GAL::StartDrawing()
{
    // Start querying GAL if it is ready
    m_refreshTimer.StartOnce( 100 );
}


void EDA_DRAW_PANEL_GAL::StopDrawing()
{
    m_refreshTimer.Stop();
    m_drawingEnabled = false;
}


void EDA_DRAW_PANEL_GAL::SetHighContrastLayer( int aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearHighContrastLayers();
    rSettings->SetLayerIsHighContrast( aLayer );

    m_view->UpdateAllLayersColor();
}


void EDA_DRAW_PANEL_GAL::SetTopLayer( int aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );
    m_view->UpdateAllLayersOrder();
}


bool EDA_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    // OpenGL needs its features (and a display that has OpenGL)
    if( aGalType == GAL_TYPE_OPENGL )
    {
        wxString errormsg = KIGFX::OPENGL_GAL::CheckFeatures( m_options );

        if( !errormsg.empty() )
        {
            KI_LOG_TRACE( traceDrawPanel, wxS( "OpenGL is not available: %s" ), errormsg );
            aGalType = GAL_FALLBACK;
        }
    }

    // Do not do anything if the currently used GAL is correct
    if( aGalType == m_backend && m_gal != nullptr )
        return true;

    VECTOR2D grid_size = m_gal ? m_gal->GetGridSize() : VECTOR2D();
    bool     grid_visibility = m_gal ? m_gal->GetGridVisibility() : true;

    // Prevent refreshing canvas during backend switch
    StopDrawing();

    KIGFX::GAL* new_gal = nullptr;

    try
    {
        if( aGalType == GAL_TYPE_OPENGL )
        {
            KIGFX::OPENGL_GAL* glGal = new KIGFX::OPENGL_GAL( GetVcSettings(), m_options, this,
                                                              this, this );
            glGal->show();
            new_gal = glGal;
        }
    }
    catch( std::runtime_error& err )
    {
        KI_LOG_TRACE( traceDrawPanel, wxS( "OpenGL GAL: %s" ), err.what() );
        aGalType = GAL_FALLBACK;
    }

    if( !new_gal )
    {
        new_gal = new KIGFX::QT_CAIRO_GAL( m_options, [this]() { present(); } );
        aGalType = GAL_TYPE_CAIRO;
    }

    // trigger update of the gal options in case they differ from the defaults
    m_options.NotifyChanged();

    delete m_gal;
    m_gal = new_gal;

    VECTOR2I clientSize = aGalType == GAL_TYPE_OPENGL ? VECTOR2I( width(), height() )
                                                      : GetClientSizeDevice();
    clientSize.x = std::max( 10, clientSize.x );
    clientSize.y = std::max( 10, clientSize.y );

    {
        KIGFX::GAL_CONTEXT_LOCKER locker( m_gal );
        m_gal->ResizeScreen( clientSize.x, clientSize.y );
    }

    if( grid_size.x > 0 && grid_size.y > 0 )
        m_gal->SetGridSize( grid_size );

    m_gal->SetGridVisibility( grid_visibility );

    // Make sure the cursor is set on the new canvas
    SetCurrentCursor( KICURSOR::ARROW );

    if( m_painter )
        m_painter->SetGAL( m_gal );

    if( m_view )
    {
        m_view->SetGAL( m_gal );
        m_view->ReverseDrawOrder( false );
    }

    m_backend = aGalType;

    return true;
}


bool EDA_DRAW_PANEL_GAL::event( QEvent* aEvent )
{
    switch( aEvent->type() )
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::NativeGesture:
        if( onInputEvent( aEvent ) )
        {
            aEvent->accept();
            return true;
        }

        return QWidget::event( aEvent );

    case QEvent::Enter:
        onEnter();
        return QWidget::event( aEvent );

    case QEvent::Leave:
        if( m_viewControls )
            m_viewControls->onLeave();

        return QWidget::event( aEvent );

    default:
        if( aEvent->type() == REFRESH_MOUSE_EVENT::EventType() )
        {
            if( m_eventDispatcher )
                m_eventDispatcher->RefreshMouse( static_cast<REFRESH_MOUSE_EVENT*>( aEvent )->m_modifiers );

            Refresh();
            return true;
        }

        if( aEvent->type() == QEvent::UpdateRequest || aEvent->type() == QEvent::Paint )
        {
            if( m_needIdleRefresh )
                m_needIdleRefresh = false;
        }

        return QWidget::event( aEvent );
    }
}


bool EDA_DRAW_PANEL_GAL::onInputEvent( QEvent* aEvent )
{
    bool shouldSetFocus = m_lostFocus && m_stealsFocus
                          && aEvent->type() != QEvent::KeyPress
                          && !QApplication::activePopupWidget()             // Don't steal from menus
                          && !QApplication::activeModalWidget()             // Don't steal from dialogs
                          && KIPLATFORM::UI::IsWindowActive( this );        // Don't steal from other windows

    if( shouldSetFocus )
        SetFocus();

    // Clicks give the focus to the canvas (it gets the hotkeys)
    if( aEvent->type() == QEvent::MouseButtonPress && !hasFocus() )
        SetFocus();

    // The view controls zoom and pan first: they let the event go on to the tools or not
    bool toTools = true;

    if( m_viewControls )
    {
        switch( aEvent->type() )
        {
        case QEvent::MouseMove:
            toTools = m_viewControls->onMotion( static_cast<QMouseEvent*>( aEvent ) );
            break;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            toTools = m_viewControls->onButton( static_cast<QMouseEvent*>( aEvent ) );
            break;

        case QEvent::Wheel:
            toTools = m_viewControls->onWheel( static_cast<QWheelEvent*>( aEvent ) );
            break;

        case QEvent::NativeGesture:
            toTools = !m_viewControls->onGesture( static_cast<QNativeGestureEvent*>( aEvent ) );
            break;

        default:
            break;
        }
    }

    bool used = !toTools;

    if( toTools && m_eventDispatcher )
        used |= m_eventDispatcher->DispatchQtEvent( aEvent );

    Refresh();

    // The wheel and the buttons are always for the canvas (no scrolling of a parent)
    return used || aEvent->type() != QEvent::KeyPress;
}


void EDA_DRAW_PANEL_GAL::onEnter()
{
    bool shouldSetFocus = m_stealsFocus
                          && !QApplication::activePopupWidget()             // Don't steal from menus
                          && !QApplication::activeModalWidget()             // Don't steal from dialogs
                          && KIPLATFORM::UI::IsWindowActive( this );        // Don't steal from other windows

    // Getting focus is necessary in order to receive key events properly
    if( shouldSetFocus )
    {
        QWidget* focus = QApplication::focusWidget();
        bool     textFocus = focus && focus->inherits( "QLineEdit" );

        if( !textFocus )
            SetFocus();
    }

    if( m_viewControls )
        m_viewControls->onEnter();
}


void EDA_DRAW_PANEL_GAL::focusOutEvent( QFocusEvent* aEvent )
{
    m_lostFocus = true;

    if( m_viewControls )
        m_viewControls->CancelDrag();

    // Reset the tool dispatcher's button state when focus is lost. This prevents
    // the dispatcher from thinking the button is still pressed when focus returns,
    // which can cause selection and drag operations to stop working.
    if( m_eventDispatcher && aEvent->reason() != Qt::PopupFocusReason )
        m_eventDispatcher->ResetState();

    QWidget::focusOutEvent( aEvent );
}


bool EDA_DRAW_PANEL_GAL::focusNextPrevChild( bool aNext )
{
    // Tab is a hotkey of the tools, not the key that moves the focus
    return false;
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer()
{
    ForceRefresh();
}


void EDA_DRAW_PANEL_GAL::showEvent( QShowEvent* aEvent )
{
    QWidget::showEvent( aEvent );

    if( m_gal && m_gal->IsInitialized() && m_gal->IsVisible() )
        OnShow();
}


void EDA_DRAW_PANEL_GAL::SetCurrentCursor( KICURSOR aCursor )
{
    if( !m_gal )
        return;

    // The cursors of KiCad have a HiDPI image; Qt scales the other one
    bool hidpi = devicePixelRatioF() >= 2.0;

    if( m_gal->SetNativeCursorStyle( aCursor, hidpi ) )
        setCursor( CURSOR_STORE::GetCursor( aCursor, hidpi ) );
}


std::shared_ptr<KIGFX::VIEW_OVERLAY> EDA_DRAW_PANEL_GAL::DebugOverlay()
{
    if( !m_debugOverlay )
    {
        m_debugOverlay.reset( new KIGFX::VIEW_OVERLAY() );
        m_view->Add( m_debugOverlay.get() );
    }

    return m_debugOverlay;
}


void EDA_DRAW_PANEL_GAL::ClearDebugOverlay()
{
    if( m_debugOverlay )
    {
        m_view->Remove( m_debugOverlay.get() );
        m_debugOverlay = nullptr;
    }
}


KIGFX::VC_SETTINGS EDA_DRAW_PANEL_GAL::GetVcSettings()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    KIGFX::VC_SETTINGS vcSettings;
    vcSettings.m_warpCursor = cfg->m_Input.center_on_zoom;
    vcSettings.m_focusFollowSchPcb = cfg->m_Input.focus_follow_sch_pcb;
    vcSettings.m_autoPanSettingEnabled = cfg->m_Input.auto_pan;
    vcSettings.m_autoPanAcceleration = cfg->m_Input.auto_pan_acceleration;
    vcSettings.m_horizontalPan = cfg->m_Input.horizontal_pan;
    vcSettings.m_zoomAcceleration = cfg->m_Input.zoom_acceleration;
    vcSettings.m_zoomSpeed = cfg->m_Input.zoom_speed;
    vcSettings.m_zoomSpeedAuto = cfg->m_Input.zoom_speed_auto;
    vcSettings.m_scrollModifierZoom = cfg->m_Input.scroll_modifier_zoom;
    vcSettings.m_scrollModifierPanH = cfg->m_Input.scroll_modifier_pan_h;
    vcSettings.m_scrollModifierPanV = cfg->m_Input.scroll_modifier_pan_v;
    vcSettings.m_motionPanModifier = cfg->m_Input.motion_pan_modifier;
    vcSettings.m_dragLeft = cfg->m_Input.drag_left;
    vcSettings.m_dragMiddle = cfg->m_Input.drag_middle;
    vcSettings.m_dragRight = cfg->m_Input.drag_right;
    vcSettings.m_scrollReverseZoom = cfg->m_Input.reverse_scroll_zoom;
    vcSettings.m_scrollReversePanH = cfg->m_Input.reverse_scroll_pan_h;

    return vcSettings;
}
