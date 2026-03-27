#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include "fileitem.h"

class FileModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class Column : int {
        Name         = 0,
        Type         = 1,
        Size         = 2,
        LastModified = 3,
        COUNT        = 4
    };

    explicit FileModel(QObject *parent = nullptr);
    ~FileModel() override;

    [[nodiscard]] int     rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] int     columnCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void addBatch(const QVector<FileItem> &batch);
    void setFiles(QVector<FileItem> files);
    void clear();

    [[nodiscard]] const FileItem     &fileAt(int row) const;
    [[nodiscard]] int                 fileCount() const;
    [[nodiscard]] QVector<FileItem>   allFiles() const { return m_data; }

private:
    QVector<FileItem> m_data;
};
