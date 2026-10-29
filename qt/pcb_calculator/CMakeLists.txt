# The GUI of the calculator tools on Qt: the sources of the pcb_calculator KIFACE of the
# wxWidgets build, built on the wxWidgets compatibility layer (qt/wxcompat).
set( KIQT_PCB_CALCULATOR_GUI_SRCS
    common_data.cpp
    eseries.cpp
    params_read_write.cpp
    pcb_calculator_frame.cpp
    pcb_calculator_settings.cpp
    datafile_read_write.cpp
    calculator_panels/panel_rf_attenuators.cpp
    calculator_panels/panel_board_class.cpp
    calculator_panels/panel_cable_size.cpp
    calculator_panels/panel_color_code.cpp
    calculator_panels/panel_eseries_display.cpp
    calculator_panels/panel_galvanic_corrosion.cpp
    calculator_panels/panel_electrical_spacing.cpp
    calculator_panels/iec60664.cpp
    calculator_panels/panel_electrical_spacing_iec60664.cpp
    calculator_panels/panel_electrical_spacing_ipc2221.cpp
    calculator_panels/panel_r_calculator.cpp
    calculator_panels/panel_fusing_current.cpp
    calculator_panels/panel_regulator.cpp
    calculator_panels/panel_track_width.cpp
    calculator_panels/panel_transline.cpp
    calculator_panels/panel_via_size.cpp
    calculator_panels/panel_wavelength.cpp
    transline_ident.cpp
    widgets/unit_selector.cpp
    transline/transline.cpp
    transline/c_microstrip.cpp
    transline/microstrip.cpp
    transline/coplanar.cpp
    transline/coax.cpp
    transline/rectwaveguide.cpp
    transline/stripline.cpp
    transline/c_stripline.cpp
    transline/twistedpair.cpp
    transline_dlg_funct.cpp
    attenuators/attenuator_classes.cpp
    dialogs/dialog_regulator_form.cpp
    pcb_calculator_utils.cpp
    resistor_substitution_utils.cpp
    )

list( TRANSFORM KIQT_PCB_CALCULATOR_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/pcb_calculator/ )

# The calculations of the transmission lines (in the common library of the wxWidgets build)
list( APPEND KIQT_PCB_CALCULATOR_GUI_SRCS
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/transline_calculation_base.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/microstrip.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/coupled_microstrip.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/stripline.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/coupled_stripline.cpp
    )

kiqt_generate_fbp_compat( KIQT_PCB_CALCULATOR_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_board_class_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_cable_size_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_color_code_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_electrical_spacing_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_electrical_spacing_iec60664_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_electrical_spacing_ipc2221_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_eseries_display_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_fusing_current_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_galvanic_corrosion_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_r_calculator_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_regulator_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_rf_attenuators_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_track_width_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_transline_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_via_size_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels/panel_wavelength_base.fbp
    ${CMAKE_SOURCE_DIR}/pcb_calculator/dialogs/dialog_regulator_form_base.fbp
    )

add_library( pcb_calculator_gui_qt OBJECT
    ${KIQT_PCB_CALCULATOR_GUI_SRCS}
    ${KIQT_PCB_CALCULATOR_COMPAT_FBP_SRCS}
    )

set_target_properties( pcb_calculator_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

target_compile_definitions( pcb_calculator_gui_qt PRIVATE QT_NO_KEYWORDS PCB_CALCULATOR_BUILD )
kiqt_gui_target( pcb_calculator_gui_qt )

# Compiled as the pcb_calculator KIFACE (its include directories), the bases of the panels
# generated from the wxFormBuilder files first
target_include_directories( pcb_calculator_gui_qt BEFORE PRIVATE
    ${KIQT_FBP_DIR}/pcb_calculator/calculator_panels
    ${KIQT_FBP_DIR}/pcb_calculator/dialogs
    )
target_include_directories( pcb_calculator_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/pcb_calculator
    ${CMAKE_SOURCE_DIR}/pcb_calculator/calculator_panels
    ${CMAKE_SOURCE_DIR}/pcb_calculator/dialogs
    )

target_link_libraries( pcb_calculator_gui_qt PRIVATE kiqt_common nlohmann_json Boost::headers )


# _pcb_calculator_qtgui.kiface: the KIFACE of the calculator tools with their GUI on Qt, as
# _pcb_calculator.kiface of the wxWidgets build.
add_headless_kiface( pcb_calculator_qtgui_kiface
    OUTPUT_NAME pcb_calculator_qtgui
    SOURCES     ${CMAKE_SOURCE_DIR}/pcb_calculator/pcb_calculator.cpp
                ${KIQT_KIFACE_SOURCES}
    OBJECTS     pcb_calculator_gui_qt
    USAGE       kiqt_common
    ARCHIVES    common kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets kicommon_model gal_model core Boost::headers
                markdown_lib ${wxWidgets_BASE_LIBRARIES}
    )

target_compile_definitions( pcb_calculator_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS PCB_CALCULATOR_BUILD )
target_include_directories( pcb_calculator_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:pcb_calculator_gui_qt,INCLUDE_DIRECTORIES>
    )

# Where KIWAY_BASE finds the KIFACE when run from the build tree
set_target_properties( pcb_calculator_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pcb_calculator
    )

install( TARGETS pcb_calculator_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )

# The lexer of the data files of the calculator tools, as in the wxWidgets build
make_lexer(
    pcb_calculator_gui_qt
    ../../pcb_calculator/pcb_calculator_datafile.keywords
    pcb_calculator_datafile_lexer.h
    pcb_calculator_datafile_keywords.cpp
    PCBCALC_DATA_T
   )
