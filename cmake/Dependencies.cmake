include(FetchContent)

# Source archives are pinned by commit AND SHA256. No Git history is fetched.
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540
    URL_HASH SHA256=7a8347c770b90b33daac2352858ca03f9c9a2ccc8ce711054870361d1a6b32e5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(SDL3)

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
    FetchContent_Declare(nicxlive
        URL https://codeload.github.com/nijigenerate/nicxlive/tar.gz/6ff544470b0f851ab998d8a12220aa750dac0d6b
        URL_HASH SHA256=685fd02d84c3e79b7e494cff36b173df37f159eed9eaed6c0c7f6e88e6676563
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(nicxlive)
endif()

if(MSVC)
    # nicxlive sources contain UTF-8 comments; CP932 can consume source lines.
    target_compile_options(nicxlive PRIVATE /utf-8)
endif()

find_package(OpenGL REQUIRED)
