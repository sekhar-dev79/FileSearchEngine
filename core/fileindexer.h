#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QFileSystemWatcher>
#include <atomic>
#include "fileitem.h"

class FileIndexer : public QObject
{
    Q_OBJECT

public:
    explicit FileIndexer(QObject *parent = nullptr);
    ~FileIndexer() override;

    void cancelScan();

public slots:
    void startWithCacheOrScan(const QString &cacheFilePath, const QString &rootPath);

    // NEW: Performs a fast, background update without blocking the UI
    void silentRescan(const QString &rootPath);

signals:
    void batchReady(const QVector<FileItem> &batch);
    void scanFinished(int totalFiles);
    void progressUpdate(const QString &currentDirectory);
    void sourceReport(const QString &message);

    // NEW: Ghost Engine Signals
    void liveChangeDetected(); // Tells MainWindow that the OS changed a file
    void silentIndexReady(const QVector<FileItem> &newIndex); // Sends the new data to hot-swap

private slots:
    // NEW: Catches the OS signal
    void onSystemPathChanged(const QString &path);

private:
    int loadAndEmitCache(const QString &cacheFilePath);

    // Updated to accept the 'isSilent' flag so we don't trigger UI loading bars
    void runScanWithIncrementalSave(const QString &rootPath,
                                    const QString &cacheFilePath,
                                    bool isSilent = false);

    std::atomic<bool> m_cancelled { false };
    QString m_currentRoot;

    // ── Ghost Engine Variables ──
    QFileSystemWatcher *m_watcher;
    QVector<FileItem> m_silentAccumulator; // Stores the hot-swap data
};
