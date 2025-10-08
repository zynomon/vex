# Vex

<div align="center">
  <img src="https://github.com/user-attachments/assets/ff3216c1-7818-4b69-aab9-f170261a632a" alt="Vex Screenshot 1" width="40%" />
  <img src="https://github.com/user-attachments/assets/54a4b958-a9d4-454d-93bd-76bd14edd845" alt="Vex Screenshot 2" width="30%" />
  <img src="https://github.com/user-attachments/assets/c7986877-23f4-4f09-a67c-780a080e4171" alt="Vex Screenshot 3" width="35%" />
  <img src="https://github.com/user-attachments/assets/cf94841e-2771-4227-ac24-e93f4b9778d9" alt="Vex Screenshot 4" width="35%" />
  <img src="https://github.com/user-attachments/assets/eafdde0c-73de-45f0-b358-c43cbff2b6b2" alt="Vex Screenshot 5" width="35%" />
  <img src="https://github.com/user-attachments/assets/76095a1f-df6e-4974-a3ca-f8b5f8f08019" alt="Vex Screenshot 6" width="31%" />
  <img src="https://github.com/user-attachments/assets/c7ba566c-8321-42cb-9b5b-2a22ddd4d01a" alt="Vex Screenshot 7" width="32%" />
</div>

<div align="center">
  <b>Vex is a fast, Vim-inspired text editor built with Qt—crafted for developers who want control, clarity, and extensibility.</b>
  <br>
  <sub>Tabbed editing, syntax highlighting, plugin support, theme customization, and more — all built-in.</sub>
  <br><br>
  <img src="https://img.shields.io/github/license/zynomon/vex?style=flat-square" alt="License" />
  <img src="https://img.shields.io/badge/build-stable-brightgreen?style=flat-square" alt="Build Status" />
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20Windows-blue?style=flat-square" alt="Platform" />
</div>

---

<p align="center">
  <img src="https://github.com/zynomon/vex/raw/main/build/vex-demo.gif" alt="Vex demo GIF" width="70%" />
</p>

###                              Installation guides
---
#### for Debian Stable (.deb) 🗃️

You can install Vex on Debian Stable using the prebuilt `.deb` package:

```bash
wget https://github.com/zynomon/vex/raw/main/build/vex-1.0.0-Linux.deb
sudo apt install ./vex-0.0.1.deb
```

### or just build 👷‍♂️

```bash
git clone https://github.com/zynomon/vex.git
cd vex
mkdir build && cd build
cmake ..
make -j$(nproc)
./vex
```
### Requirements

- **Qt** 5.15+ or Qt 6
- **CMake** 3.16+
- **Compiler:** GCC / Clang / MSVC
- **Platform:** Linux (.deb)
-# very advanced : you can rebuild to improve the operating system support


> 💡 **Tip:** Use `-DCMAKE_BUILD_TYPE=Release` for optimized builds.

---

## ✨ Features

- **Vim Mode** – Modal editing with familiar keybindings (`hjkl`, `x`, `dd`, `:`, etc.)
- **Tabbed Interface** – Work on multiple documents with closable, movable tabs.
- **Syntax Highlighting** – Powered by `QSyntaxHighlighter` for C++ keywords, comments, and strings.
- **Find & Replace** – Case-sensitive, whole-word, directional search with replace-all.
- **File Watcher** – Automatic reload when files change externally.
- **Plugin System** – Load custom `.so`/`.dll` plugins via `QPluginLoader`.
- **Theme Editor** – Live-edit and save custom QSS themes.
- **Font Customization** – Choose your favorite editor font.
- **Smart Line Numbers** – Dynamic width and color-coded display.
- **Cursor Tracking** – Real-time status bar updates for mode and position.
- **Permission Handling** – Auto-escalation in terminal if file access is denied.

---

## 🧩 Plugin Development

Extend Vex with modular plugins using a clean interface:

```cpp
class VexPluginInterface {
public:
    virtual ~VexPluginInterface() = default;
    virtual QString name() const = 0;
    virtual void initialize(VexEditor *editor) = 0;
    virtual void cleanup() {}
};
```

- Place plugins in the `plugins/` directory.
- Use `QPluginLoader` and `Q_DECLARE_INTERFACE`.
- See `plugins/sample_plugin/` for a template to get started.

---

## 🎨 Theme Customization

- Built-in theme editor tab.
- Supports QSS-based styling.
- Switch between system styles (Fusion, Breeze, etc.).
- Save and reload themes instantly.

---

## 🛠️ Contributing

We welcome clean, modular contributions:

- New plugins (UI, syntax, preview)
- Bug fixes and performance tweaks
- Theme packs and font presets

**Guidelines:**

- Follow the established C++/Qt style.
- Keep plugins isolated and well documented.
- Submit PRs with descriptive commit messages.

---

## 📄 License

Apache-2.0 © [Zynomon](https://github.com/zynomon)

---
