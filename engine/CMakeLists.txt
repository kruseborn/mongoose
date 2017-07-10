cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
cmake_policy(VERSION 3.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#Uncomment to skip creating ZERO_CHECK project
#set(CMAKE_SUPPRESS_REGENERATION true)
set(NAME engine)

project(${NAME})

OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)
OPTION(USE_WAYLAND_WSI "Build the project using Wayland swapchain" OFF)

#unset(CMAKE_CXX_FLAGS)

IF(WIN32)
  find_package(Vulkan REQUIRED)
	find_library(ASSIMP_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/engine/libs/)
  find_library(GLFW_LIBRARIES NAMES glfw3 glfw3.lib PATHS ${CMAKE_SOURCE_DIR}/engine/libs/)
	#add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
ELSE(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/engine/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLFW REQUIRED glfw3)
  include_directories(${GLFW_INCLUDE_DIRS})
  target_link_libraries(simple ${GLFW_STATIC_LIBRARIES})
	find_package(ASSIMP REQUIRED)
	find_package(Threads REQUIRED)
IF(USE_D2D_WSI)
	MESSAGE("Using direct to display extension...")
	add_definitions(-D_DIRECT2DISPLAY)
ELSEIF(USE_WAYLAND_WSI)
	find_package(Wayland REQUIRED)
	if (NOT WAYLAND_FOUND)
		message(FATAL_ERROR "Wayland development package not found")
	endif ()
	add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
	include_directories(${WAYLAND_INCLUDE_DIR})
ELSE(USE_D2D_WSI)
	find_package(XCB REQUIRED)
	add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
ENDIF(USE_D2D_WSI)
	# Todo : android?
ENDIF(WIN32)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_definitions(-Wno-switch-enum)
endif()

# Debug flags
add_definitions(-W3)
if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-Wextra -Wundef)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_definitions(
  -D_CONSOLE
  #-DVC_EXTRALEAN
  #-DWIN32_LEAN_AND_MEAN
  -DNOMINMAX
  #-D_CRT_SECURE_NO_WARNINGS
  -D_UNICODE
  -std=c++14 
)

remove_definitions(
  -DWIN32
  -D_WINDOWS
)

include_directories(
  engine/include/
)

file(GLOB sources "engine/*.cpp" "engine/*.h")
set(MAIN_CPP "engine/main.cpp")
set(SHADER_DIR "engine/shaders/")
file(GLOB SHADERS "${SHADER_DIR}/*.glsl")
add_executable(engine ${sources})
target_link_libraries(engine ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${GLFW_LIBRARIES})

# Compiler specific stuff
IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

IF(WIN32)
	# Nothing here (yet)
ELSE(WIN32)
	link_libraries(${XCB_LIBRARIES} ${Vulkan_LIBRARY})
ENDIF(WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/engine/bin/")

