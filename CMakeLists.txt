cmake_minimum_required(VERSION 3.16)

project(vex VERSION 1.0 LANGUAGES CXX)

# Qt setup
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt detection
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg SvgWidgets)

# Source files
set(PROJECT_SOURCES

    resources.qrc
)

# Executable
qt_add_executable(vex ${PROJECT_SOURCES}
    main.cpp
    main.cpp)

# Link Qt libraries
target_link_libraries(vex PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
)

# Install rules
include(GNUInstallDirs)

install(TARGETS vex
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create and install empty plugins directory (for system-wide plugins)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vex/plugins)

# Desktop entry
set(EXECUTABLE_NAME "vex")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/vex.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/vex.desktop
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vex.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        COMPONENT desktop)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons/vex.svgz")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/vex.svgz
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
            COMPONENT icons)
else()
    message(WARNING "vex.svgz not found. No branded icon will be installed.")
endif()

# CPack configuration
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "vex")
set(CPACK_PACKAGE_VENDOR "error.os")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    " ♦️ Vex is a modular, MIME-aware editor designed for expressive system integration and branded desktop environments. \
    Built with performance and clarity in mind, it aligns with error.os principles of transparency, user empowerment, and visual polish. \
    Vex supports multilingual UI, scalable icon logic, and clean packaging for Linux distributions. \
    Its install footprint is minimal, its launch behavior is predictable, and its desktop entry is fully localized. \
    Ideal for developers, system architects, and themed distributions seeking a lightweight yet intentional editing experience."
)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH 0)

set(CPACK_PACKAGE_CONTACT "Zynomon Aelius <zynomon@proton.me>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zynomon Aelius")

set(CPACK_GENERATOR "DEB;TGZ;ZIP")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6svg6")

include(CPack)
