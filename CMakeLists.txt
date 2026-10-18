cmake_minimum_required(VERSION 3.24)
project(nijiexpose_native VERSION 0.1.0 LANGUAGES C CXX)
if(NOT WIN32)
    message(FATAL_ERROR "The implementation-preserving port currently targets Windows. Other platforms follow Windows parity.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
option(NIJIEXPOSE_GPU_TESTS "Register tests requiring a working desktop OpenGL context" OFF)
set(NIJIEXPOSE_NICXLIVE_SOURCE_DIR "" CACHE PATH "Optional local nicxlive checkout (otherwise use pinned source)")

include(cmake/Vendor.cmake)
include(cmake/Dependencies.cmake)
add_subdirectory(native)
