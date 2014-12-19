
add_subdirectory(avproto)

add_library(avim_group STATIC src/avim_group.cpp src/avim_group_impl.cpp)
target_link_libraries(avim_group avim++)

target_include_directories(avim_group PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
