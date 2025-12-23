Based on your project and files, here’s a **clean, professional README.md** you can use for your Vex editor repository:

---

# Vex — A Lightweight Vim-Inspired Text Editor

![Vex Screenshot](https://github.com/user-attachments/assets/ff3216c1-7818-4b69-aab9-f170261a632a)

**Vex** is a fast, m text editor built with Qt and inspired by Vim keybindings. Designed for developers who want speed without clutter, 
![Beta](https://img.shields.io/badge/Beta-yellow?style=flat&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiIgdmlld0JveD0iMCAwIDE2IDE2Ij48cGF0aCBkPSJNOCAxLjVDMTMuNTIgMS41IDE4IDUuOTggMTggMTEuNSAxOCAxNy4wMiAxMy41MiAyMS41IDggMjEuNSAyLjQ4IDIxLjUgLTIgMTcuMDIgLTIgMTEuNSAtMiA1Ljk4IDIuNDggMS41IDggMS41em0wIDEuNEM0LjIzIDIuOSAwIDYuMjMgMCAxMC41YzAgNC4yNyA0LjIzIDcuNiA4IDcuNiA0Ljc3IDAgOCAzLjMzIDggNy42IDAgLTQuMjcgLTMuMjMtNy42IC04LTcuNnoiLz48L3N2Zz4%3D)Vex supports syntax highlighting ( custom in DSL ) ,
Vi mode,
theming,
plugins,
and more—all while staying lightweight. basically less line more work just how a string can express many thing but not the word string can 

---

## ✨ Features

- **Vim-style navigation & editing** (`h/j/k/l`, `i/a`, `o`, `dd`, `yy`, etc.)
- **Syntax highlighting** via custom `.vex` rules system
- **Drag-and-drop file support**
- **Admin-mode editing** for system files (using `sudo` + terminal)
- **Find & Replace** with regex-like options
- **Custom themes** (QSS-based) with live preview
- **Plugin system** (via `.so` Qt plugins)
- **File watcher** for external changes
- **Multi-tab interface** with unsaved-modified indicators
- **Recent files menu** & **path-based file opener**
- **Binary detection** to avoid corrupting non-text files
<img width="996" height="757" alt="image" src="https://github.com/user-attachments/assets/386f856f-bf6d-4e57-b9b0-6e9aa775445e" />
<img width="347" height="288" alt="image" src="https://github.com/user-attachments/assets/09006432-79bc-4428-8115-289427bad917" />
<img width="743" height="578" alt="image" src="https://github.com/user-attachments/assets/d564e3bc-e740-49a4-96f0-460919208ebc" />
<img width="1004" height="724" alt="image" src="https://github.com/user-attachments/assets/2e148473-2804-4138-83b9-6658451d1365" />
( the white line below applycation is provided by open box so not get confused ) 
---

## 📦 Building Mint Leaf 3.0

Vex is built with C++ and Qt 6. To install from source:


```bash
git clone https://github.com/zynomon/vex.git
cd vex
qmake vex.pro
make -j$(nproc)
sudo make install  # optional
```
Make sure to have Qtcore and Qtwidgets if no cmake would fail be careful,

Install using terminal in DEBIAN?/UBUNTU based linux distro
copy our   own repository ( gpg signed )
well by our I'm confused it's only me haha

---

## 🚀 Usage

Launch Vex from terminal:

```bash
vex                 # empty editor
vex /path/to/file   # open specific file
```

### Key Shortcuts

| Action                  | Shortcut            |
|------------------------|---------------------|
| New file               | `Ctrl+N`            |
| Open file (GUI)        | `Ctrl+Shift+O`      |
| Open file (by path)    | `Ctrl+O`            |
| Save                   | `Ctrl+S` or `:w`    |
| Save As                | `Ctrl+Shift+S`      |
| Find & Replace         | `Ctrl+F`            |
| Toggle Vim Mode        | `Esc` or `View → Vim Mode` |
| Quit                   | `:q`, `:wq`, or `Ctrl+Q` |

In **Vi mode**: `h/j/k/l`, `i`, `a`, `x`, `dd`, `yy`, `o`, `w`, `b`, `:w`, `:q`, etc.

---

## 🎨 Theming & Syntax

- Themes are stored as `~/.config/VexEditor/theme.qss`
- Syntax rules are in `~/.config/VexEditor/syntax.vex`
- Edit both live via **View → Theme Editor** or **Edit Syntax Rules...**

Example syntax rule:
```txt
X-REG "bash";
File = .sh && .bash
Sw = +ES'#!/bin/bash'ES-
+ES'if'ES- && +ES'fi'ES- = color:#FF6B6B = font:B
+HS'"'HS- to +HE'"'HE- = color:#98D8C8
```

---

## 🧩 Plugins

Place `.so` Qt plugin files in:
```
~/.config/VexEditor/plugins/
```
Then use **View → Extensions → Reload Plugins**.

Plugins must implement the `VexPluginInterface`.

---

## 📄 License

Licensed under the **Apache License 2.0**  
© [Zynomon](https://github.com/zynomon)

---


Want to contribute or report a bug? Open an [Issue](https://github.com/zynomon/vex/issues) or submit a PR!
WE will, ohh i mean i will surely read it when i have time right now i got some bugs to fix too the reason behind this version is boost out error.os development rate before 2026

--- 

*(Project Status: **v3.0 "Mint Leaf"** — LTS until May 1, 2026)*

---
