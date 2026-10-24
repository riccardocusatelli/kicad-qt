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
 * @file appearance_controls_qt.cpp
 * The widgets of the appearance panel of the board editors on Qt (see
 * pcbnew/widgets/appearance_controls_wx.cpp): the rows of the layers, objects and net classes,
 * the nets grid, the presets and viewports.  What the controls do is in
 * pcbnew/widgets/appearance_controls.cpp.
 */

#include <widgets/appearance_controls.h>

#include <QCheckBox>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadioButton>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <dialogs/dialog_color_picker.h>
#include <dialogs/eda_view_switcher.h>
#include <eda_list_dialog.h>
#include <footprint_editor_settings.h>
#include <hotkeys_basic.h>
#include <pcb_base_frame.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_local_settings.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <view/view.h>
#include <widgets/bitmap_button.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <widgets/ui_common.h>
#include <QTableWidget>
#include <widgets/wx_infobar.h>

#include <algorithm>
#include <core/ki_assert.h>
#include <qt_strings.h>
#include <qt_ui.h>
#include <ui_hooks.h>


namespace
{

/// The columns of the nets grid.
enum NET_GRID_COLUMN
{
    COL_COLOR = 0,
    COL_VISIBILITY,
    COL_LABEL
};


/// The indicator of the active layer: a triangle pointing to the row (ROW_ICON_PROVIDER of the
/// wxWidgets build).
QPixmap indicatorPixmap( const QWidget* aWidget, bool aOn )
{
    const int   size = KIUI::c_IndicatorSizeDIP;
    const qreal scale = aWidget->devicePixelRatioF();
    QPixmap     pixmap( std::lround( size * scale ), std::lround( size * scale ) );

    pixmap.setDevicePixelRatio( scale );
    pixmap.fill( Qt::transparent );

    if( aOn )
    {
        QPainter painter( &pixmap );
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( Qt::NoPen );
        painter.setBrush( aWidget->palette().color( QPalette::WindowText ) );

        QPainterPath path;
        path.moveTo( size * 0.25, size * 0.15 );
        path.lineTo( size * 0.80, size * 0.50 );
        path.lineTo( size * 0.25, size * 0.85 );
        path.closeSubpath();
        painter.drawPath( path );
    }

    return pixmap;
}


/// A collapsible pane (WX_COLLAPSIBLE_PANE of the wxWidgets build): a button which shows or
/// hides the pane under it.
QToolButton* makePaneButton( QWidget* aParent, QWidget* aPane, const wxString& aLabel )
{
    QToolButton* button = new QToolButton( aParent );

    button->setText( ToQString( aLabel ) );
    button->setCheckable( true );
    button->setChecked( false );
    button->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    button->setArrowType( Qt::RightArrow );
    button->setAutoRaise( true );

    aPane->setVisible( false );

    QObject::connect( button, &QToolButton::toggled, aPane,
            [button, aPane]( bool aExpanded )
            {
                button->setArrowType( aExpanded ? Qt::DownArrow : Qt::RightArrow );
                aPane->setVisible( aExpanded );
            } );

    return button;
}


/// Remove the items of a layout and delete their widgets.
void clearLayout( QLayout* aLayout )
{
    while( QLayoutItem* item = aLayout->takeAt( 0 ) )
    {
        if( QWidget* widget = item->widget() )
            delete widget;

        delete item;
    }
}

} // namespace


APPEARANCE_CONTROLS::APPEARANCE_CONTROLS( PCB_BASE_FRAME* aParent, QWidget* aFocusOwner,
                                          bool aFpEditorMode ) :
        APPEARANCE_CONTROLS_BASE( aParent ),
        m_frame( aParent ),
        m_focusOwner( aFocusOwner ),
        m_board( nullptr ),
        m_isFpEditor( aFpEditorMode ),
        m_currentPreset( nullptr ),
        m_lastSelectedUserPreset( nullptr ),
        m_lastSelectedViewport( nullptr ),
        m_layerContextMenu( nullptr ),
        m_togglingNetclassRatsnestVisibility( false )
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int      screenHeight = screen ? screen->size().height() : 1080;

    m_pointSize = font().pointSize();
    m_layerPanelColour = m_panelLayers->palette().color( QPalette::Window ).lighter( 110 );

    SetBorders( true, false, false, false );

    // The rows of the layers and of the objects, in the scrolled areas
    m_layersWidget = new QWidget( m_windowLayers );
    m_layersOuterSizer = new QVBoxLayout( m_layersWidget );
    m_layersOuterSizer->setContentsMargins( 0, 0, 0, 0 );
    m_layersOuterSizer->setSpacing( 0 );
    m_windowLayers->setWidget( m_layersWidget );

    m_objectsWidget = new QWidget( m_windowObjects );
    m_objectsOuterSizer = new QVBoxLayout( m_objectsWidget );
    m_objectsOuterSizer->setContentsMargins( 0, 0, 0, 0 );
    m_objectsOuterSizer->setSpacing( 0 );
    m_windowObjects->setWidget( m_objectsWidget );

    m_netclassWidget = m_netclassScrolledWindow->widget();
    m_netclassOuterSizer->setContentsMargins( 0, 0, 0, 0 );
    m_netclassOuterSizer->setSpacing( 0 );

    QFont infoFont = KIQT::InfoFont( this );
    m_staticTextNets->setFont( infoFont );
    m_staticTextNetClasses->setFont( infoFont );
    m_panelLayers->setFont( infoFont );
    m_windowLayers->setFont( infoFont );
    m_windowObjects->setFont( infoFont );
    m_presetsLabel->setFont( infoFont );
    m_viewportsLabel->setFont( infoFont );

    m_cbLayerPresets->setToolTip( ToQString( wxString::Format( _( "Save and restore layer visibility combinations.\n"
                                                                  "Use %s+Tab to activate selector.\n"
                                                                  "Successive Tabs while holding %s down will "
                                                                  "cycle through presets in the popup." ),
                                                               KeyNameFromKeyCode( PRESET_SWITCH_KEY ),
                                                               KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) ) );

    m_cbViewports->setToolTip( ToQString( wxString::Format( _( "Save and restore view location and zoom.\n"
                                                               "Use %s+Tab to activate selector.\n"
                                                               "Successive Tabs while holding %s down will "
                                                               "cycle through viewports in the popup." ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) ) );

    createControls();

    m_btnNetInspector->SetBitmap( KiBitmapBundle( BITMAPS::list_nets_16 ) );
    m_btnNetInspector->SetPadding( 2 );

    m_btnConfigureNetClasses->SetBitmap( KiBitmapBundle( BITMAPS::options_generic_16 ) );
    m_btnConfigureNetClasses->SetPadding( 2 );

    m_txtNetFilter->setPlaceholderText( ToQString( _( "Filter nets" ) ) );

    QObject::connect( m_txtNetFilter, &QLineEdit::textChanged, this,
            [this]()
            {
                rebuildNetsGrid();
            } );

    if( screenHeight <= 900 && m_pointSize >= KIUI::c_IndicatorSizeDIP )
        m_pointSize = m_pointSize * 8 / 10;

    auto setHighContrastMode =
            [this]( HIGH_CONTRAST_MODE aMode )
            {
                PCB_DISPLAY_OPTIONS opts   = m_frame->GetDisplayOptions();
                opts.m_ContrastModeDisplay = aMode;

                m_frame->SetDisplayOptions( opts );
                passOnFocus();
            };

    QObject::connect( m_rbHighContrastNormal, &QRadioButton::clicked, this,
                      [setHighContrastMode]() { setHighContrastMode( HIGH_CONTRAST_MODE::NORMAL ); } );
    QObject::connect( m_rbHighContrastDim, &QRadioButton::clicked, this,
                      [setHighContrastMode]() { setHighContrastMode( HIGH_CONTRAST_MODE::DIMMED ); } );
    QObject::connect( m_rbHighContrastOff, &QRadioButton::clicked, this,
                      [setHighContrastMode]() { setHighContrastMode( HIGH_CONTRAST_MODE::HIDDEN ); } );

    QObject::connect( m_btnNetInspector, &QAbstractButton::clicked, this,
            [this]()
            {
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::showNetInspector );
            } );

    QObject::connect( m_btnConfigureNetClasses, &QAbstractButton::clicked, this,
            [this]()
            {
                // This panel should only be visible in the PCB_EDIT_FRAME anyway
                if( PCB_EDIT_FRAME* editframe = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
                    editframe->ShowBoardSetupDialog( _( "Net Classes" ) );

                passOnFocus();
            } );

    m_cbFlipBoard->setChecked( m_frame->GetDisplayOptions().m_FlipBoardView );

    QObject::connect( m_cbFlipBoard, &QCheckBox::clicked, this,
            [this]()
            {
                m_frame->GetToolManager()->RunAction( PCB_ACTIONS::flipBoard );
                syncLayerPresetSelection();
            } );

    // The nets grid: the color, the visibility and the name of the nets
    m_netsGrid->setColumnCount( 3 );
    m_netsGrid->setRowCount( 0 );
    m_netsGrid->setFont( infoFont );
    m_netsGrid->setMouseTracking( true );
    m_netsGrid->setSelectionMode( QAbstractItemView::SingleSelection );
    m_netsGrid->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_netsGrid->horizontalHeader()->setStretchLastSection( true );

    const int cellPadding = 6;
    wxSize    swatchSize = COLOR_SWATCH::DialogUnitsToPixels( m_netsGrid, SWATCH_SIZE_SMALL_DU );
    QSize     iconSize = KiBitmapBundle( BITMAPS::visibility ).actualSize( QSize( 16, 16 ) );

    m_netsGrid->setColumnWidth( COL_COLOR, swatchSize.x + cellPadding );
    m_netsGrid->setColumnWidth( COL_VISIBILITY, iconSize.width() + cellPadding );
    m_netsGrid->setIconSize( QSize( std::max( swatchSize.x, iconSize.width() ),
                                    std::max( swatchSize.y, iconSize.height() ) ) );
    m_netsGrid->verticalHeader()->setDefaultSectionSize( QFontMetrics( infoFont ).height() + 3 );

    // The tooltips of the cells and the middle click on the colors
    QObject::connect( m_netsGrid, &QTableWidget::cellEntered, this,
            [this]( int aRow, int aCol )
            {
                QTableWidgetItem* item = m_netsGrid->item( aRow, aCol );

                if( !item || item->toolTip().size() )
                    return;

                if( aCol == COL_COLOR )
                {
                    item->setToolTip( ToQString( _( "Double click (or middle click) to change color; "
                                                    "right click for more actions" ) ) );
                }
            } );

    KIQT::FilterEvents( m_netsGrid->viewport(),
            [this]( QEvent* aEvent )
            {
                if( aEvent->type() == QEvent::MouseButtonRelease )
                {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>( aEvent );

                    if( mouseEvent->button() == Qt::MiddleButton )
                    {
                        QModelIndex index = m_netsGrid->indexAt( mouseEvent->position().toPoint() );

                        if( index.isValid() && index.column() == COL_COLOR )
                            editNetColor( index.row() );

                        return true;
                    }
                }

                return false;
            } );

    if( m_isFpEditor )
        m_notebook->removeTab( 2 );

    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
    {
        if( cfg->m_AuiPanels.appearance_expand_layer_display )
            m_paneLayerDisplayOptions->setChecked( true );

        if( cfg->m_AuiPanels.appearance_expand_net_display )
            m_paneNetDisplayOptions->setChecked( true );
    }

    loadDefaultLayerPresets();
    rebuildLayerPresetsWidget( true );
    rebuildObjects();
    OnBoardChanged();

    // Grid visibility is loaded and set to the GAL before we are constructed
    SetObjectVisible( LAYER_GRID, m_frame->IsGridVisible() );

    m_frame->AddLanguageChangedListener( this, [this]() { OnLanguageChanged(); } );
}


APPEARANCE_CONTROLS::~APPEARANCE_CONTROLS()
{
    // As the wxWidgets version: not removed from the board, which the frame may have deleted
    delete m_layerContextMenu;
}


void APPEARANCE_CONTROLS::createControls()
{
    QFont infoFont = KIQT::InfoFont( this );

    auto setupRadio =
            []( QRadioButton* aButton, const QFont& aFont )
            {
                aButton->setFont( aFont );
                return aButton;
            };

    // Layer display options
    m_layerDisplayPane = new QWidget( m_panelLayers );
    m_paneLayerDisplayOptions = makePaneButton( m_panelLayers, m_layerDisplayPane,
                                                _( "Layer Display Options" ) );

    QVBoxLayout* layerDisplayOptionsSizer = new QVBoxLayout( m_layerDisplayPane );
    layerDisplayOptionsSizer->setContentsMargins( 5, 0, 5, 0 );
    layerDisplayOptionsSizer->setSpacing( 2 );

    m_inactiveLayersLabel = new QLabel( m_layerDisplayPane );
    m_inactiveLayersLabel->setFont( infoFont );
    layerDisplayOptionsSizer->addWidget( m_inactiveLayersLabel );

    QHBoxLayout* contrastModeSizer = new QHBoxLayout();
    m_rbHighContrastNormal = setupRadio( new QRadioButton( m_layerDisplayPane ), infoFont );
    m_rbHighContrastDim = setupRadio( new QRadioButton( m_layerDisplayPane ), infoFont );
    m_rbHighContrastOff = setupRadio( new QRadioButton( m_layerDisplayPane ), infoFont );
    m_rbHighContrastNormal->setChecked( true );

    contrastModeSizer->addWidget( m_rbHighContrastNormal );
    contrastModeSizer->addStretch( 1 );
    contrastModeSizer->addWidget( m_rbHighContrastDim );
    contrastModeSizer->addStretch( 1 );
    contrastModeSizer->addWidget( m_rbHighContrastOff );
    contrastModeSizer->addStretch( 1 );
    layerDisplayOptionsSizer->addLayout( contrastModeSizer );

    QFrame* separator = new QFrame( m_layerDisplayPane );
    separator->setFrameShape( QFrame::HLine );
    separator->setFrameShadow( QFrame::Sunken );
    layerDisplayOptionsSizer->addWidget( separator );

    m_cbFlipBoard = new QCheckBox( m_layerDisplayPane );
    m_cbFlipBoard->setFont( infoFont );
    layerDisplayOptionsSizer->addWidget( m_cbFlipBoard );

    m_panelLayersSizer->addWidget( m_paneLayerDisplayOptions, 0, Qt::AlignLeft );
    m_panelLayersSizer->addWidget( m_layerDisplayPane );

    // Net display options
    m_netDisplayPane = new QWidget( m_panelNetsAndClasses );
    m_paneNetDisplayOptions = makePaneButton( m_panelNetsAndClasses, m_netDisplayPane,
                                              _( "Net Display Options" ) );

    QVBoxLayout* netDisplayOptionsSizer = new QVBoxLayout( m_netDisplayPane );
    netDisplayOptionsSizer->setContentsMargins( 5, 0, 5, 0 );
    netDisplayOptionsSizer->setSpacing( 2 );

    m_txtNetDisplayTitle = new QLabel( m_netDisplayPane );
    m_txtNetDisplayTitle->setFont( infoFont );
    netDisplayOptionsSizer->addWidget( m_txtNetDisplayTitle );

    // The radio buttons of a group are exclusive among the buttons of the same parent: each
    // group has its own widget
    QWidget*     netColorGroup = new QWidget( m_netDisplayPane );
    QHBoxLayout* netColorSizer = new QHBoxLayout( netColorGroup );
    netColorSizer->setContentsMargins( 0, 0, 0, 0 );
    m_rbNetColorAll = setupRadio( new QRadioButton( netColorGroup ), infoFont );
    m_rbNetColorRatsnest = setupRadio( new QRadioButton( netColorGroup ), infoFont );
    m_rbNetColorOff = setupRadio( new QRadioButton( netColorGroup ), infoFont );
    m_rbNetColorRatsnest->setChecked( true );

    netColorSizer->addWidget( m_rbNetColorAll );
    netColorSizer->addStretch( 1 );
    netColorSizer->addWidget( m_rbNetColorRatsnest );
    netColorSizer->addStretch( 1 );
    netColorSizer->addWidget( m_rbNetColorOff );
    netDisplayOptionsSizer->addWidget( netColorGroup );

    m_txtRatsnestVisibility = new QLabel( m_netDisplayPane );
    m_txtRatsnestVisibility->setFont( infoFont );
    netDisplayOptionsSizer->addWidget( m_txtRatsnestVisibility );

    QWidget*     ratsnestGroup = new QWidget( m_netDisplayPane );
    QHBoxLayout* ratsnestDisplayModeSizer = new QHBoxLayout( ratsnestGroup );
    ratsnestDisplayModeSizer->setContentsMargins( 0, 0, 0, 0 );
    m_rbRatsnestAllLayers = setupRadio( new QRadioButton( ratsnestGroup ), infoFont );
    m_rbRatsnestVisLayers = setupRadio( new QRadioButton( ratsnestGroup ), infoFont );
    m_rbRatsnestNone = setupRadio( new QRadioButton( ratsnestGroup ), infoFont );
    m_rbRatsnestAllLayers->setChecked( true );

    ratsnestDisplayModeSizer->addWidget( m_rbRatsnestAllLayers );
    ratsnestDisplayModeSizer->addStretch( 1 );
    ratsnestDisplayModeSizer->addWidget( m_rbRatsnestVisLayers );
    ratsnestDisplayModeSizer->addStretch( 1 );
    ratsnestDisplayModeSizer->addWidget( m_rbRatsnestNone );
    netDisplayOptionsSizer->addWidget( ratsnestGroup );

    m_netsTabOuterSizer->addWidget( m_paneNetDisplayOptions, 0, Qt::AlignLeft );
    m_netsTabOuterSizer->addWidget( m_netDisplayPane );

    QObject::connect( m_paneNetDisplayOptions, &QToolButton::toggled, this,
                      [this]() { passOnFocus(); } );

    auto onNetColorMode =
            [this]()
            {
                if( m_rbNetColorAll->isChecked() )
                    setNetColorMode( NET_COLOR_MODE::ALL );
                else if( m_rbNetColorRatsnest->isChecked() )
                    setNetColorMode( NET_COLOR_MODE::RATSNEST );
                else
                    setNetColorMode( NET_COLOR_MODE::OFF );
            };

    for( QRadioButton* button : { m_rbNetColorAll, m_rbNetColorRatsnest, m_rbNetColorOff } )
        QObject::connect( button, &QRadioButton::clicked, this, onNetColorMode );

    auto onRatsnestMode =
            [this]()
            {
                if( m_rbRatsnestAllLayers->isChecked() )
                    setRatsnestDisplay( true, RATSNEST_MODE::ALL );
                else if( m_rbRatsnestVisLayers->isChecked() )
                    setRatsnestDisplay( true, RATSNEST_MODE::VISIBLE );
                else
                    setRatsnestDisplay( false, RATSNEST_MODE::ALL );
            };

    for( QRadioButton* button : { m_rbRatsnestAllLayers, m_rbRatsnestVisLayers, m_rbRatsnestNone } )
        QObject::connect( button, &QRadioButton::clicked, this, onRatsnestMode );

    // The context menu of the layers, on the rows and on the free area under them
    m_layersWidget->setContextMenuPolicy( Qt::CustomContextMenu );

    QObject::connect( m_layersWidget, &QWidget::customContextMenuRequested, this,
                      [this]() { showLayerContextMenu(); } );
}


wxSize APPEARANCE_CONTROLS::GetBestSize() const
{
    return wxSize( 220, 480 );
}


bool APPEARANCE_CONTROLS::IsLayerOptionsExpanded()
{
    return m_paneLayerDisplayOptions->isChecked();
}


bool APPEARANCE_CONTROLS::IsNetOptionsExpanded()
{
    return m_paneNetDisplayOptions->isChecked();
}


void APPEARANCE_CONTROLS::OnNotebookPageChanged( int aPage )
{
    // Once the page is shown
    QMetaObject::invokeMethod( this, [this]() { passOnFocus(); }, Qt::QueuedConnection );
}


void APPEARANCE_CONTROLS::OnSetFocus( QWidget* aSource )
{
    passOnFocus();
}


void APPEARANCE_CONTROLS::OnNetGridClick( int aRow, int aCol )
{
    if( aCol != COL_VISIBILITY )
        return;

    QTableWidgetItem* item = m_netsGrid->item( aRow, COL_LABEL );

    if( !item )
        return;

    int code = item->data( Qt::UserRole ).toInt();

    for( NET_GRID_ENTRY& net : m_nets )
    {
        if( net.code == code )
        {
            setNetVisible( net, !net.visible );
            updateNetRow( aRow );
            break;
        }
    }
}


void APPEARANCE_CONTROLS::OnNetGridDoubleClick( int aRow, int aCol )
{
    if( aCol == COL_COLOR )
        editNetColor( aRow );
}


void APPEARANCE_CONTROLS::OnNetGridRightClick( int aRow, int aCol )
{
    QTableWidgetItem* item = m_netsGrid->item( aRow, COL_LABEL );

    if( !item )
        return;

    m_netsGrid->selectRow( aRow );

    wxString netName = UnescapeString( ToWxString( item->text() ) );
    QMenu    menu( this );

    auto addItem =
            [&]( int aId, const wxString& aText )
            {
                QAction* action = menu.addAction( ToQString( aText ) );
                QObject::connect( action, &QAction::triggered, this,
                                  [this, aId, aRow]() { onNetContextMenu( aId, aRow ); } );
            };

    addItem( ID_SET_NET_COLOR, _( "Set Net Color" ) );
    addItem( ID_CLEAR_NET_COLOR, _( "Clear Net Color" ) );

    menu.addSeparator();

    addItem( ID_HIGHLIGHT_NET, wxString::Format( _( "Highlight %s" ), netName ) );
    addItem( ID_SELECT_NET, wxString::Format( _( "Select Tracks and Vias in %s" ), netName ) );
    addItem( ID_DESELECT_NET, wxString::Format( _( "Unselect Tracks and Vias in %s" ), netName ) );

    menu.addSeparator();

    addItem( ID_SHOW_ALL_NETS, _( "Show All Nets" ) );
    addItem( ID_HIDE_OTHER_NETS, _( "Hide All Other Nets" ) );

    menu.exec( QCursor::pos() );

    m_netsGrid->clearSelection();
}


void APPEARANCE_CONTROLS::OnLanguageChanged()
{
    m_notebook->setTabText( 0, ToQString( _( "Layers" ) ) );
    m_notebook->setTabText( 1, ToQString( _( "Objects" ) ) );

    if( m_notebook->count() >= 3 )
        m_notebook->setTabText( 2, ToQString( _( "Nets" ) ) );

    m_netsGrid->clearSelection();

    setUpdatesEnabled( false );

    rebuildLayers();
    rebuildLayerContextMenu();
    rebuildLayerPresetsWidget( false );
    rebuildViewportsWidget();
    rebuildObjects();
    rebuildNets();

    syncColorsAndVisibility();
    syncObjectSettings();
    syncLayerPresetSelection();

    UpdateDisplayOptions();

    setUpdatesEnabled( true );
}


void APPEARANCE_CONTROLS::OnBoardChanged()
{
    if( !m_frame->GetBoard() )
        return;

    m_netsGrid->clearSelection();

    setUpdatesEnabled( false );

    rebuildLayers();
    rebuildLayerContextMenu();
    syncColorsAndVisibility();
    syncObjectSettings();
    rebuildNets();
    rebuildLayerPresetsWidget( true );
    syncLayerPresetSelection();
    rebuildViewportsWidget();

    UpdateDisplayOptions();

    // As the wxWidgets version: the previous board is deleted already (a new board is loaded)
    m_board = m_frame->GetBoard();

    if( m_board )
        m_board->AddListener( this );

    setUpdatesEnabled( true );
}


void APPEARANCE_CONTROLS::OnNetVisibilityChanged( int aNetCode, bool aVisibility )
{
    if( m_togglingNetclassRatsnestVisibility )
        return;

    netGridSetVisibility( aNetCode, aVisibility );
    netGridRefresh();
}


void APPEARANCE_CONTROLS::handleBoardItemsChanged()
{
    if( !m_frame->GetBoard() )
        return;

    m_netsGrid->clearSelection();

    setUpdatesEnabled( false );
    rebuildNets();
    setUpdatesEnabled( true );
}


void APPEARANCE_CONTROLS::OnDarkModeToggle()
{
    m_layerPanelColour = m_panelLayers->palette().color( QPalette::Window ).lighter( 110 );

    QPalette layersPalette = m_layersWidget->palette();
    layersPalette.setColor( QPalette::Window, m_layerPanelColour );
    m_layersWidget->setPalette( layersPalette );
    m_layersWidget->setAutoFillBackground( true );

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        if( setting->ctl_color )
            setting->ctl_color->OnDarkModeToggle();
    }

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->ctl_color )
            setting->ctl_color->OnDarkModeToggle();
    }

    for( const std::pair<const wxString, APPEARANCE_SETTING*>& pair : m_netclassSettingsMap )
    {
        if( pair.second->ctl_color )
            pair.second->ctl_color->OnDarkModeToggle();
    }

    netGridRefresh();

    OnLayerChanged();       // Update selected highlighting
}


void APPEARANCE_CONTROLS::OnLayerChanged()
{
    auto setRowColour =
            []( QWidget* aRow, const QColor& aColour )
            {
                QPalette palette = aRow->palette();
                palette.setColor( QPalette::Window, aColour );
                aRow->setPalette( palette );
                aRow->setAutoFillBackground( true );
            };

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        if( !setting->ctl_panel )
            continue;

        setRowColour( setting->ctl_panel, m_layerPanelColour );
        setting->ctl_indicator->setPixmap( indicatorPixmap( setting->ctl_indicator, false ) );
    }

    // The active layer: a row slightly lighter (or darker on light backgrounds)
    int r = m_layerPanelColour.red();
    int g = m_layerPanelColour.green();
    int b = m_layerPanelColour.blue();

    if( r < 240 || g < 240 || b < 240 )
    {
        r = std::min( r + 15, 255 );
        g = std::min( g + 15, 255 );
        b = std::min( b + 15, 255 );
    }
    else
    {
        r = std::max( r - 15, 0 );
        g = std::max( g - 15, 0 );
        b = std::max( b - 15, 0 );
    }

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    if( !m_layerSettingsMap.count( current ) )
    {
        if( !m_layerSettingsMap.count( F_Cu ) )
            return;

        current = F_Cu;
    }

    APPEARANCE_SETTING* newSetting = m_layerSettingsMap[ current ];

    if( newSetting->ctl_panel )
    {
        setRowColour( newSetting->ctl_panel, QColor( r, g, b ) );
        newSetting->ctl_indicator->setPixmap( indicatorPixmap( newSetting->ctl_indicator, true ) );
    }
}


void APPEARANCE_CONTROLS::UpdateDisplayOptions()
{
    const PCB_DISPLAY_OPTIONS& options = m_frame->GetDisplayOptions();

    switch( options.m_ContrastModeDisplay )
    {
    case HIGH_CONTRAST_MODE::NORMAL: m_rbHighContrastNormal->setChecked( true ); break;
    case HIGH_CONTRAST_MODE::DIMMED: m_rbHighContrastDim->setChecked( true );    break;
    case HIGH_CONTRAST_MODE::HIDDEN: m_rbHighContrastOff->setChecked( true );    break;
    }

    switch( options.m_NetColorMode )
    {
    case NET_COLOR_MODE::ALL:      m_rbNetColorAll->setChecked( true );      break;
    case NET_COLOR_MODE::RATSNEST: m_rbNetColorRatsnest->setChecked( true ); break;
    case NET_COLOR_MODE::OFF:      m_rbNetColorOff->setChecked( true );      break;
    }

    m_cbFlipBoard->setChecked( m_frame->GetDisplayOptions().m_FlipBoardView );

    if( !m_isFpEditor )
    {
        if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
        {
            if( !cfg->m_Display.m_ShowGlobalRatsnest )
                m_rbRatsnestNone->setChecked( true );
            else if( cfg->m_Display.m_RatsnestMode == RATSNEST_MODE::ALL )
                m_rbRatsnestAllLayers->setChecked( true );
            else
                m_rbRatsnestVisLayers->setChecked( true );

            KI_ASSERT( m_objectSettingsMap.count( LAYER_RATSNEST ) );
            APPEARANCE_SETTING* ratsnest = m_objectSettingsMap.at( LAYER_RATSNEST );
            ratsnest->ctl_visibility->SetValue( cfg->m_Display.m_ShowGlobalRatsnest );
        }
    }
}


void APPEARANCE_CONTROLS::ApplyLayerPreset( const wxString& aPresetName )
{
    updateLayerPresetSelection( aPresetName );
    onLayerPresetChanged( m_cbLayerPresets );
}


void APPEARANCE_CONTROLS::ApplyViewport( const wxString& aViewportName )
{
    updateViewportSelection( aViewportName );
    onViewportChanged( m_cbViewports );
}


void APPEARANCE_CONTROLS::rebuildLayers()
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    LSET enabled = board->GetEnabledLayers();
    LSET visible = getVisibleLayers();

    COLOR_SETTINGS* theme    = m_frame->GetColorSettings();
    COLOR4D         bgColor  = theme->GetColor( LAYER_PCB_BACKGROUND );
    bool            readOnly = theme->IsReadOnly();

    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

    // The rows are built again (the wxWidgets build keeps and hides them)
    clearLayout( m_layersOuterSizer );
    m_layerSettings.clear();
    m_layerSettingsMap.clear();

    auto appendLayer =
            [&]( std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                int layer = aSetting->id;

                QWidget*     panel = new QWidget( m_layersWidget );
                QHBoxLayout* sizer = new QHBoxLayout( panel );
                sizer->setContentsMargins( 1, 2, 0, 0 );
                sizer->setSpacing( 0 );

                aSetting->visible = visible[layer];

                QLabel* indicator = new QLabel( panel );
                indicator->setPixmap( indicatorPixmap( indicator, false ) );

                COLOR_SWATCH* swatch = new COLOR_SWATCH( panel, COLOR4D::UNSPECIFIED, layer, bgColor,
                                                         theme->GetColor( layer ), SWATCH_SMALL );
                swatch->setToolTip( ToQString( _( "Double click or middle click for color change, "
                                                  "right click for menu" ) ) );

                BITMAP_TOGGLE* btn_visible = new BITMAP_TOGGLE( panel, layer,
                                                                KiBitmapBundle( BITMAPS::visibility ),
                                                                KiBitmapBundle( BITMAPS::visibility_off ),
                                                                aSetting->visible );
                btn_visible->setToolTip( ToQString( _( "Show or hide this layer" ) ) );

                QLabel* label = new QLabel( ToQString( aSetting->label ), panel );
                label->setToolTip( ToQString( aSetting->tooltip ) );

                sizer->addWidget( indicator, 0, Qt::AlignVCenter );
                sizer->addSpacing( 5 );
                sizer->addWidget( swatch, 0, Qt::AlignVCenter );
                sizer->addSpacing( 6 );
                sizer->addWidget( btn_visible, 0, Qt::AlignVCenter );
                sizer->addSpacing( 5 );
                sizer->addWidget( label, 1, Qt::AlignVCenter );

                m_layersOuterSizer->addWidget( panel );

                aSetting->ctl_panel      = panel;
                aSetting->ctl_indicator  = indicator;
                aSetting->ctl_visibility = btn_visible;
                aSetting->ctl_color      = swatch;
                aSetting->ctl_text       = label;

                // A left click on the row makes its layer the active one
                for( QWidget* widget : std::initializer_list<QWidget*>{ panel, indicator, swatch, label } )
                {
                    KIQT::OnEvent( widget, QEvent::MouseButtonPress,
                            [this, layer]( QEvent* aEvent )
                            {
                                if( static_cast<QMouseEvent*>( aEvent )->button() == Qt::LeftButton )
                                    onLayerLeftClick( layer );
                            } );
                }

                btn_visible->SetToggleCallback(
                        [this, layer]( bool aVisible )
                        {
                            onLayerVisibilityToggled( static_cast<PCB_LAYER_ID>( layer ) );
                        } );

                swatch->SetChangedCallback(
                        [this, swatch]()
                        {
                            onLayerColorChanged( swatch->GetId(), swatch->GetSwatchColor() );
                        } );

                swatch->SetReadOnlyCallback( [this]() { onReadOnlySwatch(); } );
                swatch->SetReadOnly( readOnly );
            };

    // technical layers are shown in this order:
    // Because they are static, wxGetTranslation must be explicitly
    // called for tooltips.
    static const struct {
        PCB_LAYER_ID layerId;
        wxString     tooltip;
    } non_cu_seq[] = {
        { F_Adhes,          _HKI( "Adhesive on board's front" ) },
        { B_Adhes,          _HKI( "Adhesive on board's back" ) },
        { F_Paste,          _HKI( "Solder paste on board's front" ) },
        { B_Paste,          _HKI( "Solder paste on board's back" ) },
        { F_SilkS,          _HKI( "Silkscreen on board's front" ) },
        { B_SilkS,          _HKI( "Silkscreen on board's back" ) },
        { F_Mask,           _HKI( "Solder mask on board's front" ) },
        { B_Mask,           _HKI( "Solder mask on board's back" ) },
        { Dwgs_User,        _HKI( "Explanatory drawings" ) },
        { Cmts_User,        _HKI( "Explanatory comments" ) },
        { Eco1_User,        _HKI( "User defined meaning" ) },
        { Eco2_User,        _HKI( "User defined meaning" ) },
        { Edge_Cuts,        _HKI( "Board's perimeter definition" ) },
        { Margin,           _HKI( "Board's edge setback outline" ) },
        { F_CrtYd,          _HKI( "Footprint courtyards on board's front" ) },
        { B_CrtYd,          _HKI( "Footprint courtyards on board's back" ) },
        { F_Fab,            _HKI( "Footprint assembly on board's front" ) },
        { B_Fab,            _HKI( "Footprint assembly on board's back" ) },
        { User_1,           _HKI( "User defined layer 1" ) },
        { User_2,           _HKI( "User defined layer 2" ) },
        { User_3,           _HKI( "User defined layer 3" ) },
        { User_4,           _HKI( "User defined layer 4" ) },
        { User_5,           _HKI( "User defined layer 5" ) },
        { User_6,           _HKI( "User defined layer 6" ) },
        { User_7,           _HKI( "User defined layer 7" ) },
        { User_8,           _HKI( "User defined layer 8" ) },
        { User_9,           _HKI( "User defined layer 9" ) },
        { User_10,           _HKI( "User defined layer 10" ) },
        { User_11,           _HKI( "User defined layer 11" ) },
        { User_12,           _HKI( "User defined layer 12" ) },
        { User_13,           _HKI( "User defined layer 13" ) },
        { User_14,           _HKI( "User defined layer 14" ) },
        { User_15,           _HKI( "User defined layer 15" ) },
        { User_16,           _HKI( "User defined layer 16" ) },
        { User_17,           _HKI( "User defined layer 17" ) },
        { User_18,           _HKI( "User defined layer 18" ) },
        { User_19,           _HKI( "User defined layer 19" ) },
        { User_20,           _HKI( "User defined layer 20" ) },
        { User_21,           _HKI( "User defined layer 21" ) },
        { User_22,           _HKI( "User defined layer 22" ) },
        { User_23,           _HKI( "User defined layer 23" ) },
        { User_24,           _HKI( "User defined layer 24" ) },
        { User_25,           _HKI( "User defined layer 25" ) },
        { User_26,           _HKI( "User defined layer 26" ) },
        { User_27,           _HKI( "User defined layer 27" ) },
        { User_28,           _HKI( "User defined layer 28" ) },
        { User_29,           _HKI( "User defined layer 29" ) },
        { User_30,           _HKI( "User defined layer 30" ) },
        { User_31,           _HKI( "User defined layer 31" ) },
        { User_32,           _HKI( "User defined layer 32" ) },
        { User_33,           _HKI( "User defined layer 33" ) },
        { User_34,           _HKI( "User defined layer 34" ) },
        { User_35,           _HKI( "User defined layer 35" ) },
        { User_36,           _HKI( "User defined layer 36" ) },
        { User_37,           _HKI( "User defined layer 37" ) },
        { User_38,           _HKI( "User defined layer 38" ) },
        { User_39,           _HKI( "User defined layer 39" ) },
        { User_40,           _HKI( "User defined layer 40" ) },
        { User_41,           _HKI( "User defined layer 41" ) },
        { User_42,           _HKI( "User defined layer 42" ) },
        { User_43,           _HKI( "User defined layer 43" ) },
        { User_44,           _HKI( "User defined layer 44" ) },
        { User_45,           _HKI( "User defined layer 45" ) },
    };

    auto addSetting =
            [&]( PCB_LAYER_ID aLayer, const wxString& aLabel, const wxString& aTooltip )
            {
                m_layerSettings.push_back( std::make_unique<APPEARANCE_SETTING>() );
                std::unique_ptr<APPEARANCE_SETTING>& setting = m_layerSettings.back();

                setting->label = aLabel;
                setting->id = aLayer;
                setting->tooltip = aTooltip;
                setting->spacer = false;

                appendLayer( setting );

                m_layerSettingsMap[aLayer] = setting.get();

                if( !isLayerEnabled( aLayer ) )
                {
                    setting->ctl_text->setEnabled( false );
                    setting->ctl_color->setToolTip( QString() );
                }
            };

    // show all coppers first, with front on top, back on bottom, then technical layers
    for( PCB_LAYER_ID layer : enabled.CuStack() )
    {
        wxString dsc;

        switch( layer )
        {
            case F_Cu: dsc = _( "Front copper layer" ); break;
            case B_Cu: dsc = _( "Back copper layer" );  break;
            default:   dsc = _( "Inner copper layer" ); break;
        }

        addSetting( layer, board->GetLayerName( layer ), dsc );
    }

    for( const auto& entry : non_cu_seq )
    {
        PCB_LAYER_ID layer = entry.layerId;

        if( !enabled[layer] )
            continue;

        wxString label;

        if( m_isFpEditor && cfg )
        {
            wxString canonicalName = LSET::Name( static_cast<PCB_LAYER_ID>( layer ) );

            if( cfg->m_DesignSettings.m_UserLayerNames.contains( canonicalName.ToStdString() ) )
                label = cfg->m_DesignSettings.m_UserLayerNames[canonicalName.ToStdString()];
            else
                label = board->GetStandardLayerName( layer );
        }
        else
        {
            label = board->GetLayerName( layer );
        }

        // Because non_cu_seq is created static, we must explicitly call wxGetTranslation for
        // texts which are internationalized
        addSetting( layer, label, wxGetTranslation( entry.tooltip ) );
    }

    m_layersOuterSizer->addSpacing( 10 );
    m_layersOuterSizer->addStretch( 1 );

    QPalette layersPalette = m_layersWidget->palette();
    layersPalette.setColor( QPalette::Window, m_layerPanelColour );
    m_layersWidget->setPalette( layersPalette );
    m_layersWidget->setAutoFillBackground( true );

    m_paneLayerDisplayOptions->setText( ToQString( _( "Layer Display Options" ) ) );

    int      hotkey = PCB_ACTIONS::highContrastModeCycle.GetHotKey();
    wxString msg;

    if( hotkey )
        msg = wxString::Format( _( "Inactive layers (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Inactive layers:" );

    m_inactiveLayersLabel->setText( ToQString( msg ) );

    m_rbHighContrastNormal->setText( ToQString( _( "Normal" ) ) );
    m_rbHighContrastNormal->setToolTip( ToQString( _( "Inactive layers will be shown in full color" ) ) );

    m_rbHighContrastDim->setText( ToQString( _( "Dim" ) ) );
    m_rbHighContrastDim->setToolTip( ToQString( _( "Inactive layers will be dimmed" ) ) );

    m_rbHighContrastOff->setText( ToQString( _( "Hide" ) ) );
    m_rbHighContrastOff->setToolTip( ToQString( _( "Inactive layers will be hidden" ) ) );

    m_cbFlipBoard->setText( ToQString( _( "Flip board view" ) ) );
}


void APPEARANCE_CONTROLS::rebuildLayerContextMenu()
{
    delete m_layerContextMenu;
    m_layerContextMenu = new QMenu();

    auto addItem =
            [this]( int aId, const wxString& aText, BITMAPS aBitmap )
            {
                QAction* action = m_layerContextMenu->addAction( KiBitmapBundle( aBitmap ),
                                                                 ToQString( aText ) );
                QObject::connect( action, &QAction::triggered, this,
                                  [this, aId]() { doLayerContextMenu( aId ); } );
            };

    addItem( ID_SHOW_ALL_COPPER_LAYERS, _( "Show All Copper Layers" ), BITMAPS::show_all_copper_layers );
    addItem( ID_HIDE_ALL_COPPER_LAYERS, _( "Hide All Copper Layers" ), BITMAPS::show_no_copper_layers );

    m_layerContextMenu->addSeparator();

    addItem( ID_HIDE_ALL_BUT_ACTIVE, _( "Hide All Layers But Active" ), BITMAPS::select_w_layer );

    m_layerContextMenu->addSeparator();

    addItem( ID_SHOW_ALL_NON_COPPER, _( "Show All Non Copper Layers" ), BITMAPS::show_no_copper_layers );
    addItem( ID_HIDE_ALL_NON_COPPER, _( "Hide All Non Copper Layers" ), BITMAPS::show_all_copper_layers );

    m_layerContextMenu->addSeparator();

    addItem( ID_PRESET_ALL_LAYERS, _( "Show All Layers" ), BITMAPS::show_all_layers );
    addItem( ID_PRESET_NO_LAYERS, _( "Hide All Layers" ), BITMAPS::show_no_layers );

    m_layerContextMenu->addSeparator();

    addItem( ID_PRESET_FRONT_ASSEMBLY, _( "Show Only Front Assembly Layers" ),
             BITMAPS::show_front_assembly_layers );
    addItem( ID_PRESET_FRONT, _( "Show Only Front Layers" ), BITMAPS::show_all_front_layers );

    // Only show the internal layer option if internal layers are enabled
    if( m_frame->GetBoard() && m_frame->GetBoard()->GetCopperLayerCount() > 2 )
        addItem( ID_PRESET_INNER_COPPER, _( "Show Only Inner Layers" ), BITMAPS::show_all_copper_layers );

    addItem( ID_PRESET_BACK, _( "Show Only Back Layers" ), BITMAPS::show_all_back_layers );
    addItem( ID_PRESET_BACK_ASSEMBLY, _( "Show Only Back Assembly Layers" ),
             BITMAPS::show_back_assembly_layers );
}


void APPEARANCE_CONTROLS::showLayerContextMenu()
{
    KI_ASSERT( m_layerContextMenu );

    if( m_layerContextMenu )
        m_layerContextMenu->exec( QCursor::pos() );

    passOnFocus();
}


int APPEARANCE_CONTROLS::GetTabIndex() const
{
    return m_notebook->currentIndex();
}


void APPEARANCE_CONTROLS::SetTabIndex( int aTab )
{
    if( aTab >= 0 && aTab < m_notebook->count() )
        m_notebook->setCurrentIndex( aTab );
}


void APPEARANCE_CONTROLS::syncColorsAndVisibility()
{
    COLOR_SETTINGS* theme    = m_frame->GetColorSettings();
    bool            readOnly = theme->IsReadOnly();
    LSET            visible  = getVisibleLayers();
    GAL_SET         objects  = getVisibleObjects();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_layerSettings )
    {
        int layer = setting->id;

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( visible[layer] );

        if( setting->ctl_color )
        {
            const COLOR4D& color = theme->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
            setting->ctl_color->SetReadOnly( readOnly );
        }
    }

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        GAL_LAYER_ID layer = static_cast<GAL_LAYER_ID>( setting->id );

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( objects.Contains( layer ) );

        if( setting->ctl_color )
        {
            const COLOR4D& color = theme->GetColor( layer );
            setting->ctl_color->SetSwatchColor( color, false );
            setting->ctl_color->SetReadOnly( readOnly );
        }
    }

    // Update indicators and panel background colors
    OnLayerChanged();
}


void APPEARANCE_CONTROLS::onLayerLeftClick( int aLayer )
{
    PCB_LAYER_ID layer = ToLAYER_ID( aLayer );

    if( !isLayerEnabled( layer ) )
        return;

    m_frame->SetActiveLayer( layer );
    passOnFocus();
}


void APPEARANCE_CONTROLS::rebuildObjects()
{
    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );
    GAL_SET         visible = getVisibleObjects();
    int             swatchWidth = COLOR_SWATCH::DialogUnitsToPixels( m_windowObjects, wxSize( 8, 0 ) ).x;
    int             labelWidth = 0;
    QFontMetrics    metrics( m_windowObjects->font() );

    int btnWidth = KiBitmapBundle( BITMAPS::visibility ).actualSize( QSize( 16, 16 ) ).width();

    m_objectSettings.clear();
    m_objectSettingsMap.clear();
    clearLayout( m_objectsOuterSizer );
    m_objectsOuterSizer->addSpacing( 5 );

    auto appendObject =
            [&]( const std::unique_ptr<APPEARANCE_SETTING>& aSetting )
            {
                QWidget*     panel = new QWidget( m_objectsWidget );
                QHBoxLayout* sizer = new QHBoxLayout( panel );
                int          layer = aSetting->id;

                sizer->setContentsMargins( 5, 0, 5, 0 );
                sizer->setSpacing( 0 );

                aSetting->visible = visible.Contains( ToGalLayer( layer ) );
                COLOR4D color     = theme->GetColor( layer );
                COLOR4D defColor  = theme->GetDefaultColor( layer );

                if( color != COLOR4D::UNSPECIFIED || defColor != COLOR4D::UNSPECIFIED )
                {
                    COLOR_SWATCH* swatch = new COLOR_SWATCH( panel, color, layer, bgColor, defColor,
                                                             SWATCH_SMALL );
                    swatch->setToolTip( ToQString( _( "Left double click or middle click for color "
                                                      "change, right click for menu" ) ) );

                    sizer->addWidget( swatch, 0, Qt::AlignVCenter );
                    aSetting->ctl_color = swatch;

                    swatch->SetChangedCallback(
                            [this, swatch]()
                            {
                                onLayerColorChanged( swatch->GetId(), swatch->GetSwatchColor() );
                            } );

                    swatch->SetReadOnlyCallback( [this]() { onReadOnlySwatch(); } );
                }
                else
                {
                    sizer->addSpacing( swatchWidth );
                }

                BITMAP_TOGGLE* btn_visible = nullptr;
                wxString       tip;

                if( aSetting->can_control_visibility )
                {
                    btn_visible = new BITMAP_TOGGLE( panel, layer,
                                                     KiBitmapBundle( BITMAPS::visibility ),
                                                     KiBitmapBundle( BITMAPS::visibility_off ),
                                                     aSetting->visible );

                    tip.Printf( _( "Show or hide %s" ), aSetting->label.Lower() );
                    btn_visible->setToolTip( ToQString( tip ) );

                    aSetting->ctl_visibility = btn_visible;

                    btn_visible->SetToggleCallback(
                            [this, layer]( bool aVisible )
                            {
                                onObjectVisibilityChanged( ToGalLayer( layer ), aVisible, true );
                            } );
                }

                sizer->addSpacing( 5 );

                if( btn_visible )
                    sizer->addWidget( btn_visible, 0, Qt::AlignVCenter );
                else
                    sizer->addSpacing( btnWidth );

                sizer->addSpacing( 5 );

                QLabel* label = new QLabel( ToQString( aSetting->label ), panel );
                label->setToolTip( ToQString( aSetting->tooltip ) );

                if( aSetting->can_control_opacity )
                {
                    label->setMinimumWidth( labelWidth );
                    sizer->addWidget( label, 0, Qt::AlignVCenter );

                    QSlider* slider = new QSlider( Qt::Horizontal, panel );
                    slider->setRange( 0, 100 );
                    slider->setValue( 100 );
                    slider->setMinimumWidth( 80 );

                    tip.Printf( _( "Set opacity of %s" ), aSetting->label.Lower() );
                    slider->setToolTip( ToQString( tip ) );

                    sizer->addSpacing( 5 );
                    sizer->addWidget( slider, 1, Qt::AlignVCenter );
                    sizer->addSpacing( 5 );
                    aSetting->ctl_opacity = slider;

                    QObject::connect( slider, &QSlider::valueChanged, this,
                            [this, layer]( int aValue )
                            {
                                onObjectOpacitySlider( layer, aValue / 100.0f );
                            } );
                }
                else
                {
                    sizer->addWidget( label, 1, Qt::AlignVCenter );
                }

                aSetting->ctl_panel = panel;
                aSetting->ctl_text = label;
                m_objectsOuterSizer->addWidget( panel );

                if( !aSetting->can_control_opacity )
                    m_objectsOuterSizer->addSpacing( 2 );
            };

    for( const APPEARANCE_SETTING& s_setting : s_objectSettings )
    {
        if( m_isFpEditor && !isAllowedInFpEditor( s_setting.id ) )
            continue;

        m_objectSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>( s_setting ) );

        std::unique_ptr<APPEARANCE_SETTING>& setting = m_objectSettings.back();

        // Because s_render_rows is created static, we must explicitly call wxGetTranslation
        // for texts which are internationalized (tool tips and item names)
        setting->tooltip = wxGetTranslation( s_setting.tooltip );
        setting->label   = wxGetTranslation( s_setting.label );

        if( setting->can_control_opacity )
        {
            int width = metrics.horizontalAdvance( ToQString( setting->label ) ) + 5;
            labelWidth = std::max( labelWidth, width );
        }

        if( !s_setting.spacer )
            m_objectSettingsMap[ToGalLayer( setting->id )] = setting.get();
    }

    for( const std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->spacer )
            m_objectsOuterSizer->addSpacing( m_pointSize / 2 );
        else
            appendObject( setting );
    }

    m_objectsOuterSizer->addStretch( 1 );
}


void APPEARANCE_CONTROLS::syncObjectSettings()
{
    GAL_SET visible = getVisibleObjects();

    const PCB_DISPLAY_OPTIONS& opts = m_frame->GetDisplayOptions();

    for( std::unique_ptr<APPEARANCE_SETTING>& setting : m_objectSettings )
    {
        if( setting->spacer )
            continue;

        GAL_LAYER_ID layer = ToGalLayer( setting->id );

        if( setting->ctl_visibility )
            setting->ctl_visibility->SetValue( visible.Contains( layer ) );

        if( setting->ctl_color )
        {
            COLOR4D color = m_frame->GetColorSettings()->GetColor( setting->id );
            setting->ctl_color->SetSwatchColor( color, false );
        }
    }

    KI_ASSERT( m_objectSettingsMap.count( LAYER_TRACKS )
              && m_objectSettingsMap.count( LAYER_VIAS )
              && m_objectSettingsMap.count( LAYER_PADS )
              && m_objectSettingsMap.count( LAYER_ZONES )
              && m_objectSettingsMap.count( LAYER_DRAW_BITMAPS )
              && m_objectSettingsMap.count( LAYER_FILLED_SHAPES ) );

    auto setOpacity =
            [this]( GAL_LAYER_ID aLayer, double aOpacity )
            {
                auto it = m_objectSettingsMap.find( aLayer );

                if( it == m_objectSettingsMap.end() || !it->second->ctl_opacity )
                    return;

                // Not a change of the user: no update of the display options
                QSignalBlocker blocker( it->second->ctl_opacity );
                it->second->ctl_opacity->setValue( static_cast<int>( std::lround( aOpacity * 100 ) ) );
            };

    setOpacity( LAYER_TRACKS, opts.m_TrackOpacity );
    setOpacity( LAYER_VIAS, opts.m_ViaOpacity );
    setOpacity( LAYER_PADS, opts.m_PadOpacity );
    setOpacity( LAYER_ZONES, opts.m_ZoneOpacity );
    setOpacity( LAYER_DRAW_BITMAPS, opts.m_ImageOpacity );
    setOpacity( LAYER_FILLED_SHAPES, opts.m_FilledShapeOpacity );
}


void APPEARANCE_CONTROLS::showNetclassMenu( bool isDefaultClass, const wxString& aName )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;
    QMenu                          menu( this );

    auto addItem =
            [&]( int aId, const wxString& aText )
            {
                QAction* action = menu.addAction( ToQString( aText ) );
                QObject::connect( action, &QAction::triggered, this,
                                  [this, aId]() { doNetclassContextMenu( aId ); } );
                return action;
            };

    if( !isDefaultClass )
    {
        addItem( ID_SET_NET_COLOR, _( "Set Netclass Color" ) );

        QAction*                  schematicColor = addItem( ID_USE_SCHEMATIC_NET_COLOR,
                                                            _( "Use Color from Schematic" ) );
        std::shared_ptr<NETCLASS> nc = netSettings->GetNetClassByName( aName );

        if( nc->GetSchematicColor() == KIGFX::COLOR4D::UNSPECIFIED )
            schematicColor->setEnabled( false );

        addItem( ID_CLEAR_NET_COLOR, _( "Clear Netclass Color" ) );
        menu.addSeparator();
    }

    wxString name = UnescapeString( aName );

    addItem( ID_HIGHLIGHT_NET, wxString::Format( _( "Highlight Nets in %s" ), name ) );
    addItem( ID_SELECT_NET, wxString::Format( _( "Select Tracks and Vias in %s" ), name ) );
    addItem( ID_DESELECT_NET, wxString::Format( _( "Unselect Tracks and Vias in %s" ), name ) );

    menu.addSeparator();

    addItem( ID_SHOW_ALL_NETS, _( "Show All Netclasses" ) );
    addItem( ID_HIDE_OTHER_NETS, _( "Hide All Other Netclasses" ) );

    m_contextMenuNetclass = aName;
    menu.exec( QCursor::pos() );
}


void APPEARANCE_CONTROLS::rebuildNets()
{
    BOARD* board = m_frame->GetBoard();

    if( !board || !board->GetProject() )
        return;

    COLOR_SETTINGS* theme   = m_frame->GetColorSettings();
    COLOR4D         bgColor = theme->GetColor( LAYER_PCB_BACKGROUND );

    m_staticTextNets->setText( ToQString( _( "Nets" ) ) );
    m_staticTextNetClasses->setText( ToQString( _( "Net Classes" ) ) );

    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;

    const std::set<wxString>& hiddenClasses = m_frame->Prj().GetLocalSettings().m_HiddenNetclasses;

    clearLayout( m_netclassOuterSizer );
    m_netclassSettings.clear();
    m_netclassSettingsMap.clear();

    auto appendNetclass =
            [&]( const std::shared_ptr<NETCLASS>& aClass, bool isDefaultClass = false )
            {
                wxString name = aClass->GetName();

                m_netclassSettings.emplace_back( std::make_unique<APPEARANCE_SETTING>() );
                APPEARANCE_SETTING* setting = m_netclassSettings.back().get();
                m_netclassSettingsMap[name] = setting;

                setting->ctl_panel = new QWidget( m_netclassWidget );
                QHBoxLayout* sizer = new QHBoxLayout( setting->ctl_panel );
                sizer->setContentsMargins( 0, 0, 0, 0 );
                sizer->setSpacing( 0 );

                COLOR4D color = netSettings->HasNetclass( name )
                                        ? netSettings->GetNetClassByName( name )->GetPcbColor()
                                        : COLOR4D::UNSPECIFIED;

                setting->ctl_color = new COLOR_SWATCH( setting->ctl_panel, color, wxID_ANY, bgColor,
                                                       COLOR4D::UNSPECIFIED, SWATCH_SMALL );
                setting->ctl_color->setToolTip( ToQString( _( "Left double click or middle click for "
                                                              "color change, right click for menu" ) ) );

                COLOR_SWATCH* swatch = setting->ctl_color;

                swatch->SetChangedCallback(
                        [this, name, swatch]()
                        {
                            setNetclassColor( name, swatch->GetSwatchColor() );
                        } );

                // Default netclass can't have an override color (its space is kept)
                if( isDefaultClass )
                {
                    QSizePolicy policy = swatch->sizePolicy();
                    policy.setRetainSizeWhenHidden( true );
                    swatch->setSizePolicy( policy );
                    swatch->hide();
                }

                setting->ctl_visibility = new BITMAP_TOGGLE( setting->ctl_panel, wxID_ANY,
                                                             KiBitmapBundle( BITMAPS::visibility ),
                                                             KiBitmapBundle( BITMAPS::visibility_off ),
                                                             !hiddenClasses.count( name ) );

                wxString tip;
                tip.Printf( _( "Show or hide ratsnest for nets in %s" ), name );
                setting->ctl_visibility->setToolTip( ToQString( tip ) );

                setting->ctl_visibility->SetToggleCallback(
                        [this, name]( bool aShow )
                        {
                            showNetclass( name, aShow );
                            passOnFocus();
                        } );

                setting->ctl_text = new QLabel( ToQString( name ), setting->ctl_panel );

                sizer->addWidget( setting->ctl_color, 0, Qt::AlignVCenter );
                sizer->addSpacing( 7 );
                sizer->addWidget( setting->ctl_visibility, 0, Qt::AlignVCenter );
                sizer->addSpacing( 3 );
                sizer->addWidget( setting->ctl_text, 1, Qt::AlignVCenter );

                m_netclassOuterSizer->addWidget( setting->ctl_panel );
                m_netclassOuterSizer->addSpacing( 2 );

                // The context menu of the net class, on all the widgets of its row
                setting->ctl_panel->setContextMenuPolicy( Qt::CustomContextMenu );

                QObject::connect( setting->ctl_panel, &QWidget::customContextMenuRequested, this,
                                  [this, name, isDefaultClass]()
                                  {
                                      showNetclassMenu( isDefaultClass, name );
                                  } );
            };

    std::vector<wxString> names;

    for( const auto& [name, netclass] : netSettings->GetNetclasses() )
        names.emplace_back( name );

    std::sort( names.begin(), names.end() );

    appendNetclass( netSettings->GetDefaultNetclass(), true );

    for( const wxString& name : names )
        appendNetclass( netSettings->GetNetclasses().at( name ) );

    m_netclassOuterSizer->addStretch( 1 );

    int      hotkey;
    wxString msg;

    m_paneNetDisplayOptions->setText( ToQString( _( "Net Display Options" ) ) );

    hotkey = PCB_ACTIONS::netColorModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Net colors (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Net colors:" );

    m_txtNetDisplayTitle->setText( ToQString( msg ) );
    m_txtNetDisplayTitle->setToolTip( ToQString( _( "Choose when to show net and netclass colors" ) ) );

    m_rbNetColorAll->setText( ToQString( _( "All" ) ) );
    m_rbNetColorAll->setToolTip( ToQString( _( "Net and netclass colors are shown on all copper items" ) ) );

    m_rbNetColorRatsnest->setText( ToQString( _( "Ratsnest" ) ) );
    m_rbNetColorRatsnest->setToolTip( ToQString( _( "Net and netclass colors are shown on the ratsnest only" ) ) );

    m_rbNetColorOff->setText( ToQString( _( "None" ) ) );
    m_rbNetColorOff->setToolTip( ToQString( _( "Net and netclass colors are not shown" ) ) );

    hotkey = PCB_ACTIONS::ratsnestModeCycle.GetHotKey();

    if( hotkey )
        msg = wxString::Format( _( "Ratsnest display (%s):" ), KeyNameFromKeyCode( hotkey ) );
    else
        msg = _( "Ratsnest display:" );

    m_txtRatsnestVisibility->setText( ToQString( msg ) );
    m_txtRatsnestVisibility->setToolTip( ToQString( _( "Choose which ratsnest lines to display" ) ) );

    m_rbRatsnestAllLayers->setText( ToQString( _( "All" ) ) );
    m_rbRatsnestAllLayers->setToolTip( ToQString( _( "Show ratsnest lines to items on all layers" ) ) );

    m_rbRatsnestVisLayers->setText( ToQString( _( "Visible layers" ) ) );
    m_rbRatsnestVisLayers->setToolTip( ToQString( _( "Show ratsnest lines to items on visible layers" ) ) );

    m_rbRatsnestNone->setText( ToQString( _( "None" ) ) );
    m_rbRatsnestNone->setToolTip( ToQString( _( "Hide all ratsnest lines" ) ) );

    // The nets (NET_GRID_TABLE::Rebuild() of the wxWidgets build)
    const NETNAMES_MAP&         nets = board->GetNetInfo().NetsByName();
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::set<int>&                 hiddenNets = rs->GetHiddenNets();
    std::map<int, KIGFX::COLOR4D>& netColors  = rs->GetNetColorMap();

    m_nets.clear();

    for( const std::pair<const wxString, NETINFO_ITEM*>& pair : nets )
    {
        int netCode = pair.second->GetNetCode();

        if( netCode > 0 && !pair.first.StartsWith( wxT( "unconnected-(" ) ) )
        {
            COLOR4D color = netColors.count( netCode ) ? netColors.at( netCode )
                                                       : COLOR4D::UNSPECIFIED;

            bool visible = hiddenNets.count( netCode ) == 0;

            m_nets.emplace_back( netCode, pair.first, color, visible );
        }
    }

    std::sort( m_nets.begin(), m_nets.end(),
               []( const NET_GRID_ENTRY& a, const NET_GRID_ENTRY& b )
               {
                   return a.name < b.name;
               } );

    rebuildNetsGrid();
}


void APPEARANCE_CONTROLS::rebuildNetsGrid()
{
    wxString filter = ToWxString( m_txtNetFilter->text() ).Lower();
    int      row = 0;

    m_netsGrid->clearContents();
    m_netsGrid->setRowCount( 0 );

    for( const NET_GRID_ENTRY& net : m_nets )
    {
        if( !filter.IsEmpty() && !net.name.Lower().Contains( filter ) )
            continue;

        m_netsGrid->insertRow( row );

        for( int col : { COL_COLOR, COL_VISIBILITY, COL_LABEL } )
        {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
            m_netsGrid->setItem( row, col, item );
        }

        QTableWidgetItem* label = m_netsGrid->item( row, COL_LABEL );
        label->setText( ToQString( UnescapeString( net.name ) ) );
        label->setData( Qt::UserRole, net.code );

        updateNetRow( row );
        row++;
    }
}


void APPEARANCE_CONTROLS::updateNetRow( int aRow )
{
    QTableWidgetItem* label = m_netsGrid->item( aRow, COL_LABEL );

    if( !label )
        return;

    int code = label->data( Qt::UserRole ).toInt();

    auto it = std::find_if( m_nets.begin(), m_nets.end(),
                            [code]( const NET_GRID_ENTRY& aNet ) { return aNet.code == code; } );

    if( it == m_nets.end() )
        return;

    const NET_GRID_ENTRY& net = *it;

    // The color: a swatch on the background of the panel
    QTableWidgetItem* colorItem = m_netsGrid->item( aRow, COL_COLOR );
    COLOR4D           background = FromQColor( m_panelNets->palette().color( QPalette::Window ) );
    wxSize            size = COLOR_SWATCH::DialogUnitsToPixels( m_netsGrid, SWATCH_SIZE_SMALL_DU );
    wxSize            checkerboard = COLOR_SWATCH::DialogUnitsToPixels( m_netsGrid, CHECKERBOARD_SIZE_DU );

    colorItem->setIcon( QIcon( COLOR_SWATCH::MakeBitmap( net.color, background, size, checkerboard,
                                                         background, m_netsGrid->devicePixelRatioF() ) ) );

    QTableWidgetItem* visibilityItem = m_netsGrid->item( aRow, COL_VISIBILITY );
    visibilityItem->setIcon( KiBitmapBundle( net.visible ? BITMAPS::visibility
                                                         : BITMAPS::visibility_off ) );

    wxString tip = net.visible ? _( "Click to hide ratsnest for %s" )
                               : _( "Click to show ratsnest for %s" );
    visibilityItem->setToolTip( ToQString( wxString::Format( tip, net.name ) ) );
}


void APPEARANCE_CONTROLS::editNetColor( int aRow )
{
    QTableWidgetItem* label = m_netsGrid->item( aRow, COL_LABEL );

    if( !label )
        return;

    int code = label->data( Qt::UserRole ).toInt();

    for( NET_GRID_ENTRY& net : m_nets )
    {
        if( net.code != code )
            continue;

        DIALOG_COLOR_PICKER dialog( window(), net.color, true );

        if( dialog.ShowModal() == wxID_OK )
        {
            setNetColor( net, dialog.GetColor() );
            updateNetRow( aRow );
        }

        break;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::setNetColor( NET_GRID_ENTRY& aNet, const COLOR4D& aColor )
{
    aNet.color = aColor;

    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings() );

    std::map<int, KIGFX::COLOR4D>& netColors = rs->GetNetColorMap();

    if( aNet.color != COLOR4D::UNSPECIFIED && aNet.color.a > 0.0 )
        netColors[aNet.code] = aNet.color;
    else
        netColors.erase( aNet.code );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::setNetVisible( NET_GRID_ENTRY& aNet, bool aVisible )
{
    aNet.visible = aVisible;

    const TOOL_ACTION& action = aNet.visible ? PCB_ACTIONS::showNetInRatsnest
                                             : PCB_ACTIONS::hideNetInRatsnest;

    m_frame->GetToolManager()->RunAction( action, aNet.code );
}


void APPEARANCE_CONTROLS::onNetContextMenu( int aId, int aRow )
{
    QTableWidgetItem* label = m_netsGrid->item( aRow, COL_LABEL );

    if( !label )
        return;

    int code = label->data( Qt::UserRole ).toInt();

    auto it = std::find_if( m_nets.begin(), m_nets.end(),
                            [code]( const NET_GRID_ENTRY& aNet ) { return aNet.code == code; } );

    if( it == m_nets.end() )
        return;

    NET_GRID_ENTRY& net = *it;

    switch( aId )
    {
    case ID_SET_NET_COLOR:
        editNetColor( aRow );
        break;

    case ID_CLEAR_NET_COLOR:
        setNetColor( net, COLOR4D::UNSPECIFIED );
        updateNetRow( aRow );
        break;

    case ID_HIGHLIGHT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::highlightNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_SELECT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_DESELECT_NET:
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::deselectNet, net.code );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_SHOW_ALL_NETS:
        for( NET_GRID_ENTRY& other : m_nets )
            setNetVisible( other, true );

        netGridRefresh();
        break;

    case ID_HIDE_OTHER_NETS:
    {
        int keep = net.code;

        for( NET_GRID_ENTRY& other : m_nets )
            setNetVisible( other, other.code == keep );

        netGridRefresh();
        break;
    }

    default:
        break;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::rebuildLayerPresetsWidget( bool aReset )
{
    m_presetsLabel->setText( ToQString( wxString::Format( _( "Presets (%s+Tab):" ),
                                                          KeyNameFromKeyCode( PRESET_SWITCH_KEY ) ) ) );

    QSignalBlocker blocker( m_cbLayerPresets );

    m_cbLayerPresets->clear();

    if( aReset )
        m_presetMRU.clear();

    // Build the layers preset list.
    // By default, the presetAllLayers will be selected
    int idx = 0;
    int default_idx = 0;
    std::vector<std::pair<wxString, void*>> userPresets;

    // m_layerPresets is alphabetical: m_presetMRU should also be alphabetical, but m_cbLayerPresets
    // is split into build-in and user sections.
    for( auto& [name, preset] : m_layerPresets )
    {
        const wxString translatedName = wxGetTranslation( name );
        void*          userData = static_cast<void*>( &preset );

        if( preset.readOnly )
            m_cbLayerPresets->addItem( ToQString( translatedName ), QVariant::fromValue( userData ) );
        else
            userPresets.push_back( { name, userData } );

        if( aReset )
            m_presetMRU.push_back( translatedName );

        if( name == presetAllLayers.name )
            default_idx = idx;

        idx++;
    }

    if( !userPresets.empty() )
    {
        m_cbLayerPresets->addItem( QStringLiteral( "---" ) );

        for( auto& [name, userData] : userPresets )
            m_cbLayerPresets->addItem( ToQString( name ), QVariant::fromValue( userData ) );
    }

    m_cbLayerPresets->addItem( QStringLiteral( "---" ) );
    m_cbLayerPresets->addItem( ToQString( _( "Save preset..." ) ) );
    m_cbLayerPresets->addItem( ToQString( _( "Delete preset..." ) ) );

    // At least the built-in presets should always be present
    KI_ASSERT( !m_layerPresets.empty() );

    if( aReset )
    {
        // Default preset: all layers
        m_cbLayerPresets->setCurrentIndex( default_idx );
        m_currentPreset = &m_layerPresets[presetAllLayers.name];
    }
}


LAYER_PRESET* APPEARANCE_CONTROLS::presetAt( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= m_cbLayerPresets->count() )
        return nullptr;

    return static_cast<LAYER_PRESET*>( m_cbLayerPresets->itemData( aIndex ).value<void*>() );
}


VIEWPORT* APPEARANCE_CONTROLS::viewportAt( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= m_cbViewports->count() )
        return nullptr;

    return static_cast<VIEWPORT*>( m_cbViewports->itemData( aIndex ).value<void*>() );
}


void APPEARANCE_CONTROLS::syncLayerPresetSelection()
{
    LSET    visibleLayers  = getVisibleLayers();
    GAL_SET visibleObjects = getVisibleObjects();
    bool    flipBoard = m_cbFlipBoard->isChecked();

    auto it = std::find_if( m_layerPresets.begin(), m_layerPresets.end(),
                            [&]( const std::pair<const wxString, LAYER_PRESET>& aPair )
                            {
                                return ( aPair.second.layers == visibleLayers
                                         && aPair.second.renderLayers == visibleObjects
                                         && aPair.second.flipBoard == flipBoard );
                            } );

    if( it != m_layerPresets.end() )
    {
        // Select the right m_cbLayersPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->findText( ToQString( text ) ) );
    }
    else
    {
        m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->count() - 3 ); // separator
    }

    m_currentPreset = presetAt( m_cbLayerPresets->currentIndex() );
}


void APPEARANCE_CONTROLS::updateLayerPresetSelection( const wxString& aName )
{
    // look at m_layerPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use
    // a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
    {
        if( pair.first != aName )
            continue;

        if( pair.second.readOnly == true )
            ui_label = wxGetTranslation( aName );

        break;
    }

    int idx = m_cbLayerPresets->findText( ToQString( ui_label ) );

    if( idx >= 0 && m_cbLayerPresets->currentIndex() != idx )
    {
        m_cbLayerPresets->setCurrentIndex( idx );
        m_currentPreset = presetAt( idx );
    }
    else if( idx < 0 )
    {
        m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->count() - 3 ); // separator
    }
}


void APPEARANCE_CONTROLS::onLayerPresetChanged( QWidget* aSource )
{
    int count = m_cbLayerPresets->count();
    int index = m_cbLayerPresets->currentIndex();

    auto resetSelection =
            [&]()
            {
                int current = m_currentPreset ? m_cbLayerPresets->findText( ToQString( m_currentPreset->name ) )
                                              : -1;

                if( current >= 0 )
                    m_cbLayerPresets->setCurrentIndex( current );
                else
                    m_cbLayerPresets->setCurrentIndex( m_cbLayerPresets->count() - 3 );
            };

    if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedUserPreset )
            name = m_lastSelectedUserPreset->name;

        bool    ok = false;
        QString text = QInputDialog::getText( window(), ToQString( _( "Save Layer Preset" ) ),
                                              ToQString( _( "Layer preset name:" ) ),
                                              QLineEdit::Normal, ToQString( name ), &ok );

        if( !ok )
        {
            resetSelection();
            return;
        }

        name = ToWxString( text );
        bool exists = m_layerPresets.count( name );

        if( !exists )
        {
            m_layerPresets[name] = LAYER_PRESET( name, getVisibleLayers(), getVisibleObjects(),
                                                 UNSELECTED_LAYER, m_cbFlipBoard->isChecked() );
        }

        LAYER_PRESET* preset = &m_layerPresets[name];

        if( !exists )
        {
            rebuildLayerPresetsWidget( false );
            index = m_cbLayerPresets->findText( text );
        }
        else if( preset->readOnly )
        {
            QMessageBox::critical( window(), ToQString( _( "Error" ) ),
                                   ToQString( _( "Default presets cannot be modified.\n"
                                                 "Please use a different name." ) ) );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( window(), _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            preset->layers       = getVisibleLayers();
            preset->renderLayers = getVisibleObjects();
            preset->flipBoard    = m_cbFlipBoard->isChecked();

            index = m_cbLayerPresets->findText( text );

            if( m_presetMRU.Index( name ) != wxNOT_FOUND )
                m_presetMRU.Remove( name );
        }

        m_currentPreset = preset;
        m_cbLayerPresets->setCurrentIndex( index );
        m_presetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbLayerPresets->findText( ToQString( presetName ) );

            if( idx >= 0 )
            {
                m_layerPresets.erase( presetName );

                m_cbLayerPresets->removeItem( idx );
                m_currentPreset = nullptr;
            }

            if( m_presetMRU.Index( presetName ) != wxNOT_FOUND )
                m_presetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }
    else if( m_cbLayerPresets->itemText( index ) == QStringLiteral( "---" ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }

    // Store the objects visibility settings if the preset is not a user preset,
    // to be reused when selecting a new built-in layer preset, even if a previous
    // user preset has changed the object visibility
    if( !m_currentPreset || m_currentPreset->readOnly )
        m_lastBuiltinPreset.renderLayers = getVisibleObjects();

    LAYER_PRESET* preset = presetAt( index );
    m_currentPreset = preset;

    m_lastSelectedUserPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        // Change board layers visibility, but do not change objects visibility
        LAYER_PRESET curr_layers_choice = *preset;

        // For predefined presets that do not manage objects visibility, use
        // the objects visibility settings of the last used predefined preset.
        if( curr_layers_choice.readOnly )
            curr_layers_choice.renderLayers = m_lastBuiltinPreset.renderLayers;

        doApplyLayerPreset( curr_layers_choice );
    }

    if( m_currentPreset && !m_currentPreset->name.IsEmpty() )
    {
        const wxString translatedName = wxGetTranslation( m_currentPreset->name );

        if( m_presetMRU.Index( translatedName ) != wxNOT_FOUND )
            m_presetMRU.Remove( translatedName );

        m_presetMRU.Insert( translatedName, 0 );
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::rebuildViewportsWidget()
{
    m_viewportsLabel->setText( ToQString( wxString::Format( _( "Viewports (%s+Tab):" ),
                                                            KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY ) ) ) );

    QSignalBlocker blocker( m_cbViewports );

    m_cbViewports->clear();

    for( std::pair<const wxString, VIEWPORT>& pair : m_viewports )
        m_cbViewports->addItem( ToQString( pair.first ), QVariant::fromValue( static_cast<void*>( &pair.second ) ) );

    m_cbViewports->addItem( QStringLiteral( "---" ) );
    m_cbViewports->addItem( ToQString( _( "Save viewport..." ) ) );
    m_cbViewports->addItem( ToQString( _( "Delete viewport..." ) ) );

    m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 );
    m_lastSelectedViewport = nullptr;
}


void APPEARANCE_CONTROLS::updateViewportSelection( const wxString& aName )
{
    int idx = m_cbViewports->findText( ToQString( aName ) );

    if( idx >= 0 && idx < m_cbViewports->count() - 3 /* separator */ )
    {
        m_cbViewports->setCurrentIndex( idx );
        m_lastSelectedViewport = viewportAt( idx );
    }
    else if( idx < 0 )
    {
        m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 ); // separator
        m_lastSelectedViewport = nullptr;
    }
}


void APPEARANCE_CONTROLS::onViewportChanged( QWidget* aSource )
{
    int count = m_cbViewports->count();
    int index = m_cbViewports->currentIndex();

    auto resetSelection =
            [&]()
            {
                int last = m_lastSelectedViewport ? m_cbViewports->findText( ToQString( m_lastSelectedViewport->name ) )
                                                  : -1;

                if( last >= 0 )
                    m_cbViewports->setCurrentIndex( last );
                else
                    m_cbViewports->setCurrentIndex( m_cbViewports->count() - 3 );
            };

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT* viewport = viewportAt( index );

        KI_CHECK( viewport, /* void */ );

        doApplyViewport( *viewport );

        if( !viewport->name.IsEmpty() )
        {
            if( m_viewportMRU.Index( viewport->name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewport->name );

            m_viewportMRU.Insert( viewport->name, 0 );
        }
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        bool    ok = false;
        QString text = QInputDialog::getText( window(), ToQString( _( "Save Viewport" ) ),
                                              ToQString( _( "Viewport name:" ) ),
                                              QLineEdit::Normal, QString(), &ok );

        if( !ok )
        {
            resetSelection();
            return;
        }

        wxString name = ToWxString( text );
        bool     exists = m_viewports.count( name );

        if( !exists )
        {
            m_viewports[name] = VIEWPORT( name, m_frame->GetCanvas()->GetView()->GetViewport() );

            index = index - 1;
            m_cbViewports->insertItem( index, text, QVariant::fromValue( static_cast<void*>( &m_viewports[name] ) ) );
        }
        else
        {
            m_viewports[name].rect = m_frame->GetCanvas()->GetView()->GetViewport();
            index = m_cbViewports->findText( text );

            if( m_viewportMRU.Index( name ) != wxNOT_FOUND )
                m_viewportMRU.Remove( name );
        }

        m_cbViewports->setCurrentIndex( index );
        m_viewportMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete an existing viewport
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Viewports" ) );

        for( std::pair<const wxString, VIEWPORT>& pair : m_viewports )
        {
            wxArrayString item;
            item.Add( pair.first );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( m_frame, _( "Delete Viewport" ), headers, items );
        dlg.SetListLabel( _( "Select viewport:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString viewportName = dlg.GetTextSelection();
            int      idx = m_cbViewports->findText( ToQString( viewportName ) );

            if( idx >= 0 )
            {
                if( m_lastSelectedViewport == viewportAt( idx ) )
                    m_lastSelectedViewport = nullptr;

                m_cbViewports->removeItem( idx );
                m_viewports.erase( viewportName );
            }

            if( m_viewportMRU.Index( viewportName ) != wxNOT_FOUND )
                m_viewportMRU.Remove( viewportName );
        }

        resetSelection();
        return;
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::passOnFocus()
{
    if( m_focusOwner )
        m_focusOwner->setFocus();
}


void APPEARANCE_CONTROLS::onReadOnlySwatch()
{
    WX_INFOBAR* infobar = m_frame->GetInfoBar();

    infobar->RemoveAllButtons();
    infobar->AddButton( _( "Open Preferences" ),
            [this]()
            {
                m_frame->ShowPreferences( wxEmptyString, wxEmptyString );
            } );
    infobar->AddCloseButton();

    infobar->ShowMessageFor( _( "The current color theme is read-only.  Create a new theme in Preferences to "
                                "enable color editing." ),
                             10000, wxICON_INFORMATION );
}


void APPEARANCE_CONTROLS::RefreshCollapsiblePanes()
{
    m_paneLayerDisplayOptions->update();
}


void APPEARANCE_CONTROLS::netGridSetVisibility( int aNetCode, bool aVisible )
{
    for( NET_GRID_ENTRY& net : m_nets )
    {
        if( net.code == aNetCode )
        {
            net.visible = aVisible;
            break;
        }
    }
}


void APPEARANCE_CONTROLS::netGridRefresh()
{
    for( int row = 0; row < m_netsGrid->rowCount(); ++row )
        updateNetRow( row );
}
