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
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt debug ippiwd debug ittnotifyd debug opencv_core480d debug opencv_highgui480d debug opencv_imgcodecs480d debug opencv_imgproc480d debug opencv_objdetect480d debug opencv_tracking480d debug opencv_video480d debug opencv_calib3d480d debug opencv_flann480d debug opencv_features2d480d debug opencv_dnn480d debug zlibd debug libprotobufd optimized ippiw optimized ittnotify optimized opencv_core480 optimized opencv_highgui480 optimized opencv_imgcodecs480 optimized opencv_imgproc480 optimized opencv_objdetect480 optimized opencv_tracking480 optimized opencv_video480 optimized opencv_calib3d480 optimized opencv_flann480 optimized opencv_features2d480 optimized opencv_dnn480 optimized zlib optimized libprotobuf)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")