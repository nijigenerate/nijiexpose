find_package(Git REQUIRED)

function(nijiexpose_git_version source)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" rev-parse --short=12 HEAD
        OUTPUT_VARIABLE revision OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" describe --tags --exact-match
        OUTPUT_VARIABLE tag OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE exact)
    if(exact EQUAL 0)
        set(version "${tag}")
    else()
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" describe --tags --abbrev=0
            OUTPUT_VARIABLE tag OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE tagged)
        if(tagged EQUAL 0)
            set(version "${tag}-g${revision}")
        else()
            set(version "${revision}")
        endif()
    endif()

    # CMake, Windows VERSIONINFO/MSI and Apple's bundle build version require
    # numbers. The application and distribution filenames use the full Git ID.
    set(numeric "0.0.0")
    if(tag MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)($|[-+])")
        set(numeric "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
    endif()
    string(REGEX REPLACE "[^A-Za-z0-9._+-]" "_" package_version "${version}")
    set(NIJIEXPOSE_VERSION "${version}" PARENT_SCOPE)
    set(NIJIEXPOSE_NUMERIC_VERSION "${numeric}" PARENT_SCOPE)
    set(NIJIEXPOSE_PACKAGE_VERSION "${package_version}" PARENT_SCOPE)

    # Reconfigure an existing build after commits, checkouts or tag changes,
    # including packed refs and linked Git worktrees. Do not cache the version.
    if(NOT CMAKE_SCRIPT_MODE_FILE)
        foreach(name HEAD packed-refs)
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" rev-parse --git-path "${name}"
                OUTPUT_VARIABLE path OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
            get_filename_component(path "${path}" ABSOLUTE BASE_DIR "${source}")
            if(EXISTS "${path}")
                set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
            endif()
        endforeach()
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" rev-parse --git-common-dir
            OUTPUT_VARIABLE common OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
        get_filename_component(common "${common}" ABSOLUTE BASE_DIR "${source}")
        file(GLOB_RECURSE refs CONFIGURE_DEPENDS "${common}/refs/heads/*" "${common}/refs/tags/*")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${refs})
        # Also watch creation/removal of packed-refs itself.
        file(GLOB packed CONFIGURE_DEPENDS "${common}/packed-refs")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${packed})
    endif()
endfunction()
