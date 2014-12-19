project(libpop3)
find_package(Boost 1.54 COMPONENTS regex system locale)

file(GLOB MAILEXCHANGE_IMPL_HEADERS "*.hpp")

include_directories(${OPENSSL_INCLUDE_DIR})

add_library(mailexchange STATIC pop3.cpp smtp.cpp ${MAILEXCHANGE_IMPL_HEADERS})
target_link_libraries(mailexchange ${Boost_LIBRARIES})

if(ENABLE_STATIC)

	set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
	if( EXISTS /usr/lib/libicuuc.a AND EXISTS /usr/lib/libicudata.a)
		target_link_libraries(mailexchange /usr/lib/libicuuc.a /usr/lib/libicudata.a)
	else()
		find_package(ICU )
		if(ICU_FOUND)
			target_link_libraries(mailexchange ${ICU_LIBRARIES})
		endif()
	endif()
endif()

target_include_directories(mailexchange PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32 AND WITH_ICONV)
	if(MSVC)
	target_link_libraries(mailexchange crypt32.lib ws2_32.lib)
	else()
	target_link_libraries(mailexchange -liconv -lcrypt32 -lws2_32 )
	endif()
endif()

if( ENABLE_TEST )
add_definitions(-DDEBUG)
add_executable(pop3test pop3test.cpp)
target_link_libraries(pop3test mailexchange ${CMAKE_THREAD_LIBS_INIT} )

add_executable(smtptest smtptest.cpp)
target_link_libraries(smtptest mailexchange ${CMAKE_THREAD_LIBS_INIT} )

endif()
