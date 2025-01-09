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
                        debug     ippiwd debug     ittnotifyd debug     opencv_core4110d debug     opencv_highgui4110d debug     opencv_imgcodecs4110d debug     opencv_imgproc4110d debug     opencv_objdetect4110d debug     opencv_tracking4110d debug     opencv_video4110d debug     opencv_calib3d4110d debug     opencv_flann4110d debug     opencv_features2d4110d debug     opencv_dnn4110d debug     zlibd debug     libprotobufd
                        optimized ippiw  optimized ittnotify  optimized opencv_core4110  optimized opencv_highgui4110  optimized opencv_imgcodecs4110  optimized opencv_imgproc4110  optimized opencv_objdetect4110  optimized opencv_tracking4110  optimized opencv_video4110  optimized opencv_calib3d4110  optimized opencv_flann4110  optimized opencv_features2d4110  optimized opencv_dnn4110  optimized zlib  optimized libprotobuf
                        )

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:${MSVC_ARCH} /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")