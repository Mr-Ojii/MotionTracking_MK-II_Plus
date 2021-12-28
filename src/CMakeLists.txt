cmake_minimum_required(VERSION 3.20.0)
project(MotionTrakingMKIIPlus CXX)

add_library(${PROJECT_NAME} SHARED)

set(OPENCV_INCLUDE_DIR CACHE PATH "OpenCV include directory")
set(OPENCV_LIBRARY_DIR CACHE PATH "OpenCV library directory")

target_sources(${PROJECT_NAME} PRIVATE main.cpp main.def resource.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".auf")
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENCV_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPENCV_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt debug ippiwd debug ittnotifyd debug opencv_core3416d debug opencv_highgui3416d debug opencv_imgcodecs3416d debug opencv_imgproc3416d debug opencv_objdetect3416d debug opencv_tracking3416d debug opencv_video3416d debug zlibd optimized ippiw optimized ittnotify optimized opencv_core3416 optimized opencv_highgui3416 optimized opencv_imgcodecs3416 optimized opencv_imgproc3416 optimized opencv_objdetect3416 optimized opencv_tracking3416 optimized opencv_video3416 optimized zlib)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")