cmake_minimum_required(VERSION 2.8)

project(updater)

set(SRCS Updater.cpp ProcessUtils.cpp Utils.cpp AppInfoProvider.cpp Url.cpp)

add_definitions(-DBUILD_NUMBER=${BUILD_NUMBER})
add_library(updater ${SRCS})

if(WIN32)
	target_link_libraries(updater Netapi32.lib)
endif(WIN32)

target_link_libraries(updater ${wxWidgets_LIBRARIES} ${CURL_LIBRARIES})