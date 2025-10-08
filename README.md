Here’s a refined version of your Vex README section with improved formatting, icon usage, and removal of Windows references. I’ve also polished the language for clarity and impact while keeping your modular, cinematic vibe intact:



---



\# ⚡ Vex



<div align="center">

&nbsp; <img src="https://github.com/user-attachments/assets/ff3216c1-7818-4b69-aab9-f170261a632a" alt="Vex Screenshot 1" width="40%" />

&nbsp; <img src="https://github.com/user-attachments/assets/54a4b958-a9d4-454d-93bd-76bd14edd845" alt="Vex Screenshot 2" width="30%" />

&nbsp; <img src="https://github.com/user-attachments/assets/c7986877-23f4-4f09-a67c-780a080e4171" alt="Vex Screenshot 3" width="35%" />

&nbsp; <img src="https://github.com/user-attachments/assets/cf94841e-2771-4227-ac24-e93f4b9778d9" alt="Vex Screenshot 4" width="35%" />

&nbsp; <img src="https://github.com/user-attachments/assets/eafdde0c-73de-45f0-b358-c43cbff2b6b2" alt="Vex Screenshot 5" width="35%" />

&nbsp; <img src="https://github.com/user-attachments/assets/76095a1f-df6e-4974-a3ca-f8b5f8f08019" alt="Vex Screenshot 6" width="31%" />

&nbsp; <img src="https://github.com/user-attachments/assets/c7ba566c-8321-42cb-9b5b-2a22ddd4d01a" alt="Vex Screenshot 7" width="32%" />

</div>



<div align="center">

&nbsp; <strong>Vex is a fast, Vim-inspired text editor built with Qt — designed for developers who crave control, clarity, and modular extensibility.</strong>

&nbsp; <br>

&nbsp; <sub>Tabbed editing, syntax highlighting, plugin support, theme customization, and more — all built-in.</sub>

&nbsp; <br><br>

&nbsp; <img src="https://img.shields.io/github/license/zynomon/vex?style=flat-square" alt="License" />

&nbsp; <img src="https://img.shields.io/badge/build-stable-brightgreen?style=flat-square" alt="Build Status" />

&nbsp; <img src="https://img.shields.io/badge/platform-Linux-blue?style=flat-square" alt="Platform" />

</div>



---



<p align="center">

&nbsp; <img src="https://github.com/zynomon/vex/raw/main/build/vex-demo.gif" alt="Vex demo GIF" width="70%" />

</p>



\## 📦 Installation Guide



\### 🗃️ Debian Stable (.deb)



Install Vex using the prebuilt `.deb` package:



```bash

wget https://github.com/zynomon/vex/raw/main/build/vex-1.0.0-Linux.deb

sudo apt install ./vex-1.0.0-Linux.deb

```



\### 👷 Build from Source



```bash

git clone https://github.com/zynomon/vex.git

cd vex

mkdir build \&\& cd build

cmake ..

make -j$(nproc)

./vex

```



\### 🔧 Requirements



\- 🧰 \*\*Qt\*\* 5.15+ or Qt 6

\- 🛠️ \*\*CMake\*\* 3.16+

\- 🧪 \*\*Compiler:\*\* GCC or Clang

\- 🐧 \*\*Platform:\*\* Linux (.deb)



> 💡 Tip: Use `-DCMAKE\_BUILD\_TYPE=Release` for optimized builds.



---



\## ✨ Features



\- 🧙‍♂️ \*\*Vim Mode\*\* – Modal editing with familiar keybindings (`hjkl`, `x`, `dd`, `:`)

\- 🗂️ \*\*Tabbed Interface\*\* – Closable, movable tabs for multitasking

\- 🎨 \*\*Syntax Highlighting\*\* – Powered by `QSyntaxHighlighter`

\- 🔍 \*\*Find \& Replace\*\* – Case-sensitive, whole-word, directional search

\- 🔄 \*\*File Watcher\*\* – Auto-reload on external changes

\- 🔌 \*\*Plugin System\*\* – Load `.so` plugins via `QPluginLoader`

\- 🖌️ \*\*Theme Editor\*\* – Live QSS editing and saving

\- 🔤 \*\*Font Customization\*\* – Choose your favorite editor font

\- 🔢 \*\*Smart Line Numbers\*\* – Dynamic width, color-coded

\- 📍 \*\*Cursor Tracking\*\* – Real-time mode and position updates

\- 🔐 \*\*Permission Handling\*\* – Auto-escalation in terminal if access is denied



---



\## 🧩 Plugin Development



Extend Vex with modular plugins using a clean interface:



```cpp

class VexPluginInterface {

public:

&nbsp;   virtual ~VexPluginInterface() = default;

&nbsp;   virtual QString name() const = 0;

&nbsp;   virtual void initialize(VexEditor \*editor) = 0;

&nbsp;   virtual void cleanup() {}

};

```



\- Place plugins in the `plugins/` directory

\- Use `QPluginLoader` and `Q\_DECLARE\_INTERFACE`

\- See `plugins/sample\_plugin/` for a starter template



---



\## 🎨 Theme Customization



\- Built-in theme editor tab

\- QSS-based styling

\- Switch between system styles (Fusion, Breeze, etc.)

\- Save and reload themes instantly



---



\## 🛠️ Contributing



We welcome clean, modular contributions:



\- 🧩 New plugins (UI, syntax, preview)

\- 🐞 Bug fixes and performance tweaks

\- 🎨 Theme packs and font presets



\*\*Guidelines:\*\*



\- Follow the established C++/Qt style

\- Keep plugins isolated and well documented

\- Submit PRs with descriptive commit messages



---



\## 📄 License



Apache-2.0 © \[Zynomon](https://github.com/zynomon)



---



Let me know if you want to modularize this further into sections like “Developer Docs,” “Plugin API,” or “Theme Packs.” I can also help scaffold a CONTRIBUTING.md or plugin manifest template.

