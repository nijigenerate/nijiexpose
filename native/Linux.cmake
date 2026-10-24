# Keep the development and installed resource layouts identical.
set(linux_resources "${CMAKE_BINARY_DIR}/share/nijiexpose")
file(MAKE_DIRECTORY "${linux_resources}/ui" "${linux_resources}/fonts")
foreach(resource ui-info.png ui-warning.png ui-error.png)
    configure_file("res/ui/${resource}" "${linux_resources}/ui/${resource}" COPYONLY)
endforeach()
configure_file("${PROJECT_SOURCE_DIR}/native/res/tex/logo.png" "${linux_resources}/ui/logo.png" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/native/res/icon_x256.png" "${linux_resources}/ui/icon_x256.png" COPYONLY)
foreach(font NotoSansCJK-Regular.ttc MaterialIcons.ttf)
    configure_file("res/fonts/${font}" "${linux_resources}/fonts/${font}" COPYONLY)
endforeach()
install(DIRECTORY "${linux_resources}/" DESTINATION share/nijiexpose)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/NotoSansCJK-Regular-LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/MaterialIcons-LICENSE"
    "${PROJECT_SOURCE_DIR}/native/res/licenses/LICENSE-Lua" DESTINATION ${NIJIEXPOSE_LICENSE_DESTINATION})
if(BUILD_TESTING)
    if(NIJIEXPOSE_RENDERER STREQUAL "OPENGL")
        add_executable(nijiexpose_scene_item_test tests/scene_item_test.cpp)
        target_link_libraries(nijiexpose_scene_item_test PRIVATE nijiexpose_scene)
        add_test(NAME native_scene_item COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_model_io.py" "$<TARGET_FILE:nijiexpose_scene_item_test>")
        set_tests_properties(native_scene_item PROPERTIES TIMEOUT 30 LABELS gpu)
    endif()
    add_executable(nijiexpose_file_dialog_test tests/file_dialog_test.cpp)
    target_link_libraries(nijiexpose_file_dialog_test PRIVATE nijiexpose_file_dialog)
    target_link_options(nijiexpose_file_dialog_test PRIVATE "LINKER:--wrap=SDL_ShowFileDialogWithProperties")
    add_test(NAME native_file_dialog COMMAND nijiexpose_file_dialog_test)
    set_tests_properties(native_file_dialog PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    add_executable(nijiexpose_linux_path_test tests/linux_path_test.cpp)
    target_link_libraries(nijiexpose_linux_path_test PRIVATE nijiexpose_port)
    add_test(NAME native_linux_paths COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/linux_paths.py" "$<TARGET_FILE:nijiexpose_linux_path_test>")
    foreach(pair "animation_oracle|nijiexpose_animation" "noise_oracle|nijiexpose_tracking_input"
        "binding_oracle|nijiexpose_tracking_bindings" "control_oracle|nijiexpose_animation_controls"
        "vspace_state_test|nijiexpose_vspace")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 name)
        list(GET fields 1 library)
        add_executable(nijiexpose_${name} tests/${name}.cpp)
        target_link_libraries(nijiexpose_${name} PRIVATE ${library})
    endforeach()
    if(NIJIEXPOSE_RENDERER STREQUAL "OPENGL")
        add_executable(nijiexpose_application_test tests/application_test.cpp tests/presented_frame.cpp)
        target_link_libraries(nijiexpose_application_test PRIVATE nijiexpose_application)
        target_compile_definitions(nijiexpose_application_test PRIVATE NIJIEXPOSE_CAPTURE_PRESENTED_FRAME)
        target_link_options(nijiexpose_application_test PRIVATE "LINKER:--wrap=SDL_GL_SwapWindow")
    endif()
    foreach(pair "animation|animation" "noise|noise" "control|control")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 name)
        list(GET fields 1 reference)
        add_test(NAME native_${name}_d_parity COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_output.py"
            --cpp "$<TARGET_FILE:nijiexpose_${name}_oracle>"
            --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/${reference}_reference.txt")
    endforeach()
    add_test(NAME native_tracking_binding_d_parity COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_binding.py"
        --cpp "$<TARGET_FILE:nijiexpose_binding_oracle>"
        --reference "${CMAKE_CURRENT_SOURCE_DIR}/tests/binding_reference.txt")
    add_test(NAME native_vspace_settings COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_with_config.py" "$<TARGET_FILE:nijiexpose_vspace_state_test>")
    if(NIJIEXPOSE_GPU_TESTS AND NIJIEXPOSE_RENDERER STREQUAL "OPENGL")
        add_test(NAME native_application COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_application.py" "$<TARGET_FILE:nijiexpose_application_test>" "${CMAKE_BINARY_DIR}/application-results")
        set_tests_properties(native_application PROPERTIES TIMEOUT 120 LABELS "gpu;application"
            ENVIRONMENT "SDL_VIDEODRIVER=x11")
    endif()
endif()
