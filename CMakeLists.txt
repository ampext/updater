cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=gnu++0x -Wall")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

find_package(wxWidgets COMPONENTS xrc html adv core xml base REQUIRED)
find_package(Boost)
find_package(Curl)

include("${wxWidgets_USE_FILE}")
include_directories(${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

if(NOT BUILD_NUMBER)
	file(STRINGS "build_number" BUILD_NUMBER)
endif()

if(WIN32)
	list(APPEND CURL_LIBRARIES Ws2_32.lib)
endif(WIN32)

add_subdirectory(lib)
add_subdirectory(ui)