cmake_minimum_required(VERSION 2.6)
project(libwebqq)

find_package(Boost 1.54 REQUIRED COMPONENTS system thread regex filesystem)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(../avhttp/include/)
include_directories(..)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB AVHTTP_IMPL_SOURCES "src/impl/*.cpp")
file(GLOB AVHTTP_IMPL_HEADERS "src/impl/*.hpp")

add_library(webqq STATIC
	include/libwebqq/webqq.hpp include/libwebqq/error_code.hpp
	src/error_code.cpp src/webqq.cpp
	${AVHTTP_IMPL_SOURCES}
	${AVHTTP_IMPL_HEADERS}
)

target_link_libraries(webqq ${Boost_LIBRARIES} ${SOCI_LIBRARY})
INCLUDE (CheckLibraryExists)
if(NOT WIN32)
check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
        target_link_libraries(webqq -lrt)
endif(HAVE_CLOCK_GETTIME)
endif()

target_link_libraries(webqq ${OPENSSL_LIBRARIES})

target_include_directories(webqq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

