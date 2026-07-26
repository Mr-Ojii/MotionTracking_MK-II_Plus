cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MotionTrackingMKIIPlus CXX)

add_library(${PROJECT_NAME} SHARED)

set(OPENCV_INCLUDE_DIR CACHE PATH "OpenCV include directory")
set(OPENCV_LIBRARY_DIR CACHE PATH "OpenCV library directory")
if(NOT MSVC_ARCH)
    set(MSVC_ARCH "AVX")
endif()

message("-- OPTIONS --")
message("-- MSVC_ARCH: ${MSVC_ARCH}")
message("-------------")

target_sources(${PROJECT_NAME} PRIVATE main.cpp main.def bgs.cpp hsv.cpp resource.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".auf")
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENCV_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPENCV_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt
                        debug     ippiwd debug     ipphald debug     ittnotifyd debug     opencv_core500d debug     opencv_highgui500d debug     opencv_imgcodecs500d debug     opencv_imgproc500d debug     opencv_xobjdetect500d debug     opencv_tracking500d debug     opencv_video500d debug     opencv_calib500d debug     opencv_flann500d debug     opencv_features500d debug     opencv_dnn500d debug     opencv_geometry500d debug     zlibd debug     libprotobufd
                        optimized ippiw  optimized ipphal  optimized ittnotify  optimized opencv_core500  optimized opencv_highgui500  optimized opencv_imgcodecs500  optimized opencv_imgproc500  optimized opencv_xobjdetect500  optimized opencv_tracking500  optimized opencv_video500  optimized opencv_calib500  optimized opencv_flann500  optimized opencv_features500  optimized opencv_dnn500  optimized opencv_geometry500  optimized zlib  optimized libprotobuf
                        )

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_REV
    ERROR_QUIET)
string(REPLACE "\n" "" GIT_REV "${GIT_REV}")
set(GIT_REV "#define GIT_REV \"${GIT_REV}\"")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/config.h "${GIT_REV}")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:${MSVC_ARCH} /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
