find_program(NIJIEXPOSE_MSGFMT NAMES msgfmt HINTS "${NIJIEXPOSE_GETTEXT_PREFIX}/bin")
set(NIJIEXPOSE_TRANSLATIONS_DIR "${PROJECT_SOURCE_DIR}/out/i18n" CACHE PATH "Precompiled catalogs when host msgfmt is unavailable")
file(GLOB catalogs CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tl/*.po")
if(NOT catalogs)
    message(FATAL_ERROR "No translation catalogs in tl/")
endif()
set(compiled_catalogs)
foreach(catalog IN LISTS catalogs)
    get_filename_component(language "${catalog}" NAME_WE)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/i18n/${language}.mo")
    if(NIJIEXPOSE_MSGFMT)
        add_custom_command(OUTPUT "${output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/i18n"
            COMMAND "${NIJIEXPOSE_MSGFMT}" --check --check-format "${catalog}" -o "${output}"
            DEPENDS "${catalog}" VERBATIM)
    else()
        set(precompiled "${NIJIEXPOSE_TRANSLATIONS_DIR}/${language}.mo")
        if(NOT EXISTS "${precompiled}")
            message(FATAL_ERROR "Install GNU gettext (msgfmt), or supply compiled catalogs with NIJIEXPOSE_TRANSLATIONS_DIR: ${precompiled}")
        endif()
        add_custom_command(OUTPUT "${output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/i18n"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${precompiled}" "${output}"
            DEPENDS "${precompiled}" VERBATIM)
    endif()
    list(APPEND compiled_catalogs "${output}")
endforeach()
set(runtime_catalogs "$<TARGET_FILE_DIR:nijiexpose_native>/i18n")
if(APPLE)
    set(runtime_catalogs "$<TARGET_FILE_DIR:nijiexpose_native>/../Resources/i18n")
endif()
add_custom_target(nijiexpose_translations ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${runtime_catalogs}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${compiled_catalogs} "${runtime_catalogs}"
    DEPENDS ${compiled_catalogs} VERBATIM)
add_dependencies(nijiexpose_native nijiexpose_translations)
if(APPLE)
    install(FILES ${compiled_catalogs} DESTINATION nijiexpose.app/Contents/Resources/i18n)
else()
    install(FILES ${compiled_catalogs} DESTINATION bin/i18n)
endif()
foreach(action extract update)
    add_custom_target(translations-${action}
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/native/tools/translations.py" ${action}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" VERBATIM)
endforeach()
