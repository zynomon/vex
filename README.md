##  ![vex](https://github.com/user-attachments/assets/3687d8ab-fc65-45a4-836d-4de590366d14) Vex <sub>4.1</sub>
![Tag](https://img.shields.io/github/v/tag/zynomon/vex?style=for-the-badge&labelColor=000000&color=0F3B0F)
![Contributors](https://img.shields.io/github/contributors/zynomon/vex?style=for-the-badge&labelColor=000000&color=0F3B0F)
![Last commit](https://img.shields.io/github/last-commit/zynomon/vex?style=for-the-badge&labelColor=000000&color=0F3B0F)
![Issues](https://img.shields.io/github/issues/zynomon/vex?style=for-the-badge&labelColor=000000&color=0F3B0F)
![Downloads](https://img.shields.io/github/downloads/zynomon/vex/total?style=for-the-badge&labelColor=000000&color=0F3B0F)

Vex is an extensive Free Crossplatform C++ Text Editor.
With a unique Plugin architecture, lightweight and faster experience in mind

<img width="904" height="649" alt="image" src="https://github.com/user-attachments/assets/b07a1267-cf5d-4fbf-9914-271edf7207fd" />

> *if this looks ugly to you , you can theme it, every aspect is themable.*

---

# Download
Current version is 4.1 ( Cytoplasm )
| Platform | Download |
|----------|----------|
| [![Windows](https://custom-icon-badges.demolab.com/badge/-0078D6?logo=windows11&logoColor=white)](#) Windows Installer | [`Vex_4.1_Setup.exe`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/Vex_4.1_Setup.exe) (16.1 MB) |
| [![Windows](https://custom-icon-badges.demolab.com/badge/-3C873A?logo=windows11&logoColor=white)](#) Windows Zstd 7z | [`vex_4.1_Cytoplasm_WIN32.7z`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_WIN32.7z) (10.8 MB) |
| [![Linux](https://img.shields.io/badge/-000000?logo=appimage&logoColor=white&label=)](#) AppImage ( Linux Universal ) | [`vex_4.1_Cytoplasm_x86_64.AppImage`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.AppImage) (3.94 MB) |
| [![Debian](https://img.shields.io/badge/-A81D33?logo=debian&logoColor=white&label=)](#) Debian/Ubuntu (.deb) | [`vex_4.1_Cytoplasm_x86_64.deb`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.deb) (134 KB) |
| [![RPM](https://img.shields.io/badge/-FC1E2C?logo=speedtest&logoColor=white&label=)](#) Fedora/RHEL (.rpm) | [`vex_4.1_Cytoplasm_x86_64.rpm`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_x86_64.rpm) (213 KB) |
| [![FreeBSD](https://img.shields.io/badge/-AB2B28?logo=freebsd&logoColor=white&label=)](#) FreeBSD Package | [`vex-4.1_Cytoplasm.pkg`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex-4.1_Cytoplasm.pkg) (546 B) |
| [![FreeBSD](https://img.shields.io/badge/-3C873A?logo=freebsd&logoColor=white&label=)](#) FreeBSD Zstd Tarball | [`vex_4.1_Cytoplasm_FREEBSD.tar.zst`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm_FREEBSD.tar.zst) (193 KB) |
| [![Source](https://img.shields.io/badge/-3C873A?logo=sourceforge&logoColor=white&label=)](#) GNU/Linux Zstd Tarball | [`vex_4.1_Cytoplasm.tar.zst`](https://github.com/zynomon/vex/releases/download/4.1-cytoplasm/vex_4.1_Cytoplasm.tar.zst) (145 KB) |

# For Installing check:

https://github.com/zynomon/vex/wiki/Install
Its avilable for windows, FreeBSD, And any Linux Distro ( APPIMAGE +  DEB + RPM + TARGZ ),
Except Mac for now.

# Features

## Find & Replace
<img width="529" height="331" alt="image" src="https://github.com/user-attachments/assets/342ab0ba-c9e0-46ea-a6b7-440fa2c0ee71" />



Case-sensitive search, Whole word matching , Replace one or all Search wraps around document
Keybind : ``CRTL+F``

## Open by name
open by name gives the feeling of 
```bash
 ~/.bashrc # this opens a file named  bashrc in  vi 
``` 
if file doesnt exists it creates it
you  have to specify the path of which file you want to open.
it has  auto-completion to help reduce typos. 


![nJCtlZBHQZ_fast723b](https://github.com/user-attachments/assets/93bcefbc-4ba9-4921-8c27-d98a79ea6ae4)

- Keybind : ``CRTL+O``


## Syntax engine
![5Rc7AE8sOX_fast728b](https://github.com/user-attachments/assets/662bfcc2-7e51-4a74-9c98-f631d66a791b)




Vex uses an independent syntax definition Language for defining syntax, its made for coloring keywords heriodics and delimiters are supported.
written Entirely in c++ check 
https://github.com/zynomon/vex/wiki/Syntax 
to know more
## Plugin system
instead of like normal application   click and run , we used plugin system


```mermaid
graph TD
  direction TB
  A[Initialize] --> C{Vex Main Binary}
  
  C --> |Takes less than 1 second in prior| X[CMD Register]
  C --> |Plugins?| D[Xylem - High Importance]
  
  D --> |Then run| E[Phloem - Medium Importance]
  E --> |Then run| Y[Simple - Normal]
  
  Y --> X
  X --> e(Run)
```

# Build

Vex uses Qt framework so surely its possible to be compiled in any linux distro , MAC, newer windows versions ( VISTA ONWARDS 11 ), 
vex doesnt relies on any other libraries making the build system take lesser space Than an entire chromium engine, 
https://github.com/zynomon/vex/wiki/Compile

# Fork
Vex is an opensource project its easy to be forked with its Build system 
<img width="814" height="274" alt="6u2XrHz6MQ_fast701b" src="https://github.com/user-attachments/assets/a4bd52af-25cf-45f5-8d90-e5312117e891" />
For forking check [ https://github.com/zynomon/vex/wiki/Compile](https://github.com/zynomon/vex/wiki/Compile#creating-distribution-packages)


Script and Batch files were made to save times  instead of wasting on building for other oses
Scripts are capable of creating a package

