# The project manager of KiCad (KICAD_MANAGER_FRAME, its project tree, tools, dialogs and the
# plugin and content manager) on Qt: the sources of the kicad program of the wxWidgets build,
# built on the wxWidgets compatibility layer (qt/wxcompat).

set( KIQT_MANAGER_SRCS
    dialogs/dialog_destination.cpp
    dialogs/dialog_copyfiles_job_settings.cpp
    dialogs/dialog_executecommand_job_settings.cpp
    dialogs/dialog_update_notice.cpp
    dialogs/dialog_template_selector.cpp
    dialogs/dialog_edit_cfg.cpp
    dialogs/panel_kicad_launcher.cpp
    dialogs/panel_jobset.cpp
    files-io.cpp
    jobs_runner.cpp
    import_proj.cpp
    import_project.cpp
    kicad_manager_frame.cpp
    local_history_pane.cpp
    menubar.cpp
    project_template.cpp
    project_tree_pane.cpp
    project_tree.cpp
    project_tree_item.cpp
    project_tree_traverser.cpp
    update_manager.cpp
    widgets/panel_recent_projects.cpp
    widgets/home_widgets.cpp
    toolbars_kicad_manager.cpp
    tools/kicad_manager_actions.cpp
    tools/kicad_manager_control.cpp
    pcm/dialogs/dialog_manage_repositories.cpp
    pcm/dialogs/dialog_pcm.cpp
    pcm/dialogs/dialog_pcm_progress.cpp
    pcm/dialogs/panel_package.cpp
    pcm/dialogs/panel_packages_view.cpp
    pcm/pcm.cpp
    pcm/pcm_data.cpp
    pcm/pcm_task_manager.cpp
    )

list( TRANSFORM KIQT_MANAGER_SRCS PREPEND ${CMAKE_SOURCE_DIR}/kicad/ )

kiqt_generate_fbp_compat( KIQT_MANAGER_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/dialog_copyfiles_job_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/dialog_destination_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/dialog_executecommand_job_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/dialog_template_selector_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/dialog_update_notice_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/panel_jobset_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/dialogs/panel_kicad_launcher_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs/dialog_manage_repositories_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs/dialog_pcm_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs/dialog_pcm_progress_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs/panel_package_base.fbp
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs/panel_packages_view_base.fbp
    )

add_library( kicad_manager_qt OBJECT
    ${KIQT_MANAGER_SRCS}
    ${KIQT_MANAGER_COMPAT_FBP_SRCS}
    )

target_compile_definitions( kicad_manager_qt PRIVATE QT_NO_KEYWORDS KICAD PCM )
kiqt_gui_target( kicad_manager_qt )

# Compiled as the kicad program (its include directories), the bases of the dialogs generated
# from the wxFormBuilder files first
target_include_directories( kicad_manager_qt BEFORE PRIVATE
    ${KIQT_FBP_DIR}/kicad
    ${KIQT_FBP_DIR}/kicad/dialogs
    ${KIQT_FBP_DIR}/kicad/pcm/dialogs
    )
set( KIQT_MANAGER_INCLUDE_DIRS
    ${KIQT_FBP_DIR}/kicad
    ${KIQT_FBP_DIR}/kicad/dialogs
    ${KIQT_FBP_DIR}/kicad/pcm/dialogs
    ${CMAKE_SOURCE_DIR}/kicad
    ${CMAKE_SOURCE_DIR}/kicad/dialogs
    ${CMAKE_SOURCE_DIR}/kicad/pcm
    ${CMAKE_SOURCE_DIR}/kicad/pcm/dialogs
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/eeschema
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    CACHE INTERNAL "The include directories of the project manager (kicad-qt and its test)"
    )

target_include_directories( kicad_manager_qt PRIVATE ${KIQT_MANAGER_INCLUDE_DIRS} )

target_link_libraries( kicad_manager_qt PRIVATE kiqt_common nlohmann_json Boost::headers )
