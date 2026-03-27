// core/fileindexer.cpp
#include "fileindexer.h"
#include "constants.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QQueue>
#include <QSet>
#include <QDebug>

// ── QDataStream serialization ────────────────────────────────────────────────
QDataStream &operator<<(QDataStream &out, const FileItem &item)
{
    out << item.name << item.path << item.size << item.lastModified;
    return out;
}

QDataStream &operator>>(QDataStream &in, FileItem &item)
{
    in >> item.name >> item.path >> item.size >> item.lastModified;
    return in;
}

// ─────────────────────────────────────────────────────────────────────────────
FileIndexer::FileIndexer(QObject *parent)
    : QObject(parent)
{
    // ── Initialize the Ghost Engine ──
    m_watcher = new QFileSystemWatcher(this);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileIndexer::onSystemPathChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileIndexer::onSystemPathChanged);
}

FileIndexer::~FileIndexer() = default;

void FileIndexer::cancelScan()
{
    m_cancelled.store(true, std::memory_order_relaxed);
}

void FileIndexer::onSystemPathChanged(const QString &path)
{
    qDebug() << "[Ghost Engine] OS reported change at:" << path;
    emit liveChangeDetected(); // Tells MainWindow to start the 2-second debounce timer
}

// ── startWithCacheOrScan ─────────────────────────────────────────────────────
void FileIndexer::startWithCacheOrScan(const QString &cacheFilePath,
                                       const QString &rootPath)
{
    m_cancelled.store(false, std::memory_order_relaxed);
    m_currentRoot = rootPath;

    qDebug() << "[FSE] startWithCacheOrScan | root:" << rootPath;

    const int cached = loadAndEmitCache(cacheFilePath);
    if (cached >= 0)
    {
        qDebug() << "[FSE] ✅ Cache hit:" << cached << "files";
        emit sourceReport(QStringLiteral("Loaded %1 files from cache").arg(cached));
        emit scanFinished(cached);

        // Even on a cache hit, we need to attach the Ghost Engine.
        // Doing a silent rescan immediately maps the folders to the watcher.
        silentRescan(rootPath);
        return;
    }

    qDebug() << "[FSE] Cache miss — starting BFS scan";
    emit sourceReport(QStringLiteral("Scanning: %1").arg(rootPath));

    // Normal scan (isSilent = false)
    runScanWithIncrementalSave(rootPath, cacheFilePath, false);
}

// ── silentRescan (Ghost Engine Background Sync) ──────────────────────────────
void FileIndexer::silentRescan(const QString &rootPath)
{
    m_cancelled.store(false, std::memory_order_relaxed);
    m_currentRoot = rootPath;

    qDebug() << "[Ghost Engine] Starting silent background sync...";

    // Silent scan (isSilent = true)
    runScanWithIncrementalSave(rootPath, QStringLiteral(""), true);
}

// ── loadAndEmitCache ─────────────────────────────────────────────────────────
int FileIndexer::loadAndEmitCache(const QString &cacheFilePath)
{
    QFile file(cacheFilePath);
    if (!file.exists())              { qDebug() << "[FSE] No cache file"; return -1; }
    if (!file.open(QIODevice::ReadOnly)) { qDebug() << "[FSE] Cannot open cache"; return -1; }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    quint32   magic   = 0;
    quint32   version = 0;
    QDateTime timestamp;
    QString   cachedRoot;

    in >> magic >> version >> timestamp >> cachedRoot;

    if (in.status() != QDataStream::Ok) { file.close(); file.remove(); return -1; }
    if (magic != Constants::INDEX_CACHE_MAGIC) { file.close(); file.remove(); return -1; }
    if (version != Constants::INDEX_CACHE_VERSION) { file.close(); file.remove(); return -1; }
    if (cachedRoot != m_currentRoot) { file.close(); file.remove(); return -1; }

    QVector<FileItem> batch;
    batch.reserve(Constants::INDEX_BATCH_SIZE);
    int total = 0;

    while (!in.atEnd())
    {
        if (m_cancelled.load(std::memory_order_relaxed)) return -1;
        FileItem item;
        in >> item;
        if (in.status() != QDataStream::Ok) break;
        batch.append(std::move(item));
        ++total;
        if (batch.size() >= Constants::INDEX_BATCH_SIZE)
        {
            emit batchReady(batch);
            batch.clear();
            batch.reserve(Constants::INDEX_BATCH_SIZE);
        }
    }
    if (!batch.isEmpty()) emit batchReady(batch);

    return total;
}

// ── runScanWithIncrementalSave ────────────────────────────────────────────────
void FileIndexer::runScanWithIncrementalSave(const QString &rootPath,
                                             const QString &cacheFilePath,
                                             bool isSilent)
{
    qDebug() << "[FSE] runScanWithIncrementalSave() | isSilent:" << isSilent;

    QFile cacheFile;
    bool cacheOpen = false;
    QDataStream cacheOut;

    // Only touch the cache file if this is a primary, user-facing scan
    if (!isSilent)
    {
        cacheFile.setFileName(cacheFilePath);
        cacheOpen = cacheFile.open(QIODevice::WriteOnly);
        if (cacheOpen)
        {
            cacheOut.setDevice(&cacheFile);
            cacheOut.setVersion(QDataStream::Qt_6_0);
            cacheOut << Constants::INDEX_CACHE_MAGIC;
            cacheOut << Constants::INDEX_CACHE_VERSION;
            cacheOut << QDateTime::currentDateTime();
            cacheOut << m_currentRoot;
        }
    }
    else
    {
        // Prepare accumulator for silent hot-swapping
        m_silentAccumulator.clear();
        m_silentAccumulator.reserve(100'000);
    }

    // ── Build skip set ──
    QSet<QString> skipSet;
    for (const QString &s : Constants::skippedDirectories())
        skipSet.insert(s.toLower());

    const QFileInfo rootInfo(rootPath);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        emit scanFinished(0);
        return;
    }

    QQueue<QString> dirQueue;
    dirQueue.enqueue(rootPath);

    QSet<QString> visited;
    visited.insert(rootInfo.canonicalFilePath());

    QVector<FileItem> batch;
    batch.reserve(Constants::INDEX_BATCH_SIZE);
    int total = 0;
    // int dirsVisited = 0;

    // ── BFS Main Loop ─────────────────────────────────────────
    while (!dirQueue.isEmpty())
    {
        if (m_cancelled.load(std::memory_order_relaxed)) break;

        const QString currentDir = dirQueue.dequeue();
        // ++dirsVisited;

        // ── THE GHOST ENGINE MAGIC ──
        // Add safe, canonical directories to the watcher
        if (m_watcher->directories().size() < 4000) {
            if (!m_watcher->directories().contains(currentDir)) {
                m_watcher->addPath(currentDir);
            }
        }

        if (!isSilent) emit progressUpdate(currentDir);

        QDirIterator it(currentDir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::NoIteratorFlags);

        while (it.hasNext())
        {
            if (m_cancelled.load(std::memory_order_relaxed)) break;

            it.next();
            const QFileInfo info = it.fileInfo();

            if (info.isDir())
            {
                if (info.isJunction() || info.isSymLink()) continue;

                const QString absPath  = info.absoluteFilePath();
                const QString pathLow  = absPath.toLower();

                bool skip = false;
                for (const QString &s : skipSet) {
                    if (pathLow.contains(s)) { skip = true; break; }
                }
                if (skip) continue;

                const QString canonical = info.canonicalFilePath();
                if (canonical.isEmpty() || visited.contains(canonical)) continue;

                visited.insert(canonical);
                dirQueue.enqueue(absPath);
            }
            else if (info.isFile())
            {
                FileItem item;
                item.name         = info.fileName();
                item.path         = info.absoluteFilePath();
                item.size         = info.size();
                item.lastModified = info.lastModified();

                if (!isSilent) {
                    if (cacheOpen) cacheOut << item;

                    batch.append(std::move(item));
                    ++total;

                    if (total >= Constants::MAX_INDEX_FILES) break;

                    if (batch.size() >= Constants::INDEX_BATCH_SIZE) {
                        emit batchReady(batch);
                        batch.clear();
                        batch.reserve(Constants::INDEX_BATCH_SIZE);
                    }
                } else {
                    // Accumulate silently
                    m_silentAccumulator.append(std::move(item));
                    ++total;
                    if (total >= Constants::MAX_INDEX_FILES) break;
                }
            }
        }
    }

    // ── Finish & Cleanup ──────────────────────────────────────
    if (!isSilent)
    {
        if (!batch.isEmpty()) emit batchReady(batch);
        if (cacheOpen) {
            cacheFile.flush();
            cacheFile.close();
        }
        emit sourceReport(QStringLiteral("Index ready: %1 files").arg(total));
        emit scanFinished(total);
    }
    else
    {
        // ── SILENT HOT-SWAP ──
        if (!m_cancelled.load(std::memory_order_relaxed)) {
            qDebug() << "[Ghost Engine] Emitting silentIndexReady to hot-swap UI";
            emit silentIndexReady(m_silentAccumulator);
            emit scanFinished(total);
        }
    }
}
