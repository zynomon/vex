---
title: "Install"
description: "How to download and install Vex 4.1 on Windows, Linux, and FreeBSD"
icon: "/wiki/4.1/headericon.svg"
image: "/thumb.png"

layout: default
title: "Install"
---

# Downloading Vex
 
To download Vex, check the [latest release](https://github.com/zynomon/vex/releases/latest). Here's a quick reference chart for all versions:
 
| Version | Package Links |
|---------|---------------|
| `4.1` (Cytoplasm) | [![Windows](https://custom-icon-badges.demolab.com/badge/.exe-0078D6?style=flat-square&logo=windows11&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/Vex_4.1_Setup.exe) [![.7z](https://img.shields.io/badge/.7z-3C873A?style=flat-square&logo=files&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_WIN32.7z) [![.AppImage](https://img.shields.io/badge/.AppImage-000000?style=flat-square&logo=appimage&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.AppImage) [![.deb](https://img.shields.io/badge/.deb-A81D33?style=flat-square&logo=debian&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.deb) [![.rpm](https://img.shields.io/badge/.rpm-FC1E2C?style=flat-square&logo=redhat&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.rpm) [![.pkg](https://img.shields.io/badge/.pkg-AB2B28?style=flat-square&logo=freebsd&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex-4.1_Cytoplasm.pkg) [![.tar.zst](https://img.shields.io/badge/.tar.zst-2C8EBB?style=flat-square&logo=gnu&logoColor=white)](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm.tar.zst) [![Source](https://img.shields.io/badge/Source-000000?style=flat-square&logo=github&logoColor=white)](https://github.com/zynomon/vex/commit/5729205c1472ec2a55f57ba035c0a0cf054700cd) |
| `4.0` (Cytoplasm) | [![.deb](https://img.shields.io/badge/.deb-A81D33?style=flat-square&logo=debian&logoColor=white)](https://github.com/zynomon/vex/raw/e0fbae142d0092eea36b10081606f2c7b46dcb02/build/vex.deb) [![.7z](https://img.shields.io/badge/.7z-3C873A?style=flat-square&logo=files&logoColor=white)](https://github.com/zynomon/vex/raw/e0fbae142d0092eea36b10081606f2c7b46dcb02/vex.7z) [![Source](https://img.shields.io/badge/Source-000000?style=flat-square&logo=github&logoColor=white)](https://github.com/zynomon/vex/commit/e0fbae142d0092eea36b10081606f2c7b46dcb02) |
| `3.0` (Mint Leaf) | [![.deb](https://img.shields.io/badge/.deb-A81D33?style=flat-square&logo=debian&logoColor=white)](https://github.com/zynomon/vex/raw/4a1e397532fc329fc7bf0ac85a39f3b66bb16c21/build/vex.deb) [![Source](https://img.shields.io/badge/Source-000000?style=flat-square&logo=github&logoColor=white)](https://github.com/zynomon/vex/commit/4a1e397532fc329fc7bf0ac85a39f3b66bb16c21) |
| `1.0` (Emerald) | [![.deb](https://img.shields.io/badge/.deb-A81D33?style=flat-square&logo=debian&logoColor=white)](https://github.com/zynomon/vex/blob/cb60d5b52507199841cb2fd31099b3510b8aa376/build/vex-1.0.0-Linux.deb) [![Source](https://img.shields.io/badge/Source-000000?style=flat-square&logo=github&logoColor=white)](https://github.com/zynomon/vex/commit/6cdf591de5541d8d5c4604d38fa1ffb48818aaa0) |
 
> **Note:** This chart may be incomplete or outdated. Always check the [README](https://github.com/zynomon/vex#readme) or [Releases](https://github.com/zynomon/vex/releases) page as your first priority.
 
**Important for FreeBSD users:** Use the `FREEBSD.tar.zst` package instead of the `.pkg` file for better reliability.
 
### SmartScreen Warning on Windows
 
On Windows or some web browsers, you might see this warning:
 
<img width="360" height="598" alt="image" src="https://github.com/user-attachments/assets/29d894e1-d7df-48fd-9b48-756c65efae5b" />
<img width="388" height="184" alt="image" src="https://github.com/user-attachments/assets/f8db6640-8aba-419d-b5dd-bde290826f22" /> 
 
This is especially common in Chromium-based browsers. Who knows how they verify with SmartScreen...
 
**Solution:** Just click "Keep" and then "Keep anyway" to proceed with the download.
 
---
 
# Table of Contents 
 
- [Windows](#-windows)
   - [Windows Installer](#-windows-installer)
   - [Windows Zip (7z)](#-windows-zstd-7z)
- [Linux](#-linux)
   - [Debian/Ubuntu/Mint/DEB Based](#-debian)
   - [Fedora/CentOS/RPM Based](#-rhel)
   - [AppImage (GNU/Linux Universal)](#-appimage)
   - [Linux Tarball (GNU/Linux Universal)](#-linux-tarball)
- [FreeBSD](#-freebsd)
- [Other OSes](#other-oses)
 
--- 
 
# ![Windows](https://custom-icon-badges.demolab.com/badge/-000000?style=for-the-badge&logo=windows11&logoColor=white) Windows
 
Windows support for Vex started from version 4.0 onwards. There are two installation options available:
 
## [![Windows](https://custom-icon-badges.demolab.com/badge/-0078D6?logo=windows11&logoColor=white)](#) Windows Installer
 
Let's assume you've downloaded [Vex_4.1_Setup.exe](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/Vex_4.1_Setup.exe) to your Downloads folder.
 
<img width="221" height="157" alt="image" src="https://github.com/user-attachments/assets/f084fba0-b89c-4bb2-9b3a-22c701a305dd" />
 
Double-click the installer to begin.
 
### UAC Prompt
 
Click **Yes** instead of No (obviously).
 
### Step 1: Welcome Screen
 
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/3d65dc1d-4c94-4272-b257-831bc6610a13" />
 
Click **Next**.
 
### Step 2: License Agreement
 
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/817745ce-4aaf-4d5d-8583-2c90238dff80" />
 
Click **I Agree**. You're agreeing that Vex was made by its maintainer, not you. Pretty logical, right?
 
### Step 3: Installation Options
 
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/1ae5e3dd-80bf-4010-ba4d-d7f04186e0bf" />
 
Keep **"Add Vex to 'Open With'"** checked. This is highly recommended for easy file associations.
 
### Step 4: Installation Scope
 
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/669e9f3d-b09c-4492-8072-be76a9d792da" />
 
If you're running Windows, you most likely have one user, or you don't even know what users are in Windows. It doesn't matter much, but click **"User"** (recommended option).
 
### Step 5 & 6: Installation Progress
 
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/a8904068-62ac-4e18-8bfe-03f654717364" />
<img width="499" height="388" alt="image" src="https://github.com/user-attachments/assets/76fd6367-9989-47fa-9041-461492b058d1" />
 
These steps are self-explanatory. Just wait for the installation to complete.
 
### Launching Vex
 
Press the **Win** key and type "vex". You'll see Vex in your Start menu:
 
<img width="553" height="211" alt="image" src="https://github.com/user-attachments/assets/9421bc9d-fb4f-4c7d-8830-d3232e6e2ea1" /> 
 
**Troubleshooting:** If you run into any issues, check the [Known Issues](https://github.com/zynomon/vex/wiki/Known_issues) page.
 
## [![Windows](https://custom-icon-badges.demolab.com/badge/-3C873A?logo=windows11&logoColor=white)](#) Windows Zstd 7z
 
We provide a Windows 7z archive for using Vex without installing it (portable version).
 
### Extracting the Archive
 
For extracting Zstd-compressed archives on Windows, we recommend:
- **7-Zip** ([download here](https://www.7-zip.org/))
- **NanaZip** (modern 7-Zip alternative from Microsoft Store)
 
Extract the archive somewhere safe (e.g., `C:\Programs\Vex` or your Desktop).
 
### Running Vex
 
1. Navigate to the extracted folder
2. Double-click `vex.exe`
 
**Important:** Make sure `vex.exe` stays in the same folder with all the DLL files, or it won't run and will show DLL errors.
 
---
 
# ![Linux](https://img.shields.io/badge/-000000?style=for-the-badge&logo=linux&logoColor=black) Linux
 
Vex has supported Linux from the beginning. It later evolved into a cross-platform text editor.
 
## [![Debian](https://img.shields.io/badge/-A81D33?logo=debian&logoColor=white&label=)](#) Debian
 
Ubuntu, Linux Mint, Zorin OS, and pretty much any "user-friendly" distro is likely a Debian derivative. This means the source code comes from Debian GNU/Linux, and the rest is customized by the OS maintainers. They all share the same kernel, same libc, and the same package manager (APT).
 
Just download the `.deb` package and install it via terminal (terminal is faster than opening software stores).
 
### Installation Steps
 
Open your terminal and run:
 
**Download:**
```bash
wget https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.deb
```
 
**Install:**
```bash
sudo apt install ./vex_4.1_Cytoplasm_x86_64.deb
``` 
 
![Installation demo](https://github.com/user-attachments/assets/fa0f1eb0-98e2-447e-8e5a-2e566d2acf57)
 
**Launch:**
```bash
vex
```
 
## [![RPM](https://img.shields.io/badge/-FC1E2C?logo=speedtest&logoColor=white&label=)](#) RHEL
 
RHEL stands for Red Hat Enterprise Linux. Fedora, CentOS, and Nobara all share the same package manager called RPM (it's quite a remarkable package manager).
 
### Installation Steps
 
Open your terminal and run:
 
**Download:**
```bash
wget https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.rpm
```
 
**Install:**
```bash
sudo rpm -i vex_4.1_Cytoplasm_x86_64.rpm 
```
 
**Launch:**
```bash
vex
```
 
## [![Linux](https://img.shields.io/badge/-000000?logo=appimage&logoColor=white&label=)](#) AppImage
 
AppImage is a portable, universal Linux format. You don't have to install Qt libraries explicitly because everything is bundled. This is perfect for beginners and Linux distributions with package managers not covered here.
 
### Prerequisites
 
To run `.AppImage` files, you need the FUSE library:
 
**Ubuntu 22.04+ / Debian 12+**
```bash
sudo apt install libfuse2
```
 
**Fedora**
```bash
sudo dnf install fuse
```
 
**Arch / Manjaro**
```bash
sudo pacman -S fuse2
```
 
**openSUSE**
```bash
sudo zypper install fuse
``` 
 
**Void (glibc)**
```bash
sudo xbps-install fuse
``` 
 
### Running the AppImage
 
```bash
# 1. Make the file executable
chmod +x vex_4.1_Cytoplasm_x86_64.AppImage 
 
# 2. Run the application
./vex_4.1_Cytoplasm_x86_64.AppImage
``` 
 
**Tip:** You can move the AppImage to `~/Applications` or `/opt` for easier access.
 
## [![Source](https://img.shields.io/badge/-3C873A?logo=sourceforge&logoColor=white&label=)](#) Linux Tarball
 
If you want to install Vex system-wide without compiling manually, the Linux Tarball is another great option.
 
Download the Linux tarball from the releases page. Here's the file structure you'll find:
 
```
.
├── bin
│   └── vex                    # Main binary
├── lib
│   └── vex                    # Plugins directory
│       ├── LookAndFeelCore.so
│       ├── SyntaxCore.so
│       └── VexCore.so 
├── README.txt
└── share
    ├── applications
    │   └── vex.desktop        # Desktop entry
    └── icons
        └── hicolor
            └── scalable
                └── apps   
                    └── vex.svg # Application icon
 
10 directories, 7 files
```
 
### Installation Steps
 
As you can see from the structure above, if you remove `README.txt` and copy the contents globally, Vex will be installed on your system.
 
**Quick install:**
```bash
# Extract the archive
tar -xf vex_4.1_Cytoplasm.tar.zst
 
# Navigate into the extracted directory
cd vex_4.1_Cytoplasm
 
# Copy files to system directories (requires root)
sudo cp -r bin/* /usr/local/bin/
sudo cp -r lib/* /usr/local/lib/
sudo cp -r share/* /usr/local/share/
 
# Update desktop database
sudo update-desktop-database
```
 
**Launch:**
```bash
vex
```
 
**Note:** This is only for GNU/Linux with glibc. For musl or other libc implementations, consider compiling from source or waiting for the next version with more definitive packages.
 
---

# ![FreeBSD](https://img.shields.io/badge/-6d1b19?style=for-the-badge&logo=freebsd&logoColor=white) FreeBSD
 
Vex has a compiled package for FreeBSD, but the `.pkg` file is mainly metadata. For a reliable installation, we recommend using the `FreeBSD.tar.zst` archive.
 
### Download
 
```bash
fetch https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_FREEBSD.tar.zst
```
 
### File Structure
 
The FreeBSD package contains:
 
```
vex-4.1-Cytoplasm-(FREEBSD).tar.zst
├── LookAndFeelCore.so    # Shared Library (137.1 KiB)
├── SyntaxCore.so          # Shared Library (153.1 KiB)
├── vex                    # Main executable (94.5 KiB)
├── vex.desktop            # Desktop entry (1.0 KiB)
├── vex.svg                # Application icon (5.0 KiB)
└── VexCore.so             # Shared Library (323.5 KiB)
```
 
### Installation Methods
 
You have two options: system-wide installation or user-specific installation.
 
---
 
#### Option 1: System-Wide Installation (Recommended)
 
This installs Vex to `/opt/vex/` and keeps the binary and libraries in the same directory.
 
```bash
# 1. Extract the archive
tar -xf vex_4.1_Cytoplasm_FREEBSD.tar.zst
 
# 2. Create the installation directory
sudo mkdir -p /opt/vex
 
# 3. Copy all files to /opt/vex
cd vex-4.1-Cytoplasm
sudo cp vex *.so /opt/vex/
 
# 4. Create a symlink to make vex accessible system-wide
sudo ln -s /opt/vex/vex /usr/local/bin/vex
 
# 5. Install desktop entry and icon
sudo cp vex.desktop /usr/local/share/applications/
sudo mkdir -p /usr/local/share/icons/hicolor/scalable/apps
sudo cp vex.svg /usr/local/share/icons/hicolor/scalable/apps/
```
 
**Why this method?** Keeping the binary and libraries together in `/opt/vex/` ensures Vex can find its plugins without issues.
 
---
 
#### Option 2: User Installation (No Root Required)
 
This installs Vex to your home directory at `~/.local/`.
 
```bash
# 1. Extract the archive
tar -xf vex_4.1_Cytoplasm_FREEBSD.tar.zst
 
# 2. Create necessary directories
mkdir -p ~/.local/lib/vex
mkdir -p ~/.local/bin
mkdir -p ~/.local/share/applications
mkdir -p ~/.local/share/icons/hicolor/scalable/apps
 
# 3. Copy files to appropriate locations
cd vex-4.1-Cytoplasm
cp vex ~/.local/bin/
cp *.so ~/.local/lib/vex/
cp vex.desktop ~/.local/share/applications/
cp vex.svg ~/.local/share/icons/hicolor/scalable/apps/
 
# 4. Ensure ~/.local/bin is in your PATH
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.profile
source ~/.profile
```
 
**Note:** With this method, make sure the vex binary can find the libraries in `~/.local/lib/vex/`. You may need to set `LD_LIBRARY_PATH`:
 
```bash
echo 'export LD_LIBRARY_PATH="$HOME/.local/lib/vex:$LD_LIBRARY_PATH"' >> ~/.profile
source ~/.profile
```
 
---
 
### Launch
 
```bash
vex
```
 
Or launch it from your application menu.
 
---
 
# Other OSes
 
**macOS Support:** Vex is not currently available for macOS. We're working on it for future releases. but you can compile vex for macos
 
**Other Unix-like Systems:** If you're using a Unix-like system not covered here (Solaris, Haiku, etc.), try compiling from source using the instructions in our [Compile Guide](https://github.com/zynomon/vex/wiki/Compile).
 
---
 
## Need Help?
 
- **Installation Issues:** Check our [Known Issues](https://github.com/zynomon/vex/wiki/Known_issues) page
- **Compilation:** See the [Compile Guide](https://github.com/zynomon/vex/wiki/Compile)
- **General Help:** Visit the [Wiki](https://github.com/zynomon/vex/wiki)
- **Report Bugs:** Open an issue on [GitHub](https://github.com/zynomon/vex/issues)