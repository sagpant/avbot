include(FindPackageHandleStandardArgs)

find_package(Boost 1.55 REQUIRED COMPONENTS date_time regex system coroutine context atomic thread)

include_directories(${Boost_INCLUDE_DIRS})

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
