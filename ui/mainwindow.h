#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QThread>
#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QCloseEvent>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QProgressBar>
#include <QStringListModel>
#include <QCompleter>
#include <QSplitter>
#include <QStackedWidget>

#include "previewpanel.h"
#include "fileitem.h"
#include "filterbar.h"
#include "highlightdelegate.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

class SearchBar;
class ResultView;
class FileIndexer;
class FileModel;
class SearchEngine;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void startIndexing();

signals:
    void searchRequested(const QString &query, bool isRegex);

private slots:
    void onSearchTriggered(const QString &query);
    void onBatchReady(const QVector<FileItem> &batch);
    void onScanFinished(int total);
    void onProgressUpdate(const QString &path);
    void onRowDoubleClicked(const QModelIndex &index);
    void onResultsReady(const QVector<FileItem> &results, const QString &query);
    void onFiltersChanged();
    void onContextMenuRequested(int row, const QPoint &globalPos);

    // ── keyboard shortcut slots ──────────────────────────
    void onEscapePressed();       // clear search + restore full index
    void onFocusSearch();         // Ctrl+F → focus search bar
    void onRescan();              // F5 → rescan with new root

    void onOpenSettings();

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowAbout();
    void toggleWindowVisibility();
    void onQuitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

// ── NATIVE OS EVENT HOOK ──
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    void setupUi();
    void setupStatusBar();
    void setupConnections();
    void setupShortcuts();
    void applyStyles();
    void startSearchEngine();
    void applyFilters();
    void refreshExtensionList();

    // ── context menu actions ─────────────────────────────
    void actionOpenFile(const FileItem &item);
    void actionShowInExplorer(const FileItem &item);
    void actionCopyFullPath(const FileItem &item);
    void actionCopyFileName(const FileItem &item);

    void saveSettings();        // called in closeEvent
    void loadSettings();        // called in constructor
    void saveColumnState();     // saves QHeaderView column widths
    void restoreColumnState();  // restores QHeaderView column widths

    void setupTrayIcon();
    // void setupMenuBar();

    void addToSearchHistory(const QString &query);



    // ── Widgets ───────────────────────────────────────────────
    QWidget     *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    SearchBar   *m_searchBar;
    FilterBar   *m_filterBar;

    QStackedWidget  *m_tableStack;
    QLabel          *m_loadingLabel;
    ResultView  *m_resultView;
    QLabel      *m_statsLabel;

    QTimer      *m_debounceTimer;
    QTimer      *m_liveSyncTimer;

    QString      m_pendingQuery;
    QString      m_cacheFilePath;
    QString      m_scanRoot;

    // ── Data layers ───────────────────────────────────────────
    // m_masterIndex  — full index, never modified after scan
    // m_displayList  — current search results (pre-filter)
    // m_model->data  — currently visible rows (post-filter)
    QVector<FileItem>  m_masterIndex;
    QVector<FileItem>  m_displayList;

    FileModel   *m_model;

    // ── Threads ───────────────────────────────────────────────
    QThread     *m_workerThread;
    FileIndexer *m_indexer;
    QThread     *m_searchThread;
    SearchEngine *m_searchEngine;

    QSystemTrayIcon *m_trayIcon;
    QMenu           *m_trayMenu;

    HighlightDelegate *m_highlightDelegate;

    QProgressBar     *m_scanProgressBar;
    QPushButton      *m_cancelScanBtn;
    QStringListModel *m_historyModel;
    QCompleter       *m_searchCompleter;

    QPushButton *m_settingsBtn;
    QPushButton *m_rescanBtn;
    QPushButton *m_aboutBtn;
    QPushButton *m_quitBtn;

    QSplitter    *m_mainSplitter;
    PreviewPanel *m_previewPanel;

};
