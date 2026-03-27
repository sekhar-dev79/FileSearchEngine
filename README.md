# FileSearchEngine 🚀

A blazing-fast, minimalist file search utility for Windows, built with C++ and Qt 6. 
Designed for power users, it features a highly compact UI, native global hotkeys, and real-time background file syncing.

![Screenshot of FileSearchEngine](link-to-your-screenshot-here.png)

## ✨ Key Features
* **Global Summon Shortcut:** Instantly bring the app to the center of your screen from anywhere using `Ctrl + Space` (Native Windows API hook).
* **The "Ghost Engine":** Utilizes `QFileSystemWatcher` to silently monitor the OS in the background, keeping the search index perfectly up-to-date without freezing the UI.
* **Multi-Threaded Architecture:** Search queries and file indexing are offloaded to dedicated background threads (`QThread`), ensuring the UI stays butter-smooth even when searching millions of files.
* **Classic Minimalist UI:** Information-dense, tight layout designed for speed. Features a dynamic Preview Panel with syntax highlighting for code files and thumbnail generation for images.
* **Rich Data Tooltips:** Hover over any search result to reveal instant, formatted metadata without cluttering the main table.

## 🛠️ Tech Stack
* **Language:** C++17
* **Framework:** Qt 6.7.3 (Widgets module)
* **OS Integration:** Native Windows API (`windows.h`) for global hotkey registration.

## ⚙️ Building from Source
1. Clone this repository.
2. Open `FileSearchEngine.pro` in Qt Creator.
3. Configure the project with the **MinGW 64-bit** compiler.
4. Build and Run in **Release** mode.