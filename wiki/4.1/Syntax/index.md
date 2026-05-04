---
layout: default
title: "Syntax"
---

<img src="https://github.com/user-attachments/assets/7b3c437f-a1c7-48a6-b346-bdfa970400e8" height="256" width="256" align="left">

***# Vex Syntax<sub>vxsyn</sub>***
<hr>

**V**ex syntax is a part of VexCore Plugin it has been a part of vex since the 1st release. 
Later after 4.0=< onwards When Vex introduced plugin architecture The logic is being served by SyntaxCore Plugin.

So if you see symptoms of the screenshot not being related to you that usually will mean SyntaxCore.so or SyntaxCore.dll is removed or misplaced.
<p>
‎ 
‎ 
‎ 
‎ ‎ 
</p>

---

<img width="373" height="197" alt="image" src="https://github.com/user-attachments/assets/e82c4406-03a8-4d2d-af3e-65873fc9b347" />

# Table of Contents

 
1. [Syntax Definition Files (.vxsyn)](#1-syntax-definition-files-vxsyn)
   - [1.1 Creating a Syntax File](#11-creating-a-syntax-file)
   - [1.2 REG: Registering the Language Name](#12-reg-registering-the-language-name)
   - [1.3 ICNS: Icon Set](#13-icns-icon-set)
   - [1.4 File: Matching File Extensions](#14-file-matching-file-extensions)
   - [1.5 Sw: Starts With (File Sniffer)](#15-sw-starts-with-file-sniffer)
   - [1.6 +ES'...'ES-: Enter Script (Exact Match)](#16-esex--enter-script-exact-match)
   - [1.7 +HS'...'HS- to +HE'...'HE-: Periodic Blocks](#17-hshs--to-hehe--periodic-blocks)
   - [1.8 &&: Combining Multiple Patterns](#18--combining-multiple-patterns)
   - [1.9 Style Attributes: color and font](#19-style-attributes-color-and-font)
   - [1.10 delem: Delimiter Capture](#110-delem-delimiter-capture)

2. [How It Works](#2-how-it-works)
   - [2.1 What Happens When You Open a File](#21-what-happens-when-you-open-a-file)
   - [2.2 How Highlighting Is Applied](#22-how-highlighting-is-applied)
   - [2.3 How Multi-line Blocks Work](#23-how-multi-line-blocks-work)

3. [Complete Examples](#3-complete-examples)
   - [3.1 Python Syntax](#31-python-syntax)
   - [3.2 C-style Comments](#32-c-style-comments)
   - [3.3 HTML-like Tags](#33-html-like-tags)
   - [3.4 JavaScript](#34-javascript)

4. [Quick Reference](#4-quick-reference)
   - [4.1 Core Directives](#41-core-directives)
   - [4.2 Pattern Matching](#42-pattern-matching)
   - [4.3 Operators and Keywords](#43-operators-and-keywords)
   - [4.4 Style Attributes](#44-style-attributes)
   - [4.5 Processing Priority](#45-processing-priority)
   - [4.6 File System](#46-file-system)
   - [4.7 UI Elements](#47-ui-elements)

5. [Advanced Notes](#5-advanced-notes)
   - [5.1 Parser Behavior](#51-parser-behavior)
   - [5.2 Block State Management](#52-block-state-management)
   - [5.3 Extension Matching](#53-extension-matching)
   - [5.4 Content Detection](#54-content-detection)
   - [5.5 Syntax File Reloading](#55-syntax-file-reloading)
   - [5.6 Multiple Language Definitions](#56-multiple-language-definitions)

6. [Troubleshooting](#6-troubleshooting)
   - [6.1 Language Not Appearing in Dropdown](#61-language-not-appearing-in-dropdown)
   - [6.2 Highlighting Not Working](#62-highlighting-not-working)
   - [6.3 Multi-line Blocks Not Closing](#63-multi-line-blocks-not-closing)
   - [6.4 Wrong File Detected](#64-wrong-file-detected)

 
---
 
## 1. Syntax Definition Files (.vxsyn)
![5Rc7AE8sOX_fast728b](https://github.com/user-attachments/assets/662bfcc2-7e51-4a74-9c98-f631d66a791b)

Quick gif Demonstrating simple register
( This exact thing is already in the Readme.md )

### 1.1 Creating a Syntax File

1. Create a file with `.vxsyn` extension in the `~/.vex/syntax/` folder
2. Add `REG = "Language Name"` at the top of the file to register your syntax
3. Write your syntax rules below
4. Save. The editor detects changes automatically using QFileSystemWatcher
5. Lines starting with `⇏ ` are treated as comments and ignored

---

### 1.2 REG: Registering the Language Name

**What it does:** Sets the name that appears in the language dropdown menu. Without this, the language will not be registered.

**Syntax:**
```
REG = "Name"
```

**Example:**
```
REG = "Python"
```

**Rules:**
- Must appear before any other directives for a language
- The name must be unique
- Use double quotes around the name
- Multiple languages can be defined in a single .vxsyn file by using REG multiple times

**Notes:**
- When REG is encountered, any previous language definition is saved
- Each REG starts a new SyntaxDef structure

---

### 1.3 ICNS: Icon Set

**What it does:** Sets the icon shown next to the language name in the selector dropdown.

**Syntax:**
```
ICNS = "icon-name"
```

**Example:**
```
ICNS = "python"
```

**Available icons:**
- XDG icon themes at `/usr/share/icons` (Unix)
- Qt's built-in theme icons (see QIcon ThemeIcon enum)
- If icon is not found, defaults to "text-x-generic"

**Notes:**
- Icons are resolved using `Settings::resolveIcon()`
- The selector uses 16x16 icon size

---

### 1.4 File: Matching File Extensions

**What it does:** Tells the editor which file extensions belong to this language.

**Syntax:**
```
File = .ext1 && .ext2 && .ext3
```

**Example:**
```
File = .py && .pyw && .pyx
```

**How it works:**
- When you open a file, the editor checks its extension
- Extensions are compared after converting to lowercase
- If the extension matches, this language is suggested (after content detection)

**Notes:**
- Extensions can start with or without a dot (the parser adds it if missing)
- Use `&&` to separate multiple extensions
- `&&` can only appear AFTER the `=` for File directives

---

### 1.5 Sw: Starts With (File Sniffer)

**What it does:** Detects the language by looking at the first few lines of the file.

**Syntax:**
```
Sw = pattern1 && pattern2
```

**Example:**
```
Sw = #!/usr/bin/env python && # -*- coding: utf-8 -*-
```
```
Sw = #include && /**
```

**How it works:**
- The editor reads the first 10 lines of the file
- If any line starts with one of your patterns, this language is selected
- Content detection runs BEFORE extension matching (higher priority)

**Notes:**
- Use `&&` to separate multiple patterns
- `&&` can only appear AFTER the `=` for Sw directives
- Patterns are trimmed before comparison

---

### 1.6 +ES'...'ES-: Enter Script (Exact Match)

**What it does:** Highlights specific words or strings exactly as they appear.

**Syntax:**
```
+ES'text'ES- = style
```

**Naming:**
- `+ES'` : Enter Script begin (marks the start of an exact match pattern)
- `'ES-` : Enter Script close (marks the end of an exact match pattern)

**Example:**
```
+ES'def'ES- = color:keyword font:B
+ES'return'ES- = color:keyword font:B
```

**How it works:**
- Searches for the exact text between `+ES'` and `'ES-`
- Uses `QString::indexOf()` to find all occurrences in the line
- Every occurrence gets the specified style applied
- Processed AFTER block matches (lower priority)

**Use for:**
- Keywords (`if`, `else`, `while`)
- Operators (`+`, `-`, `*`)
- Built-in functions
- Any exact text match that should be highlighted

---

### 1.7 +HS'...'HS- to +HE'...'HE-: Periodic Blocks

**What it does:** Highlights everything between a start pattern and an end pattern.

**Syntax:**
```
+HS'start'HS- to +HE'end'HE- = style
```

**Naming:**
- `+HS'` : Periodic Start begin (marks the start of a block opener)
- `'HS-` : Periodic Start close (closes the start pattern)
- `to` : Separates start and end patterns (interrupts conflict)
- `+HE'` : Periodic End begin (marks the start of a block closer)
- `'HE-` : Periodic End close (closes the end pattern)

**Example 1: Single line (EOL):**
```
+HS'#'HS- to +HE''HE- = color:comment
```
Highlights from `#` to the end of the line. (Empty `+HE''HE-` means end of line)

**Example 2: Multi-line:**
```
+HS'/*'HS- to +HE'*/'HE- = color:comment
```
Highlights everything between `/*` and `*/`, across multiple lines.

**Example 3: String literals:**
```
+HS'"'HS- to +HE'"'HE- = color:string
```
Highlights everything between two double quotes.

**How it works:**
- Block matches are processed FIRST (higher priority than exact matches)
- Each block is assigned a unique ID (starting from 1, incrementing)
- If the closer is found on the same line, only that range is highlighted
- If the closer is not found, the entire rest of the line is highlighted and the block state is saved
- The block state carries over to the next line via `setCurrentBlockState(block.id)`
- When a line starts in a block state, it continues highlighting until the closer is found

**Special behavior:**
- If `+HE''HE-` is empty, `endsAtNewline` is set to true
- When `endsAtNewline` is true, the block always ends at the current line
- Multi-line blocks use `previousBlockState()` to resume highlighting

---

### 1.8 &&: Combining Multiple Patterns

**What it does:** Applies the same style to multiple patterns OR lists multiple values for directives.

**Syntax:**
```
pattern1 && pattern2 = style
```

**Example:**
```
+ES'if'ES- && +ES'else'ES- && +ES'elif'ES- = color:keyword font:B
```

**Rules:**
- For `File` and `Sw`: `&&` appears AFTER `=` to list multiple values
  ```
  File = .py && .pyw && .pyx
  Sw = #!/usr/bin/python && #!/usr/bin/env python
  ```
- For `+ES'...'ES-` and `+HS'...'HS- to +HE'...'HE-`: `&&` appears BEFORE `=` to combine patterns
  ```
  +ES'if'ES- && +ES'else'ES- = color:keyword
  ```

**How it works:**
- The line is split by `&&`
- Each part is processed separately
- For syntax patterns, each part gets the same style format applied

---

### 1.9 Style Attributes: color and font

**What it does:** Controls how matched text looks.

**Syntax:**
```
= color:value font:value
```

#### color: Text Color

| Value Type | Example | Description |
|------------|---------|-------------|
| Hex | `color:#FF5733` | Any hex color code |
| Named | `color:comment` | Predefined palette color |

**Predefined palette colors:**
- `comment` : Gray (#808080 or 128, 128, 128)
- `critical` : White (#FFFFFF or 255, 255, 255)
- `quote` : Green (#00FF00 or 0, 255, 0)
- `keyword` : Light blue (#87CEEB or 135, 206, 250)
- `string` : Orange (#FFA500 or 255, 165, 0)

*Using these fixed colors is recommended for later theme syntax matching.*

**How it works:**
- The `ColorPalette::lookup()` method checks if the color name matches a predefined color
- If not, it treats the value as a hex color using `QColor(name)` constructor
- The color is applied via `QTextCharFormat::setForeground(QBrush(color))`

#### font: Font Style

| Value | Effect |
|-------|--------|
| `font:N` | Normal |
| `font:B` | Bold |
| `font:I` | Italic |
| `font:BI` | Bold + Italic |

**How it works:**
- `font:B` sets `QFont::Bold` weight
- `font:I` calls `setFontItalic(true)`
- `font:BI` applies both bold and italic
- `font:N` resets to normal (not italic, normal weight)

**Example combining both:**
```
+ES'class'ES- = color:keyword font:BI
```

**Notes:**
- Style attributes are separated by whitespace
- Attributes are parsed using `split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)`
- Multiple spaces or tabs between attributes are allowed

---

### 1.10 delem: Delimiter Capture

**What it does:** Special keyword that recognizes anything within the syntax and matches closing delimiters properly.

**Syntax:**
```
+HS'delem"'HS- to +HE'delem"'HE- = style
```

**Most useful for:** String literals with escaped quotes and nested structures

**Example: String with escaped quotes:**
```
+HS'delem"'HS- to +HE'delem"'HE- = color:string
+HS"delem'"HS- to +HE"delem'"HE- = color:string
```

**How it works:**
- When the opener or closer contains the word "delem", `capturesDelimiter` is set to true
- The current implementation sets a flag but does not have special delimiter handling logic active
- This is a placeholder for future enhanced delimiter matching

**Note:** In the current source code, `capturesDelimiter` is set but not actively used in the highlighting logic. This is a framework for future enhancement.

---

## 2. How It Works

### 2.1 What Happens When You Open a File

1. **Tab is created** : `SyntaxCorePlugin` connects to `QTabWidget::currentChanged` signal
2. **Language detection runs** (via `detectLanguage()`):
   - First, the editor reads the first 10 lines of content
   - Checks each line against all `Sw` patterns from all loaded syntax files
   - If a line starts with any pattern, that language is selected
   - If no match, it extracts the file extension from the tab text
   - Compares the extension (lowercased, with dot) against all `File` patterns
   - If a match is found, that language is selected
3. **If a language is found:** The syntax file content is passed to `TextPainter::activate()`
4. **If no language is found:** Auto mode shows "No language detected" and no highlighting is applied
5. **Highlighting begins:** `rehighlight()` is called to apply syntax highlighting

### 2.2 How Highlighting Is Applied

The `TextPainter::highlightBlock()` processes each line:

1. **Check if active** : If highlighting is not active or line is empty, exit early
2. **Check previous state** : If `previousBlockState() > 0`, a multi-line block continues
   - Call `resumeBlock()` to continue highlighting
   - Return (skip normal processing)
3. **Process block patterns** (higher priority):
   - Loop through all `BlockMatch` entries
   - Find the opener using `indexOf()`
   - If opener not found, continue to next block pattern
   - If closer is on the same line, highlight from opener to closer
   - If closer is not found and `endsAtNewline` is true, highlight to end of line
   - If closer is not found and `endsAtNewline` is false, highlight to end of line and save block state
4. **Process exact matches** (lower priority):
   - Loop through all `ExactMatch` entries
   - Find all occurrences using `indexOf()` in a while loop
   - Apply formatting to each occurrence

### 2.3 How Multi-line Blocks Work

When a block starts on one line and ends on a later line:

1. **Line 1:** 
   - Opening pattern found, no closing pattern on same line
   - Format from opener to end of line
   - Call `setCurrentBlockState(block.id)` to save state
2. **Line 2 (and subsequent lines):**
   - `previousBlockState()` returns the saved block ID
   - `resumeBlock()` is called
   - Searches for the closing pattern
   - If not found, entire line is formatted and state is preserved
3. **Line N:**
   - `resumeBlock()` finds the closing pattern
   - Formats from start of line to closer
   - Calls `setCurrentBlockState(-1)` to clear the state
   - Next line processes normally

**Block state mechanics:**
- Each `BlockMatch` gets a unique `id` (1, 2, 3, ...)
- The `id` is stored via `setCurrentBlockState(block.id)`
- When resuming, `blockIdx = state - 1` to find the correct `BlockMatch`
- If the block index is out of range, state is cleared

---

## 3. Complete Examples

### 3.1 Python Syntax

**File:** `~/.vex/syntax/python.vxsyn`

```
REG = "Python"
ICNS = "python"
File = .py && .pyw && .pyx

⇏ Keywords
+ES'def'ES- && +ES'return'ES- && +ES'if'ES- && +ES'else'ES- && +ES'elif'ES- && +ES'while'ES- && +ES'for'ES- && +ES'in'ES- && +ES'class'ES- && +ES'import'ES- && +ES'from'ES- && +ES'as'ES- = color:keyword font:B

⇏ Built-in functions
+ES'print'ES- && +ES'len'ES- && +ES'range'ES- && +ES'str'ES- && +ES'int'ES- && +ES'list'ES- = color:keyword

⇏ Single-line comments
+HS'#'HS- to +HE''HE- = color:comment

⇏ String literals (double quotes)
+HS'delem"'HS- to +HE'delem"'HE- = color:string

⇏ String literals (single quotes)
+HS"delem'"HS- to +HE"delem'"HE- = color:string

⇏ Triple-quoted strings
+HS'"""'HS- to +HE'"""'HE- = color:string
+HS"'''"HS- to +HE"'''"HE- = color:string

⇏ Numbers
+ES'0'ES- && +ES'1'ES- && +ES'2'ES- && +ES'3'ES- && +ES'4'ES- && +ES'5'ES- && +ES'6'ES- && +ES'7'ES- && +ES'8'ES- && +ES'9'ES- = color:quote

⇏ Shebang detection
Sw = #!/usr/bin/env python && #!/usr/bin/python && # -*- coding: utf-8 -*-
```

### 3.2 C-style Comments

**File:** `~/.vex/syntax/c-comments.vxsyn`

```
REG = "C Comments Demo"
File = .c && .h && .cpp && .hpp && .cc

⇏ Multi-line comments
+HS'/*'HS- to +HE'*/'HE- = color:comment

⇏ Single-line comments
+HS'//'HS- to +HE''HE- = color:comment
```

### 3.3 HTML-like Tags

**File:** `~/.vex/syntax/html.vxsyn`

```
REG = "HTML"
ICNS = "text-html"
File = .html && .htm
Sw = <!DOCTYPE html && <html

⇏ Opening and closing tags
+HS'<'HS- to +HE'>'HE- = color:keyword font:B

⇏ Tag attributes
+ES'class='ES- && +ES'id='ES- && +ES'href='ES- && +ES'src='ES- = color:quote

⇏ Strings in attributes (double quotes)
+HS'delem"'HS- to +HE'delem"'HE- = color:string

⇏ Strings in attributes (single quotes)
+HS"delem'"HS- to +HE"delem'"HE- = color:string

⇏ HTML comments
+HS'<!--'HS- to +HE'-->'HE- = color:comment
```

### 3.4 JavaScript

**File:** `~/.vex/syntax/javascript.vxsyn`

```
REG = "JavaScript"
ICNS = "application-javascript"
File = .js && .jsx && .mjs

⇏ Keywords
+ES'function'ES- && +ES'return'ES- && +ES'if'ES- && +ES'else'ES- && +ES'for'ES- && +ES'while'ES- && +ES'const'ES- && +ES'let'ES- && +ES'var'ES- && +ES'class'ES- && +ES'import'ES- && +ES'export'ES- = color:keyword font:B

⇏ Single-line comments
+HS'//'HS- to +HE''HE- = color:comment

⇏ Multi-line comments
+HS'/*'HS- to +HE'*/'HE- = color:comment

⇏ String literals
+HS'delem"'HS- to +HE'delem"'HE- = color:string
+HS"delem'"HS- to +HE"delem'"HE- = color:string
+HS'`'HS- to +HE'`'HE- = color:string

⇏ Content detection
Sw = #!/usr/bin/env node && // JavaScript
```

---

## 4. Quick Reference

### 4.1 Core Directives

| Directive | Purpose | Example |
|-----------|---------|---------|
| `REG = "Name"` | Register language name | `REG = "Python"` |
| `ICNS = "icon"` | Set language icon | `ICNS = "python"` |
| `File = .ext` | File extension matching | `File = .py && .pyw` |
| `Sw = pattern` | Content detection (first 10 lines) | `Sw = #!/usr/bin/python` |

---

### 4.2 Pattern Matching

| Pattern | Name | Purpose | Example |
|---------|------|---------|---------|
| `+ES'text'ES-` | Enter Script | Exact text match | `+ES'if'ES- = color:keyword` |
| `+HS'start'HS- to +HE'end'HE-` | Periodic Block | Block match (start to end) | `+HS'/*'HS- to +HE'*/'HE- = color:comment` |
| `+HS'start'HS- to +HE''HE-` | EOL Block | Block to end of line | `+HS'#'HS- to +HE''HE- = color:comment` |

---

### 4.3 Operators and Keywords

| Symbol | Name | Usage | Rules |
|--------|------|-------|-------|
| `&&` | And operator | Combine patterns or list values | After `=` for File/Sw, before `=` for ES/HS |
| `=` | Equals | Initialize or define | Separates pattern from style |
| `to` | Separator | Between HS and HE | Interrupts conflict in block patterns |
| `delem` | Delimiter | Delimiter capture (planned feature) | `+HS'delem"'HS- to +HE'delem"'HE-` |
| `⇏ ` | Comment | Line comment in .vxsyn files | Lines starting with `⇏ ` are ignored |

---

### 4.4 Style Attributes

| Attribute | Values | Effect |
|-----------|--------|--------|
| `color:` | Hex code or named | Text color |
| | `#FF5733` | Custom hex color |
| | `comment` | Gray (128, 128, 128) |
| | `critical` | White (255, 255, 255) |
| | `quote` | Green (0, 255, 0) |
| | `keyword` | Light blue (135, 206, 250) |
| | `string` | Orange (255, 165, 0) |
| `font:` | Style code | Font weight/style |
| | `N` | Normal |
| | `B` | Bold |
| | `I` | Italic |
| | `BI` | Bold + Italic |

---

### 4.5 Processing Priority

1. **Detection order:** Sw patterns (content) → File extensions
2. **Highlighting order:** Block patterns (+HS...+HE) → Exact matches (+ES)
3. **Multi-line handling:** Previous block state → Current line patterns

---

### 4.6 File System

- **Syntax directory:** `~/.vex/syntax/`
- **File extension:** `.vxsyn`
- **Auto-reload:** Yes (via QFileSystemWatcher)
- **Multiple languages:** Supported in single file (use multiple REG directives)

---

### 4.7 UI Elements

- **Selector location:** Status bar (permanent widget, index 0)
- **Auto mode:** Runs detection automatically
- **Plain Text mode:** Disables highlighting
- **Manual selection:** Override auto-detection
- **Icon size:** 16x16 pixels

---

## 5. Advanced Notes

### 5.1 Parser Behavior

- Empty lines and lines starting with `⇏ ` are skipped
- Whitespace is trimmed from all patterns and values
- Quote extraction uses `indexOf()` to find first and second `"` characters
- Pattern extraction uses `indexOf()` with marker strings (+ES', 'ES-, etc.)

---

### 5.2 Block State Management

- Block states are 1-indexed (state 1 = block 0, state 2 = block 1, etc.)
- State -1 means no active block
- States persist across lines until closer is found
- If multiple blocks could match, the first one in the list wins

---

### 5.3 Extension Matching

- Extensions are normalized to lowercase with leading dot
- If user writes `.PY`, it becomes `.py`
- If user writes `py`, it becomes `.py`
- Matching is case-insensitive

---

### 5.4 Content Detection

- Only first 10 lines are checked
- Empty lines are skipped
- Lines are trimmed before checking
- First pattern match wins (stops checking other languages)

---

### 5.5 Syntax File Reloading

- Directory changes trigger full reload
- Individual file changes trigger single file reload
- Watched files that are deleted are automatically re-added when recreated
- Current selection is preserved across reloads when possible

---

### 5.6 Multiple Language Definitions

You can define multiple languages in one .vxsyn file:

```
REG = "C"
File = .c && .h
+ES'int'ES- = color:keyword

REG = "C++"
File = .cpp && .hpp
+ES'class'ES- = color:keyword
```

Each REG starts a new language definition. The previous definition is saved when a new REG is encountered.

---

## 6. Troubleshooting

### 6.1 Language Not Appearing in Dropdown

- Check that REG is defined
- Ensure .vxsyn file is in `~/.vex/syntax/`
- Check status bar for "Syntax dir changed" message
- Verify file has `.vxsyn` extension

---

### 6.2 Highlighting Not Working

- Verify language is selected (not "Plain Text")
- Check that patterns are correctly formatted
- Ensure `=` separates pattern from style
- Check that style attributes are valid (color: and font:)

---

### 6.3 Multi-line Blocks Not Closing

- Verify closer pattern exactly matches what's in the file
- Check that block.id is being set correctly
- Ensure closer pattern is not empty (unless EOL intended)

---

### 6.4 Wrong File Detected

- Content detection (Sw) takes priority over extensions
- Check if Sw patterns match file content unintentionally
- Use more specific Sw patterns
- Consider removing Sw if extension matching is sufficient

---