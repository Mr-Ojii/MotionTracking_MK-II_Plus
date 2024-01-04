cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MotionTrackingMKIIPlus CXX)

add_library(${PROJECT_NAME} SHARED)

set(OPENCV_INCLUDE_DIR CACHE PATH "OpenCV include directory")
set(OPENCV_LIBRARY_DIR CACHE PATH "OpenCV library directory")


target_sources(${PROJECT_NAME} PRIVATE main.cpp main.def resource.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".auf")
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENCV_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPENCV_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt debug ippiwd debug ittnotifyd debug opencv_core490d debug opencv_highgui490d debug opencv_imgcodecs490d debug opencv_imgproc490d debug opencv_objdetect490d debug opencv_tracking490d debug opencv_video490d debug opencv_calib3d490d debug opencv_flann490d debug opencv_features2d490d debug opencv_dnn490d debug zlibd debug libprotobufd optimized ippiw optimized ittnotify optimized opencv_core490 optimized opencv_highgui490 optimized opencv_imgcodecs490 optimized opencv_imgproc490 optimized opencv_objdetect490 optimized opencv_tracking490 optimized opencv_video490 optimized opencv_calib3d490 optimized opencv_flann490 optimized opencv_features2d490 optimized opencv_dnn490 optimized zlib optimized libprotobuf)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")