cmake_minimum_required(VERSION 2.8)
project(libavbot)


add_library(libavbot STATIC
	avbot.hpp avbot.cpp
	avchannel.hpp avchannel.cpp
)

target_include_directories(libavbot PUBLIC
    $<TARGET_PROPERTY:xmpp,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(libavbot irc xmpp mailexchange avim_group)

if(NOT NO_WEBQQ)
target_link_libraries(libavbot webqq)
endif()

set_target_properties(libavbot
		PROPERTIES
		OUTPUT_NAME avbot
		PREFIX "lib"
		CLEAN_DIRECT_OUTPUT 1)
