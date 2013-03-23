cmake_minimum_required(VERSION 2.8)

project(updater)

set(SRCS Updater.cpp ProcessUtils.cpp Utils.cpp AppInfoProvider.cpp)

add_definitions(-DBUILD_NUMBER=${BUILD_NUMBER})
add_library(updater ${SRCS})
target_link_libraries(updater ${wxWidgets_LIBRARIES})