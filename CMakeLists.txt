cmake_minimum_required(VERSION 3.16)
project(vex VERSION 4.2 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_subdirectory(Core)

qt_add_executable(vex
    main.cxx
    Plugvex.H
    Settings.H
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Dists/win/ico.qrc>
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/Dists/win/vex.rc>
    $<$<PLATFORM_ID:Darwin>:${CMAKE_CURRENT_SOURCE_DIR}/Dists/mac/icns.qrc>
)

target_link_libraries(vex PRIVATE
    Qt6::Core
    Qt6::Widgets
)

include(GNUInstallDirs)

install(TARGETS vex
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .

)

if(WIN32)
    install(TARGETS VexCore SyntaxCore LookAndFeelCore
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(APPLE)

    install(TARGETS VexCore SyntaxCore LookAndFeelCore
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(UNIX AND NOT APPLE)

    install(FILES
        ${CMAKE_BINARY_DIR}/Core/VexCore.so
        ${CMAKE_BINARY_DIR}/Core/SyntaxCore.so
        ${CMAKE_BINARY_DIR}/Core/LookAndFeelCore.so
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/vex
    )
endif()

if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Dists/vex.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Dists/vex.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    )
endif()

if(WIN32)
    set_target_properties(vex PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(APPLE)
    set_target_properties(vex PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Dists/mac/Info.plist
    )
endif()
