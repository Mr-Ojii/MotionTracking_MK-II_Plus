cmake_minimum_required(VERSION 4.0)
project(aviutl2_sdk_mirror)

include_directories(./include/aviutl2_sdk)

file(GLOB EXAMPLES ./examples/*.cpp)
foreach(example_file ${EXAMPLES})
    get_filename_component(example_file ${example_file} NAME_WE)
    add_library(${example_file} SHARED ./examples/${example_file}.cpp)
    target_compile_features(${example_file} PRIVATE cxx_std_17)
    target_compile_options(${example_file} PRIVATE /FIstdint.h /utf-8)
    target_compile_definitions(${example_file} PRIVATE UNICODE _UNICODE)
endforeach()
