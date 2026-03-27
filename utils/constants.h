// utils/constants.h
#pragma once

#include <QString>
#include <QStringList>

namespace Constants {

inline constexpr char    APP_NAME[]             = "FileSearchEngine";
inline constexpr char    APP_VERSION[]          = "1.0.0";
inline constexpr int     WINDOW_MIN_WIDTH       = 850;
inline constexpr int     WINDOW_MIN_HEIGHT      = 550;
inline constexpr int     WINDOW_DEF_WIDTH       = 950;
inline constexpr int     WINDOW_DEF_HEIGHT      = 600;
inline constexpr int     SEARCH_DEBOUNCE_MS     = 150;
inline constexpr int     INDEX_BATCH_SIZE       = 500;
inline constexpr int     STATUSBAR_MSG_TIMEOUT  = 3000;
inline constexpr char    INDEX_CACHE_FILENAME[] = "fileindex.cache";
inline constexpr quint32 INDEX_CACHE_MAGIC      = 0xF5E1CA4E;
inline constexpr quint32 INDEX_CACHE_VERSION    = 4;

// Maximum files to index — safety cap.
// Prevents runaway scans on machines with millions of build artifacts.
inline constexpr int MAX_INDEX_FILES = 500'000;

// ── Directories to SKIP entirely before descending ────────
// Matched against the FULL lowercase absolute path using contains().
// Add any noisy path here — the scanner will not enter it at all.
inline QStringList skippedDirectories()
{
    return {
        // ── Windows OS internals ──────────────────────────
        "c:/windows/winsxs",
        "c:/windows/softwaredistribution",
        "c:/windows/temp",
        "c:/windows/prefetch",
        "c:/windows/installer",
        "c:/windows/assembly",
        "c:/windows/microsoft.net",
        "c:/windows/servicing",
        "c:/windows/logs",
        "c:/windows/panther",
        "c:/windows/debug",
        "c:/windows/inf",
        "c:/windows/drivers",

        // ── Windows Store / UWP ───────────────────────────
        "c:/program files/windowsapps",
        "c:/programdata/microsoft",
        "c:/programdata/package cache",

        // ── Recycle bin / shadow copies ───────────────────
        "$recycle.bin",
        "system volume information",
        "recovery",

        // ── Browser caches (Chrome, Edge, Firefox) ────────
        "appdata/local/google/chrome/user data/default/cache",
        "appdata/local/google/chrome/user data/default/code cache",
        "appdata/local/microsoft/edge/user data/default/cache",
        "appdata/local/mozilla/firefox/profiles",
        "appdata/local/temp",
        "appdata/locallow/temp",

        // ── Visual Studio / MSBuild ───────────────────────
        ".vs",                              // VS solution hidden dir
        "appdata/local/microsoft/visualstudio",
        "appdata/local/microsoft/vsbuildtools",
        "appdata/local/microsoft/msbuild",

        // ── Node.js / npm / yarn ──────────────────────────
        "node_modules",                     // single most common noise
        "appdata/roaming/npm-cache",
        "appdata/local/npm-cache",
        ".yarn/cache",

        // ── Java build tools ──────────────────────────────
        ".gradle",
        ".m2",                              // Maven local repository
        ".ivy2",

        // ── Python ────────────────────────────────────────
        "__pycache__",
        ".tox",
        "appdata/local/pip/cache",
        "appdata/roaming/python",

        // ── Qt / CMake build artifacts ────────────────────
        // These are build output folders — not source files.
        // Named "build" and "debug"/"release" are too generic to skip globally.
        // Only skip the Qt installation's own generated dirs:
        "appdata/local/qt",
        "appdata/roaming/qt",
        "/qt/qt-everywhere",                // Qt source builds on Linux

        // ── Docker / VM / container layers ───────────────
        "appdata/local/docker",
        ".docker",
        ".vagrant",

        // ── Package manager caches ────────────────────────
        "appdata/local/nuget/cache",
        "appdata/roaming/nuget",

        // ── macOS / Linux virtual fs ──────────────────────
        "/proc", "/sys", "/dev", "/run",
    };
}

} // namespace Constants
