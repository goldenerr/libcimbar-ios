cmake_minimum_required(VERSION 3.10)

set(SOURCES
    test.cpp
    CimbarReceiveSessionTest.cpp
)

include_directories(
    ${libcimbar_SOURCE_DIR}/test
    ${libcimbar_SOURCE_DIR}/test/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(ios_recv_test ${SOURCES})
add_test(ios_recv_test ios_recv_test)

target_link_libraries(ios_recv_test
    ios_recv
    cimb_translator
    extractor
    fountain
    correct_static
    wirehair
    zstd
    ${OPENCV_LIBS}
)
