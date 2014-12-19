include(FindPackageHandleStandardArgs)

find_package(Boost 1.55 REQUIRED COMPONENTS date_time regex system coroutine context atomic thread)

include_directories(${Boost_INCLUDE_DIRS})

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

if(GLOOX_FOUND)
	message(STATUS "gloox found on your system. nice!")
else()
	message(STATUS "gloox not found on your system, try using internal one now")
	add_subdirectory(src/gloox)
	set(GLOOX_LIBRARIES gloox)
endif()

add_library(xmpp STATIC src/xmpp_impl.hpp src/xmpp_impl.cpp include/libxmpp/xmpp.hpp src/xmpp.cpp)

target_include_directories(xmpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(NOT GLOOX_FOUND)
	target_include_directories(xmpp PRIVATE src/gloox)
else()
	target_include_directories(xmpp PRIVATE ${GLOOX_INCLUDE_DIR})
endif()

target_link_libraries(xmpp ${Boost_LIBRARIES})

target_link_libraries(xmpp ${GLOOX_LIBRARIES})

if(ENABLE_TEST)
	add_executable(xmpptest test.cpp)
	target_link_libraries(xmpptest xmpp)
endif()
