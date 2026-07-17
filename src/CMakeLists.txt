cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_BUILD_TYPE Release)
project(MotionTrackingMKIIPlusforAviUtl2 CXX)

add_library(${PROJECT_NAME} SHARED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

set(OPENCV_INCLUDE_DIR "/usr/x86_64-w64-mingw32/include/opencv4")
set(OPENCV_LIBRARY_DIR "/usr/x86_64-w64-mingw32/lib")

target_sources(
    ${PROJECT_NAME}
    PRIVATE
    tracker/tracker.cpp
    ui/progress_dlg/progress_dlg.cpp
    ui/mainframe.cpp
    ui/create_frame.cpp
    ui/ownerdraw.cpp
    insert/insert_object.cpp
    filter.cpp
    main.cpp
    utils.cpp
    )

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aux2")
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENCV_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/aviutl2_sdk/include
)
target_link_directories(${PROJECT_NAME} PRIVATE
    ${OPENCV_LIBRARY_DIR}
    ${OPENCV_LIBRARY_DIR}/opencv4/3rdparty
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32
    comdlg32
    shell32
    opencv_tracking4120
    opencv_objdetect4120
    opencv_video4120
    opencv_dnn4120
    opencv_highgui4120
    opencv_imgcodecs4120
    opencv_calib3d4120
    opencv_features2d4120
    opencv_flann4120
    opencv_imgproc4120
    opencv_core4120
    libprotobuf
    zlib
    pthread
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
)

execute_process(COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE GIT_REV
    ERROR_QUIET)
string(REPLACE "\n" "" GIT_REV "${GIT_REV}")
set(GIT_REV "#define GIT_REV \"${GIT_REV}\"")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/config.h "${GIT_REV}")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
