#pragma once
#include <QStyledItemDelegate>
#include <QString>

class HighlightDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HighlightDelegate(QObject *parent = nullptr);

    // Updates the current word we are looking for
    void setSearchTerm(const QString &term);

    // Overrides the default drawing behavior
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QString m_searchTerm;
};
