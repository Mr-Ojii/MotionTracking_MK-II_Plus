cmake_minimum_required(VERSION 3.20.0)
project(MotionTrackingMKIIPlus CXX)

add_library(${PROJECT_NAME} SHARED)

set(OPENCV_INCLUDE_DIR CACHE PATH "OpenCV include directory")
set(OPENCV_LIBRARY_DIR CACHE PATH "OpenCV library directory")


target_sources(${PROJECT_NAME} PRIVATE main.cpp main.def resource.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".auf")
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENCV_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPENCV_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 ippicvmt debug ippiwd debug ittnotifyd debug opencv_core460d debug opencv_highgui460d debug opencv_imgcodecs460d debug opencv_imgproc460d debug opencv_objdetect460d debug opencv_tracking460d debug opencv_video460d debug opencv_calib3d460d debug opencv_flann460d debug opencv_features2d460d debug zlibd optimized ippiw optimized ittnotify optimized opencv_core460 optimized opencv_highgui460 optimized opencv_imgcodecs460 optimized opencv_imgproc460 optimized opencv_objdetect460 optimized opencv_tracking460 optimized opencv_video460 optimized opencv_calib3d460 optimized opencv_flann460 optimized opencv_features2d460 optimized zlib)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX /source-charset:utf-8 /execution-charset:shift_jis)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")