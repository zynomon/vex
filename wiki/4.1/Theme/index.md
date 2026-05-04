<img src="https://github.com/user-attachments/assets/4e8ae436-72f8-4b81-89dc-7b995f94e3a3" height="256" width="256" align="left">
 

***# Vex Theming Guide***
<hr>
 
**V**ex theming is powered by the LookAndFeelCore Plugin, introduced in Vex 4.0 alongside the plugin architecture. This system allows complete visual customization using Qt Style Sheets (QSS) with custom Vex-specific extensions.
 
If theming features are unavailable, check that LookAndFeelCore.so or LookAndFeelCore.dll is present in your plugins directory.
<p>


‎ 

‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ ‎ 
</p>

---


![1q6l2vpSWz_fast736b](https://github.com/user-attachments/assets/d43e0e78-ed48-4062-957a-fc77875bb5c6)
  


# Table of Contents
 
1. [Understanding Qt Style Sheets (QSS)](#1-understanding-qt-style-sheets-qss)
   - [1.1 What is QSS?](#11-what-is-qss)
   - [1.2 QSS vs CSS: Key Differences](#12-qss-vs-css-key-differences)
   - [1.3 QSS Limitations](#13-qss-limitations)
   - [1.4 QSS Capabilities](#14-qss-capabilities)
 
2. [Vex Theme System](#2-vex-theme-system)
   - [2.1 Theme File Location](#21-theme-file-location)
   - [2.2 VEX#QSS Block: Custom Properties](#22-vexqss-block-custom-properties)
   - [2.3 Syntax Highlighting Colors](#23-syntax-highlighting-colors)
   - [2.4 Editor-Specific Properties](#24-editor-specific-properties)
   - [2.5 Theme Hot Reloading](#25-theme-hot-reloading)
 
3. [Creating Your First Theme](#3-creating-your-first-theme)
   - [3.1 Using the Built-in Theme Creator](#31-using-the-built-in-theme-creator)
   - [3.2 Manual Theme Creation](#32-manual-theme-creation)
   - [3.3 Theme File Structure](#33-theme-file-structure)
 
4. [QSS Styling Guide](#4-qss-styling-guide)
   - [4.1 Basic Selectors](#41-basic-selectors)
   - [4.2 Pseudo-States](#42-pseudo-states)
   - [4.3 Sub-Controls](#43-sub-controls)
   - [4.4 Common Properties](#44-common-properties)
   - [4.5 Gradients](#45-gradients)
   - [4.6 Colors and Transparency](#46-colors-and-transparency)
 
5. [Styling Vex Components](#5-styling-vex-components)
   - [5.1 Main Window](#51-main-window)
   - [5.2 Text Editor (QPlainTextEdit)](#52-text-editor-qplaintextedit)
   - [5.3 Menu Bar and Menus](#53-menu-bar-and-menus)
   - [5.4 Status Bar](#54-status-bar)
   - [5.5 Tab Widget](#55-tab-widget)
   - [5.6 Combo Boxes (Language Selector)](#56-combo-boxes-language-selector)
 
6. [Advanced Theming](#6-advanced-theming)
   - [6.1 Creating Dark Themes](#61-creating-dark-themes)
   - [6.2 Creating Light Themes](#62-creating-light-themes)
   - [6.3 Accent Color Systems](#63-accent-color-systems)
   - [6.4 Gradient Techniques](#64-gradient-techniques)
   - [6.5 Border Styling Patterns](#65-border-styling-patterns)
 
7. [Icon Themes](#7-icon-themes)
   - [7.1 Icon Theme System](#71-icon-theme-system)
   - [7.2 Creating Icon Themes](#72-creating-icon-themes)
   - [7.3 Icon Resolution](#73-icon-resolution)
 
8. [Troubleshooting](#8-troubleshooting)
   - [8.1 Theme Not Applying](#81-theme-not-applying)
   - [8.2 Syntax Colors Not Changing](#82-syntax-colors-not-changing)
   - [8.3 Hot Reload Not Working](#83-hot-reload-not-working)
   - [8.4 Common QSS Mistakes](#84-common-qss-mistakes)
 
---

<img width="177" height="116" alt="image" src="https://github.com/user-attachments/assets/8f64bad7-0076-4e62-b466-b5c5ff4482ef" />

## 1. Understanding Qt Style Sheets (QSS)
 
### 1.1 What is QSS?
 
Qt Style Sheets (QSS) is Qt's styling language, similar to CSS but designed specifically for Qt widgets. QSS allows you to customize the appearance of Qt applications without modifying the underlying C++ code.
 
**Key Points:**
- Based on CSS2 syntax
- Widget-specific, not DOM-based
- Applied at runtime
- Supports cascading and inheritance
- Uses Qt-specific selectors and properties
 
---
 
### 1.2 QSS vs CSS: Key Differences
 
While QSS syntax resembles CSS, there are important differences:
 
| Feature | CSS | QSS |
|---------|-----|-----|
| **Target** | HTML elements | Qt widgets |
| **Box Model** | Full CSS box model | Simplified Qt layout |
| **Selectors** | Class, ID, element | Widget class, object name |
| **Units** | px, em, rem, %, vh/vw | px only |
| **Flexbox/Grid** | Yes | No (use Qt layouts) |
| **Animations** | CSS animations, transitions | No (use Qt animations) |
| **Shadow** | box-shadow, text-shadow | **NOT SUPPORTED** |
| **Transforms** | rotate, scale, skew | **NOT SUPPORTED** |
| **Gradients** | Linear, radial, conic | Linear only (limited) |
 
---
 
### 1.3 QSS Limitations
 
**What QSS CANNOT do:**
 
1. **No Drop Shadows**
   ```css
   /* This DOES NOT WORK in QSS */
   QPushButton {
       box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.3);
       text-shadow: 2px 2px 4px #000000;
   }
   ```
   **Workaround:** Use border highlights/lowlights to simulate depth.
 
2. **No CSS Transforms**
   ```css
   /* This DOES NOT WORK in QSS */
   QLabel {
       transform: rotate(45deg);
       transform: scale(1.2);
   }
   ```
 
3. **No Animations or Transitions**
   ```css
   /* This DOES NOT WORK in QSS */
   QPushButton {
       transition: background-color 0.3s ease;
   }
   QPushButton:hover {
       animation: pulse 1s infinite;
   }
   ```
 
4. **No Flexbox or Grid**
   - QSS does not support modern CSS layout systems
   - Use Qt's layout managers (QHBoxLayout, QVBoxLayout, QGridLayout)
 
5. **Limited Pseudo-Elements**
   - No `::before` or `::after` for content insertion
   - Only widget-specific sub-controls (e.g., `::drop-down`, `::handle`)
 
6. **No calc() Function**
   ```css
   /* This DOES NOT WORK in QSS */
   width: calc(100% - 20px);
   ```
 
7. **No Media Queries**
   - No responsive design via @media
   - One stylesheet for all screen sizes
 
8. **Limited Font Control**
   - No `@font-face` for custom fonts
   - Use Qt's font database instead
 
---
 
### 1.4 QSS Capabilities
 
**What QSS CAN do:**
 
1. **Colors and Backgrounds**
   - Solid colors (hex, rgb, rgba, named)
   - Linear gradients (qlineargradient)
   - Background images
 
2. **Borders**
   - Border width, style, color
   - Border radius (rounded corners)
   - Per-side border styling
 
3. **Spacing**
   - Padding and margin
   - Min/max width and height
 
4. **Fonts**
   - Font family, size, weight, style
   - Text alignment and decoration
 
5. **Pseudo-States**
   - :hover, :pressed, :checked, :disabled, :focus
   - :selected, :enabled, :on, :off
 
6. **Sub-Controls**
   - Widget-specific parts (QComboBox::drop-down, QScrollBar::handle)
 
7. **Selectors**
   - Type selectors (QWidget)
   - ID selectors (#objectName)
   - Descendant selectors (QWidget QPushButton)
   - Property selectors ([property="value"])
 
---
 
## 2. Vex Theme System
 
### 2.1 Theme File Location
 
Vex themes are stored as `.qss` files in the following directory:
 
```
~/.vex/themes/stylesheets/
```
 
**On different platforms:**
- **Linux:** `/home/username/.vex/themes/stylesheets/`
- **Windows:** `C:\Users\username\.vex\themes\stylesheets\`
- **macOS:** `/Users/username/.vex/themes/stylesheets/`
 
**Built-in themes** are stored in the application resources:
```
:/vex.qss (default theme)
```
 
---
 
### 2.2 VEX#QSS Block: Custom Properties
 
Vex extends standard QSS with a custom `VEX#QSS` block for editor-specific properties. This block must appear at the top of your theme file.
 
**Syntax:**
```css
VEX#QSS {
    property-name: value;
}
```
 
**Example:**
```css
VEX#QSS {
    line-highlight-color: rgba(0, 60, 30, 102);
    line-number-color-fg: rgb(100, 180, 100);
    line-number-color-bg: rgb(30, 30, 30);
    comment: rgb(128, 128, 128);
    critical: rgb(255, 255, 255);
    quote: rgb(0, 255, 0);
    keyword: rgb(135, 206, 250);
    string: rgb(255, 165, 0);
}
```
 
---
 
### 2.3 Syntax Highlighting Colors
 
The VEX#QSS block defines five core syntax highlighting colors that are sent to the SyntaxCore plugin:
 
| Property | Purpose | Example |
|----------|---------|---------|
| `comment` | Comments in code | `rgb(128, 128, 128)` (gray) |
| `critical` | Critical/important text | `rgb(255, 255, 255)` (white) |
| `quote` | Quoted strings, numbers | `rgb(0, 255, 0)` (green) |
| `keyword` | Language keywords | `rgb(135, 206, 250)` (light blue) |
| `string` | String literals | `rgb(255, 165, 0)` (orange) |
 
**How it works:**
1. LookAndFeelCore extracts these colors from the VEX#QSS block
2. Creates a `SyntaxColorEvent` with the color map
3. Posts the event to the main window
4. SyntaxCore plugin receives and applies the colors
 
**Supported color formats:**
```css
comment: rgb(128, 128, 128);
keyword: rgba(135, 206, 250, 255);
string: #FFA500;
quote: rgba(0, 255, 0, 200);  /* with transparency */
```
 
---
 
### 2.4 Editor-Specific Properties
 
Beyond syntax colors, the VEX#QSS block supports editor visual properties:
 
| Property | Purpose | Example |
|----------|---------|---------|
| `line-highlight-color` | Current line highlight | `rgba(0, 60, 30, 102)` |
| `line-number-color-fg` | Line number text color | `rgb(100, 180, 100)` |
| `line-number-color-bg` | Line number background | `rgb(30, 30, 30)` |
 
**How it works:**
- Properties are extracted via regex: `VEX#QSS\s*\{([^}]*)\}`
- Applied to all `QPlainTextEdit` widgets with objectName "VexEditor"
- Set as Qt properties: `lineHighlightColor`, `lineNumberFg`, `lineNumberBg`
- Editor viewport is updated to reflect changes
 
---
 
### 2.5 Theme Hot Reloading
 
Vex supports **hot reloading** of themes: changes to the active theme file are automatically detected and applied without restarting the editor.
 
**How it works:**
1. `QFileSystemWatcher` monitors the stylesheets directory
2. When a file changes, `onThemeFileChanged()` is triggered
3. If the changed file is the current theme, it's reloaded
4. Status bar shows: "Theme hot-reloaded: [name]"
 
**To use hot reloading:**
1. Open your theme file in Vex
2. Make changes and save (Ctrl+S)
3. Watch the changes apply instantly
 
**Note:** Hot reload does NOT work for built-in resource themes (:/vex.qss).
 
---
 
## 3. Creating Your First Theme
 
### 3.1 Using the Built-in Theme Creator
 
The easiest way to create a theme is through Vex's built-in creator:
 
**Steps:**
1. Go to **View → Themes → Create New Theme...**
2. Enter a name for your theme (e.g., "My Dark Theme")
3. Click OK
4. The theme file opens automatically in the editor
5. The default template (:/vex.qss) is copied as a starting point
6. The theme is immediately applied
7. Edit and save (Ctrl+S) to see changes live
 
**What happens behind the scenes:**
```cpp
// Template is copied
QFile templateFile(":/vex.qss");
QFile newFile("~/.vex/themes/stylesheets/My Dark Theme.qss");
 
// File is opened via fileReq system
// Theme is applied and selected in menu
// File watcher is added for hot reloading
```
 
---
 
### 3.2 Manual Theme Creation
 
You can also create themes manually:
 
**Steps:**
1. Navigate to `~/.vex/themes/stylesheets/`
2. Create a new file: `mytheme.qss`
3. Copy the structure from the default theme or start from scratch
4. Add your VEX#QSS block
5. Add standard QSS styling
6. Restart Vex or use **View → Themes** to select it
 
---
 
### 3.3 Theme File Structure
 
A complete Vex theme follows this structure:
 
```css
/*
     Theme Name: My Dark Theme
     Author: Your Name
     Description: A custom dark theme for Vex
*/
 
/* ============================================
   VEX CUSTOM PROPERTIES
   ============================================ */
VEX#QSS {
    line-highlight-color: rgba(0, 60, 30, 102);
    line-number-color-fg: rgb(100, 180, 100);
    line-number-color-bg: rgb(30, 30, 30);
    comment: rgb(128, 128, 128);
    critical: rgb(255, 255, 255);
    quote: rgb(0, 255, 0);
    keyword: rgb(135, 206, 250);
    string: rgb(255, 165, 0);
}
 
/* ============================================
   MAIN WINDOW
   ============================================ */
QMainWindow {
    background-color: #1e1e1e;
    color: #d4d4d4;
}
 
/* ============================================
   TEXT EDITOR
   ============================================ */
QPlainTextEdit {
    background-color: #1e1e1e;
    color: #d4d4d4;
    selection-background-color: #264f78;
    selection-color: #ffffff;
}
 
/* ============================================
   MENUS
   ============================================ */
QMenuBar {
    background-color: #2d2d30;
    color: #cccccc;
}
 
/* ... more styling ... */
```
 
**Structure guidelines:**
1. Start with a comment block (theme name, author, description)
2. VEX#QSS block comes first
3. Organize by component (main window, editor, menus, etc.)
4. Use comments to separate sections
5. Group related selectors together
 
---
 
## 4. QSS Styling Guide
 
### 4.1 Basic Selectors
 
**Type Selector** (matches all widgets of a class):
```css
QPushButton {
    background-color: #3a6a3a;
}
```
 
**ID Selector** (matches specific objectName):
```css
#VexTab {
    background-color: #252526;
}
 
QComboBox#SyntaxLanguageSelector {
    min-width: 80px;
}
```
 
**Descendant Selector** (hierarchy):
```css
QStatusBar QComboBox {
    background: #3a6a3a;
}
 
QTabWidget QPushButton {
    border-radius: 4px;
}
```
 
**Universal Selector**:
```css
* {
    font-family: "Consolas", "Courier New", monospace;
}
```
 
---
 
### 4.2 Pseudo-States
 
Pseudo-states apply styles based on widget state:
 
| Pseudo-State | Trigger |
|--------------|---------|
| `:hover` | Mouse over the widget |
| `:pressed` | Widget is being clicked |
| `:checked` | Checkable widget is checked |
| `:unchecked` | Checkable widget is unchecked |
| `:enabled` | Widget is enabled |
| `:disabled` | Widget is disabled |
| `:focus` | Widget has keyboard focus |
| `:selected` | Item is selected (lists, menus) |
| `:on` | Toggle widget is on |
| `:off` | Toggle widget is off |
 
**Examples:**
```css
QPushButton:hover {
    background-color: #4a7a4a;
    border-color: #7ab07a;
}
 
QPushButton:pressed {
    background-color: #1a3a1a;
}
 
QAction:disabled {
    color: #666666;
}
 
QTabBar::tab:selected {
    background-color: #1e3a1e;
    color: #e0ffe0;
}
```
 
---
 
### 4.3 Sub-Controls
 
Sub-controls style specific parts of complex widgets:
 
**QComboBox sub-controls:**
```css
QComboBox::drop-down {
    background: #3a6a3a;
    border-left: 1px solid #4a7a4a;
    width: 16px;
}
 
QComboBox::down-arrow {
    image: url(:/icons/arrow-down.png);
    width: 12px;
    height: 12px;
}
```
 
**QScrollBar sub-controls:**
```css
QScrollBar::handle:vertical {
    background: #454545;
    min-height: 20px;
    border-radius: 4px;
}
 
QScrollBar::add-line:vertical {
    height: 0px;
}
 
QScrollBar::sub-line:vertical {
    height: 0px;
}
```
 
**QTabBar sub-controls:**
```css
QTabBar::tab {
    background-color: #2d2d30;
    padding: 6px 14px;
}
 
QTabBar::tab:selected {
    background-color: #1e3a1e;
}
 
QTabBar::close-button {
    image: url(:/icons/close.png);
}
```
 
---
 
### 4.4 Common Properties
 
**Color Properties:**
```css
color: #d4d4d4;                    /* text color */
background-color: #1e1e1e;         /* background */
border-color: #454545;             /* border color */
selection-background-color: #264f78;
selection-color: #ffffff;
```
 
**Border Properties:**
```css
border: 1px solid #454545;         /* all sides */
border-top: 2px solid #6aa06a;
border-bottom: 2px solid #0a1a0a;
border-left: 1px solid #4a7a4a;
border-right: 1px solid #1a2a1a;
border-radius: 4px;                /* rounded corners */
border-top-left-radius: 8px;
border-top-right-radius: 8px;
```
 
**Spacing Properties:**
```css
padding: 6px;                      /* all sides */
padding: 6px 14px;                 /* vertical horizontal */
padding: 2px 6px 2px 6px;          /* top right bottom left */
margin: 4px;
margin-left: 10px;
```
 
**Size Properties:**
```css
min-width: 80px;
max-width: 120px;
min-height: 18px;
max-height: 22px;
width: 100px;
height: 30px;
```
 
**Font Properties:**
```css
font-family: "Consolas", monospace;
font-size: 12px;
font-weight: bold;                 /* or normal */
font-style: italic;                /* or normal */
```
 
---
 
### 4.5 Gradients
 
QSS supports **linear gradients only** via `qlineargradient`:
 
**Vertical Gradient:**
```css
background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
    stop: 0 #3a6a3a,
    stop: 1 #1a3a1a);
```
 
**Horizontal Gradient:**
```css
background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
    stop: 0 #2a5a2a,
    stop: 0.5 #454545,
    stop: 1 #0a1a0a);
```
 
**Diagonal Gradient:**
```css
background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
    stop: 0 #4a7a4a,
    stop: 1 #2a5a2a);
```
 
**Multi-Stop Gradient:**
```css
background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
    stop: 0 #2a5a2a,
    stop: 0.4 #1a3a1a,
    stop: 0.7 #0a1a0a,
    stop: 1 #000000);
```
 
**Gradient Coordinates:**
- `x1, y1` = start point (0 to 1, relative to widget)
- `x2, y2` = end point (0 to 1, relative to widget)
- `stop: position color` = color at position (0.0 to 1.0)
 
---
 
### 4.6 Colors and Transparency
 
**Color Formats:**
```css
/* Named colors */
background-color: white;
color: black;
 
/* Hex (6-digit) */
background-color: #1e1e1e;
border-color: #454545;
 
/* RGB */
color: rgb(212, 212, 212);
background: rgb(30, 30, 30);
 
/* RGBA (with transparency) */
background-color: rgba(0, 60, 30, 102);  /* last value: 0-255 alpha */
border-color: rgba(100, 180, 100, 200);
```
 
**Transparency Tips:**
- Use `rgba()` for semi-transparent colors
- Alpha value: 0 = fully transparent, 255 = fully opaque
- Useful for highlights, overlays, and subtle effects
 
---
 
## 5. Styling Vex Components
 
### 5.1 Main Window
 
```css
QMainWindow {
    background-color: #1e1e1e;
    color: #d4d4d4;
}
```
 
**Properties to customize:**
- `background-color` - main background
- `color` - default text color
 
---
 
### 5.2 Text Editor (QPlainTextEdit)
 
```css
QPlainTextEdit {
    background-color: #1e1e1e;
    color: #d4d4d4;
    selection-background-color: #264f78;
    selection-color: #ffffff;
    font-family: "Consolas", "Courier New", monospace;
    font-size: 12px;
}
```
 
**Properties to customize:**
- `background-color` - editor background
- `color` - default text color
- `selection-background-color` - selected text background
- `selection-color` - selected text color
- `font-family` - code font
- `font-size` - text size
 
**Additional customization via VEX#QSS:**
- `line-highlight-color` - current line highlight
- `line-number-color-fg` - line number text
- `line-number-color-bg` - line number gutter
 
---
 
### 5.3 Menu Bar and Menus
 
**Menu Bar:**
```css
QMenuBar {
    background-color: #2d2d30;
    color: #cccccc;
}
 
QMenuBar::item:selected {
    background-color: #3e3e42;
}
```
 
**Menu:**
```css
QMenu {
    background-color: #2d2d30;
    color: #cccccc;
    border: 1px solid #454545;
    border-radius: 6px;
    padding: 4px 0px;
}
 
QMenu::item {
    padding: 6px 24px 6px 24px;
    margin: 2px 4px;
    border-radius: 4px;
}
 
QMenu::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #2a5a2a,
        stop: 1 #1a3a1a);
    color: #c0f0c0;
}
 
QMenu::item:disabled {
    color: #666666;
}
 
QMenu::separator {
    height: 1px;
    background: #454545;
    margin: 6px 10px;
}
```
 
---
 
### 5.4 Status Bar
 
```css
QStatusBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
        stop: 0 #2a5a2a,
        stop: 0.4 #1a3a1a,
        stop: 0.7 #0a1a0a,
        stop: 1 #000000);
    color: #c0f0c0;
    border-top: 2px solid #4a8a4a;
    padding: 2px 6px;
}
 
QStatusBar QLabel {
    background: rgba(60, 100, 60, 100);
    border: 1px solid #3a6a3a;
    border-radius: 3px;
    padding: 1px 8px;
}
```
 
---
 
### 5.5 Tab Widget
 
```css
QTabWidget::pane {
    border: 1px solid #454545;
    background-color: #252526;
}
 
QTabBar::tab {
    background-color: #2d2d30;
    color: #969696;
    padding: 6px 14px;
    border: 1px solid #454545;
}
 
QTabBar::tab:hover {
    background: #2a5a2a;
    color: #c0f0c0;
}
 
QTabBar::tab:selected {
    background: #1e3a1e;
    color: #e0ffe0;
    border-top: 2px solid #4a8a4a;
}
 
QTabBar::tab:last {
    border-top-right-radius: 8px;
}
```
 
---
 
### 5.6 Combo Boxes (Language Selector)
 
```css
QStatusBar QComboBox {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #3a6a3a,
        stop: 1 #1a3a1a);
    color: #e0ffe0;
    border: 1px solid #4a7a4a;
    border-radius: 3px;
    padding: 2px 6px;
    min-width: 80px;
}
 
QStatusBar QComboBox:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #4a7a4a,
        stop: 1 #2a5a2a);
}
 
QStatusBar QComboBox::drop-down {
    background: #3a6a3a;
    border-left: 1px solid #4a7a4a;
    width: 16px;
}
 
QStatusBar QComboBox::down-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-top: 3px solid #e0ffe0;
    width: 0;
    height: 0;
}
 
QStatusBar QComboBox QAbstractItemView {
    background: #2a4a2a;
    color: #e0ffe0;
    border: 1px solid #4a7a4a;
    selection-background-color: #3a6a3a;
}
 
QStatusBar QComboBox QAbstractItemView::item {
    padding: 2px 6px;
    min-height: 16px;
}
 
QStatusBar QComboBox QAbstractItemView::item:hover {
    background: #4a7a4a;
}
```
 
---
 
## 6. Advanced Theming
 
### 6.1 Creating Dark Themes
 
**Dark Theme Principles:**
1. Use dark backgrounds (#1e1e1e, #252526, #2d2d30)
2. Light text (#d4d4d4, #cccccc, #ffffff)
3. Subtle borders (#454545, #3e3e42)
4. Accent colors for highlights
5. Lower contrast for less eye strain
 
**Recommended Color Palette:**
```css
/* Background Layers */
--bg-deepest: #1e1e1e;
--bg-deep: #252526;
--bg-medium: #2d2d30;
--bg-light: #3e3e42;
 
/* Text */
--text-primary: #d4d4d4;
--text-secondary: #cccccc;
--text-muted: #969696;
--text-disabled: #666666;
 
/* Borders */
--border-dark: #454545;
--border-light: #5a5a5a;
 
/* Accent (customize to your preference) */
--accent-primary: #2a5a2a;
--accent-hover: #4a7a4a;
--accent-active: #1a3a1a;
```
 
---
 
### 6.2 Creating Light Themes
 
**Light Theme Principles:**
1. Light backgrounds (#ffffff, #f3f3f3, #e8e8e8)
2. Dark text (#000000, #333333, #666666)
3. Visible but not harsh borders (#cccccc, #d4d4d4)
4. Softer accent colors
5. Adequate contrast for readability
 
**Recommended Color Palette:**
```css
/* Background Layers */
--bg-lightest: #ffffff;
--bg-light: #f3f3f3;
--bg-medium: #e8e8e8;
--bg-dark: #d4d4d4;
 
/* Text */
--text-primary: #000000;
--text-secondary: #333333;
--text-muted: #666666;
--text-disabled: #999999;
 
/* Borders */
--border-light: #cccccc;
--border-dark: #a0a0a0;
 
/* Accent */
--accent-primary: #007acc;
--accent-hover: #005a9e;
--accent-active: #003d6b;
```
 
**Example Light Theme VEX#QSS:**
```css
VEX#QSS {
    line-highlight-color: rgba(200, 220, 255, 80);
    line-number-color-fg: rgb(80, 80, 80);
    line-number-color-bg: rgb(245, 245, 245);
    comment: rgb(0, 128, 0);
    critical: rgb(0, 0, 0);
    quote: rgb(163, 21, 21);
    keyword: rgb(0, 0, 255);
    string: rgb(163, 21, 21);
}
 
QMainWindow {
    background-color: #ffffff;
    color: #000000;
}
 
QPlainTextEdit {
    background-color: #ffffff;
    color: #000000;
    selection-background-color: #add6ff;
    selection-color: #000000;
}
```
 
---
 
### 6.3 Accent Color Systems
 
**Single Accent Color:**
```css
/* Define your accent */
--accent: #2a5a2a;
 
/* Use it throughout */
QMenu::item:selected {
    background-color: #2a5a2a;  /* your accent */
}
 
QTabBar::tab:selected {
    border-top-color: #2a5a2a;  /* your accent */
}
 
QPushButton:hover {
    background-color: #2a5a2a;  /* your accent */
}
```
 
**Multi-Tone Accent System:**
```css
/* Lightest to darkest */
--accent-100: #8ac08a;
--accent-200: #6aa06a;
--accent-300: #4a7a4a;
--accent-400: #3a6a3a;
--accent-500: #2a5a2a;  /* primary */
--accent-600: #1a3a1a;
--accent-700: #0a1a0a;
 
/* Use different tones for depth */
QPushButton {
    background: #2a5a2a;       /* primary */
    border-top: 1px solid #4a7a4a;  /* lighter */
    border-bottom: 1px solid #0a1a0a;  /* darker */
}
 
QPushButton:hover {
    background: #3a6a3a;       /* one shade lighter */
}
```
 
---
 
### 6.4 Gradient Techniques
 
**Depth and Dimension:**
```css
/* 3D raised effect */
QPushButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #4a7a4a,      /* lighter top */
        stop: 1 #2a5a2a);     /* darker bottom */
    border-top: 1px solid #6aa06a;
    border-bottom: 1px solid #1a3a1a;
}
 
/* 3D pressed effect */
QPushButton:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #1a3a1a,      /* darker top (inverted) */
        stop: 1 #3a6a3a);     /* lighter bottom */
    border-top: 1px solid #1a2a1a;
    border-bottom: 1px solid #6aa06a;
}
```
 
**Ambient Highlight:**
```css
/* Subtle glow effect */
QMenu::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
        stop: 0 #2a5a2a,
        stop: 0.5 #1a3a1a,
        stop: 1 #0a1a0a);
}
```
 
**Metallic Effect:**
```css
/* Simulating metal sheen */
QStatusBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
        stop: 0 #2a5a2a,
        stop: 0.3 #3a6a3a,
        stop: 0.5 #4a7a4a,
        stop: 0.7 #3a6a3a,
        stop: 1 #2a5a2a);
}
```
 
---
 
### 6.5 Border Styling Patterns
 
**Inset (Recessed) Effect:**
```css
/* Looks pressed in */
QPlainTextEdit {
    border-top: 2px solid #0a1a0a;     /* dark top */
    border-left: 1px solid #1a2a1a;    /* dark left */
    border-bottom: 1px solid #4a7a4a;  /* light bottom */
    border-right: 1px solid #3a6a3a;   /* light right */
}
```
 
**Outset (Raised) Effect:**
```css
/* Looks raised */
QPushButton {
    border-top: 2px solid #6aa06a;     /* light top */
    border-left: 1px solid #4a7a4a;    /* light left */
    border-bottom: 2px solid #0a1a0a;  /* dark bottom */
    border-right: 1px solid #1a2a1a;   /* dark right */
}
```
 
**Glow Effect (Without box-shadow):**
```css
/* Multi-layer borders */
QTabBar::tab:selected {
    border: 2px solid #4a8a4a;
    outline: 1px solid #2a5a2a;
    outline-offset: -3px;
}
 
/* Or use gradient borders */
QMenu::item:selected {
    border-top: 1px solid #4a8a4a;     /* bright */
    border-bottom: 1px solid #0a150a;  /* dark */
    border-left: 1px solid #3a6a3a;    /* medium */
    border-right: 1px solid #1a2a1a;   /* darker */
}
```
 
---
 
## 7. Icon Themes
 
### 7.1 Icon Theme System
 
Vex supports custom icon themes through the LookAndFeelCore plugin. Icons are used for:
- Language icons in the syntax selector
- Menu icons
- Toolbar icons
- Tab close buttons
 
**Icon Storage:**
```
~/.vex/themes/icon/
  ├── my-icon-theme/
  │   ├── python.svg
  │   ├── javascript.svg
  │   ├── text-x-generic.svg
  │   └── ...
  └── another-theme/
      └── ...
```
 
**Icon Resolution Priority:**
1. User icon theme (`~/.vex/themes/icon/[theme-name]/`)
2. System icon themes (Linux: XDG paths, `/usr/share/icons`)
3. Qt built-in theme icons
4. Fallback to default
 
---
 
### 7.2 Creating Icon Themes
 
**Steps:**
1. Create a folder in `~/.vex/themes/icon/`
   ```bash
   mkdir -p ~/.vex/themes/icon/my-icons
   ```
 
2. Add icon files (supported formats: SVG, PNG, ICO, ICNS, SVGZ)
   ```
   my-icons/
   ├── python.svg
   ├── python.png
   ├── text-x-generic.svg
   ├── application-javascript.svg
   └── text-html.svg
   ```
 
3. In Vex, go to **View → Icon Theme → my-icons (User)**
 
**Icon Naming:**
- Use standard icon names from XDG specification
- For syntax highlighting: match the ICNS value in .vxsyn files
- Common names: `text-x-generic`, `python`, `text-html`, `application-javascript`
 
**Icon Size:**
- Vex uses 16x16 for most UI elements
- SVG is preferred (scales well)
- PNG should be at least 16x16, ideally 32x32 or higher for HiDPI
 
---
 
### 7.3 Icon Resolution
 
**How icons are resolved:**
```cpp
QIcon resolveIcon(const QString &baseName) {
    // 1. Check user theme
    if (!currentIconTheme.isEmpty()) {
        QString userPath = "~/.vex/themes/icon/" + currentIconTheme;
        // Try: baseName.svg, .png, .ico, .icns, .svgz
        if (found) return QIcon(path);
    }
    
    // 2. Fall back to system
    return Settings::resolveIcon(baseName);
}
```
 
**Example:**
```cpp
// In SyntaxCore plugin
ICNS = "python"
 
// LookAndFeelCore searches:
// 1. ~/.vex/themes/icon/[current-theme]/python.svg
// 2. ~/.vex/themes/icon/[current-theme]/python.png
// 3. System theme: /usr/share/icons/.../python.svg
// 4. Qt theme icons
// 5. Fallback: text-x-generic
```
 
---
 
## 8. Troubleshooting
 
### 8.1 Theme Not Applying
 
**Problem:** Selected theme doesn't appear to load.
 
**Solutions:**
1. **Check file location:**
   - Themes must be in `~/.vex/themes/stylesheets/`
   - File must have `.qss` extension
 
2. **Check file permissions:**
   ```bash
   chmod 644 ~/.vex/themes/stylesheets/mytheme.qss
   ```
 
3. **Check for syntax errors:**
   - Missing semicolons
   - Unclosed braces `{}`
   - Invalid property names
   - Malformed gradient syntax
 
4. **Restart Vex:**
   - Some changes require a restart
   - Or use **View → Themes** to re-select
 
5. **Check status bar:**
   - Look for error messages like "Failed to load theme"
 
---
 
### 8.2 Syntax Colors Not Changing
 
**Problem:** Syntax highlighting colors don't update with theme.
 
**Solutions:**
1. **Check VEX#QSS block exists:**
   ```css
   VEX#QSS {
       comment: rgb(128, 128, 128);
       critical: rgb(255, 255, 255);
       quote: rgb(0, 255, 0);
       keyword: rgb(135, 206, 250);
       string: rgb(255, 165, 0);
   }
   ```
 
2. **Verify color format:**
   - Use `rgb()` or `rgba()` or hex `#RRGGBB`
   - No spaces in hex: `#FF5733` not `# FF 57 33`
 
3. **Check SyntaxCore plugin is loaded:**
   - Syntax highlighting requires SyntaxCore.so/dll
   - Without it, colors won't apply
 
4. **Reopen file or switch language:**
   - Syntax colors apply when language is activated
   - Switch to "Plain Text" then back to your language
 
5. **Check property names exactly:**
   - Must be: `comment`, `critical`, `quote`, `keyword`, `string`
   - Case-sensitive
 
---
 
### 8.3 Hot Reload Not Working
 
**Problem:** Changes to theme file don't apply automatically.
 
**Solutions:**
1. **Verify theme is file-based:**
   - Hot reload does NOT work for `:/vex.qss` (built-in resource)
   - Only works for files in `~/.vex/themes/stylesheets/`
 
2. **Check file watcher:**
   - File must be the currently active theme
   - Status bar should show "Theme file changed: [name]"
 
3. **Save file properly:**
   - Use Ctrl+S or File → Save
   - Ensure no write errors
 
4. **Manual reload:**
   - Go to **View → Themes**
   - Select "None (No Theme)"
   - Select your theme again
 
5. **Restart if needed:**
   - Some changes (like font-family) may require restart
 
---
 
### 8.4 Common QSS Mistakes
 
**1. Trying to use CSS shadows:**
```css
/* WRONG - does not work in QSS */
QPushButton {
    box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.3);
}
 
/* RIGHT - use borders for depth */
QPushButton {
    border-top: 1px solid #6aa06a;
    border-bottom: 2px solid #0a1a0a;
}
```
 
**2. Missing semicolons:**
```css
/* WRONG */
QPushButton {
    background-color: #2a5a2a
    color: #e0ffe0
}
 
/* RIGHT */
QPushButton {
    background-color: #2a5a2a;
    color: #e0ffe0;
}
```
 
**3. Incorrect gradient syntax:**
```css
/* WRONG - CSS gradient syntax */
background: linear-gradient(to bottom, #3a6a3a, #1a3a1a);
 
/* RIGHT - QSS gradient syntax */
background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
    stop: 0 #3a6a3a,
    stop: 1 #1a3a1a);
```
 
**4. Forgetting objectName selectors need #:**
```css
/* WRONG */
QComboBox VexTab {
    background: #252526;
}
 
/* RIGHT */
#VexTab {
    background: #252526;
}
```
 
**5. Using calc() or CSS variables:**
```css
/* WRONG - QSS doesn't support these */
width: calc(100% - 20px);
color: var(--primary-color);
 
/* RIGHT - use fixed values */
width: 200px;
color: #2a5a2a;
```
 
**6. Comma-separated selectors without full paths:**
```css
/* WRONG */
QPushButton, QToolButton {
    background: #2a5a2a;
}
 
/* BETTER - repeat full selector */
QPushButton {
    background: #2a5a2a;
}
 
QToolButton {
    background: #2a5a2a;
}
```
 
**7. Over-specific selectors causing conflicts:**
```css
/* Can cause specificity issues */
QMainWindow QWidget QPlainTextEdit {
    background: #1e1e1e;
}
 
/* Better - be as specific as needed */
QPlainTextEdit {
    background: #1e1e1e;
}
```
---
 