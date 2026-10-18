set(NIJIEXPOSE_GETTEXT_PREFIX "${PROJECT_SOURCE_DIR}/out/vcpkg/x64-windows-native" CACHE PATH
    "GNU gettext/libiconv installation prefix")
list(PREPEND CMAKE_PREFIX_PATH "${NIJIEXPOSE_GETTEXT_PREFIX}")
find_package(Intl REQUIRED)
if(NOT TARGET Intl::Intl)
    add_library(Intl::Intl INTERFACE IMPORTED)
    set_target_properties(Intl::Intl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Intl_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${Intl_LIBRARIES}")
endif()
file(GLOB NIJIEXPOSE_GETTEXT_DLLS "${NIJIEXPOSE_GETTEXT_PREFIX}/bin/*.dll")
if(WIN32 AND NOT NIJIEXPOSE_GETTEXT_DLLS)
    message(FATAL_ERROR "GNU gettext DLLs are missing. Build cmake/vcpkg dependencies or set NIJIEXPOSE_GETTEXT_PREFIX.")
endif()
