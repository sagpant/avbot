
project(avbotluascript)

if(ENABLE_LUA)

add_definitions(-DENABLE_LUA)
include_directories(
	luabind
	${LUAJIT_INCLUDE_DIRS}
	)

add_library(libluascript STATIC luascript.cpp
		luabind/src/open.cpp
		luabind/src/create_class.cpp
		luabind/src/class.cpp
		luabind/src/class_info.cpp
		luabind/src/class_rep.cpp
		luabind/src/class_registry.cpp
		luabind/src/object_rep.cpp
		luabind/src/scope.cpp
		luabind/src/function.cpp
		luabind/src/exception_handler.cpp
		luabind/src/error.cpp
		luabind/src/inheritance.cpp
		luabind/src/pcall.cpp
		luabind/src/link_compatibility.cpp

		)
target_link_libraries(libluascript ${LUAJIT_LIBRARIES} libavbot)

if(MSVC)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:lua51.dll" )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:lua51.dll" PARENT_SCOPE)
target_link_libraries(libluascript delayimp.lib)

set_target_properties(libluascript PROPERTIES LINK_FLAGS "/DELAYLOAD:lua51.dll")
endif()

add_custom_command(TARGET libluascript POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory libs ${CMAKE_BINARY_DIR})

endif()

target_include_directories(libluascript PRIVATE
    $<TARGET_PROPERTY:xmpp,INTERFACE_INCLUDE_DIRECTORIES>)
