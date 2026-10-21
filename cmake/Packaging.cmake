set(CPACK_PACKAGE_NAME nijiexpose)
set(CPACK_PACKAGE_VENDOR "nijigenerate Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "nijiexpose native streaming application")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY nijiexpose)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_FILE_NAME "nijiexpose-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_EXECUTABLES "nijiexpose-native;nijiexpose")
set(CPACK_CREATE_DESKTOP_LINKS nijiexpose-native)
if(WIN32)
    set(CPACK_GENERATOR "ZIP;WIX")
    set(CPACK_WIX_UPGRADE_GUID "F509F975-F15C-4E61-A436-8FD202A77BCC")
    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/build-aux/windows/icon.ico")
    set(CPACK_WIX_UI_BANNER "${PROJECT_SOURCE_DIR}/build-aux/windows/wix/res/banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${PROJECT_SOURCE_DIR}/build-aux/windows/wix/res/dialog.bmp")
    set(CPACK_WIX_LICENSE_RTF "${PROJECT_SOURCE_DIR}/build-aux/windows/wix/res/LICENSE.rtf")
    set(CPACK_WIX_UI_REF WixUI_InstallDir)
    set(CPACK_WIX_PROGRAM_MENU_FOLDER nijiexpose)
    include(InstallRequiredSystemLibraries)
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
    set(CPACK_DMG_VOLUME_NAME nijiexpose)
    set(CPACK_DMG_BACKGROUND_IMAGE "${PROJECT_SOURCE_DIR}/build-aux/osx/dmgbg.png")
    # fixup_bundle embeds Homebrew dylibs; carry their installed notices with
    # the app, including transitive FreeType/gettext dependencies.
    execute_process(COMMAND "${BREW_EXECUTABLE}" deps --installed freetype gettext sdl3
        OUTPUT_VARIABLE brew_dependencies OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    string(REPLACE "\n" ";" brew_dependencies "${brew_dependencies}")
    list(APPEND brew_dependencies freetype gettext sdl3)
    list(REMOVE_DUPLICATES brew_dependencies)
    foreach(dependency IN LISTS brew_dependencies)
        execute_process(COMMAND "${BREW_EXECUTABLE}" --prefix "${dependency}"
            OUTPUT_VARIABLE prefix OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        file(GLOB notices LIST_DIRECTORIES FALSE "${prefix}/LICENSE*" "${prefix}/COPYING*" "${prefix}/FTL.TXT"
            "${prefix}/share/doc/*/LICENSE*" "${prefix}/share/doc/*/COPYING*"
            "${prefix}/share/doc/*/FTL.TXT")
        if(NOT notices)
            message(FATAL_ERROR "No installed license notices found for ${dependency}: ${prefix}")
        endif()
        install(FILES ${notices} DESTINATION "${NIJIEXPOSE_LICENSE_DESTINATION}/homebrew/${dependency}")
    endforeach()
    install(FILES "${PROJECT_SOURCE_DIR}/native/res/licenses/LICENSE-Lua"
        "${PROJECT_SOURCE_DIR}/native/res/fonts/NotoSansCJK-Regular-LICENSE"
        "${PROJECT_SOURCE_DIR}/native/res/fonts/MaterialIcons-LICENSE"
        DESTINATION "${NIJIEXPOSE_LICENSE_DESTINATION}")
else()
    set(CPACK_GENERATOR "ZIP;TGZ")
    install(FILES "${PROJECT_SOURCE_DIR}/build-aux/linux/nijiexpose.desktop" DESTINATION share/applications)
    install(FILES "${PROJECT_SOURCE_DIR}/build-aux/linux/nijiexpose.appdata.xml" DESTINATION share/metainfo)
    install(FILES "${PROJECT_SOURCE_DIR}/build-aux/linux/icon.png" DESTINATION share/icons/hicolor/256x256/apps RENAME nijiexpose.png)
endif()
install(DIRECTORY "${PROJECT_SOURCE_DIR}/native/licenses/" DESTINATION "${NIJIEXPOSE_LICENSE_DESTINATION}/native")
# Fix paths and sign only after the tracker subdirectory has installed its dylibs.
if(APPLE)
    install(CODE [[
        include(BundleUtilities)
        get_filename_component(nijiexpose_bundle
            "${CMAKE_INSTALL_PREFIX}/nijiexpose-native.app" ABSOLUTE)
        # The tracker now contains nijitrack's static SDK. Remove the obsolete
        # shared face SDK left by earlier macOS installations.
        file(REMOVE "${nijiexpose_bundle}/Contents/Frameworks/libface_landmarker.dylib")
        fixup_bundle("${nijiexpose_bundle}" "" "${nijiexpose_bundle}/Contents/Frameworks")
    ]])
    install(CODE [[
        execute_process(
            COMMAND /usr/bin/codesign --force --deep --sign - "${nijiexpose_bundle}"
            RESULT_VARIABLE codesign_result
            OUTPUT_VARIABLE codesign_output
            ERROR_VARIABLE codesign_error)
        if(NOT codesign_result EQUAL 0)
            message(FATAL_ERROR "Could not ad-hoc sign macOS app bundle: ${codesign_output}${codesign_error}")
        endif()
    ]])
endif()

include(CPack)
