cmake_minimum_required(VERSION 3.16)
project(VexCore LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(VexCore SHARED VexCore.cxx)
add_library(SyntaxCore SHARED SyntaxCore.cxx)
add_library(LookAndFeelCore SHARED LookAndFeelCore.cxx themes/t.qrc)

target_link_libraries(VexCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(SyntaxCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(LookAndFeelCore PRIVATE Qt6::Core Qt6::Widgets)

target_include_directories(VexCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(SyntaxCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(LookAndFeelCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(UNIX AND NOT APPLE)
    set_target_properties(VexCore SyntaxCore LookAndFeelCore PROPERTIES
        PREFIX ""  

    )
endif()

if(WIN32 OR APPLE)
    set_target_properties(VexCore SyntaxCore LookAndFeelCore PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"  
    )
endif()