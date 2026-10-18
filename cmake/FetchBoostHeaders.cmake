# CI helper: download the unmodified upstream header distribution.
cmake_minimum_required(VERSION 3.24)
if(NOT DEFINED BOOST_DESTINATION)
    message(FATAL_ERROR "Pass -DBOOST_DESTINATION=<directory>")
endif()
file(MAKE_DIRECTORY "${BOOST_DESTINATION}")
include("${CMAKE_CURRENT_LIST_DIR}/DependencyLock.cmake")
nijiexpose_dependency(boost)
file(DOWNLOAD
    "${dependency_url}"
    "${BOOST_DESTINATION}/boost.tar.gz"
    EXPECTED_HASH "SHA256=${dependency_hash}"
    TLS_VERIFY ON)
file(ARCHIVE_EXTRACT INPUT "${BOOST_DESTINATION}/boost.tar.gz" DESTINATION "${BOOST_DESTINATION}")
file(READ "${CMAKE_CURRENT_LIST_DIR}/dependencies.lock.json" lock)
string(JSON boost_directory GET "${lock}" boost directory)
file(WRITE "${BOOST_DESTINATION}/prefix.txt" "${BOOST_DESTINATION}/${boost_directory}")
