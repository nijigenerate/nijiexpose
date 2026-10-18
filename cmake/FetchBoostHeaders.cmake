# CI helper: download the unmodified upstream header distribution.
cmake_minimum_required(VERSION 3.24)
if(NOT DEFINED BOOST_DESTINATION)
    message(FATAL_ERROR "Pass -DBOOST_DESTINATION=<directory>")
endif()
file(MAKE_DIRECTORY "${BOOST_DESTINATION}")
file(DOWNLOAD
    https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.gz
    "${BOOST_DESTINATION}/boost.tar.gz"
    EXPECTED_HASH SHA256=5e93d582aff26868d581a52ae78c7d8edf3f3064742c6e77901a1f18a437eea9
    TLS_VERIFY ON)
file(ARCHIVE_EXTRACT INPUT "${BOOST_DESTINATION}/boost.tar.gz" DESTINATION "${BOOST_DESTINATION}")
