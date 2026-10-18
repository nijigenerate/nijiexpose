include(FetchContent)

include("${CMAKE_CURRENT_LIST_DIR}/DependencyLock.cmake")
nijiexpose_dependency(sdl3)
FetchContent_Declare(sdl3_binary URL "${dependency_url}" URL_HASH "SHA256=${dependency_hash}" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(sdl3_binary)
set(SDL3_SOURCE_DIR "${sdl3_binary_SOURCE_DIR}")
find_package(SDL3 CONFIG REQUIRED PATHS "${SDL3_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)

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
