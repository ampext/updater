cmake_minimum_required(VERSION 2.8)

project(upd_ui)

set(SRCS UpdaterApp.cpp UpdateDlg.cpp WaitDlg.cpp UpdateThreads.cpp Tray.cpp DownloadPanel.cpp DownloadDlg.cpp DataViewBitmapRenderer.cpp)

if (WIN32)
    set(SRCS ${SRCS} Updater.rc)
endif()

wxwidgets_add_resources(SRCS Updater.xrc OPTIONS -c -v -o Resource.cpp)

if(NOT WIN_SUBSYSTEM)
	add_executable(upd_ui ${SRCS})
else()
	add_executable(upd_ui WIN32 ${SRCS})
endif()

target_link_libraries(upd_ui updater ${wxWidgets_LIBRARIES})
