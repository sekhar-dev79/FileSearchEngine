#include "mainwindow.h"
#include "searchbar.h"
#include "resultview.h"
#include "fileindexer.h"
#include "fileitem.h"
#include "filemodel.h"
#include "constants.h"
#include "searchengine.h"
#include "filterbar.h"
#include "settingsdialog.h"
#include "appSettings.h"
#include "aboutdialog.h"

#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QDebug>
#include <QStatusBar>
#include <QFileDialog>
#include <QDialog>
#include <QPushButton>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QKeySequence>
#include <QIcon>
#include <QHeaderView>
#include <QToolTip>

static QString chooseScanRoot(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QStringLiteral("Choose Scan Location"));
    // COMPACT: Reduced dialog size
    dlg.setFixedSize(380, 160);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    auto *label = new QLabel(
        QStringLiteral("Select which directory to index:\n"
                       "(Scanning all of C:\\ takes several minutes)"), &dlg);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *btnHome   = new QPushButton(QIcon(":/icons/home.svg"), QStringLiteral(" Home folder only  (%1)").arg(QDir::homePath()), &dlg);
    auto *btnFull   = new QPushButton(QIcon(":/icons/hard-drive.svg"), QStringLiteral(" Entire C:\\ drive  (slow, complete)"), &dlg);
    auto *btnCustom = new QPushButton(QIcon(":/icons/folder.svg"), QStringLiteral(" Choose folder..."), &dlg);

    const QString btnStyle = "QPushButton { text-align: left; padding: 4px 12px; border: 1px solid #CBD5E1; border-radius: 2px; background: #FFFFFF; } QPushButton:hover { background: #F1F5F9; }";
    btnHome->setStyleSheet(btnStyle);
    btnFull->setStyleSheet(btnStyle);
    btnCustom->setStyleSheet(btnStyle);

    layout->addWidget(btnHome);
    layout->addWidget(btnFull);
    layout->addWidget(btnCustom);

    QString result = QDir::homePath();

    QObject::connect(btnHome, &QPushButton::clicked, [&]() { result = QDir::homePath(); dlg.accept(); });
    QObject::connect(btnFull, &QPushButton::clicked, [&]() { result = QDir::rootPath(); dlg.accept(); });
    QObject::connect(btnCustom, &QPushButton::clicked, [&]() {
        const QString chosen = QFileDialog::getExistingDirectory(&dlg, QStringLiteral("Select Folder to Index"), QDir::homePath());
        if (!chosen.isEmpty()) { result = chosen; dlg.accept(); }
    });

    dlg.exec();
    return result;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_indexer(nullptr)
    , m_searchThread(nullptr)
    , m_searchEngine(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    setWindowTitle(QStringLiteral("FileSearchEngine"));
    setMinimumSize(Constants::WINDOW_MIN_WIDTH, Constants::WINDOW_MIN_HEIGHT);
    resize(Constants::WINDOW_DEF_WIDTH, Constants::WINDOW_DEF_HEIGHT);

    setupUi();
    setupStatusBar();
    setupConnections();
    setupShortcuts();
    setupTrayIcon();
    applyStyles();
    loadSettings();

    m_searchBar->setFocus();

#if defined(Q_OS_WIN)
    RegisterHotKey((HWND)winId(), 1001, MOD_CONTROL, VK_SPACE);
#endif
}

MainWindow::~MainWindow()
{
#if defined(Q_OS_WIN)
    UnregisterHotKey((HWND)winId(), 1001);
#endif

    if (m_workerThread) {
        m_indexer->cancelScan();
        m_indexer->disconnect();
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_indexer;
    }
    if (m_searchThread) {
        m_searchEngine->cancelSearch();
        m_searchEngine->disconnect();
        m_searchThread->quit();
        m_searchThread->wait();
        delete m_searchEngine;
    }
}

void MainWindow::startSearchEngine()
{
    if (m_searchThread) {
        QMetaObject::invokeMethod(m_searchEngine, [this]() {
            m_searchEngine->setIndex(m_masterIndex);
        }, Qt::QueuedConnection);
        return;
    }

    m_searchThread = new QThread(this);
    m_searchEngine = new SearchEngine();
    m_searchEngine->moveToThread(m_searchThread);

    connect(this, &MainWindow::searchRequested, m_searchEngine, &SearchEngine::search, Qt::QueuedConnection);
    connect(m_searchEngine, &SearchEngine::resultsReady, this, &MainWindow::onResultsReady, Qt::QueuedConnection);

    m_searchThread->start();

    const QVector<FileItem> snapshot = m_masterIndex;
    QMetaObject::invokeMethod(m_searchEngine, [this, snapshot]() {
        m_searchEngine->setIndex(snapshot);
    }, Qt::QueuedConnection);
}

void MainWindow::applyFilters()
{
    const FilterCriteria c = m_filterBar->criteria();

    if (m_filterBar->isDefault()) {
        m_model->setFiles(m_displayList);
        m_statsLabel->setText(QStringLiteral("%1 result%2").arg(m_displayList.size()).arg(m_displayList.size() == 1 ? u"" : u"s"));
        return;
    }

    QVector<FileItem> filtered;
    filtered.reserve(m_displayList.size());

    for (const FileItem &item : m_displayList) {
        if (!c.extensions.isEmpty()) {
            bool match = false;
            for (const QString &ext : c.extensions) {
                if (item.extension().compare(ext, Qt::CaseInsensitive) == 0) { match = true; break; }
            }
            if (!match) continue;
        }
        if (c.minSizeBytes > 0 && item.size < c.minSizeBytes) continue;
        if (c.modifiedAfter.isValid() && item.lastModified < c.modifiedAfter) continue;
        filtered.append(item);
    }

    m_model->setFiles(filtered);
    const QString msg = filtered.isEmpty() ? QStringLiteral("No results match filters")
                                           : QStringLiteral("%1 of %2 result%3 — filters active").arg(filtered.size()).arg(m_displayList.size()).arg(m_displayList.size() == 1 ? u"" : u"s");
    m_statsLabel->setText(msg);
}

void MainWindow::refreshExtensionList()
{
    QSet<QString> seen;
    QVector<QString> extensions;
    extensions.reserve(200);

    for (const FileItem &item : m_masterIndex) {
        const QString ext = item.extension();
        if (!ext.isEmpty() && !seen.contains(ext)) {
            seen.insert(ext);
            extensions.append(ext);
        }
    }
    std::sort(extensions.begin(), extensions.end());
    m_filterBar->updateExtensions(extensions);
}

void MainWindow::actionOpenFile(const FileItem &item)
{
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(item.path))) {
        QMessageBox::warning(this, QStringLiteral("Cannot Open File"), QStringLiteral("Failed to open:\n%1").arg(item.path));
    } else {
        statusBar()->showMessage(QStringLiteral("Opened: %1").arg(item.name), Constants::STATUSBAR_MSG_TIMEOUT);
    }
}

void MainWindow::actionShowInExplorer(const FileItem &item)
{
#if defined(Q_OS_WIN)
    const QStringList args = { QStringLiteral("/select,"), QDir::toNativeSeparators(item.path) };
    if (!QProcess::startDetached(QStringLiteral("explorer"), args)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(item.path).absolutePath()));
    }
#elif defined(Q_OS_MAC)
    QProcess::startDetached(QStringLiteral("open"), { QStringLiteral("-R"), item.path });
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(item.path).absolutePath()));
#endif
}

void MainWindow::actionCopyFullPath(const FileItem &item)
{
    QGuiApplication::clipboard()->setText(item.path);
    statusBar()->showMessage(QStringLiteral("Copied path: %1").arg(item.path), Constants::STATUSBAR_MSG_TIMEOUT);
}

void MainWindow::actionCopyFileName(const FileItem &item)
{
    QGuiApplication::clipboard()->setText(item.name);
    statusBar()->showMessage(QStringLiteral("Copied name: %1").arg(item.name), Constants::STATUSBAR_MSG_TIMEOUT);
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.beginGroup(QLatin1StringView(AppSettings::GROUP_WINDOW));
    s.setValue(QLatin1StringView(AppSettings::KEY_GEOMETRY), saveGeometry());
    s.setValue(QLatin1StringView(AppSettings::KEY_WINDOW_STATE), saveState());
    s.endGroup();

    s.beginGroup(QLatin1StringView(AppSettings::GROUP_SCAN));
    s.setValue(QLatin1StringView(AppSettings::KEY_SCAN_ROOT), m_scanRoot);
    s.setValue(QLatin1StringView(AppSettings::KEY_INDEX_CAP), Constants::MAX_INDEX_FILES);
    s.endGroup();

    const FilterCriteria c = m_filterBar->criteria();
    s.beginGroup(QLatin1StringView(AppSettings::GROUP_FILTER));
    s.setValue(QLatin1StringView(AppSettings::KEY_EXT_FILTER),  c.extensions);
    s.setValue(QLatin1StringView(AppSettings::KEY_SIZE_FILTER), static_cast<int>(c.minSizeBytes / 1024));
    s.setValue(QLatin1StringView(AppSettings::KEY_DATE_FILTER), c.modifiedAfter.isValid() ? static_cast<int>(c.modifiedAfter.daysTo(QDateTime::currentDateTime())) : 0);
    s.endGroup();

    s.beginGroup(QLatin1StringView(AppSettings::GROUP_VIEW));
    s.setValue(QLatin1StringView(AppSettings::KEY_COLUMN_STATE), m_resultView->horizontalHeader()->saveState());
    s.endGroup();

    s.beginGroup(QStringLiteral("Search"));
    s.setValue(QStringLiteral("History"), m_historyModel->stringList());
    s.endGroup();
}

void MainWindow::loadSettings()
{
    QSettings s;
    s.beginGroup(QLatin1StringView(AppSettings::GROUP_WINDOW));
    const QByteArray geo = s.value(QLatin1StringView(AppSettings::KEY_GEOMETRY)).toByteArray();
    if (!geo.isEmpty()) restoreGeometry(geo);
    const QByteArray state = s.value(QLatin1StringView(AppSettings::KEY_WINDOW_STATE)).toByteArray();
    if (!state.isEmpty()) restoreState(state);
    s.endGroup();

    s.beginGroup(QLatin1StringView(AppSettings::GROUP_SCAN));
    m_scanRoot = s.value(QLatin1StringView(AppSettings::KEY_SCAN_ROOT), AppSettings::defaultScanRoot()).toString();
    s.endGroup();

    s.beginGroup(QLatin1StringView(AppSettings::GROUP_VIEW));
    const QByteArray colState = s.value(QLatin1StringView(AppSettings::KEY_COLUMN_STATE)).toByteArray();
    if (!colState.isEmpty()) m_resultView->horizontalHeader()->restoreState(colState);
    s.endGroup();

    s.beginGroup(QStringLiteral("Search"));
    QStringList history = s.value(QStringLiteral("History")).toStringList();
    m_historyModel->setStringList(history);
    s.endGroup();
}

void MainWindow::addToSearchHistory(const QString &query)
{
    const QString q = query.trimmed();
    if (q.isEmpty() || q.length() < 3) return;
    QStringList hist = m_historyModel->stringList();
    hist.removeAll(q);
    hist.prepend(q);
    while (hist.size() > 20) hist.removeLast();
    m_historyModel->setStringList(hist);
}

void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    m_trayIcon = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/app.png")), this);
    m_trayMenu = new QMenu(this);

    QAction *showHideAction = m_trayMenu->addAction(QStringLiteral("Show / Hide"));
    m_trayMenu->addSeparator();
    QAction *settingsAction = m_trayMenu->addAction(QIcon(":/icons/settings.svg"), QStringLiteral("Settings  (Ctrl+,)"));
    QAction *rescanAction   = m_trayMenu->addAction(QIcon(":/icons/refresh-cw.svg"), QStringLiteral("Re-scan  (F5)"));
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QIcon(":/icons/power.svg"), QStringLiteral("Quit"));

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(showHideAction, &QAction::triggered, this, &MainWindow::toggleWindowVisibility);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onOpenSettings);
    connect(rescanAction, &QAction::triggered, this, &MainWindow::onRescan);
    connect(quitAction, &QAction::triggered, this, &MainWindow::onQuitRequested);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::toggleWindowVisibility()
{
    if (isVisible()) { hide(); }
    else { show(); raise(); activateWindow(); }
}

void MainWindow::onQuitRequested()
{
    if (QMessageBox::question(this, QStringLiteral("Confirm Quit"), QStringLiteral("Are you sure you want to quit FileSearchEngine?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        saveSettings();
        qApp->quit();
    }
}

void MainWindow::onShowAbout()
{
    AboutDialog dlg(m_masterIndex.size(), this);
    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        saveSettings();
        event->accept();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        toggleWindowVisibility();
    }
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    // COMPACT: Minimal margins across the entire app
    m_mainLayout->setContentsMargins(4, 4, 4, 2);
    m_mainLayout->setSpacing(4);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(4);

    m_searchBar = new SearchBar(m_centralWidget);
    // COMPACT: Slim 26px search bar
    m_searchBar->setFixedHeight(26);

    QLineEdit *innerEdit = m_searchBar->lineEdit();
    QAction *clearAction = innerEdit->addAction(QIcon(":/icons/x.svg"), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);

    connect(clearAction, &QAction::triggered, innerEdit, &QLineEdit::clear);
    connect(innerEdit, &QLineEdit::textChanged, this, [clearAction](const QString &text) { clearAction->setVisible(!text.isEmpty()); });

    m_rescanBtn   = new QPushButton(QIcon(":/icons/refresh-cw.svg"), QStringLiteral(""), m_centralWidget);
    m_settingsBtn = new QPushButton(QIcon(":/icons/settings.svg"),   QStringLiteral(""), m_centralWidget);
    m_aboutBtn    = new QPushButton(QIcon(":/icons/info.svg"),       QStringLiteral(""), m_centralWidget);
    m_quitBtn     = new QPushButton(QIcon(":/icons/power.svg"),      QStringLiteral(""), m_centralWidget);

    for (auto *btn : {m_rescanBtn, m_settingsBtn, m_aboutBtn, m_quitBtn}) {
        btn->setFixedSize(24, 24); // COMPACT: Smaller buttons
        btn->setIconSize(QSize(14, 14));
        btn->setObjectName(QStringLiteral("HeaderButton"));
        btn->setCursor(Qt::PointingHandCursor);
    }

    headerLayout->addWidget(m_searchBar, 1);
    headerLayout->addWidget(m_rescanBtn);
    headerLayout->addWidget(m_settingsBtn);
    headerLayout->addWidget(m_aboutBtn);
    headerLayout->addWidget(m_quitBtn);
    m_mainLayout->addLayout(headerLayout);

    m_filterBar = new FilterBar(m_centralWidget);
    m_filterBar->setFixedHeight(28); // COMPACT: Slimmer filter bar

    m_statsLabel = new QLabel(QStringLiteral("Ready — start typing to search"), m_centralWidget);
    m_statsLabel->setObjectName(QStringLiteral("StatsLabel"));

    // ── Result View & Loading Overlay (QStackedWidget) ──
    m_tableStack = new QStackedWidget(m_centralWidget);

    // Sleek, minimalist loading label
    m_loadingLabel = new QLabel(QStringLiteral("Scanning drive for files...\nPlease wait."), m_tableStack);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet(QStringLiteral(
        "background-color: #FFFFFF; border: 1px solid #CBD5E1; "
        "border-radius: 2px; color: #64748B; font-size: 13px; font-weight: 500;"
        ));

    m_resultView = new ResultView(m_tableStack);
    m_model = new FileModel(this);
    m_resultView->setModel(m_model);
    m_resultView->setMouseTracking(true);

    m_highlightDelegate = new HighlightDelegate(this);
    m_resultView->setItemDelegateForColumn(0, m_highlightDelegate);

    // COMPACT COLUMNS: Name, Type, Size
    m_resultView->setColumnWidth(0, 320); // Name
    m_resultView->setColumnWidth(1, 70);  // Type
    m_resultView->setColumnWidth(2, 90);  // Size

    m_tableStack->addWidget(m_loadingLabel); // Index 0: Loading Screen
    m_tableStack->addWidget(m_resultView);   // Index 1: The Table

    m_mainSplitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    m_mainSplitter->setHandleWidth(1);
    m_mainSplitter->setStyleSheet("QSplitter::handle { background-color: #CBD5E1; }");

    m_previewPanel = new PreviewPanel(m_mainSplitter);
    m_previewPanel->setMinimumWidth(340); // Compact preview width

    m_mainSplitter->addWidget(m_tableStack); // Add the stack INSTEAD of the raw result view
    m_mainSplitter->addWidget(m_previewPanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 2);

    m_mainLayout->addWidget(m_filterBar);
    m_mainLayout->addWidget(m_statsLabel);
    m_mainLayout->addWidget(m_mainSplitter, 1);

    m_liveSyncTimer = new QTimer(this);
    m_liveSyncTimer->setSingleShot(true);
    m_liveSyncTimer->setInterval(2000);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(Constants::SEARCH_DEBOUNCE_MS);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage(QStringLiteral("FileSearchEngine v%1 — Ready").arg(QLatin1StringView(Constants::APP_VERSION)));

    m_scanProgressBar = new QProgressBar(this);
    m_scanProgressBar->setObjectName(QStringLiteral("ScanProgressBar"));
    m_scanProgressBar->setRange(0, 0);
    m_scanProgressBar->setFixedSize(120, 8); // Ultra compact
    m_scanProgressBar->setTextVisible(false);
    m_scanProgressBar->setVisible(false);

    m_cancelScanBtn = new QPushButton(QIcon(":/icons/x.svg"), QStringLiteral(""), this);
    m_cancelScanBtn->setObjectName(QStringLiteral("CancelScanBtn"));
    m_cancelScanBtn->setFixedSize(16, 16);
    m_cancelScanBtn->setVisible(false);

    statusBar()->addPermanentWidget(m_scanProgressBar);
    statusBar()->addPermanentWidget(m_cancelScanBtn);

    connect(m_cancelScanBtn, &QPushButton::clicked, this, [this]() {
        if (m_indexer && m_workerThread && m_workerThread->isRunning()) {
            m_indexer->cancelScan();
            m_cancelScanBtn->setEnabled(false);
        }
    });

    m_historyModel = new QStringListModel(this);
    m_searchCompleter = new QCompleter(m_historyModel, this);
    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setFilterMode(Qt::MatchStartsWith);
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion);

    if (m_searchCompleter->popup()) {
        m_searchCompleter->popup()->setStyleSheet(
            "QListView { background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 2px; padding: 2px; outline: none; font-size: 11px; }"
            "QListView::item { padding: 4px 8px; border-radius: 2px; color: #1E293B; }"
            "QListView::item:selected { background-color: #F1F5F9; color: #0F172A; }"
            );
    }
    m_searchBar->lineEdit()->setCompleter(m_searchCompleter);
}

void MainWindow::setupConnections()
{
    connect(m_searchBar->lineEdit(), &QLineEdit::textChanged, this, [this](const QString &q) { m_pendingQuery = q; m_debounceTimer->start(); });
    connect(m_debounceTimer, &QTimer::timeout, this, [this]() { onSearchTriggered(m_pendingQuery); });

    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);
    connect(m_rescanBtn,   &QPushButton::clicked, this, &MainWindow::onRescan);
    connect(m_aboutBtn,    &QPushButton::clicked, this, &MainWindow::onShowAbout);
    connect(m_quitBtn,     &QPushButton::clicked, this, &MainWindow::onQuitRequested);

    connect(m_resultView, &QAbstractItemView::doubleClicked, this, &MainWindow::onRowDoubleClicked);
    connect(m_filterBar, &FilterBar::filtersChanged, this, &MainWindow::onFiltersChanged);
    connect(m_resultView, &ResultView::contextMenuRequestedForRow, this, &MainWindow::onContextMenuRequested);

    connect(m_resultView, &ResultView::enterPressedOnRow, this, [this](int row) { if (row >= 0 && row < m_model->fileCount()) actionOpenFile(m_model->fileAt(row)); });
    connect(m_resultView, &ResultView::copyPressedOnRow, this, [this](int row) { if (row >= 0 && row < m_model->fileCount()) actionCopyFullPath(m_model->fileAt(row)); });

    connect(m_resultView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        if (current.isValid() && current.row() < m_model->fileCount()) {
            m_previewPanel->setFile(m_model->fileAt(current.row()));
        } else {
            m_previewPanel->clearPreview();
        }
    });
}

void MainWindow::setupShortcuts()
{
    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &MainWindow::onEscapePressed);

    auto *focusShortcut = new QShortcut(QKeySequence::Find, this);
    connect(focusShortcut, &QShortcut::activated, this, &MainWindow::onFocusSearch);

    auto *rescanShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(rescanShortcut, &QShortcut::activated, this, &MainWindow::onRescan);

    auto *settingsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
    connect(settingsShortcut, &QShortcut::activated, this, &MainWindow::onOpenSettings);

    auto *showShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this);
    connect(showShortcut, &QShortcut::activated, this, &MainWindow::toggleWindowVisibility);
}

void MainWindow::onContextMenuRequested(int row, const QPoint &globalPos)
{
    if (row < 0 || row >= m_model->fileCount()) return;
    const FileItem &item = m_model->fileAt(row);

    QMenu menu(this);
    menu.addAction(QIcon(":/icons/external-link.svg"), QStringLiteral("Open File"), this, [&]() { actionOpenFile(item); });
    menu.addAction(QIcon(":/icons/folder-open.svg"), QStringLiteral("Show in Explorer"), this, [&]() { actionShowInExplorer(item); });
    menu.addSeparator();
    menu.addAction(QIcon(":/icons/copy.svg"), QStringLiteral("Copy Full Path"), this, [&]() { actionCopyFullPath(item); });
    menu.addAction(QIcon(":/icons/file-text.svg"), QStringLiteral("Copy File Name"), this, [&]() { actionCopyFileName(item); });

    menu.exec(globalPos);
}

void MainWindow::onEscapePressed()
{
    if (!m_searchBar->text().isEmpty()) {
        m_searchBar->clear();
        m_searchBar->setFocus();
    } else {
        m_resultView->setFocus();
    }
}

void MainWindow::onFocusSearch()
{
    m_searchBar->setFocus();
    m_searchBar->selectAll();
}

void MainWindow::onRescan()
{
    if (m_workerThread) {
        m_indexer->cancelScan();
        m_indexer->disconnect();
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_indexer;
        delete m_workerThread;
    }
    if (m_searchThread) {
        m_searchEngine->cancelSearch();
        m_searchEngine->disconnect();
        m_searchThread->quit();
        m_searchThread->wait();
        delete m_searchEngine;
        delete m_searchThread;
    }

    m_searchThread = nullptr; m_searchEngine = nullptr;
    m_workerThread = nullptr; m_indexer = nullptr;

    m_masterIndex.clear(); m_displayList.clear(); m_model->clear();
    m_searchBar->clear(); m_filterBar->resetFilters();

    startIndexing();
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QSettings s;
    s.beginGroup(QLatin1StringView(AppSettings::GROUP_SCAN));
    s.setValue(QLatin1StringView(AppSettings::KEY_SCAN_ROOT),  dlg.scanRoot());
    s.setValue(QLatin1StringView(AppSettings::KEY_INDEX_CAP),  dlg.indexCap());
    s.setValue(QLatin1StringView(AppSettings::KEY_SKIP_HIDDEN), dlg.skipHidden());
    s.endGroup();

    m_scanRoot = dlg.scanRoot();
    onRescan();
}

void MainWindow::startIndexing()
{
    if (m_scanRoot.isEmpty() || !QDir(m_scanRoot).exists()) {
        m_scanRoot = chooseScanRoot(this);
    }

    m_tableStack->setCurrentIndex(0);
    m_workerThread = new QThread(this);
    m_indexer      = new FileIndexer();
    m_indexer->moveToThread(m_workerThread);

    connect(m_indexer, &FileIndexer::batchReady, this, &MainWindow::onBatchReady, Qt::QueuedConnection);
    connect(m_indexer, &FileIndexer::scanFinished, this, &MainWindow::onScanFinished, Qt::QueuedConnection);
    connect(m_indexer, &FileIndexer::progressUpdate, this, &MainWindow::onProgressUpdate, Qt::QueuedConnection);
    connect(m_indexer, &FileIndexer::sourceReport, this, [this](const QString &msg) { statusBar()->showMessage(msg, Constants::STATUSBAR_MSG_TIMEOUT); }, Qt::QueuedConnection);

    connect(m_indexer, &FileIndexer::liveChangeDetected, this, [this]() { if (m_liveSyncTimer) m_liveSyncTimer->start(); }, Qt::QueuedConnection);
    connect(m_liveSyncTimer, &QTimer::timeout, this, [this]() {
        if (m_indexer && m_workerThread && m_workerThread->isRunning()) {
            QMetaObject::invokeMethod(m_indexer, "silentRescan", Qt::QueuedConnection, Q_ARG(QString, m_scanRoot));
        }
    });

    connect(m_indexer, &FileIndexer::silentIndexReady, this, [this](const QVector<FileItem> &newIndex) {
        m_masterIndex = newIndex;
        if (m_searchBar->text().isEmpty()) { m_displayList = m_masterIndex; applyFilters(); }
        startSearchEngine();
    }, Qt::QueuedConnection);

    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(cacheDir);
    m_cacheFilePath = cacheDir + QLatin1Char('/') + QLatin1StringView(Constants::INDEX_CACHE_FILENAME);

    const QString cachePath = m_cacheFilePath;
    const QString scanRoot  = m_scanRoot;

    connect(m_workerThread, &QThread::started, m_indexer, [this, cachePath, scanRoot]() { m_indexer->startWithCacheOrScan(cachePath, scanRoot); }, Qt::QueuedConnection);

    m_scanProgressBar->setVisible(true);
    m_cancelScanBtn->setVisible(true);
    m_cancelScanBtn->setEnabled(true);

    m_workerThread->start();
}

void MainWindow::onBatchReady(const QVector<FileItem> &batch)
{
    if (m_tableStack->currentIndex() == 0 && !m_masterIndex.isEmpty()) {
        m_tableStack->setCurrentIndex(1);
    }
    m_masterIndex.reserve(m_masterIndex.size() + batch.size());
    m_masterIndex.append(batch);
    m_model->addBatch(batch);
    m_statsLabel->setText(QStringLiteral("Indexing... %1 files found").arg(m_masterIndex.size()));
}

void MainWindow::onScanFinished(int total)
{
    m_tableStack->setCurrentIndex(1);

    m_scanProgressBar->setVisible(false);
    m_cancelScanBtn->setVisible(false);
    m_displayList = m_masterIndex;

    m_statsLabel->setText(QStringLiteral("Index ready — %1 files. Start typing to search.").arg(m_masterIndex.size()));
    setWindowTitle(QStringLiteral("FileSearchEngine — %1 files indexed").arg(total));

    refreshExtensionList();
    startSearchEngine();
}

void MainWindow::onProgressUpdate(const QString &path)
{
    const QString d = path.length() > 60 ? QStringLiteral("...") + path.right(57) : path;
    statusBar()->showMessage(QStringLiteral("Scanning: %1").arg(d));
}

void MainWindow::onSearchTriggered(const QString &query)
{
    if (!m_searchEngine) return;
    m_searchEngine->cancelSearch();

    if (m_highlightDelegate) m_highlightDelegate->setSearchTerm(query);

    if (query.trimmed().isEmpty()) {
        m_displayList = m_masterIndex;
        applyFilters();
        return;
    }

    m_statsLabel->setText(QStringLiteral("Searching..."));
    addToSearchHistory(query);
    emit searchRequested(query, m_searchBar->isRegexMode());
}

void MainWindow::onRowDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    actionOpenFile(m_model->fileAt(index.row()));
}

void MainWindow::onResultsReady(const QVector<FileItem> &results, const QString &query)
{
    m_displayList = results;
    applyFilters();
    if (!query.trimmed().isEmpty()) {
        setWindowTitle(QStringLiteral("FileSearchEngine — \"%1\" — %2 result%3").arg(query).arg(results.size()).arg(results.size() == 1 ? u"" : u"s"));
    }
}

void MainWindow::onFiltersChanged() { applyFilters(); }

void MainWindow::applyStyles()
{
    // CLASSIC COMPACT STYLESHEET
    qApp->setStyleSheet(R"(
        QWidget {
            background-color: #F8FAFC;
            color: #1E293B;
            font-family: "Segoe UI", -apple-system, sans-serif;
            font-size: 11px; /* STRICTLY SMALL FONT */
        }

        /* ── Tooltips (The Hover UI) ── */
        QToolTip {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 1px solid #94A3B8;
            border-radius: 2px;
            padding: 4px 6px;
            font-size: 11px;
        }

        QLabel { background: transparent; color: #334155; }
        #StatsLabel { font-weight: 600; padding: 2px 4px; }

        /* ── Search Box (Ultra-Compact) ── */
        #SearchLineEdit {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px; /* Sharp corners */
            padding: 2px 8px;
            font-size: 12px;
        }
        #SearchLineEdit:hover { border: 1px solid #94A3B8; }
        #SearchLineEdit:focus { border: 1px solid #3B82F6; background-color: #FFFFFF; }

        /* ── Menus ── */
        QMenu { background-color: #FFFFFF; border: 1px solid #CBD5E1; padding: 2px 0px; }
        QMenu::item { padding: 4px 24px 4px 28px; margin: 1px 4px; border-radius: 2px; }
        QMenu::item:selected { background-color: #E2E8F0; color: #0F172A; }
        QMenu::separator { height: 1px; background-color: #E2E8F0; margin: 2px 8px; }

        /* ── The Table (ResultView) ── */
        #ResultView {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            outline: none;
            gridline-color: #F1F5F9;
        }
        #ResultView::item {
            padding: 2px 4px; /* Tightly packed rows */
            border-bottom: 1px solid #F8FAFC;
        }
        #ResultView::item:selected { background-color: #DBEAFE; color: #1E3A8A; }
        #ResultView::item:hover:!selected { background-color: #F1F5F9; }

        /* ── Table Headers ── */
        QHeaderView::section {
            background-color: #F1F5F9;
            color: #475569;
            border: none;
            border-bottom: 1px solid #CBD5E1;
            border-right: 1px solid #E2E8F0;
            padding: 4px 8px;
            font-weight: 600;
            text-align: left;
        }
        QHeaderView::down-arrow, QHeaderView::up-arrow {
            width: 10px; height: 10px; padding-right: 4px;
            subcontrol-origin: padding; subcontrol-position: center right;
        }
        QHeaderView::down-arrow { image: url(:/icons/chevron-down.svg); }
        QHeaderView::up-arrow { image: url(:/icons/chevron-up.svg); }

        /* ── Scrollbars (Thinner) ── */
        QScrollBar:vertical { background: transparent; width: 8px; margin: 1px; }
        QScrollBar::handle:vertical { background: #CBD5E1; border-radius: 4px; min-height: 20px; }
        QScrollBar::handle:vertical:hover { background: #94A3B8; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; height: 0px; }

        /* ── Status Bar ── */
        QStatusBar { background-color: #F1F5F9; color: #475569; border-top: 1px solid #CBD5E1; font-size: 10px; }
        QStatusBar QLabel { background: transparent; }

        /* ── Header Action Buttons ── */
        QPushButton#HeaderButton { background-color: transparent; border: 1px solid transparent; border-radius: 2px; padding: 2px; }
        QPushButton#HeaderButton:hover { background-color: #E2E8F0; border-color: #CBD5E1; }
        QPushButton#HeaderButton:pressed { background-color: #CBD5E1; border-color: #94A3B8; }

        QPushButton#RegexToggle { background-color: transparent; border: 1px solid #CBD5E1; border-radius: 2px; color: #64748B; font-weight: bold; font-family: "Courier New"; font-size: 10px; padding: 2px; }
        QPushButton#RegexToggle:checked { background-color: #3B82F6; color: white; border: none; }

        QProgressBar#ScanProgressBar { background-color: #E2E8F0; border-radius: 2px; border: none; max-height: 8px; }
        QProgressBar#ScanProgressBar::chunk { background-color: #3B82F6; border-radius: 2px; }
        QPushButton#CancelScanBtn { border: none; background: transparent; border-radius: 2px; }
        QPushButton#CancelScanBtn:hover { background-color: #FEE2E2; border: 1px solid #FECACA; }

        QComboBox { border-radius: 2px; padding: 2px 6px; border: 1px solid #CBD5E1; }
        QComboBox QAbstractItemView::item { min-height: 20px; }
    )");
}

#if defined(Q_OS_WIN)
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType); Q_UNUSED(result);
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == 1001) {
        if (isVisible() && isActiveWindow()) { hide(); }
        else { showNormal(); raise(); activateWindow(); m_searchBar->setFocus(); m_searchBar->selectAll(); }
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif
