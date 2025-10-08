# say
![VirtualBoxVM_MerK3xA3OY](https://github.com/user-attachments/assets/ff3216c1-7818-4b69-aab9-f170261a632a)
<img width="731" height="505" alt="VirtualBoxVM_pj5zISQSeN" src="https://github.com/user-attachments/assets/54a4b958-a9d4-454d-93bd-76bd14edd845" />
<img width="997" height="719" alt="VirtualBoxVM_sxFLSDHp08" src="https://github.com/user-attachments/assets/c7986877-23f4-4f09-a67c-780a080e4171" />
<img width="1031" height="556" alt="VirtualBoxVM_YtaHxACppW" src="https://github.com/user-attachments/assets/cf94841e-2771-4227-ac24-e93f4b9778d9" />
<img width="1000" height="726" alt="VirtualBoxVM_sY9EEQPrjo" src="https://github.com/user-attachments/assets/eafdde0c-73de-45f0-b358-c43cbff2b6b2" />
<img width="899" height="450" alt="VirtualBoxVM_NgJr9AUcur" src="https://github.com/user-attachments/assets/76095a1f-df6e-4974-a3ca-f8b5f8f08019" />
<img width="1077" height="541" alt="VirtualBoxVM_SH2SPhDUK5" src="https://github.com/user-attachments/assets/c7ba566c-8321-42cb-9b5b-2a22ddd4d01a" />
# vex

**Vex is a fast, Vim-inspired text editor built with Qt—designed for developers who want control, clarity, and extensibility.**  
Tabbed editing, syntax highlighting, plugin support, and theme customization come built-in.

![License](https://img.shields.io/github/license/zynomon/vex?style=flat-square)
![Status](https://img.shields.io/badge/build-stable-brightgreen?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-blue?style=flat-square)

---

## 🔧 Features

- 🧠 **Vim Mode** — Modal editing with intuitive keybindings (`hjkl`, `x`, `dd`, `:`, etc.)
- 🗂️ **Tabbed Interface** — Multiple documents with closable, movable tabs
- 📝 **Syntax Highlighting** — C++ keywords, comments, and strings via `QSyntaxHighlighter`
- 🔍 **Find & Replace Dialog** — Case-sensitive, whole-word, directional search with replace-all
- 📁 **File Watcher** — Auto-reload on external changes
- 🧩 **Plugin System** — Loadable `.so`/`.dll` plugins via `QPluginLoader`
- 🎨 **Theme Editor** — Live editing and saving of custom QSS themes
- 🖋️ **Font Customization** — Choose your preferred editor font
- 🧠 **Smart Line Numbers** — Dynamic width and color-coded line display
- 🧭 **Cursor Tracking** — Live status bar updates for mode and position
- 🛡️ **Permission Handling** — Auto-escalation via terminal if file access is denied

---

## 🚀 Getting Started

### Requirements

- Qt 5.15+ or Qt 6
- CMake 3.16+
- GCC / Clang / MSVC
- Linux or Windows (WSL-friendly)

### Build Instructions

```bash
git clone https://github.com/zynomon/vex.git
cd vex
mkdir build && cd build
cmake ..
make -j$(nproc)
./vex
```

> 💡 Use `-DCMAKE_BUILD_TYPE=Release` for optimized builds.

---

## 🧩 Plugin Development

Vex supports modular plugins with clean interface boundaries:

```cpp
class VexPluginInterface {
public:
    virtual ~VexPluginInterface() = default;
    virtual QString name() const = 0;
    virtual void initialize(VexEditor *editor) = 0;
    virtual void cleanup() {}
};
```

- Plugins are loaded from the `plugins/` directory
- Use `QPluginLoader` and `Q_DECLARE_INTERFACE`
- See `plugins/sample_plugin/` for a starter template

---

## 🎨 Theme Customization

- Built-in theme editor tab
- Supports QSS-based styling
- Switch between system styles (Fusion, Breeze, etc.)
- Save and reload themes on the fly

---

## 🛠️ Contributing

We welcome clean, modular contributions:

- New plugins (UI, syntax, preview)
- Bug fixes and performance tweaks
- Theme packs and font presets

### Guidelines

- Follow the existing C++/Qt style
- Keep plugins isolated and documented
- Submit PRs with clear commit messages

---

## 📄 License

Apache-2.0 © [Zynomon](https://github.com/zynomon)


---

```
