include(FetchContent)

include("${CMAKE_CURRENT_LIST_DIR}/DependencyLock.cmake")
nijiexpose_dependency(sdl3)
if(WIN32)
    FetchContent_Declare(sdl3_binary URL "${dependency_url}" URL_HASH "SHA256=${dependency_hash}" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(sdl3_binary)
    set(SDL3_SOURCE_DIR "${sdl3_binary_SOURCE_DIR}")
    find_package(SDL3 CONFIG REQUIRED PATHS "${SDL3_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)
elseif(APPLE)
    find_program(BREW_EXECUTABLE brew REQUIRED)
    execute_process(COMMAND "${BREW_EXECUTABLE}" --prefix sdl3
        OUTPUT_VARIABLE NIJIEXPOSE_SDL3_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    set(SDL3_SOURCE_DIR "${NIJIEXPOSE_SDL3_PREFIX}")
    find_package(SDL3 CONFIG REQUIRED
        PATHS "${NIJIEXPOSE_SDL3_PREFIX}/lib/cmake/SDL3" NO_DEFAULT_PATH)
else()
    find_package(SDL3 3.4 CONFIG QUIET)
    if(SDL3_FOUND)
        find_file(NIJIEXPOSE_SDL3_LICENSE NAMES copyright LICENSE.txt
            PATHS /usr/share/doc/libsdl3-0 /usr/share/doc/libsdl3-dev REQUIRED)
    endif()
    if(NOT SDL3_FOUND)
        # Ubuntu 24.04 has no SDL3 development package. Use the official source
        # release only when a system binary installation is unavailable.
        FetchContent_Declare(SDL3
            URL "https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz"
            URL_HASH "SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL3)
    endif()

endif()

set(NICXLIVE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(NICXLIVE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NICXLIVE_ENABLE_AVX2 OFF CACHE BOOL "" FORCE)
set(NICXLIVE_ENABLE_FAST_MATH OFF CACHE BOOL "" FORCE)
if(NIJIEXPOSE_NICXLIVE_SOURCE_DIR)
    if(NOT EXISTS "${NIJIEXPOSE_NICXLIVE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "NIJIEXPOSE_NICXLIVE_SOURCE_DIR must point to a nicxlive source tree")
    endif()
    add_subdirectory("${NIJIEXPOSE_NICXLIVE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/nicxlive")
    set(nicxlive_SOURCE_DIR "${NIJIEXPOSE_NICXLIVE_SOURCE_DIR}")
else()
    nijiexpose_dependency(nicxlive)
    FetchContent_Declare(nicxlive
        URL "${dependency_url}"
        URL_HASH "SHA256=${dependency_hash}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(nicxlive)
endif()

if(MSVC)
    # nicxlive sources contain UTF-8 comments; CP932 can consume source lines.
    target_compile_options(nicxlive PRIVATE /utf-8)
endif()

find_package(OpenGL REQUIRED)
