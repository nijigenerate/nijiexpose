nijiexpose_dependency(gettext)
FetchContent_Declare(gettext_binary URL "${dependency_url}" URL_HASH "SHA256=${dependency_hash}" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(gettext_binary)
set(NIJIEXPOSE_GETTEXT_PREFIX "${gettext_binary_SOURCE_DIR}")
add_library(Intl::Intl SHARED IMPORTED GLOBAL)
set_target_properties(Intl::Intl PROPERTIES
    IMPORTED_IMPLIB "${NIJIEXPOSE_GETTEXT_PREFIX}/lib/intl.dll.lib"
    IMPORTED_LOCATION "${NIJIEXPOSE_GETTEXT_PREFIX}/bin/intl-8.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${NIJIEXPOSE_GETTEXT_PREFIX}/include")
set(NIJIEXPOSE_GETTEXT_DLLS
    "${NIJIEXPOSE_GETTEXT_PREFIX}/bin/intl-8.dll"
    "${NIJIEXPOSE_GETTEXT_PREFIX}/bin/iconv-2.dll"
    "${NIJIEXPOSE_GETTEXT_PREFIX}/bin/charset-1.dll")
