cmake_minimum_required(VERSION 3.16)
project(VexCore LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

function(add_vex_library NAME SOURCES)
    add_library(${NAME} SHARED ${SOURCES} ${ARGN})
    target_link_libraries(${NAME} PRIVATE Qt6::Core Qt6::Widgets)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    set_target_properties(${NAME} PROPERTIES PREFIX "")

    if(WIN32 OR APPLE)
        set_target_properties(${NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
    endif()
endfunction()

add_vex_library(EditorCore EditorCore.cxx)
add_vex_library(SyntaxCore SyntaxCore.cxx)
add_vex_library(LookAndFeelCore LookAndFeelCore.cxx rc/t.qrc)
add_vex_library(Uplugin Uplugin.cxx Uplugin.H $<$<PLATFORM_ID:Windows>:rc/u.qrc>)

add_subdirectory(VIDE/)
