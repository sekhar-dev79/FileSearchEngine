// utils/appsettings.h
//
// All QSettings keys are defined here as string constants.
// Centralising them prevents typos — a mistyped key silently
// returns a default value, which is very hard to debug.
// Using a namespace instead of a class keeps the syntax clean:
//     AppSettings::Keys::SCAN_ROOT  instead of  AppSettings::SCAN_ROOT

#pragma once
#include <QString>
#include <QDir>

namespace AppSettings {

// ── Group names ───────────────────────────────────────────
// QSettings groups act like subdirectories in the registry.
// beginGroup("Window") → all window keys go under Window/...
inline constexpr char GROUP_WINDOW[]  = "Window";
inline constexpr char GROUP_SCAN[]    = "Scan";
inline constexpr char GROUP_FILTER[]  = "Filter";
inline constexpr char GROUP_VIEW[]    = "View";

// ── Window keys ───────────────────────────────────────────
inline constexpr char KEY_GEOMETRY[]      = "geometry";
inline constexpr char KEY_WINDOW_STATE[]  = "windowState";

// ── Scan keys ─────────────────────────────────────────────
inline constexpr char KEY_SCAN_ROOT[]     = "scanRoot";
inline constexpr char KEY_INDEX_CAP[]     = "indexCap";
inline constexpr char KEY_SKIP_HIDDEN[]   = "skipHidden";

// ── Filter keys ───────────────────────────────────────────
inline constexpr char KEY_EXT_FILTER[]    = "extensionFilter";
inline constexpr char KEY_SIZE_FILTER[]   = "minSizeKB";
inline constexpr char KEY_DATE_FILTER[]   = "dateFilterDays";

// ── View keys ─────────────────────────────────────────────
inline constexpr char KEY_COLUMN_STATE[]  = "columnState"; // QHeaderView state

// ── Default values ────────────────────────────────────────
inline QString defaultScanRoot()  { return QDir::homePath(); }
inline int     defaultIndexCap()  { return 500'000; }
inline bool    defaultSkipHidden(){ return true; }

} // namespace AppSettings
