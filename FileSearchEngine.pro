# ============================================================
# FileSearchEngine.pro
# Qt 6.7.3 | C++17 | qmake | Qt Widgets
# ============================================================

QT += core widgets concurrent svg

CONFIG += c++17
CONFIG -= debug_and_release   # Use separate debug/release builds

TARGET   = FileSearchEngine
TEMPLATE = app

# Strict compiler warnings — catch bugs early
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# Treat warnings as errors in release builds
CONFIG(release, debug|release): QMAKE_CXXFLAGS += -Werror

# ── Sources ──────────────────────────────────────────────────
SOURCES += \
    core/fileindexer.cpp \
    main.cpp \
    models/filemodel.cpp \
    search/searchengine.cpp \
    ui/aboutdialog.cpp \
    ui/filterbar.cpp \
    ui/highlightdelegate.cpp \
    ui/mainwindow.cpp \
    ui/previewpanel.cpp \
    ui/resultview.cpp \
    ui/searchbar.cpp \
    ui/settingsdialog.cpp

# ── Headers ──────────────────────────────────────────────────
HEADERS += \
    core/fileindexer.h \
    models/fileitem.h \
    models/filemodel.h \
    search/searchengine.h \
    ui/aboutdialog.h \
    ui/filterbar.h \
    ui/highlightdelegate.h \
    ui/mainwindow.h \
    ui/previewpanel.h \
    ui/resultview.h \
    ui/searchbar.h \
    ui/settingsdialog.h \
    utils/appSettings.h \
    utils/constants.h \
    utils/helpers.h

# ── Include paths ─────────────────────────────────────────────
INCLUDEPATH += \
    core    \
    models  \
    search  \
    ui      \
    utils

RESOURCES += \
    resources.qrc



