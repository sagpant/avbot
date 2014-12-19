
find_package(Boost 1.55 REQUIRED COMPONENTS system thread)

add_library(avbotgui STATIC avbotcli.cpp)
target_link_libraries(avbotgui avbotui_base)

target_link_libraries(avbotgui ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
