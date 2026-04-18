cmake_minimum_required(VERSION 3.10)

set(SOURCES
    CimbarReceiveTypes.h
    CimbarReceiveSession.h
    CimbarReceiveSession.cpp
)

add_library(ios_recv STATIC ${SOURCES})

target_link_libraries(ios_recv
    cimb_translator
    extractor
    fountain
    correct_static
    wirehair
    zstd
    ${OPENCV_LIBS}
)

if(NOT DEFINED DISABLE_TESTS)
    add_subdirectory(test)
endif()
