cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
cmake_policy(VERSION 3.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#Uncomment to skip creating ZERO_CHECK project
#set(CMAKE_SUPPRESS_REGENERATION true)
set(NAME VulkanEngine)

project(${NAME})

OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)
OPTION(USE_WAYLAND_WSI "Build the project using Wayland swapchain" OFF)
OPTION(USE_XCB_WSI "Build the project using XCB swapchain" ON)

#unset(CMAKE_CXX_FLAGS)

find_package(Vulkan REQUIRED)
find_package(assimp REQUIRED)
find_package(Threads REQUIRED)
IF(NOT assimp_FOUND)
  MESSAGE("System assimp not found")
  find_library(assimp_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/engine/libs/)
  IF(assimp_LIBRARIES)
    set(assimp_FOUND ON)
    MESSAGE("Using bundled assimp library version")
  ELSE()
    message(FATAL_ERROR "Could not find assimp library!")
  ENDIF()
ENDIF()
IF(WIN32)
  find_library(GLFW_LIBRARIES NAMES glfw3 glfw3.lib PATHS ${CMAKE_SOURCE_DIR}/engine/libs/)
  IF(GLFW_LIBRARIES)
    set(Glfw_FOUND ON)
    MESSAGE("Using bundled GLFW library version")
  ENDIF()
ELSE(WIN32)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLFW REQUIRED glfw3)
ENDIF(WIN32)
IF(NOT Glfw_FOUND)
  message(FATAL_ERROR "GLFW not found")
ENDIF ()
IF (NOT Vulkan_FOUND)
  find_library(Vulkan_LIBRARIES NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/engine/libs/vulkan" REQUIRED)
  IF (Vulkan_LIBRARY)
    set(Vulkan_FOUND ON)
    MESSAGE("Using bundled Vulkan library version")
  ELSE()
    message(FATAL_ERROR "Could not find Vulkan library!")
  ENDIF()
  message(STATUS ${Vulkan_LIBRARY})
ENDIF()

IF(NOT WIN32)
  IF(USE_D2D_WSI)
    MESSAGE("Using direct to display extension...")
    add_definitions(-D_DIRECT2DISPLAY)
  ELSEIF(USE_WAYLAND_WSI)
    find_package(Wayland REQUIRED)
    IF (NOT WAYLAND_FOUND)
      message(FATAL_ERROR "Wayland development package not found")
    ENDIF ()
    add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    include_directories(${WAYLAND_INCLUDE_DIR})
    link_libraries(${WAYLAND_LIBRARIES})
  ELSE(USE_D2D_WSI)
    find_package(XCB REQUIRED)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    link_libraries(${XCB_LIBRARIES})
  ENDIF(USE_D2D_WSI)
ENDIF(NOT WIN32)

find_package(Sanitizers)

link_libraries(
  ${assimp_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  )

# Clang specific stuff
IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_definitions(-Wno-switch-enum)
ENDIF()

# Debug flags
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	add_definitions(-W3 -DVC_EXTRALEAN -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_UNICODE)
	remove_definitions(-DWIN32 -D_WINDOWS)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	# Update if necessary
	add_definitions(-Wextra -Wundef -Wall -Wno-long-long -Wpedantic)
	add_definitions(-D__STDC_LIB_EXT1__ -D__STDC_WANT_LIB_EXT1__)
	# Hopefully replaced by the sanitizers above
	#add_definitions(-g3 -O0 -Wextra -fsanitize=undefined
		#-fno-sanitize-recover -fsanitize=shift
		#-fsanitize=integer-divide-by-zero -fsanitize=unreachable
		#-fsanitize=vla-bound -fsanitize=null -fsanitize=return
		#-fsanitize=signed-integer-overflow -fsanitize=bounds
		#-fsanitize=alignment -fsanitize=object-size
		#-fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
		#-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute
		#-fsanitize=bool -fsanitize=enum -fsanitize=vptr)
endif()

add_definitions(
	-std=c++17 
	)

include_directories(
  engine/include/
  ${assimp_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  )

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/engine/")

set(SHADER_DIR "engine/shaders/")
file(GLOB SHADERS "${SHADER_DIR}/*.glsl")
file(GLOB sources "engine/*.cpp" "engine/*.h")
set(MAIN_CPP "engine/main.cpp")
add_executable(${NAME} ${sources})
add_sanitizers(${NAME})
install(TARGETS ${NAME} DESTINATION ${CMAKE_SOURCE_DIR}/engine/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

