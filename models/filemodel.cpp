#include "filemodel.h"
#include "helpers.h"

#include <QColor>
#include <algorithm>
#include <QFileIconProvider>
#include <QFileInfo>

FileModel::FileModel(QObject *parent) : QAbstractTableModel(parent) {}
FileModel::~FileModel() = default;

int FileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int FileModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(Column::COUNT);
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) return {};

    const FileItem &item = m_data.at(index.row());
    const auto col = static_cast<Column>(index.column());

    // Fetch Native OS Icons
    if (role == Qt::DecorationRole && col == Column::Name) {
        static QFileIconProvider provider;
        return provider.icon(QFileInfo(item.path));
    }

    switch (role)
    {
    case Qt::DisplayRole:
        switch (col)
        {
        case Column::Name:         return item.name;
        case Column::Type:         return item.extension().toUpper(); // e.g., "PNG", "CPP"
        case Column::Size:         return Helpers::formatFileSize(item.size);
        case Column::LastModified: return item.lastModified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
        default: return {};
        }

    // ── THE RICH HOVER TOOLTIP ──
    case Qt::ToolTipRole:
        return QStringLiteral(
                   "<div style='white-space:nowrap;'>"
                   "<b>Name:</b> %1<br>"
                   "<b>Path:</b> <span style='color:#3B82F6;'>%2</span><br>"
                   "<b>Size:</b> <span style='color:#0F172A;'>%3</span><br>"
                   "<b>Modified:</b> %4"
                   "</div>"
                   ).arg(item.name.toHtmlEscaped(),
                 item.path.toHtmlEscaped(),
                 Helpers::formatFileSize(item.size),
                 item.lastModified.toString("MMM d, yyyy h:mm ap"));

    case Qt::TextAlignmentRole:
        if (col == Column::Size)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::ForegroundRole:
        if (col == Column::Type)
            return QColor(QStringLiteral("#64748B")); // Soft gray for extension text
        return QVariant();

    default: return {};
    }
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return {};

    if (role == Qt::DisplayRole)
    {
        switch (static_cast<Column>(section))
        {
        case Column::Name:         return QStringLiteral("Name");
        case Column::Type:         return QStringLiteral("Type"); // Header title updated
        case Column::Size:         return QStringLiteral("Size");
        case Column::LastModified: return QStringLiteral("Date Modified");
        default: return {};
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        if (static_cast<Column>(section) == Column::Size)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return {};
}

void FileModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    const auto col = static_cast<Column>(column);

    std::sort(m_data.begin(), m_data.end(),
              [col, order](const FileItem &a, const FileItem &b) -> bool {
                  if (order == Qt::AscendingOrder) {
                      switch (col) {
                      case Column::Name:         return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                      case Column::Type:         return a.extension().compare(b.extension(), Qt::CaseInsensitive) < 0; // Sort by extension
                      case Column::Size:         return a.size < b.size;
                      case Column::LastModified: return a.lastModified < b.lastModified;
                      default: return false;
                      }
                  } else {
                      switch (col) {
                      case Column::Name:         return b.name.compare(a.name, Qt::CaseInsensitive) < 0;
                      case Column::Type:         return b.extension().compare(a.extension(), Qt::CaseInsensitive) < 0; // Sort by extension
                      case Column::Size:         return b.size < a.size;
                      case Column::LastModified: return b.lastModified < a.lastModified;
                      default: return false;
                      }
                  }
              });

    emit layoutChanged();
}

void FileModel::addBatch(const QVector<FileItem> &batch)
{
    if (batch.isEmpty()) return;
    const int first = m_data.size();
    const int last  = first + batch.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    m_data.reserve(m_data.size() + batch.size());
    m_data.append(batch);
    endInsertRows();
}

void FileModel::setFiles(QVector<FileItem> files)
{
    beginResetModel();
    m_data = std::move(files);
    endResetModel();
}

void FileModel::clear()
{
    if (m_data.isEmpty()) return;
    beginResetModel();
    m_data.clear();
    endResetModel();
}

const FileItem &FileModel::fileAt(int row) const
{
    Q_ASSERT(row >= 0 && row < m_data.size());
    return m_data.at(row);
}

int FileModel::fileCount() const { return m_data.size(); }
