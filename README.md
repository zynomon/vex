##  ![vex](https://github.com/user-attachments/assets/3687d8ab-fc65-45a4-836d-4de590366d14) Vex
is an extensive Free Crossplatform Qt C++ Text Editor.
With a unique architecture and lightweight and faster experience in mind

---

# Download
Current version is 4.1 ( Cytoplasm )
for download guide check

https://github.com/zynomon/vex/wiki/Download
https://github.com/zynomon/vex/wiki/Install
Its avilable for windows, FreeBSD, And any Linux Distro ( APPIMAGE +  DEB + RPM + TARGZ ),



# Features

## Find & Replace
<img width="529" height="331" alt="image" src="https://github.com/user-attachments/assets/342ab0ba-c9e0-46ea-a6b7-440fa2c0ee71" />
Case-sensitive search, Whole word matching , Replace one or all Search wraps around document
Keybind : ``CRTL+F``

## Open by name
open by name gives the feeling of ``vi ~/.bashrc`` if file doesnt exists it creates it you  have to specify the path of which file you want to open although its not that advanced 
![nJCtlZBHQZ_fast723b](https://github.com/user-attachments/assets/93bcefbc-4ba9-4921-8c27-d98a79ea6ae4)
Keybind : ``CRTL+O``


## Syntax engine
![5Rc7AE8sOX_fast728b](https://github.com/user-attachments/assets/662bfcc2-7e51-4a74-9c98-f631d66a791b)
Vex uses an independent syntax definition Language for defining syntax, its made for coloring keywords heriodics and delimiters are supported.
written Entirely in c++

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
