#include "searchengine.h"
#include <QDebug>
#include <QRegularExpression>

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent)
{}

SearchEngine::~SearchEngine() = default;

void SearchEngine::cancelSearch()
{
    m_cancelled.store(true, std::memory_order_relaxed);
}

void SearchEngine::setIndex(const QVector<FileItem> &index)
{
    m_index = index;
    qDebug() << "[SearchEngine] Index set:" << m_index.size() << "files";
}

QStringList SearchEngine::tokenize(const QString &query)
{
    QStringList tokens;
    const QStringList parts = query.split(u' ', Qt::SkipEmptyParts);
    tokens.reserve(parts.size());
    for (const QString &p : parts)
        tokens.append(p.toLower());
    return tokens;
}

bool SearchEngine::matchesAllTokens(const FileItem &item, const QStringList &tokens)
{
    for (const QString &token : tokens)
    {
        const bool inName = item.name.contains(token, Qt::CaseInsensitive);
        const bool inPath = inName ? true : item.path.contains(token, Qt::CaseInsensitive);

        if (!inName && !inPath)
            return false;
    }
    return true;
}

// ── search ────────────────────────────────────────────────────────────────────
// Updated signature to accept the Regex toggle state
void SearchEngine::search(const QString &query, bool isRegex)
{
    m_cancelled.store(false, std::memory_order_relaxed);

    if (query.trimmed().isEmpty() || m_index.isEmpty())
    {
        emit resultsReady({}, query);
        return;
    }

    QVector<FileItem> results;
    results.reserve(qMin(static_cast<int>(m_index.size()), MAX_RESULTS));

    int scanned = 0;

    if (isRegex)
    {
        // ── REGEX SEARCH MODE ──
        QRegularExpression re(query, QRegularExpression::CaseInsensitiveOption);

        // If user types an invalid Regex pattern, stop immediately to prevent crashes
        if (!re.isValid()) {
            qDebug() << "[SearchEngine] Invalid Regex pattern:" << query;
            emit resultsReady({}, query);
            return;
        }

        for (const FileItem &item : m_index)
        {
            if (scanned % 1000 == 0 && m_cancelled.load(std::memory_order_relaxed)) return;

            // Match against filename OR path
            if (re.match(item.name).hasMatch() || re.match(item.path).hasMatch()) {
                results.append(item);
                if (results.size() >= MAX_RESULTS) break;
            }
            ++scanned;
        }
    }
    else
    {
        // ── STANDARD TOKEN SEARCH MODE ──
        const QStringList tokens = tokenize(query);
        if (tokens.isEmpty()) {
            emit resultsReady({}, query);
            return;
        }

        for (const FileItem &item : m_index)
        {
            if (scanned % 1000 == 0 && m_cancelled.load(std::memory_order_relaxed)) return;

            if (matchesAllTokens(item, tokens)) {
                results.append(item);
                if (results.size() >= MAX_RESULTS) break;
            }
            ++scanned;
        }
    }

    qDebug() << "[SearchEngine] Done — found:" << results.size() << "| Mode:" << (isRegex ? "Regex" : "Tokens");
    emit resultsReady(results, query);
}
