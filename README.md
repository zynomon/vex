# ![vex](https://github.com/user-attachments/assets/cb79fc75-ed18-491c-ac6a-f146d28da42a) Vex - a text editor
---
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
  <strong>Vex is a fast, Vim-inspired text editor built with Qt — designed for developers who crave control, clarity, and modular extensibility.</strong>
  <br>
  <sub>Tabbed editing, syntax highlighting, plugin support, theme customization, and more — all built-in.</sub>
  <br><br>
  <img src="https://img.shields.io/github/license/zynomon/vex?style=flat-square" alt="License" />
  <img src="https://img.shields.io/badge/build-stable-brightgreen?style=flat-square" alt="Build Status" />
  <img src="https://img.shields.io/badge/platform-Linux-blue?style=flat-square" alt="Platform" />
</div>

---


## 📦 Installation Guide

### 🗃️ Debian Stable (.deb)

Install Vex using the prebuilt `.deb` package:

```bash
wget https://github.com/zynomon/vex/raw/main/build/vex-1.0.0-Linux.deb
sudo apt install ./vex-1.0.0-Linux.deb
```

### 👷 Build from Source

```bash
git clone https://github.com/zynomon/vex.git
cd vex
mkdir build && cd build
cmake ..
make -j$(nproc)
./vex
```
coming soon..  RPM and dedicated debian repositories
### 🔧 Requirements

- 🧰 **Qt** 5.15+ or Qt 6
- 🛠️ **CMake** 3.16+
- 🧪 **Compiler:** GCC or Clang
- 🐧 **Platform:** Linux (.deb)

> 💡 Tip: Use `-DCMAKE_BUILD_TYPE=Release` for optimized builds.

---

## ✨ Features
<p align="center">
  <img src="https://github.com/user-attachments/assets/ff3216c1-7818-4b69-aab9-f170261a632a" alt="Vex demo GIF" width="70%" />
</p>

- 🧙‍♂️ **Vim Mode** – Modal editing with familiar keybindings (`hjkl`, `x`, `dd`, `:`)
- 🗂️ **Tabbed Interface** – Closable, movable tabs for multitasking
- 🎨 **Syntax Highlighting** – Powered by `QSyntaxHighlighter`
- 🔍 **Find & Replace** – Case-sensitive, whole-word, directional search
- 🔄 **File Watcher** – Auto-reload on external changes
- 🔌 **Plugin System** – Load `.so` plugins via `QPluginLoader`
- 🖌️ **Theme Editor** – Live QSS editing and saving
- 🔤 **Font Customization** – Choose your favorite editor font
- 🔢 **Smart Line Numbers** – Dynamic width, color-coded
- 📍 **Cursor Tracking** – Real-time mode and position updates
- 🔐 **Permission Handling** – Auto-escalation in terminal if access is denied

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

- Place plugins in the `plugins/` directory
- Use `QPluginLoader` and `Q_DECLARE_INTERFACE`
- See `plugins/sample_plugin/` for a starter template

---

## 🎨 Theme Customization

- Built-in theme editor tab
- QSS-based styling
- Switch between system styles (Fusion, Breeze, etc.)
- Save and reload themes instantly

---

## 🛠️ Contributing

We welcome clean, modular contributions:

- 🧩 New plugins (UI, syntax, preview)
- 🐞 Bug fixes and performance tweaks
- 🎨 Theme packs and font presets

**Guidelines:**

- Follow the established C++/Qt style
- Keep plugins isolated and well documented
- Submit PRs with descriptive commit messages

---

## 📄 License

Apache-2.0 © [Zynomon](https://github.com/zynomon)

---
