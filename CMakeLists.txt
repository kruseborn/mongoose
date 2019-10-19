cmake_minimum_required(VERSION 3.12)

project(Mongoose)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake-modules/copts.cmake)
include(cmake-modules/platform.cmake)
include(cmake-modules/helper.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

fix_default_compiler_settings_()

add_subdirectory(libs)
add_subdirectory(src)