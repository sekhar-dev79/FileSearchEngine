#pragma once

#include <QObject>
#include <QVector>
#include <atomic>
#include "fileitem.h"

class SearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit SearchEngine(QObject *parent = nullptr);
    ~SearchEngine() override;

    static constexpr int MAX_RESULTS = 2000;

public slots:
    void setIndex(const QVector<FileItem> &index);

    // FIX: Match the signature in searchengine.cpp
    void search(const QString &query, bool isRegex = false);

    void cancelSearch();

signals:
    void resultsReady(const QVector<FileItem> &results, const QString &query);
    void searchProgress(int found);

private:
    static QStringList tokenize(const QString &query);
    static bool matchesAllTokens(const FileItem &item, const QStringList &tokens);

    QVector<FileItem>  m_index;
    std::atomic<bool>  m_cancelled { false };
};
