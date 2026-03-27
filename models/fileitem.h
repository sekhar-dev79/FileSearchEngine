#pragma once

#include <QString>
#include <QDateTime>
#include <QMetaType>

struct FileItem
{
    QString   name;
    QString   type;
    QString   path;
    qint64    size         = 0;
    QDateTime lastModified;

    [[nodiscard]] QString extension() const
    {
        const int dot = name.lastIndexOf(u'.');
        if (dot < 0 || dot == name.size() - 1) return {};
        return name.mid(dot + 1).toLower();
    }
};

Q_DECLARE_METATYPE(FileItem)
Q_DECLARE_METATYPE(QVector<FileItem>)
