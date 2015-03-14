set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

cmake_policy(SET CMP0020 NEW)
add_definitions(-DQT_NO_KEYWORDS)

find_package(Qt5Widgets REQUIRED)


if(WIN32 AND MSVC AND ENABLE_QT_STATIC)
	# For static Windows builds, we also need to pull in the Qt5 Platform
	# support library, which is not exposed to CMake properly, unfortunately
	GET_TARGET_PROPERTY(QT_LIB_DIR "${Qt5Widgets_LIBRARIES}" LOCATION)
	GET_FILENAME_COMPONENT(QT_LIB_DIR "${QT_LIB_DIR}" PATH)
	FIND_LIBRARY(Qt5Platform_LIBRARIES_RELEASE Qt5PlatformSupport
		HINTS "${QT_LIB_DIR}"
	)
	FIND_LIBRARY(Qt5Platform_LIBRARIES_DEBUG Qt5PlatformSupportd
		HINTS "${QT_LIB_DIR}"
	)
	FIND_LIBRARY(Qt5QWindows_LIBRARIES_RELEASE qwindows
		HINTS "${QT_LIB_DIR}/../plugins/platforms"
	)
	FIND_LIBRARY(Qt5QWindows_LIBRARIES_DEBUG qwindowsd
		HINTS "${QT_LIB_DIR}/../plugins/platforms"
	)
	FIND_LIBRARY(Qt5QICO_LIBRARIES_RELEASE qico
		HINTS "${QT_LIB_DIR}/../plugins/imageformats"
	)
	FIND_LIBRARY(Qt5QICO_LIBRARIES_DEBUG qicod
		HINTS "${QT_LIB_DIR}/../plugins/imageformats"
	)

	FIND_LIBRARY(Qt5HB_LIBRARIES_RELEASE qtharfbuzzng
		HINTS "${QT_LIB_DIR}"
	)
	FIND_LIBRARY(Qt5HB_LIBRARIES_DEBUG qtharfbuzzngd
		HINTS "${QT_LIB_DIR}"
	)

	SET(Qt5Platform_LIBRARIES optimized ${Qt5Platform_LIBRARIES_RELEASE} debug ${Qt5Platform_LIBRARIES_DEBUG})
	SET(Qt5QWindows_LIBRARIES optimized ${Qt5QWindows_LIBRARIES_RELEASE} debug ${Qt5QWindows_LIBRARIES_DEBUG})
	SET(Qt5QICO_LIBRARIES optimized ${Qt5QICO_LIBRARIES_RELEASE} debug ${Qt5QICO_LIBRARIES_DEBUG})
	if (Qt5HB_LIBRARIES_RELEASE)
		SET(Qt5HB_LIBRARIES optimized ${Qt5HB_LIBRARIES_RELEASE} debug ${Qt5HB_LIBRARIES_DEBUG})
	endif()
	ADD_DEFINITIONS(-DSTATIC_QT5)
endif()


file(GLOB HDRS include/*.h*)
file(GLOB SRCS *.cpp *.h*)
file(GLOB UIS *.ui)

qt5_wrap_ui(UI_SRCS ${UIS})

set(SRCS ${SRCS} ${UI_SRCS})

add_library(avbotgui STATIC ${SRCS})

target_link_libraries(avbotgui Qt5::Widgets)
target_link_libraries(avbotgui avbotui_base)
target_link_libraries(avbotgui ${Qt5Platform_LIBRARIES} ${Qt5QWindows_LIBRARIES} ${Qt5QICO_LIBRARIES} ${Qt5HB_LIBRARIES})
