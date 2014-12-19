include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../soci/src/core)
include_directories(../soci/src/backends/sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

add_library(avbotrpc OBJECT server.cpp )