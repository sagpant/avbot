cmake_minimum_required(VERSION 3.0)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project(avbot)

#-----------------------------------------------------------------------------
# force off-tree build
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "do not run cmake inside avbot source directory
Remove the CMakeCache.txt file and try again from another folder, e.g.:
   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake  ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/soci/src/cmake/modules)

if(MSVC)
	if(NOT PROTOBUF_SRC_ROOT_FOLDER AND NOT PROTOBUF_LIBRARY)
		if(DEFINED ENV{PROTOBUF_SRC_ROOT_FOLDER})
#			message(FATAL_ERROR "shit dvafdddd")
			set(PROTOBUF_SRC_ROOT_FOLDER $ENV{PROTOBUF_SRC_ROOT_FOLDER})
		else()
			set(PROTOBUF_SRC_ROOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf-2.6.1/)

			if( NOT EXISTS ${PROTOBUF_SRC_ROOT_FOLDER}/Debug)
				message(FATAL_ERROR "please compile protobuf in VC -- open and press F7")
			endif()
		endif()
	endif()
endif()

include(cpacksettings.cmake)
INCLUDE(CPack)

include(cxx11.cmake)
include(tweakvcflasgs.cmake)

#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp/pop3 test" OFF)
option(ENABLE_LIBSTDCXX_STATIC "link to static libstdc++" OFF)
option(ENABLE_LIBGCC_STATIC "link to static libgcc" OFF)
option(WITH_ICONV "boost is build with iconv" ON)
option(WITH_ICU "boost is build with icu " OFF)
option(WITH_QT_GUI "a qt gui interface" OFF)
option(NO_WEBQQ "disable webqq protocol" OFF)

option(USE_INTERNAL_SOCI "use bundled soci lib" WIN32)
option(INTERNAL_OPENSSL "use bundled openssl" WIN32)

if(NO_WEBQQ)
	add_definitions(-DNO_WEBQQ)
endif()

if(WITH_ICU)
	link_libraries(icudata)
	link_libraries(icuuc)
endif()

if(WIN32)
	set(ENABLE_LIBGCC_STATIC ON)
	set(ENABLE_LIBSTDCXX_STATIC ON)
	set(ENABLE_STATIC ON)
	add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)
	add_definitions(-U__STRICT_ANSI__)

if(MSVC)
	set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} DbgHelp.lib secur32.lib ws2_32.lib comctl32.lib mswsock.lib winmm.lib crypt32.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:winmm.dll")
else()
	link_libraries(dbghelp secur32 ws2_32  comctl32  mswsock winmm crypt32)
endif()
endif()

if(NOT MSVC AND NOT APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g0 -O3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g0 -O3")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
endif()

if(CMAKE_BUILD_TYPE EQUAL "TEST")
	set(ENABLE_STATIC ON)
endif()

if(ENABLE_STATIC)
	set(Boost_USE_STATIC_LIBS        ON)
	set(Boost_USE_STATIC_RUNTIME    ON)
else(ENABLE_STATIC)
	ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
endif()

if(ENABLE_LIBSTDCXX_STATIC OR WIN32)
	if(NOT MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ ")
	endif()

	set(Boost_USE_STATIC_LIBS        ON)
	set(Boost_USE_STATIC_RUNTIME    ON)
endif()

if(ENABLE_LIBGCC_STATIC OR WIN32)
	if(NOT MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc ")
	endif()
endif()

set(REQUIRED_QT_VERSION 5.3.0)

# find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core DBus Widgets Network)
find_package(Threads REQUIRED)
find_package(Boost 1.57 REQUIRED COMPONENTS timer chrono date_time filesystem system program_options regex locale)
find_package(Qt5Core REQUIRED)

find_package(Qt5DBus)
find_package(Qt5Widgets)
find_package(systemd)

if(INTERNAL_OPENSSL)
add_subdirectory(third_party/openssl)
add_definitions(-DOPENSSL_NO_ENGINE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPENSSL_NO_ENGINE -DOPENSSL_NO_COMP")
include_directories(third_party/openssl/include)
else()
find_package(OpenSSL REQUIRED)
endif()

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LIBPATH:${Boost_LIBRARY_DIR}")
endif()

if(WIN32)
	add_definitions(-DBOOST_ALL_STATIC_LINK)
	add_definitions(-DBOOST_THREAD_USE_LIB)
	add_definitions(-DBOOST_FILESYSTEM_STATIC_LINK)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	add_definitions(-DNOMINMAX)
endif()

add_definitions(-DAVHTTP_ENABLE_OPENSSL -DAVHTTP_DISABLE_MULTI_DOWNLOAD -DDISABLE_LOGGER)

configure_file(avbot.rc.in ${PROJECT_BINARY_DIR}/avbot.rc)
configure_file(avbot.service.in ${PROJECT_BINARY_DIR}/avbot.service)

find_package(SQLite3)

if(SQLITE3_FOUND)
	message (STATUS "found sqlite3")
else()
	message (STATUS "sqlite3 not found, try to use internal one")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")
	add_subdirectory(third_party/sqlite)

	set(sqlite_lib_location  $<TARGET_LINKER_FILE:sqlite3>)
	message (STATUS "sqlite_lib_location == ${sqlite_lib_location}")

	set(SQLITE_ROOT_DIR "${PROJECT_BINARY_DIR}/third_party/sqlite/")
	set(SQLITE3_INCLUDES "${PROJECT_SOURCE_DIR}/third_party/sqlite/include/" CACHE PATH "include dir for sqlite3.h")
	set(SQLITE3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/sqlite/include/")
	set(SQLITE3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/sqlite/include/" CACHE PATH "include dir for sqlite3.h" FORCE)
	set(SQLITE3_LIBRARIES $<TARGET_LINKER_FILE:sqlite3>)
	set(SQLITE3_LIBRARIES $<TARGET_LINKER_FILE:sqlite3> CACHE FILEPATH "lib file to link to sqlite")
	set(SQLITE3_LIBRARY "sqlite3")
	set(SQLite3_FOUND ON)

endif()

IF(USE_INTERNAL_SOCI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")
	add_subdirectory(third_party/soci/src EXCLUDE_FROM_ALL)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	include_directories(third_party/soci/src/core)
	include_directories(third_party/soci/src/backends/sqlite3)
	include_directories(third_party/sqlite/include)
	set(SOCI_LIBRARY soci_core_static)
	set(SOCI_sqlite3_PLUGIN soci_sqlite3_static  sqlite3)
else()
	find_package(Soci REQUIRED)
	include_directories(${SOCI_INCLUDE_DIRS})
	include_directories(${SOCI_INCLUDE_DIRS}/sqlite3)
endif()

option(INTERNALGLOOX "use bundled gloox" ON)

find_path(GLOOX_INCLUDE_DIR
  NAMES gloox/gloox.h
  PATHS
  /usr/include
  /usr/local/include
  $ENV{LIB_DIR}/include
  $ENV{LIB_DIR}/include/sqlite
  $ENV{LIB_DIR}/include/sqlite3
  $ENV{ProgramFiles}/SQLite/*/include
  $ENV{ProgramFiles}/SQLite3/*/include
  $ENV{SystemDrive}/SQLite/*/include
  $ENV{SystemDrive}/SQLite3/*/include
  $ENV{GLOOX_ROOT_DIR}/include
  ${GLOOX_ROOT_DIR}/include)

find_library(GLOOX_LIBRARIES NAMES gloox HINTS /usr/local/lib /usr/local/lib64 /usr/lib64 /usr/lib /usr/local/lib)

find_package_handle_standard_args(GLOOX
  DEFAULT_MSG
  GLOOX_LIBRARIES
  GLOOX_INCLUDE_DIR)

if(GLOOX_FOUND AND NOT INTERNALGLOOX)
	message(STATUS "gloox found on your system. nice!")
else()
	if (GLOOX_FOUND)
		message(STATUS "gloox found on your system. but we are not going to use it!")
	else()
		message(STATUS "gloox not found on your system, try using internal one now")
	endif()

	add_subdirectory(third_party/gloox)
	set(GLOOX_LIBRARIES gloox)
	set(GLOOX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gloox)
endif()


add_subdirectory(third_party/avhtml EXCLUDE_FROM_ALL)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

include_directories(avboost/include)
include_directories(avhttp/include)
include_directories(deCAPTCHA/include)
include_directories(avhttpd/include)
include_directories(third_party/acceptor/include)
include_directories(third_party/avhtml/include)

add_subdirectory(protocols)
add_subdirectory(libavbot)
add_subdirectory(libavlog)
add_subdirectory(extension)
add_subdirectory(ui)

configure_file(version.c.in version.c @ONLY)

set(AVBOT_SOURCES
	auto_welcome.hpp
	botctl.hpp
	input.hpp
	main.cpp
	botctl.cpp
	input.cpp
	avbot_log_search.cpp
	spirit.cpp
	${PROJECT_BINARY_DIR}/version.c
	${PROJECT_BINARY_DIR}/avbot.rc
)

if(NOT WIN32)
	list(APPEND AVBOT_SOURCES backtrace.cpp)
endif()

if(WIN32)
	list(APPEND AVBOT_SOURCES minidump.cpp)
endif()

if(MSVC)

	if(CMAKE_CL_64)
		LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x64/")
	else()
		LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x86/")
	endif()

endif()

add_subdirectory(rpc)

qt5_add_resources(RC_SRCS avbot.qrc)

add_executable(avbot WIN32 ${AVBOT_SOURCES} ${RC_SRCS})

target_link_libraries(avbot avbotrpc)

if (Qt5Widgets_FOUND AND WITH_QT_GUI)
	set_target_properties(avbot PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(NOT WIN32)
	target_link_libraries(avbot -rdynamic)
endif()

target_link_libraries(avbot ${OPENSSL_LIBRARIES})


if(APPLE AND WITH_ICONV)
	target_link_libraries(avbot -liconv)
endif()

target_link_libraries(avbot avimproto avim++ libavbot avlog extension ${SOCI_LIBRARY} ${SOCI_sqlite3_PLUGIN} ${SQLITE3_LIBRARIES})

target_link_libraries(avbot avbotui)

target_link_libraries(avbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(avbot html5)

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()

check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
	target_link_libraries(avbot -lrt)
endif(HAVE_CLOCK_GETTIME)

if (NOT WIN32)
	set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/third_party/soci/src/cmake/modules ${CMAKE_MODULE_PATH})
	find_package(DL)
	if(DL_FOUND)
		include_directories(${DL_INCLUDE_DIR})
		target_link_libraries(avbot ${DL_LIBRARY})
	endif()
endif()

if (ALSA_FOUND)
	target_link_libraries(avbot ${ALSA_LIBRARY})
endif()

if(SYSTEMD_FOUND)
	target_link_libraries(avbot ${SYSTEMD_LIBRARY})
	add_definitions(-DHAVE_SYSTEMD)
endif()

install(TARGETS avbot RUNTIME DESTINATION bin)
if(NOT WIN32)
	install(FILES ${PROJECT_BINARY_DIR}/avbot.service DESTINATION /lib/systemd/system)
endif()

if(WIN32)
	if(MSVC)
		set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/dpi.manifest")
	endif()

	if(NOT MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lcrypt32 -lmswsock")
	endif()

	if(WITH_ICONV AND NOT MSVC)
		target_link_libraries(avbot -liconv)
	endif()
endif()

