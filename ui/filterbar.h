// ui/filterbar.h
#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDateTime>
#include <QStringList>
#include <QVector>

struct FilterCriteria
{
    QStringList extensions;
    qint64      minSizeBytes;
    QDateTime   modifiedAfter;
};

class FilterBar : public QWidget
{
    Q_OBJECT

public:
    explicit FilterBar(QWidget *parent = nullptr);

    [[nodiscard]] FilterCriteria criteria() const;
    void updateExtensions(const QVector<QString> &extensions);
    void resetFilters();
    [[nodiscard]] bool isDefault() const;

signals:
    void filtersChanged();

private slots:
    void onCategoryChanged();
    void onAnyFilterChanged();

private:
    void applyStyles();

    QHBoxLayout *m_mainLayout;

    // ── Cascading Type Filters ──
    QLabel    *m_typeLabel;
    QComboBox *m_categoryCombo;
    QComboBox *m_extCombo;

    QLabel    *m_sizeLabel;
    QSpinBox  *m_sizeSpinBox;

    QLabel    *m_dateLabel;
    QComboBox *m_dateCombo;

    QPushButton *m_resetButton;

    QVector<QString> m_allScannedExtensions;
};
