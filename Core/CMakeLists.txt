cmake_minimum_required(VERSION 3.16)
project(VexCore LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(VexCore SHARED VexCore.cxx)
add_library(SyntaxCore SHARED SyntaxCore.cxx)
add_library(LookAndFeelCore SHARED LookAndFeelCore.cxx)
target_link_libraries(VexCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(SyntaxCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(LookAndFeelCore PRIVATE Qt6::Core Qt6::Widgets)

set_target_properties(VexCore SyntaxCore LookAndFeelCore PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

include(GNUInstallDirs)
install(TARGETS VexCore SyntaxCore LookAndFeelCore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}

)
