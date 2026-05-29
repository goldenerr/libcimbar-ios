cmake_minimum_required(VERSION 3.10)
project(cimbar_headless_encode)

set(SOURCES encode_headless.cpp)

add_executable(cimbar_headless_encode ${SOURCES})

target_link_libraries(cimbar_headless_encode
    cimb_translator
    extractor
    correct_static
    wirehair
    zstd
    ${OPENCV_LIBS}
)

target_include_directories(cimbar_headless_encode PRIVATE
    ${libcimbar_SOURCE_DIR}/src/lib
    ${libcimbar_SOURCE_DIR}/src/third_party_lib
)
