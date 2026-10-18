if(NOT DEFINED DESTINATION)
    message(FATAL_ERROR "Set DESTINATION for the MediaPipe portrait fixture")
endif()
get_filename_component(directory "${DESTINATION}" DIRECTORY)
file(MAKE_DIRECTORY "${directory}")
file(DOWNLOAD "https://storage.googleapis.com/mediapipe-assets/portrait.jpg" "${DESTINATION}"
    EXPECTED_HASH SHA256=a6f11efaa834706db23f275b6115058fa87fc7f14362681e6abe14e82749de3e
    TLS_VERIFY ON)
