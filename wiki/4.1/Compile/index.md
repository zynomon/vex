---
description: "How to compile Vex 4.1 from source on any supported platform"
headericon: "/wiki/4.1/headericon.svg"


layout: default
title: "Compile"
---

<img src="https://github.com/user-attachments/assets/ca3ca28e-9170-4cf5-8aca-106886905949" height="256" width="256" align="left">
 
***# Vex Compilation Guide***
 
Vex is built on the Qt Framework, the same technology powering Oracle VirtualBox, VLC Media Player, Telegram, Tesla's car interfaces, and the KDE desktop environment. If you're running KDE or LXQt, Qt is likely already on your system.
 
To compile Vex, you need a C++ compiler (GCC or Clang) and Qt 6's Core and Widgets modules. That's it.
 
<br clear="left"/>
 
---
 
## Table of Contents
 
- [Installing Qt and Build Tools](#installing-qt-and-build-tools)
  - [Windows Setup](#windows-setup)
  - [Debian and Ubuntu](#debian-and-ubuntu)
  - [Fedora and RHEL](#fedora-and-rhel)
  - [Arch Linux](#arch-linux)
  - [OpenSUSE](#opensuse)
  - [macOS](#macos)
  - [FreeBSD](#freebsd)
  - [OpenBSD](#openbsd)
  - [Other Systems](#other-systems)
- [Understanding the Codebase](#understanding-the-codebase)
  - [Entry Point: main.cxx](#entry-point-maincxx)
  - [Plugin System: Plugvex.H](#plugin-system-plugvexh)
  - [Configuration: Settings.H](#configuration-settingsh)
  - [Core Plugins Directory](#core-plugins-directory)
  - [Build Configuration: CMakeLists.txt](#build-configuration-cmakeliststxt)
  - [Compiling with CMake](#compiling-with-cmake)
- [Creating Distribution Packages](#creating-distribution-packages)
  - [Windows Packages](#windows-packages)
  - [Linux and BSD Packages](#linux-and-bsd-packages)
  - [Customizing Your Package](#customizing-your-package)
  - [Licensing and Distribution](#licensing-and-distribution)
 
---
 
## Installing Qt and Build Tools
 
### Windows Setup
 
**What you need:**
- Windows 10 or later (64-bit)
- 4GB RAM minimum
- 5GB free disk space ( 2GB for IDE and 3 GB for general usage )
- Internet connection
 
**Installation steps:**
 
1. **Download the Qt installer**
   - Get it from https://www.qt.io/download-qt-installer
   - Look for `qt-online-installer-windows-x64-<version>.exe`
 
<img width="127" height="156" alt="image" src="https://github.com/user-attachments/assets/b71136bc-a4c3-44ee-8881-c5548a8a3ec3" />

2. **Run the installer**
   - Double-click the downloaded file
   - If SmartScreen blocks it, click More info, then Run anyway
 
3. **Sign in or create an account**
   - Qt requires a free account to download, open qt's website and register a new account
   - Accept the license terms
 
4. **Select components**

<img width="1082" height="727" alt="image" src="https://github.com/user-attachments/assets/f6f7b0fb-dcb9-48ea-a1d3-849ace7a8a7c" />

Choose Qt for Desktop Development

  -   or cherry pick after clicking on custom installation 

![csZuTezLH1_fast808b](https://github.com/user-attachments/assets/06be5d63-876d-40b6-8097-b434c45cc6a4)

if you face any issues or did any mistakes make sure to open your start menu and you will see something called "Qt maintenance tool"
<img width="548" height="281" alt="image" src="https://github.com/user-attachments/assets/69c18453-8c13-4873-a232-07958c5a5190" />
This will help you.

5. **Choose where to install**
   - Default location: `C:\Qt`
   - Avoid paths with spaces
 
6. **Wait for installation**
   - This takes 30+ minutes to few hours depending on your connection
 
**Verify installation:**
```cmd
"C:\Qt\Tools\CMake_64\bin\cmake.exe" --version
"C:\Qt\6.x.x\mingw_64\bin\qmake.exe" --version
```

---
 
### Debian and Ubuntu
 
**Supported versions:**
- Debian 11 (Bullseye), 12 (Bookworm) , 13 ( Trixie ) and newer
- Ubuntu 20.04+ and newer
- Linux Mint 20+
 
**Install everything at once:**
 
```bash
# Update package lists
sudo apt update
 
# Install Qt 6 development packages
sudo apt install -y qt6-base-dev qt6-base-dev-tools
 
# Install Qt Creator
sudo apt install -y qtcreator
 
# Install build tools
sudo apt install -y build-essential cmake ninja-build
 
# Optional: Additional Qt modules
sudo apt install -y qt6-tools-dev qt6-tools-dev-tools
```
 
**Verify installation:**
```bash
qmake6 --version
cmake --version
gcc --version
qtcreator --version
```
 
**For KDE Plasma or LXQt users:**
 
Qt is already on your system. Just add the development files:
```bash
sudo apt install -y qt6-base-dev qtcreator cmake build-essential
```
 
---
 
### Fedora and RHEL
 
**Supported versions:**
- Fedora 38+
- Fedora Workstation, KDE Spin, LXQt Spin
- Any RHEL / CENTOS / FEDORA-BASED Oses
 
**Install packages:**
 
```bash
# Install Qt 6 development packages
sudo dnf install -y qt6-qtbase-devel qt6-qttools-devel
 
# Install Qt Creator
sudo dnf install -y qt-creator
 
# Install build tools
sudo dnf install -y gcc gcc-c++ cmake ninja-build
 
# Install graphics dependencies
sudo dnf install -y mesa-libGL-devel libxkbcommon-devel
```
 
**Verify installation:**
```bash
qmake-qt6 --version
cmake --version
gcc --version
qtcreator --version
```
 
**KDE Plasma users:**
 
Most Qt libraries are already installed. Just add development headers:
```bash
sudo dnf install -y qt6-qtbase-devel qt-creator cmake gcc-c++
```
 
---
 
### Arch Linux
 
**Supported distributions:**
- Arch Linux
- Manjaro
- EndeavourOS
- Garuda Linux
- And many more Arch Based. Distros

**Install packages:**
 
```bash
# Update system
sudo pacman -Syu
 
# Install Qt 6 and tools
sudo pacman -S --needed qt6-base qt6-tools qtcreator
 
# Install build tools
sudo pacman -S --needed base-devel cmake ninja
 
# Optional: Additional Qt modules
sudo pacman -S --needed qt6-svg qt6-declarative
```
 
**Verify installation:**
```bash
qmake6 --version
cmake --version
g++ --version
qtcreator --version
```
 
**Using AUR for latest Qt Creator (optional):**
```bash
yay -S qtcreator-git
# or
paru -S qtcreator-git
```
 
---
 
### OpenSUSE
 
**Supported versions:**
- openSUSE Leap 15.x
- openSUSE Tumbleweed
 
**For Tumbleweed:**
 
```bash
# Install Qt 6 development packages
sudo zypper install -y libqt6-qtbase-devel libqt6-qttools-devel
 
# Install Qt Creator
sudo zypper install -y qtcreator
 
# Install build tools
sudo zypper install -y gcc-c++ cmake ninja
 
# Install graphics dependencies
sudo zypper install -y libqt6-qtsvg-devel Mesa-libGL-devel
```
 
**For Leap 15.x:**
 
Leap uses Qt 5 by default, so add the Qt 6 repository:
```bash
# Add Qt 6 repository
sudo zypper addrepo https://download.opensuse.org/repositories/KDE:/Qt:/6/15.5/ qt6
sudo zypper refresh
 
# Install packages
sudo zypper install -y libqt6-qtbase-devel qtcreator cmake gcc-c++
```
 
**Verify installation:**
```bash
qmake6 --version
cmake --version
g++ --version
qtcreator --version
```
 
---
 
### macOS
 
**Requirements:**
- macOS 11 (Big Sur) or later
- Xcode Command Line Tools
- 8GB RAM minimum
 
**Step 1: Install Xcode Command Line Tools**
 
Open Terminal and run:
```bash
xcode-select --install
```
 
Click Install when the dialog appears.
 
**Step 2: Install Homebrew**
 
If you don't have Homebrew:
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
 
**Step 3: Install Qt via Homebrew**
 
```bash
# Install Qt 6
brew install qt@6
 
# Install CMake and Ninja
brew install cmake ninja
 
# Add Qt to PATH
echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```
 
**Verify installation:**
```bash
qmake --version
cmake --version
clang --version
```
 
**Alternative: Qt Online Installer (includes Qt Creator)**
 
1. Download from https://www.qt.io/download-qt-installer
2. Get `qt-online-installer-macos-x64-<version>.dmg`
3. Mount the DMG and run the installer
4. Select:
   - Qt 6.x.x → macOS
   - Qt Creator
   - CMake, Ninja
 
Check installation:
```bash
/Users/$USER/Qt/6.x.x/macos/bin/qmake --version
```
 
---
 
### FreeBSD
 
**Supported versions:**
- FreeBSD 13.x, 14.x
 
**Install using pkg:**
 
```bash
# Update package repository
sudo pkg update
 
# Install Qt 6 development packages
sudo pkg install -y qt6-base qt6-tools
 
# Install Qt Creator
sudo pkg install -y qt6-creator
 
# Install build tools (Clang is default)
sudo pkg install -y cmake ninja
 
# Install graphics libraries
sudo pkg install -y libglvnd mesa-libs
```
 
**Verify installation:**
```bash
qmake6 --version
cmake --version
clang --version
qtcreator --version
```
 
**Building from ports (alternative):**
 
```bash
cd /usr/ports/devel/qt6
sudo make install clean
 
cd /usr/ports/devel/qtcreator
sudo make install clean
```
 
---
 
### OpenBSD
 
**Supported versions:**
- OpenBSD 7.3+, 7.4+
 
**Install packages:**
 
```bash
# Install Qt 6 packages
doas pkg_add qt6-qtbase qt6-qttools
 
# Install Qt Creator (if available)
doas pkg_add qt6-creator
 
# Install build tools
doas pkg_add cmake ninja
 
# Clang is already installed by default
```
 
**Verify installation:**
```bash
qmake6 --version
cmake --version
clang --version
```
 
**Note:** Qt Creator availability varies on OpenBSD. If unavailable, build Vex using command-line tools only.
 
---
 
### Other Systems
 
Vex compiles on any platform supporting Qt 6 and C++20.
 
**Resources:**
- Qt Framework: https://github.com/qt/qtbase
- Qt Creator: https://github.com/qt-creator/qt-creator
- Qt Documentation: https://doc.qt.io/qt-6/
 
**Supported architectures:**
- x86_64 (AMD64)
- ARM64 (AArch64)
- ARMv7
- RISC-V (experimental in Qt)
 
**Platform-specific examples:**
 
**Haiku OS:**
```bash
pkgman install qt6_devel cmake ninja
```
 
**Solaris/illumos:**
```bash
pkg install qt6 cmake gcc
```
 
**Android/iOS:**
Use Qt Creator's mobile deployment features. Cross-compilation required.
 
**WebAssembly:**
Requires Emscripten toolchain and Qt for WebAssembly build.
 
---
 
## Understanding the Codebase
 
### Entry Point: main.cxx
 
**Location:** `main.cxx` (project root)
 
**What it does:** Starts the application and loads the plugin system
 
**Code structure:**
 
```cpp
#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include "Settings.H"
#include "Plugvex.H"
 
class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    QLabel* m_loadingIcon;
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("Vex :/");
        setWindowIcon(Settings::resolveIcon("vex"));
        resize(1000, 700);
        
        m_loadingIcon = new QLabel(this);
        m_loadingIcon->setPixmap(Settings::resolveIcon("vex").pixmap(200, 200));
        m_loadingIcon->setAlignment(Qt::AlignCenter);
        setCentralWidget(m_loadingIcon);
    }
    
    void setMainUI(QWidget* mainUI) {
        if (m_loadingIcon) {
            m_loadingIcon->deleteLater();
            m_loadingIcon = nullptr;
        }
        setCentralWidget(mainUI);
    }
};
 
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow vex;
    CmdLine& cmdLine = CmdLine::instance();
    
    PluginLoader::loadAndInitialize(&vex, &Settings::instance(), cmdLine, argc, argv);
    
    vex.show();
    int result = app.exec();
    
    PluginLoader::cleanup();
    return result;
}
 
#include "main.moc"
```
 
**Key components:**
 
1. **MainWindow class**
   - Minimal QMainWindow subclass
   - Shows loading icon during plugin initialization
   - VexCore plugin replaces loading screen with actual UI
 
2. **main() function**
   - Creates QApplication instance
   - Creates MainWindow
   - Initializes plugin system
   - Runs Qt event loop
   - Cleans up on exit
 
**Design philosophy:**
 
The main.cxx file is intentionally minimal. All functionality lives in plugins, keeping the core clean and testable. The loading screen prevents a blank window while plugins start up.
 
---
 
### Plugin System: Plugvex.H
 
**Location:** `Plugvex.H` (project root)
 
**What it does:** Defines the plugin architecture that makes Vex extensible
 
**Major components:**
 
1. **CmdLine class**
   - Singleton command-line parser
   - Shared across all plugins
   - Plugins register their own command-line options
 
2. **PluginMetadata**
   - Defines plugin importance tiers:
     - **Xylem:** Structural plugins (core UI, essential features)
     - **Phloem:** Enhancement plugins (syntax highlighting, themes)
     - **Simple:** Standalone plugins (independent tools)
 
3. **CorePlugin interface**
   - Pure virtual interface all plugins implement
   - Required methods:
     - `meta()`: Returns plugin metadata
     - `initialize()`: Called when plugin loads
 
4. **PluginLoader class**
   - Static plugin loading system
   - Searches plugin directories
   - Loads, categorizes, and initializes plugins in correct order
   - Isolates plugin errors (one broken plugin won't crash Vex)
 
**Plugin discovery:**
 
On Linux/BSD, Vex searches:
- Application directory (where the vex binary is located)
- `/usr/lib/vex/` (system-wide install)
 
On Windows:
- Application directory (same folder as vex.exe)
 
On macOS:
- Application directory (inside the bundle)
 
**Command-line parser:**
 
The CmdLine class provides version and help information:
- Version: `Vex 4.0 (Cytoplasm)`
- Debug mode: `vex -d` or `vex --debug`
 
---
 
### Configuration: Settings.H
 
**Location:** `Settings.H` (project root)
 
**What it does:** Manages application settings and configuration
 
**Key features:**
 
1. **Singleton pattern**
   - Global access via `Settings::instance()`
   - Ensures one configuration source
 
2. **INI file storage**
   - Location: `~/.vex/vex.conf` (Linux/BSD)
   - Location: `%APPDATA%/vex/vex.conf` (Windows)
   - Location: `~/Library/Application Support/vex/vex.conf` (macOS)
   - Standard INI format: `key=value`
 
3. **basePath() function**
   - Returns `~/.vex/` directory
   - Creates directory automatically if missing
   - On Windows: creates as hidden folder
 
4. **Icon resolution**
   - `resolveIcon()` finds icons from theme or embedded resources
   - Searches user themes first, falls back to built-in icons
 
**Usage example:**
 
```cpp
Settings& settings = Settings::instance();
 
// Save a setting
settings.setValue("editor.fontSize", 14);
 
// Load a setting with default value
int fontSize = settings.get<int>("editor.fontSize", 12);
 
// Get config directory
QString configPath = settings.basePath();
 
// Resolve an icon from theme or embedded resources
QIcon icon = Settings::resolveIcon("vex");
```
 
**How icon resolution works:**
 
1. First checks system icon theme (via `QIcon::fromTheme()`)
2. If not found, searches embedded resources for:
   - `:/vex.svg`
   - `:/vex.ico`
   - `:/vex.icns`
   - `:/vex.svgz`
3. Returns empty QIcon if nothing found
 
---
 
### Core Plugins Directory
 
**Location:** `Core/` directory
 
**What's inside:**
 
```
Core/
├── CMakeLists.txt          # Build configuration for plugins
├── VexCore.cxx             # Main UI plugin (Xylem tier)
├── SyntaxCore.cxx          # Syntax highlighting (Phloem tier)
├── LookAndFeelCore.cxx     # Theming system (Phloem tier)
└── themes/
    ├── t.qrc               # Embedded theme resources
    └── vex.qss             # Default theme stylesheet
```
 
**Plugin breakdown:**
 
**VexCore (Xylem)**
- Creates the main UI structure
- Manages tabs and editor instances
- Handles file operations (open, save, close)
- Provides menu bar and toolbar
- This is the heart of Vex
 
**SyntaxCore (Phloem)**
- Syntax highlighting engine
- Detects programming languages
- Loads .vxsyn syntax definition files
- Applies color schemes to code
- Extensible through custom syntax files
 
**LookAndFeelCore (Phloem)**
- Theme system implementation
- Parses QSS (Qt StyleSheets) and VEX#QSS extensions
- Icon theme support
- Hot-reloading for theme development
- Qt style wrapper selection
 
**Why separate plugins?**
 
Each plugin has a specific job. VexCore handles the editor, SyntaxCore makes code colorful, and LookAndFeelCore makes it pretty. You can replace any plugin without touching the others.
 
---
 
### Build Configuration: CMakeLists.txt
 
**Main CMakeLists.txt** (project root):
 
```cmake
cmake_minimum_required(VERSION 3.16)
project(vex VERSION 4.1 LANGUAGES CXX)
 
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
```
 
**Core/CMakeLists.txt:**
 
```cmake
cmake_minimum_required(VERSION 3.16)
project(VexCore LANGUAGES CXX)
 
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
 
# Build each core plugin as a shared library
add_library(VexCore SHARED VexCore.cxx)
add_library(SyntaxCore SHARED SyntaxCore.cxx)
add_library(LookAndFeelCore SHARED LookAndFeelCore.cxx themes/t.qrc)
 
# Link Qt to each plugin
target_link_libraries(VexCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(SyntaxCore PRIVATE Qt6::Core Qt6::Widgets)
target_link_libraries(LookAndFeelCore PRIVATE Qt6::Core Qt6::Widgets)
 
# Include parent directory for Plugvex.H and Settings.H
target_include_directories(VexCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(SyntaxCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(LookAndFeelCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
 
# Remove 'lib' prefix from plugin names
set_target_properties(VexCore SyntaxCore LookAndFeelCore PROPERTIES PREFIX "")
 
# On Windows and macOS, put plugins in the same directory as the executable
if(WIN32 OR APPLE)
    set_target_properties(VexCore SyntaxCore LookAndFeelCore PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
```
 
---
 
### Compiling with CMake
 
**Basic build process:**
 
```bash
# Clone the repository
git clone https://github.com/zynomon/vex.git
cd vex
 
# Create build directory
mkdir build
cd build
 
# Configure the build
cmake ..
 
# Compile
cmake --build .
 
# Optional: Install system-wide
sudo cmake --install .
```
 
**Build type options:**
 
```bash
# Debug build (includes debugging symbols)
cmake -DCMAKE_BUILD_TYPE=Debug ..
 
# Release build (optimized, recommended for daily use)
cmake -DCMAKE_BUILD_TYPE=Release ..
 
# Release with debug info (optimized but debuggable)
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
 
# Minimum size release (smallest binary)
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
```
 
**Custom install location:**
 
```bash
# Install to /opt/vex instead of /usr/local
cmake -DCMAKE_INSTALL_PREFIX=/opt/vex ..
cmake --build .
sudo cmake --install .
```
 
**Specify Qt location:**
 
If CMake can't find Qt automatically:
```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64 ..
```
 
**Platform-specific examples:**
 
**Linux:**
```bash
cmake ..
make -j$(nproc)
sudo make install
```
 
**Windows (MinGW):**
```cmd
cmake -G "MinGW Makefiles" ..
mingw32-make
mingw32-make install
```
 
**Windows (Visual Studio):**
```cmd
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
cmake --install .
```
 
**macOS:**
```bash
cmake ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```
 
**FreeBSD/OpenBSD:**
```bash
cmake ..
gmake -j$(sysctl -n hw.ncpu)
sudo gmake install
```
 
---
 
## Creating Distribution Packages
 
### Windows Packages
 
Vex includes `Dists/win/windows.bat` for automated Windows packaging.
 
**What the script does:**
 
1. Auto-detects installed Qt versions
2. Finds your build directory (supports multiple build types)
3. Copies the Vex executable and plugins
4. Runs `windeployqt` to bundle Qt dependencies
5. Generates installer graphics from SVG sources
6. Creates an NSIS installer package
 
**Prerequisites:**
 
- NSIS (Nullsoft Scriptable Install System)
  - Download from https://nsis.sourceforge.io
  - Default install location: `C:\Program Files\NSIS`
 
- ImageMagick (optional, for graphics generation)
  - Download from https://imagemagick.org
  - Converts SVG icons to BMP and ICO formats
 
**Running the script:**
 
```cmd
cd Dists\win
windows.bat
```
 
**What you'll see:**
 
```
[1/5] Detecting Qt installation...
  1: 6.6.0
  2: 6.7.0
Select version (1-2): 2
 
[2/5] Detecting build directory...
  1: ..\..\build\release
  2: ..\..\build\debug
Select build directory (1-2): 1
 
[3/5] Preparing staging directory...
[4/5] Generating installer graphics...
[5/5] Deploying and packaging...
 
Success! Vex_4.1_Setup.exe created
```
 
**Output:** `Vex_4.1_Setup.exe` in `Packages/` directory
 
**Manual packaging (without the script):**
 
```cmd
REM Set your Qt path
set QT_PATH=C:\Qt\6.6.0\mingw_64
 
REM Create staging directory
mkdir staging
cd staging
 
REM Copy executable and plugins
copy ..\build\vex.exe .
copy ..\build\VexCore.dll .
copy ..\build\SyntaxCore.dll .
copy ..\build\LookAndFeelCore.dll .
 
REM Deploy Qt dependencies
%QT_PATH%\bin\windeployqt.exe vex.exe
 
REM Create installer with NSIS
"C:\Program Files\NSIS\makensis.exe" ..\Dists\win\installer.nsi
```
 
---
 
### Linux and BSD Packages
 
Vex includes `Dists/build` for automated multi-format packaging on Unix systems.
 
**What the script does:**
 
1. Interactive build profile selection (Debug, Release, RelWithDebInfo, MinSizeRel)
2. Multi-target packaging (DEB, RPM, Arch, macOS, FreeBSD, TGZ, AppImage, or all)
3. Colorful terminal output with Nerd Font support
4. Parallel compilation using all CPU cores
5. Individual package script execution
6. Build summary with success/failure report
 
**Prerequisites:**
 
Distribution-specific package tools:
- **DEB:** `dpkg-deb` (pre-installed on Debian/Ubuntu)
- **RPM:** `rpmbuild` (`sudo dnf install rpm-build`)
- **Arch:** `makepkg` (pre-installed on Arch)
- **AppImage:** `linuxdeploy` and `linuxdeploy-plugin-qt`
- **TGZ:** `tar` (pre-installed everywhere)
- **FreeBSD:** `pkg` (pre-installed)
 
**Running the script:**
 
```bash
cd Dists
chmod +x build
./build
```
 
**Interactive mode:**
![dVWdCOC5rd_fast811b](https://github.com/user-attachments/assets/961b1751-4c8b-4b99-979f-3dcbc272b8f6)



**Non-interactive mode:**
 
```bash
# Build Release DEB package
./build --options 2 1
 
# Build Debug RPM package
./build --options 1 2
 
# Build all packages in Release mode
./build --options 2 8
```
 
**Clean build artifacts:**
 
```bash
./build --clean
```
 
**Output location:**
 
Packages are created in `Packages/` directory:
```
Packages/
├── vex_4.1_Cytoplasm_amd64.deb
├── vex_4.1_Cytoplasm_x86_64.rpm
├── vex_4.1_Cytoplasm_x86_64.AppImage
├── vex-4.1-Cytoplasm.tar.zst
└── vex-4.1-Cytoplasm-(FREEBSD).tar.zst
```
 
---
 
### Customizing Your Package
 
**Configuration file:** `Dists/CONF`
 
This file contains all package metadata:
 
```bash
#!/usr/bin/env sh
 
PKG_NAME="vex"
PKG_VERSION="4.1" 
PKG_RELEASE="Cytoplasm"
PKG_MAINTAINER="Zynomon Aelius <zynomon@proton.me>"
PKG_VENDOR="Zynomon Aelius"
PKG_SUMMARY="An extensive Qt text editor"
PKG_LICENSE="Apache 2.0"
PKG_URL="https://github.com/zynomon/vex"
 
# Debian/Ubuntu dependencies
DEB_DEPENDS="libc6, libqt6core6, libqt6gui6, libqt6widgets6"
 
# Fedora/RHEL dependencies
RPM_DEPENDS="qt6-qtbase, glibc"
 
# Arch dependencies
ARCH_DEPENDS="qt6-base"
 
# Build directories
BUILD_DIR="../build"
STAGE_DIR="${BUILD_DIR}/stage"
PKG_BUILD_BASE="${BUILD_DIR}/pkg"
PKG_OUT="../Packages"
 
# Package scripts directory
DBS_DIR="./DBS"
```
 
**Customizing for your fork:**
 
1. Edit `Dists/CONF`:
   ```bash
   PKG_NAME="your-editor-name"
   PKG_VERSION="1.0"
   PKG_RELEASE="YourReleaseName"
   PKG_MAINTAINER="Your Name <your@email.com>"
   ```
 
2. Replace icons in `Dists/`:
   - `vex.svg` (application icon)
   - `Dists/win/src/` (Windows installer graphics)
 
3. Update desktop file `Dists/vex.desktop`:
   ```ini
   [Desktop Entry]
   Name=Your Editor Name
   Comment=Your Description
   Exec=your-editor %F
   Icon=your-editor
   ```
 
4. Rebuild packages:
   ```bash
   cd Dists
   ./build
   ```
 
---
 
### Licensing and Distribution
 
**Qt Framework licensing:**
 
Vex uses Qt under the GNU Lesser General Public License v3 (LGPL v3).
 
**LGPL compliance checklist:**
 
✓ Dynamic linking to Qt (Vex complies)  
✓ Users can replace Qt libraries  
✓ No static linking of Qt  
✓ LGPL license notice included  
 
**Choosing a license for Vex:**
 
The original Vex uses Apache 2.0. For your own fork, consider:
 
**Apache 2.0 (Permissive):**
```
Copyright 2026 Zynomon Aelius
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
 
**GPL v3 (Copyleft):**
```
Vex Text Editor
Copyright (C) 2026 Zynomon Aelius
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
```
 
**MIT License (Permissive):**
```
MIT License
 
Copyright (c) 2026 Zynomon Aelius
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
```
 
**Third-party notices:**
 
Create `THIRD_PARTY_NOTICES.txt` in your repository:
 
```
Vex uses the following third-party software:
 
Qt Framework
  License: LGPL v3
  Website: https://www.qt.io/
  Copyright: The Qt Company Ltd.
  Source code: https://download.qt.io/archive/qt/
 
Compliance notes:
  - Vex dynamically links Qt libraries
  - Users may replace Qt libraries with compatible versions
  - Qt source code is available at the URL above
  - LGPL modifications (if any) are shared
```
 
---
