cmake_minimum_required(VERSION 2.6)
project(libirc)

find_package(Boost 1.54 REQUIRED COMPONENTS date_time system regex thread)

include_directories(${Boost_INCLUDE_DIRS})

add_library(irc STATIC include/irc.hpp src/irc.cpp)

if( ENABLE_TEST )

add_executable(irc3test EXCLUDE_FROM_ALL test.cpp )
target_link_libraries(irc3test irc ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

endif()

target_include_directories(irc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
