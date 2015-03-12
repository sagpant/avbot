include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../soci/src/core)
include_directories(../soci/src/backends/sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

set(CMAKE_AUTOMOC ON)

if (Qt5DBus_FOUND)
	SET(RPC_SOURCE  dbusrpc.cpp dbusrpc.hpp avbotrpc_adaptor.cpp avbotrpc_adaptor.h)
endif()

add_library(avbotrpc STATIC server.cpp ${RPC_SOURCE})

if (Qt5DBus_FOUND)
	target_link_libraries(avbotrpc Qt5::Core Qt5::DBus)
endif()

target_link_libraries(avbotrpc libavbot)
