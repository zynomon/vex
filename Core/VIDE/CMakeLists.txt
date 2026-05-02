cmake_minimum_required(VERSION 3.16)
project(V.I.D.E. VERSION 1.0.0 LANGUAGES CXX) # VEX INTEGRATED DEVELOPMENT ENVIRONMENT

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(PLUGIN_DEST "$ENV{HOME}/.vex/plugins")
if(WIN32)
    set(PLUGIN_DEST "$ENV{USERPROFILE}/.vex/plugins")
endif()

function(add_plugin NAME)
    add_library(${NAME} MODULE ${ARGN} ${CMAKE_CURRENT_SOURCE_DIR}/../Uplugin.H)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_link_libraries(${NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    set_target_properties(${NAME} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin")
    add_custom_command(TARGET ${NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${NAME}>" "${PLUGIN_DEST}/")
endfunction()

add_plugin(ProjectCore ProjectCore.cxx)

target_sources(ProjectCore
  PRIVATE
    i.qrc
)
add_plugin(Terminal Terminal.cxx st/qlightterminal.cpp st/qlightterminal.h st/st-utils.h st/st.cpp st/st.h)
target_include_directories(Terminal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/st)
if(WIN32)
    target_link_libraries(Terminal PRIVATE kernel32)
else()
    target_link_libraries(Terminal PRIVATE util)
endif()
# TERMINAL NEEDS MORE UPGRADE FOR WINDOWS PTY < THUS I'll  DO IN 4.4
