option(WITH_QT_GUI "a qt gui interface" OFF)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Qt5Widgets)

add_library(avbotui_base STATIC src/avbotui.cpp)

if (Qt5Widgets_FOUND AND WITH_QT_GUI)
	add_definitions(-DWITH_QT_GUI)
	add_definitions(-DSTATIC_QT5)
	set(ENABLE_QT_STATIC ON)
	add_subdirectory(src/qt-gui)
else()
	# 暂时不实现 win32 GUI
	#if(WIN32)
	#	add_subdirectory(src/win32-gui)
	#else()
		add_subdirectory(src/cli)
	#endif()
endif()

target_include_directories(avbotui_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(avbotui STATIC uifactory.cpp)

target_link_libraries(avbotui avbotui_base avbotgui)
