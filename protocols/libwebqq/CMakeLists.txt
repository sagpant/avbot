cmake_minimum_required(VERSION 2.6)
project(libwebqq)

find_package(PkgConfig)
find_package(Boost 1.54 REQUIRED COMPONENTS system thread regex filesystem)

pkg_check_modules(MOZJS24 mozjs-25)
pkg_check_modules(MOZJS185 mozjs185)

if(MOZJS24_FOUND)
set(MOZJS_INCLUDE_DIRS ${MOZJS24_INCLUDE_DIRS})
set(MOZJS_LIBS ${MOZJS24_LIBRARIES})
elseif(MOZJS185_FOUND)
set(MOZJS_INCLUDE_DIRS ${MOZJS185_INCLUDE_DIRS})
set(MOZJS_LIBS ${MOZJS185_LIBRARIES})
else()
message(FATAL_ERROR "no mozjs found, install mozjs(on fedora) or spidermonkey(on gentoo)")
endif()

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

target_link_libraries(webqq ${Boost_LIBRARIES} ${MOZJS_LIBS} ${SOCI_LIBRARY} Qt5::Core)
INCLUDE (CheckLibraryExists)
if(NOT WIN32)
check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
        target_link_libraries(webqq -lrt)
endif(HAVE_CLOCK_GETTIME)
endif()

target_include_directories(webqq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(webqq PRIVATE ${MOZJS_INCLUDE_DIRS})

