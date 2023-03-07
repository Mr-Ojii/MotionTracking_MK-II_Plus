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
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt debug ippiwd debug ittnotifyd debug opencv_core470d debug opencv_highgui470d debug opencv_imgcodecs470d debug opencv_imgproc470d debug opencv_objdetect470d debug opencv_tracking470d debug opencv_video470d debug opencv_calib3d470d debug opencv_flann470d debug opencv_features2d470d debug opencv_dnn470d debug zlibd debug libprotobufd optimized ippiw optimized ittnotify optimized opencv_core470 optimized opencv_highgui470 optimized opencv_imgcodecs470 optimized opencv_imgproc470 optimized opencv_objdetect470 optimized opencv_tracking470 optimized opencv_video470 optimized opencv_calib3d470 optimized opencv_flann470 optimized opencv_features2d470 optimized opencv_dnn470 optimized zlib optimized libprotobuf)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")